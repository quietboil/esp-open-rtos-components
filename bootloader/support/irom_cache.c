#include <common_macros.h>
#include <stdint.h>

#define BOOT_MB (*(uint8_t*) 0x3fffd6ff)

void rom_Cache_Read_Enable(uint32_t, uint32_t, uint32_t);

void IRAM Cache_Read_Enable(uint32_t odd_even_mb, uint32_t two_mb_count, uint32_t unknown_value)
{
    // BOOT_MB will ever be either 0 or 1
    rom_Cache_Read_Enable(BOOT_MB, 0, 1);
}