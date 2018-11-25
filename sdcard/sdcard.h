/**
 * \file  sdcard.h
 * \brief SD Card API
 */
#ifndef __SDCARD_H
#define __SDCARD_H

#include <hspi_config.h>
#include "sdcard_regs.h"
#include <stdint.h>

/**
 * \brief SD card operation completion results
 */
typedef enum {
    SDCARD_SUCCESS,         ///< Operation was successful
    SDCARD_ERROR_TIMEOUT,   ///< Card did not return an expected response in time
    SDCARD_ERROR_IO,        ///< Card responded with an error
    SDCARD_ERROR_CRC        ///< Data transfer error
} sdcard_result_t;

/**
 * \brief  Initializes SD card
 * \param[out]  card  Pointer to the card descriptor
 * \return SDCARD_SUCCESS       if card is ready for data transfer
 *         SDCARD_ERROR_TIMEOUT when card did not respond to initialization commands
 *                              (maybe card is not inserted)
 *         SDCARD_ERROR_IO      if card failed to accept initialization sequence (MMC V4?)
 */
sdcard_result_t sdcard_init(sdcard_t * card);

/**
 * \brief  Reads data (blocks) from the SD card
 * \param       card        Card descriptor
 * \param       block       First block to read data from
 * \param       num_blocks  Number of 512-byte blocks to read
 * \param[out]  data        Pointer to the destination buffer
 * \return SDCARD_SUCCESS        if data were transfered from card into the buffer
 *         SDCARD_ERROR_TIMEOUT  if card was still busy finishing previous operation
 *                               and did not respond to the read request
 *         SDCARD_ERROR_IO       if card failed to accept of one the read commands
 *         SDCARD_ERROR_CRC      if data were corrupted during transfer (CRC did not match)
 */
sdcard_result_t sdcard_read(sdcard_t card, uint32_t block, uint32_t num_blocks, uint8_t * data);

/**
 * \brief  Writes data (blocks) to the SD card
 * \param  card        Card descriptor
 * \param  block       First block to write data to
 * \param  num_blocks  Number of 512-byte blocks to write
 * \param  data        Pointer to the data
 * \return SDCARD_SUCCESS        if data were written to the card
 *         SDCARD_ERROR_TIMEOUT  if card was still busy finishing previous operation
 *                               and did not respond to the write request
 *                               or the card stayed busy much longer than expected after
 *                               accepting a block of data
 *         SDCARD_ERROR_IO       if card failed to accept of one the write commands
 */
sdcard_result_t sdcard_write(sdcard_t card, uint32_t block, uint32_t num_blocks, const uint8_t * data);

/**
 * \brief  Erases SD card blocks
 * \param  card        Card descriptor
 * \param  block       First block to erase
 * \param  num_blocks  Number of 512-byte blocks to erase
 * \return SDCARD_SUCCESS        if data were written to the card
 *         SDCARD_ERROR_TIMEOUT  if card was still busy finishing previous operation
 *                               and did not respond to the write request
 *                               or the card stayed busy much longer than expected after
 *                               accepting a block of data
 *         SDCARD_ERROR_IO       if card failed to accept of one the write commands
 */
sdcard_result_t sdcard_erase(sdcard_t card, uint32_t block, uint32_t num_blocks);

/**
 * \brief  Reads CID - card identification register
 * \param       card  Card descriptor
 * \param[out]  data  Pointer to the destination buffer
 * \return SDCARD_SUCCESS        if data were transfered from card into the buffer
 *         SDCARD_ERROR_TIMEOUT  if card was still busy finishing previous operation
 *                               and did not respond to the read request
 *         SDCARD_ERROR_IO       if card failed to accept of one the read commands
 *         SDCARD_ERROR_CRC      if data were corrupted during transfer (CRC did not match)
 */
sdcard_result_t sdcard_read_cid(sdcard_t card, sdcard_cid_t * data);

/**
 * \brief  Reads CSD - card specific data register
 * \param       card  Card descriptor
 * \param[out]  data  Pointer to the destination buffer
 * \return SDCARD_SUCCESS        if data were transfered from card into the buffer
 *         SDCARD_ERROR_TIMEOUT  if card was still busy finishing previous operation
 *                               and did not respond to the read request
 *         SDCARD_ERROR_IO       if card failed to accept of one the read commands
 *         SDCARD_ERROR_CRC      if data were corrupted during transfer (CRC did not match)
 */
sdcard_result_t sdcard_read_csd(sdcard_t card, sdcard_csd_t * data);

/**
 * \brief  Reads card size in 512 byte blocks
 * \param  card  Card descriptor
 * \return size of the device in 512-byte blocks
 *         or 0 if CSD read failed
 * \note   This function offers a simplified access to possibly the only relevant bit
 *         of info about the card in the CSD (especially for the SDHC/SDXC cards).
 */
uint32_t sdcard_get_size(sdcard_t card);

#endif