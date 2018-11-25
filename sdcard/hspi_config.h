/**
 * \file  hspi_config.h
 * \brief SD Card Device Descriptor
 * 
 * This file provides definitions of functions that have to be
 * implemented by the program to create a functional SD Card SPI
 * device descriptor.
 * 
 * \note This file will not be used to compile firmware.
 *       The program has to provide the actual implementation for that.
 */

// The program would have to provide implementation for all functions
// required to support traits of different devices. Here we just "include"
// the stubs for functions needed by HSPI itself.
#include_next <hspi_config.h>

#include <stdbool.h>

/**
 * \brief SD card descriptor
 *
 * SD card is an SPI slave device and as such it is a subtype of the #hspi_dev_t
 */
typedef hspi_dev_t sdcard_t;

/**
 * \brief Saves "SD card is an SDHC card" flag in the card descriptor
 * \param card Pointer to the card descriptor.
 */
static inline void sdcard_set_sdhc_flag(sdcard_t * card, bool is_sdhc)
{
}

/**
 * \brief Queries whether the card behind the provided descriptor is an SDHC one
 * \param card Card descriptor
 * \return `true` if SD card and an SDHC card.
 */
static inline bool sdcard_is_sdhc(sdcard_t card)
{
    return false;
}
