/**
 * \file  sdcard.c
 * \brief SD Card API implementation
 */

#include "sdcard.h"
#include <hspi.h>
#include <esp/gpio.h>
#include <espressif/esp_system.h>
#include <esplibs/libmain.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>

#define HSPI_CS 15

#define INIT_TIMEOUT 500000
#define IO_TIMEOUT   100000

static inline bool expired(uint32_t duration, uint32_t start)
{
    return sdk_system_relative_time(start) > duration;
}

static inline uint32_t timestamp()
{
    return sdk_system_relative_time(0);
}

static inline void set_cs_high()
{
    GPIO.OUT_SET = BIT(HSPI_CS);
}

static inline void set_cs_low()
{
    GPIO.OUT_CLEAR = BIT(HSPI_CS);
}

static uint8_t wait_until_card_returns(uint8_t value)
{
    hspi_reset();
    hspi_set_pattern(8, 0xffffffff);
    hspi_config_exec((hspi_tx_t){ .big_endian_output = true });
    uint32_t t0 = timestamp();
    uint8_t resp;
    do {
        hspi_exec();
        resp = hspi_read(0);
    } while (resp != value && !expired(IO_TIMEOUT,t0));
    return resp;
}

static inline uint8_t wait_until_card_not_busy()
{
    return wait_until_card_returns(0xff);
}

static uint8_t wait_r1()
{
    hspi_reset();
    hspi_set_pattern(8, 0xffffffff);
    hspi_config_exec((hspi_tx_t){ .big_endian_output = true });
    uint32_t count = 7; // there should not be more than 8 Ncr bytes
    uint32_t resp;
    do {
        hspi_exec();
        resp = hspi_read(0);
    } while ((resp & 0x80) && --count);
    return resp;
}

static uint8_t r1cmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    if (!wait_until_card_not_busy()) {
        return 0x80;
    }
    hspi_set_command(16, 0x4000 | cmd << 8 | arg >> 24);
    hspi_set_address(32, arg << 8 | crc);
    hspi_set_pattern(16, 0xffff); // typically 1 Ncr then R1
    hspi_config_exec((hspi_tx_t){});
    hspi_exec();
    uint32_t resp = hspi_read(0);
    if (resp & 0x80) { // Ncr
        resp >>= 8;
        if (resp & 0x80) { // up to 8 is possible
            resp = wait_r1();
        }
    }
    return resp;
}

static uint8_t r3cmd(uint8_t cmd, uint32_t arg, uint8_t crc, uint32_t * resp_data)
{
    uint8_t resp = r1cmd(cmd, arg, crc);
    if (!(resp & 0xfe)) {
        hspi_reset();
        hspi_set_pattern(32, 0xffffffff);
        hspi_config_exec((hspi_tx_t){ .big_endian_output = true, .big_endian_input = true });
        hspi_exec();
        *resp_data = hspi_read(0);
    }
    return resp;
}

static uint8_t acmd(uint8_t cmd, uint32_t arg, uint8_t crc)
{
    uint8_t resp = r1cmd(55, 0, 0x65);
    if (!(resp & 0xfe)) {
        resp = r1cmd(cmd, arg, crc);
    }
    return resp;
}

// static uint16_t block_crc(uint16_t crc, size_t size, const uint8_t * data)
// {
//     const uint8_t * end = data + size;
//     while (data < end) {
//         crc  = ((crc >> 8) & 0xff) | (crc << 8);
//         crc ^= *data++;
//         crc ^= (crc & 0xff) >> 4;
//         crc ^= crc << 12;
//         crc ^= (crc & 0xff) << 5;
//     }
//     return crc;
// }

typedef enum {
    UNRECOGNIZED,
    MMC,
    SD1,
    SD2,
    SDHC
} sdcard_type_t;

typedef uint8_t (*sdcard_init_proc_t)();

static uint8_t init_mmc()
{
    return r1cmd(1,0,0xf9);
}

static uint8_t init_sd1()
{
    return acmd(41,0,0xe5);
}

static uint8_t init_sd2()
{
    return acmd(41,BIT(30),0x77);
}

#define raise_error(code) err = code; goto done

sdcard_result_t sdcard_init(sdcard_t * card)
{
    sdcard_result_t err = SDCARD_SUCCESS;

    hspi_select(*card);

    uint32_t orig_clock = hspi_get_clock();
    hspi_set_clock(
        HSPI_CLOCK(5, 40) // 400 kHz
    );

    set_cs_high();
    hspi_reset();
    hspi_set_pattern(80, 0xffffffff); // at least 74 is required
    hspi_exec();
    hspi_wait();

    set_cs_low();
    // Software reset
    uint8_t resp = r1cmd(0,0,0x95);
    if (resp & 0x80) {
        raise_error(SDCARD_ERROR_TIMEOUT);
    } else if (resp != 0x01) {
        raise_error(SDCARD_ERROR_IO);
    }

    sdcard_type_t type = UNRECOGNIZED;
    // Check acceptable voltage
    uint32_t resp_data;
    resp = r3cmd(8, 0x1aa, 0x87, &resp_data);
    if (resp == 0x05) {
        type = SD1;
    } else if (resp == 0x01 && resp_data == 0x1aa) {
        type = SD2;
    } else if (resp & 0x80) {
        raise_error(SDCARD_ERROR_TIMEOUT);
    } else {
        raise_error(SDCARD_ERROR_IO);
    }

    // Card initialization
    sdcard_init_proc_t card_init;
    if (type == SD2) {
        card_init = init_sd2;
    } else {
        card_init = init_sd1;
        resp = card_init();
        if (resp > 1) {
            card_init = init_mmc;
            resp = 0x01;
        }
    }

    uint32_t t0 = timestamp();
    while (resp == 0x01) {
        if (expired(INIT_TIMEOUT,t0)) {
            raise_error(SDCARD_ERROR_TIMEOUT);
        }
        resp = card_init();
    }
    if (resp != 0x00) {
        raise_error(SDCARD_ERROR_IO);
    }

    // check whether this is a high capacity card
    sdcard_set_sdhc_flag(card,
        r3cmd(58, 0, 0xfd, &resp_data) == 0 && (resp_data & BIT(31)) && (resp_data & BIT(30))
    );

    // set uniform block size
    if (!sdcard_is_sdhc(*card) && r1cmd(16, 512, 0x15) != 0) {
        raise_error(SDCARD_ERROR_IO);
    }

done:
    hspi_set_clock(orig_clock);
    set_cs_high();
    hspi_release();
    return err;
}

static sdcard_result_t read_data(uint32_t size, uint8_t * data)
{
    if (wait_until_card_returns(0xfe) != 0xfe) {
        return SDCARD_ERROR_TIMEOUT;
    }
    hspi_reset();
    // uint16_t rx_crc = 0;
    while (size) {
        uint32_t num_bytes = size < 64 ? size : 64;
        hspi_set_pattern(num_bytes * 8, 0xffffffff);
        hspi_exec();
        hspi_get_data(num_bytes, data);
        // rx_crc = block_crc(rx_crc, num_bytes, data);
        data += num_bytes;
        size -= num_bytes;
    }
    hspi_set_pattern(16, 0xffff);
    hspi_config_exec((hspi_tx_t){ .big_endian_input = true });
    hspi_exec();
    // uint16_t tx_crc = hspi_read(0) >> 16;
    // return rx_crc != tx_crc ? SDCARD_ERROR_CSC : SDCARD_SUCCESS;
    return SDCARD_SUCCESS;
}

sdcard_result_t sdcard_read(sdcard_t card, uint32_t addr, uint32_t num_blocks, uint8_t * data)
{
    sdcard_result_t err = SDCARD_SUCCESS;
    hspi_select(card);
    if (!sdcard_is_sdhc(card)) {
        addr <<= 9;
    }
    uint8_t cmd = (num_blocks == 1 ? 17 : 18);

    set_cs_low();
    uint8_t resp = r1cmd(cmd, addr, 0xff);
    if (resp & 0x80) {
        raise_error(SDCARD_ERROR_TIMEOUT);
    }
    if (resp != 0) {
        raise_error(SDCARD_ERROR_IO);
    }
    while (!err && num_blocks) {
        err = read_data(512, data);
        data += 512;
        --num_blocks;
    }
    if (cmd == 18) {
        r1cmd(12,0,0x61);
    }
done:
    set_cs_high();
    hspi_release();
    return err;
}

static sdcard_result_t sdcard_read_register(sdcard_t card, uint8_t * data, uint8_t cmd, uint8_t crc)
{
    sdcard_result_t err = SDCARD_SUCCESS;
    hspi_select(card);
    set_cs_low();
    uint8_t resp = r1cmd(cmd, 0, crc);
    if (resp & 0x80) {
        raise_error(SDCARD_ERROR_TIMEOUT);
    }
    if (resp != 0) {
        raise_error(SDCARD_ERROR_IO);
    }
    err = read_data(16, data);

done:
    set_cs_high();
    hspi_release();
    return err;
}

sdcard_result_t sdcard_read_cid(sdcard_t card, sdcard_cid_t * cid)
{
    return sdcard_read_register(card, (uint8_t*) cid, 10, 0x1b);
}

sdcard_result_t sdcard_read_csd(sdcard_t card, sdcard_csd_t * csd)
{
    uint8_t data[sizeof(sdcard_csd_t)];
    sdcard_result_t err = sdcard_read_register(card, data, 9, 0xaf);
    if (!err) {
        uint8_t * src = data;
        uint8_t * end = src + sizeof(data);
        uint8_t * dst = (uint8_t*) csd + sizeof(sdcard_csd_t);
        while (src < end) {
            *--dst = *src++;
        }
    }
    return err;
}

uint32_t sdcard_get_size(sdcard_t card)
{
    uint8_t data[16];
    sdcard_result_t err = sdcard_read_register(card, data, 9, 0xaf);
    uint32_t size = 0;
    if (!err) {
        if (data[0] >> 6 == 0) {
            // C_SIZE [62:73]
            uint32_t c_size = (uint32_t)(data[6] & 0x3) << 10 | (uint32_t)data[7] << 2 | data[8] >> 6;
            // C_SIZE_MULT [47:49]
            uint32_t c_size_mult = (data[9] & 0x3) << 1 | data[10] >> 7;
            // READ_BL_LEN [80:83]
            uint32_t read_bl_len = data[5] & 0xf;
            size = (c_size + 1) << (c_size_mult + 2) << (read_bl_len - 9);
        } else {
            // C_SIZE [48:69]
            uint32_t c_size = (uint32_t)(data[7] & 0x3f) << 16 | (uint32_t)data[8] << 8 | data[9];
            size = (c_size + 1) << 10;
        }
    }
    return size;
}

#define DATA_RESPONSE 0x1f
#define DATA_ACCEPTED 0x05

static sdcard_result_t write_block(uint8_t start_token, const uint8_t * data)
{
    hspi_reset();
    hspi_set_command(8, start_token);
    hspi_set_data(64 * 8, data);
    hspi_exec();

    const uint8_t * end = data + 512;
    data += 64;

    hspi_reset();
    while (data < end) {
        hspi_set_data(64 * 8, data);
        hspi_exec();
        data += 64;
        hspi_wait();
    }

    hspi_set_command(16, 0xffff); // pretend CRC
    hspi_set_pattern(8, 0xff);
    hspi_exec();
    uint8_t resp = hspi_read(0);
    return (resp & DATA_RESPONSE) == DATA_ACCEPTED ? SDCARD_SUCCESS : SDCARD_ERROR_IO;
}

static sdcard_result_t end_transmission()
{
    if (!wait_until_card_not_busy()) {
        return SDCARD_ERROR_TIMEOUT;
    }
    hspi_reset();
    // The "busy" may appear within Nbr clocks (max 8) after Stop Tran token.
    hspi_set_pattern(16, 0xfffd);
    hspi_exec();
    hspi_wait();
    return SDCARD_SUCCESS;
}

sdcard_result_t sdcard_write(sdcard_t card, uint32_t addr, uint32_t num_blocks, const uint8_t * data)
{
    sdcard_result_t err = SDCARD_SUCCESS;
    hspi_select(card);
    if (!sdcard_is_sdhc(card)) {
        addr <<= 9;
    }
    set_cs_low();
    if (num_blocks == 1) {
        if (r1cmd(24,addr,0xff) || write_block(0xfe, data)) {
            raise_error(SDCARD_ERROR_IO);
        }
    } else {
        if (acmd(23,num_blocks,0xff) || r1cmd(25,addr,0xff)) {
            raise_error(SDCARD_ERROR_IO);
        }
        err = write_block(0xfc, data);
        while (!err && --num_blocks) {
            data += 512;
            if (!wait_until_card_not_busy()) {
                raise_error(SDCARD_ERROR_TIMEOUT);
            }
            err = write_block(0xfc, data);
        }
        if (err) {
            // In case of any error (CRC or Write) during Write Multiple Block operation,
            // the host will stop the data transmission using CMD12
            r1cmd(12,0,0x61);
        } else {
            err = end_transmission();
        }
    }

done:
    set_cs_high();
    hspi_release();
    return err;
}

sdcard_result_t sdcard_erase(sdcard_t card, uint32_t addr, uint32_t num_blocks)
{
    sdcard_result_t err = SDCARD_SUCCESS;
    hspi_select(card);
    uint32_t last = addr + num_blocks - 1;
    if (!sdcard_is_sdhc(card)) {
        addr <<= 9;
        last <<= 9;
    }
    set_cs_low();

    if (r1cmd(32,addr,0xff) || r1cmd(33,last,0xff) || r1cmd(38,0,0xa5)) {
        raise_error(SDCARD_ERROR_IO);
    }

done:
    set_cs_high();
    hspi_release();
    return err;
}