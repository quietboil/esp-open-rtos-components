/**
 * \file  boot.c
 * \brief Simple firmware bootloader.
 * 
 * This bootloader will loaded firware from the 1st or the 2nd megabyte
 * of flash. In other words from image stored at 0x00X000 or 0x10X000.
 * It tries both slots and loads first active - the one where first byte
 * has the valid magic value. It expects that OTA update will deactivate
 * the current image after it saved the new one into the other MB.
 */
#include <stdint.h>

/**
 * \brief Offset from the MB boundary to the beginning of the image.
 * 
 * This is the same value that "flash" make target uses for the FW_FILE
 * address, which depends on the origin of the irom0_0_seg. 
 */
#ifndef IMG_OFFSET
#define IMG_OFFSET 0x2000
#endif

// There will be only 2 slots for firmware images in the flash
#define IMG_0 IMG_OFFSET
#define IMG_1 (0x100000 + IMG_OFFSET)

/**
 * \brief Location where the bootloader saves the MB of the image it loaded.
 * 
 * Bootloader stores the slot (well, MB) number here, so that later, during
 * firmware initialization, Cache_Read_Enable would know which MB of flash
 * to map for instuction caching. 
 * 
 * \note  The specific location was picked in one of the many "holes" left
 *        in the boot ROM data by data alignment.
 */
#define BOOT_MB     (*(uint8_t*) 0x3fffd6ff)

#define SPIRead     ((int (*)(uint32_t offset, void * dest, uint32_t size)) 0x40004b1c)
#define ets_printf  ((void (*)(const char *, ...)) 0x400024cc)

/**
 * \brief V1 image header
 */
struct image_header {
    uint8_t   image_magic;
    uint8_t   num_segments;
    uint8_t   flash_mode;
    uint8_t   flash_size_freq;
    void *    entry_point;
};

#define V1_IMAGE 0xe9
#define V2_IMAGE 0xea

struct segment_header {
    void *    addr;
    uint32_t  size;
};

/**
 * \brief Structure of the beginning of the V2 program image.
 */
struct image_start {
    struct image_header   header;
    struct segment_header segment;
};

static inline void * error(const char * msg)
{
    ets_printf(msg);
    return 0;
}

static inline uint8_t update_checksum(uint8_t checksum, struct segment_header * segment)
{
    uint32_t * p = segment->addr;
    uint32_t * e = p + segment->size / sizeof(uint32_t);
    while (p < e) {
        uint32_t t = *p++;
        checksum ^= (uint8_t) t;
        checksum ^= (uint8_t) (t >>  8);
        checksum ^= (uint8_t) (t >> 16);
        checksum ^= (uint8_t) (t >> 24);
    }
    return checksum;
}

#define spi_read(offset,dest,size) \
    if (SPIRead(offset,dest,size) != 0) \
        return error("Flash read error\n")

/**
 * \brief Locates an active image and loads it into memory.
 * \return program entry point or 0 if program loading failed.
 */
void * load_image()
{
    struct image_start img;
    uint32_t offset;

    offset = IMG_0;
    spi_read(offset, &img, sizeof(img));
    if (img.header.image_magic != V2_IMAGE) {
        offset = IMG_1;
        spi_read(offset, &img, sizeof(img));
        if (img.header.image_magic != V2_IMAGE) {
            return error("No active programs\n");
        }
    }
    BOOT_MB = offset >> 20 & 0x01;
    ets_printf("Loading program #%d\n", BOOT_MB + 1);
    offset += sizeof(img);

    // skip the first (.irom) segment and read the V1 (.iram, .dram) header
    offset += img.segment.size;
    spi_read(offset, &img.header, sizeof(img.header));
    if (img.header.image_magic != V1_IMAGE) {
        return error("No RAM segments found\n");
    }
    offset += sizeof(img.header);

    // load segments into RAM
    uint8_t checksum = 0xef;
    for (int i = 0; i < img.header.num_segments; i++) {
        spi_read(offset, &img.segment, sizeof(img.segment));
        if (img.segment.size & (sizeof(uint32_t) - 1)) {
            return error("Unexpected RAM segment size\n");
        }
        offset += sizeof(img.segment);
        spi_read(offset, img.segment.addr, img.segment.size);
        checksum = update_checksum(checksum, &img.segment);
        offset += img.segment.size;
    }
    uint8_t bin_checksum;
    spi_read(offset | 0xf, &bin_checksum, sizeof(bin_checksum));
    if (bin_checksum != checksum) {
        return error("Checksum error\n");
    }
    return img.header.entry_point;
}

void load_program()
{
    __asm__ goto (
        "s32i.n a0, a1, 12" "\n" // piggy-backing on main's (0x40000fec) unused slot
        "call0  load_image" "\n"
        "l32i.n a0, a1, 12" "\n"
        "beqz.n a2, %l0"    "\n"
        "jx     a2"         "\n"
        :
        :
        :
        : failed
    );
failed:
    return;
}
