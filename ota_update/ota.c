#include <stdbool.h>
#include <string.h>
#include <FreeRTOS.h>
#include <timers.h>
#include <espressif/esp_system.h>
#include <spiflash.h>
#include <lwip/apps/tftp_server.h>
#include "md5.h"

#ifdef OTA_DEBUG
#define LOG(fmt,...) printf("OTA> " fmt,## __VA_ARGS__)
#else
#define LOG(fmt,...)
#endif

#ifndef PROGRAM_OFFSET
#define PROGRAM_OFFSET 0x2000
#endif

#define BOOT_MB (*(uint8_t*) 0x3fffd6ff)

/**
 * \bried Success code for TFTP `read` and `write` functions
 */ 
#define OK   0
/**
 * \bried Failure code for TFTP `read` and `write` functions
 */ 
#define ERR -1

/**
 * \brief Handles scheduled system restart.
 */
static void on_restart_timer(TimerHandle_t timer)
{
    sdk_system_restart();
}

/**
 * \brief Schedules system restart.
 * 
 * We do not restart immediately to give UDP time to push that last ACK
 * 
 * \note  The current delay is 2 sec. On my network 1 sec sometimes was
 *        not enough.
 */
static void trigger_system_restart()
{
    TimerHandle_t timer = xTimerCreate("restart", pdMS_TO_TICKS(2000), pdFALSE, NULL, on_restart_timer);
    if (timer) {
        xTimerStart(timer, 0);
    }
}

/**
 * \brief Returns the offset to where the current program is stored in flash
 */
static inline uint32_t get_current_program_flash_offset()
{
    return BOOT_MB << 20;
}

/**
 * \brief Decodes hex encoded hash string
 * \param src    Hex encoded hash string
 * \param dst    Array where the binary hash is to be stored
 * \param maxlen Size of the `dst` array
 * \return Number of bytes actually stored
 * 
 * \note  The decoder skips over invalid characters.
 */
static uint32_t hex2bin(const char * src, uint8_t * dst, uint32_t maxlen)
{
    const uint8_t * const end = dst + maxlen;
    uint32_t binlen = 0;
    bool high_nibble = true;
    uint8_t acc;
    char c;
    while (dst < end && (c = *src++) != 0) {
        if ('0' <= c && c <= '9') {
            c = c - '0';
        } else if ('A' <= c && c <= 'F') {
            c = c - 'A' + 10;
        } else if ('a' <= c && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            continue;
        }
        if (high_nibble) {
            acc = c << 4;
        } else {
            *dst++ = acc | c;
            ++binlen;
        }
        high_nibble ^= true;
    }
    return binlen;
}

typedef struct _ota_ctx {
    uint32_t    offset;
    uint8_t     hash[MD5_SIZE];
    md5_ctx_t   hash_ctx;
} ota_ctx_t;

static inline ota_ctx_t * ota_ctx(void * handle)
{
    return (ota_ctx_t *) handle;
}

static void * tftp_open(const char * fname, const char * mode, uint8_t write)
{
    LOG("%s %s\n", (write ? "write" : "read"), fname);
    if (!write || strlen(fname) != MD5_SIZE * 2) {
        return NULL;
    }
    uint8_t firmware_hash[MD5_SIZE];
    if (hex2bin(fname, firmware_hash, sizeof(firmware_hash)) != sizeof(firmware_hash)) {
        return NULL;
    }
    ota_ctx_t * update_handle = malloc(sizeof(ota_ctx_t));
    if (update_handle) {
        update_handle->offset = (get_current_program_flash_offset() ^ 0x100000) + PROGRAM_OFFSET;
        LOG("flash offset %06x\n", update_handle->offset);
        memcpy(update_handle->hash, firmware_hash, sizeof(firmware_hash));
        md5_init(&update_handle->hash_ctx);
    }
    return update_handle;
}

static void tftp_close(void * handle)
{
    LOG("close\n");
    uint8_t hash[MD5_SIZE];
    md5_out(&ota_ctx(handle)->hash_ctx, hash);
    if (memcmp(hash, ota_ctx(handle)->hash, sizeof(hash)) != 0) {
        LOG("data hash failed\n");
    } else {
        // check content of the flash too
        md5_init(&ota_ctx(handle)->hash_ctx);
        uint8_t buf[256];
        uint32_t end = ota_ctx(handle)->offset;
        uint32_t last_page = end & 0xffffff00;
        uint32_t offset = (end & 0xfff00000) + PROGRAM_OFFSET;
        while (offset < last_page) {
            spiflash_read(offset, buf, sizeof(buf));
            md5_update(&ota_ctx(handle)->hash_ctx, buf, sizeof(buf));
            offset += sizeof(buf);
        }
        if (end > offset) {
            spiflash_read(offset, buf, end - offset);
            md5_update(&ota_ctx(handle)->hash_ctx, buf, end - offset);
        }
        md5_out(&ota_ctx(handle)->hash_ctx, hash);
        if (memcmp(hash, ota_ctx(handle)->hash, sizeof(hash)) != 0) {
            LOG("flash hash failed\n");
        } else {
            LOG("deactivate current program\n");
            uint32_t header_update = 0xffffffca;
            offset = get_current_program_flash_offset() + PROGRAM_OFFSET;
            if (spiflash_write(offset, (uint8_t*)&header_update, sizeof(header_update))) {
                LOG("schedule a delayed restart\n");
                // restart is delayed to give the last ACK a chance to be pushed out
                trigger_system_restart();
            }
        }
    }
    free(handle);
}

static int tftp_read(void * handle, void * buf, int bytes)
{
    return ERR;
}

static int tftp_write(void * handle, struct pbuf * p)
{
    while (p) {
        md5_update(&ota_ctx(handle)->hash_ctx, p->payload, p->len);
        if ((ota_ctx(handle)->offset & (SPI_FLASH_SECTOR_SIZE - 1)) == 0) {
            bool erased = spiflash_erase_sector(ota_ctx(handle)->offset);
            if (!erased) {
                LOG("failed to erase %06x\n", ota_ctx(handle)->offset);
                return ERR;
            }
        }
        bool written = spiflash_write(ota_ctx(handle)->offset, p->payload, p->len);
        if (!written) {
            LOG("write failed @ %06x\n", ota_ctx(handle)->offset);
            return ERR;
        }
        ota_ctx(handle)->offset += p->len;
        p = p->next;
    }
    return OK;
}

struct tftp_context const OTA_VFS = {
    .open  = tftp_open,
    .close = tftp_close,
    .read  = tftp_read,
    .write = tftp_write
};