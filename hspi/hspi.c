/**
 * \file  hspi.c
 * \brief Implementation of the demux aware user SPI (HSPI) driver
 */
#include "hspi.h"
#include <string.h>
#include <sys/types.h>
#include <esp/iomux.h>
#include <esp/gpio.h>
#include <FreeRTOS.h>
#include <semphr.h>

static SemaphoreHandle_t hspi_mutex = NULL;

#define MISO_GPIO 12
#define MOSI_GPIO 13
#define SCK_GPIO  14
#define CS0_GPIO  15

#define HSPI_FUNC IOMUX_FUNC(2)
#define HSPI_CS_DEMUX_GPIO_PIN_MASK (HSPI_CS_DEMUX_GPIO_PINS_M & ~BIT(16))

void hspi_init()
{
    // in case init is called more than once
    if (!hspi_mutex) {
        hspi_mutex = xSemaphoreCreateMutex();
    }
#ifndef HSPI_WITHOUT_MISO
    gpio_set_iomux_function(MISO_GPIO, HSPI_FUNC);
#endif
    gpio_set_iomux_function(MOSI_GPIO, HSPI_FUNC);
    gpio_set_iomux_function(SCK_GPIO,  HSPI_FUNC);
    gpio_set_iomux_function(CS0_GPIO,  HSPI_FUNC);

#ifdef HSPI_CS_DEMUX_GPIO_PINS
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(16)
    gpio_enable(16, GPIO_OUTPUT);
#endif
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(12)
    gpio_enable(12, GPIO_OUTPUT);
#endif
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(5)
    gpio_enable(5, GPIO_OUTPUT);
#endif
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(4)
    gpio_enable(4, GPIO_OUTPUT);
#endif
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(3)
    gpio_enable(3, GPIO_OUTPUT);
#endif
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(2)
    gpio_enable(2, GPIO_OUTPUT);
#endif
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(1)
    gpio_enable(1, GPIO_OUTPUT);
#endif
#if HSPI_CS_DEMUX_GPIO_PINS & BIT(0)
    gpio_enable(0, GPIO_OUTPUT);
#endif
#endif
}

void hspi_select(hspi_dev_t device)
{
    // get exclusive access before changing HSPI configuration
    while (hspi_mutex && xSemaphoreTake(hspi_mutex, portMAX_DELAY) != pdTRUE);

#ifdef HSPI_CS_DEMUX_GPIO_PINS
    // demux CS output
    uint32_t demux_pins = hspi_dev_demux_cs(device);
    GPIO.OUT_CLEAR = HSPI_CS_DEMUX_GPIO_PINS & HSPI_CS_DEMUX_GPIO_PIN_MASK;
    GPIO.OUT_SET = demux_pins & HSPI_CS_DEMUX_GPIO_PIN_MASK;
#if HSPI_CS_DEMUX_SEL_PINS & BIT(16)
    RTC.GPIO_OUT = (RTC.GPIO_OUT & 0xfffffffe) | (demux_pins & BIT(16) ? 1 : 0);
#endif
#endif
    // configure HSPI clock
    hspi_set_clock(hspi_dev_clock(device));

    uint32_t spi_mode = hspi_dev_transfer_mode(device);
    if (spi_mode & 1) // clock idle state is high
        HSPI.PIN |= SPI_PIN_IDLE_EDGE;
    else
        HSPI.PIN &= ~SPI_PIN_IDLE_EDGE;

    uint32_t hspi_user0 =
        // Data sampling edge
        // Note that in the esp8266 speak "CLOCK EDGE" means "falling" not "trailing"
        spi_mode == 1 || spi_mode == 2
        ? SPI_USER0_CLOCK_OUT_EDGE | SPI_USER0_CLOCK_IN_EDGE
        : 0
    ;

    if (hspi_dev_software_cs(device)) {
        if ((IOMUX_GPIO15 & IOMUX_PIN_FUNC_MASK) != IOMUX_GPIO15_FUNC_GPIO ) {
            gpio_enable(CS0_GPIO, GPIO_OUTPUT);
        }
    } else {
        if ((IOMUX_GPIO15 & IOMUX_PIN_FUNC_MASK) != HSPI_FUNC ) {
            gpio_set_iomux_function(CS0_GPIO, HSPI_FUNC);
        }
        hspi_user0 |= SPI_USER0_CS_SETUP | SPI_USER0_CS_HOLD;
    }

    HSPI.USER0 = hspi_user0 | (hspi_dev_shared_io(device) ? SPI_USER0_SIO : SPI_USER0_DUPLEX);

    // bit order
    if (hspi_dev_is_msb(device))
        HSPI.CTRL0 &= ~(SPI_CTRL0_WR_BIT_ORDER | SPI_CTRL0_RD_BIT_ORDER);
    else
        HSPI.CTRL0 |= SPI_CTRL0_WR_BIT_ORDER | SPI_CTRL0_RD_BIT_ORDER;
}

void hspi_release()
{
    if (hspi_mutex) {
        xSemaphoreGive(hspi_mutex);
    }
}

void hspi_set_clock(uint32_t clock)
{
    if (clock & SPI_CLOCK_EQU_SYS_CLOCK) {
        IOMUX.CONF |= IOMUX_CONF_SPI1_CLOCK_EQU_SYS_CLOCK;
        HSPI.CLOCK = SPI_CLOCK_EQU_SYS_CLOCK;
    } else {
        IOMUX.CONF &= ~IOMUX_CONF_SPI1_CLOCK_EQU_SYS_CLOCK;
        HSPI.CLOCK = clock;
    }
}

void hspi_set_command(uint32_t cmd_len, uint16_t cmd)
{
    if (cmd_len == 0) {
        hspi_clear_command();
    } else if (cmd_len <= 16) {
        HSPI.USER0 |= SPI_USER0_COMMAND;
        // The commands are always sent using *little* endian byte order
        if (HSPI.CTRL0 & SPI_CTRL0_WR_BIT_ORDER) {
            // LSB first. As `cmd` is natively little endian
            // its bits arrangement is ready to be sent as-is
        } else {
            // MSB first
            cmd = __bswap16(cmd << (16 - cmd_len));
        }
        HSPI.USER2 = VAL2FIELD(SPI_USER2_COMMAND_BITLEN, cmd_len - 1)
                   | VAL2FIELD(SPI_USER2_COMMAND_VALUE, cmd);
    }
}

void hspi_set_address(uint32_t addr_len, uint32_t addr)
{
    if (addr_len == 0) {
        hspi_clear_address();
    } else if (addr_len <= 32) {
        HSPI.USER0 |= SPI_USER0_ADDR;
        // The address is always sent using *big* endian byte order
        if (HSPI.CTRL0 & SPI_CTRL0_WR_BIT_ORDER) {
            // LSB first (starting with byte[3])
            addr = __bswap32(addr);
        } else {
            // MSB first
            addr <<= 32 - addr_len;
        }
        HSPI.ADDR = addr;
        HSPI.USER1 = SET_FIELD(HSPI.USER1, SPI_USER1_ADDR_BITLEN, addr_len - 1);
    }
}

void hspi_set_data(uint32_t num_bits, const void * data)
{
    if (num_bits == 0) {
        hspi_clear_data();
    } else if (num_bits <= 512) {
        HSPI.USER0 |= SPI_USER0_MOSI;
        if ((uintptr_t)data & 3) {
            memcpy((void*)HSPI.W, data, (num_bits + 7) / 8);
        } else {
            const uint32_t * data_ptr = data;
            uint32_t * w = (uint32_t*)HSPI.W;
            uint32_t * end = w + (num_bits + 31) / 32;
            while (w < end) {
                *w++ = *data_ptr++;
            }
        }
        HSPI.USER1 = SET_FIELD(HSPI.USER1, SPI_USER1_MOSI_BITLEN, num_bits - 1);
    }
}

void hspi_set_pattern(uint32_t num_bits, uint32_t pattern)
{
    if (num_bits == 0) {
        hspi_clear_data();
    } else if (num_bits <= 512) {
        HSPI.USER0 |= SPI_USER0_MOSI;
        uint32_t * w = (uint32_t*)HSPI.W;
        uint32_t * end = w + (num_bits + 31) / 32;
        while (w < end) {
            *w++ = pattern;
        }
        HSPI.USER1 = SET_FIELD(HSPI.USER1, SPI_USER1_MOSI_BITLEN, num_bits - 1);
    }
}

void hspi_config_exec(hspi_tx_t tx)
{
    uint32_t user0_flags = HSPI.USER0 & ~(
        SPI_USER0_WR_BYTE_ORDER |
        SPI_USER0_DUMMY	        |
        SPI_USER0_MISO	        |
        SPI_USER0_RD_BYTE_ORDER
    );
    if (tx.big_endian_output) {
        user0_flags |= SPI_USER0_WR_BYTE_ORDER;
    }
    if (0 < tx.dummy_cycles && tx.dummy_cycles <= 256) {
        user0_flags |= SPI_USER0_DUMMY;
        HSPI.USER1 = SET_FIELD(HSPI.USER1, SPI_USER1_DUMMY_CYCLELEN, tx.dummy_cycles - 1);
    }
    if (0 < tx.recv_bits && tx.recv_bits <= 512) {
        user0_flags |= SPI_USER0_MISO;
        HSPI.USER1 = SET_FIELD(HSPI.USER1, SPI_USER1_MISO_BITLEN, tx.recv_bits - 1);
    }
    if (tx.big_endian_input) {
        user0_flags |= SPI_USER0_RD_BYTE_ORDER;
    }
    HSPI.USER0 = user0_flags;
}

void hspi_get_data(uint32_t buf_len, void * buf)
{
    if (buf_len > 0) {
        hspi_wait();
        if (buf_len > sizeof(HSPI.W)) {
            buf_len = sizeof(HSPI.W);
        }
        memcpy(buf, (const void *)HSPI.W, buf_len);
    }
}

void hspi_reset()
{
    hspi_wait();
    HSPI.USER0 &= ~(
        SPI_USER0_COMMAND       |
        SPI_USER0_ADDR          |
        SPI_USER0_DUMMY         |
        SPI_USER0_MISO          |
        SPI_USER0_MOSI          |
        SPI_USER0_WR_BYTE_ORDER |
        SPI_USER0_RD_BYTE_ORDER |
        SPI_USER0_FLASH_MODE
    );
}
