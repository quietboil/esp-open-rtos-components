/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2016        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <ff.h>			/* Obtains integer types */
#include <diskio.h>		/* Declarations of disk functions */
#include <sdcard.h>

#if (FF_MIN_SS != FF_MAX_SS || FF_MIN_SS != 512)
#error "Unsupported sector size"
#endif

static sdcard_t card[FF_VOLUMES];

/**
 * \brief Get Drive Status
 * \param pdrv Physical drive number to identify the drive
 * \return drive status
 */
DSTATUS disk_status (BYTE pdrv)
{
    // The simple check here assumes that the hardware we are dealing with
    // has no means to let us know whether a card has been removed (and thus
    // any attempts to track the card initalization status are inherently futile)
    return pdrv >= FF_VOLUMES ? STA_NOINIT : 0;
}

/**
 * \brief Initialize a Drive
 * \param pdrv Physical drive number to identify the drive
 * \return drive status
 */
DSTATUS disk_initialize (BYTE pdrv)
{
    if (pdrv >= FF_VOLUMES) return STA_NOINIT;

    sdcard_result_t err = sdcard_init(&card[pdrv]);
    return err ? STA_NOINIT : 0;
}

/**
 * \brief Read Sector(s)
 * \param pdrv Physical drive number to identify the drive
 * \param buff Data buffer to store read data
 * \param sector Start sector in LBA
 * \param count Number of sectors to read
 * \return RES_OK (0) The function succeeded.
 *         RES_ERROR An unrecoverable hard error occured during the read operation.
 *         RES_PARERR Invalid parameter.
 *         RES_NOTRDY The device has not been initialized.
 */
DRESULT disk_read (BYTE pdrv, BYTE * buff, DWORD sector, UINT count)
{
    if (pdrv >= FF_VOLUMES) return RES_PARERR;

    sdcard_result_t err = sdcard_read(card[pdrv], sector, count, buff);
    return err ? RES_ERROR : RES_OK;
}

/**
 * \brief Write Sector(s)
 * \param pdrv Physical drive number to identify the drive
 * \param buff Data to be written
 * \param sector Start sector in LBA
 * \param count Number of sectors to write
 * \return RES_OK (0) The function succeeded.
 *         RES_ERROR An unrecoverable hard error occured during the write operation.
 *         RES_WRPRT The medium is write protected.
 *         RES_PARERR Invalid parameter.
 *         RES_NOTRDY The device has not been initialized.
 *
 */
DRESULT disk_write (BYTE pdrv, const BYTE * buff, DWORD sector, UINT count)
{
    if (pdrv >= FF_VOLUMES) return RES_PARERR;

    sdcard_result_t err = sdcard_write(card[pdrv], sector, count, buff);
    return err ? RES_ERROR : RES_OK;
}

/**
 * \brief Miscellaneous Functions
 * \param pdrv Physical drive nmuber (0..)
 * \param cmd  Control code
 * \param buff Buffer to send/receive control data
 * \return RES_OK (0) The function succeeded.
 *         RES_ERROR An error occured.
 *         RES_PARERR The command code or parameter is invalid.
 *         RES_NOTRDY The device has not been initialized.
 */
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void * buff)
{
    if (pdrv >= FF_VOLUMES) return RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC: {
            // Make sure that the device has finished pending write process.
            // If the disk I/O module has a write back cache, the dirty buffers
            // must be written back to the media immediately.
            // Nothing to do for this command if each write operation to
            // the media is completed within the disk_write function.
            break;
        }
        case GET_SECTOR_COUNT: {
            // Returns number of available sectors on the drive into the DWORD variable
            // pointed by buff. This command is used by f_mkfs and f_fdisk functions to
            // determine the volume/partition size to be created. Required if FF_USE_MKFS == 1.
            *(DWORD*)buff = sdcard_get_size(card[pdrv]);
            if (*(DWORD*)buff == 0) {
                return RES_ERROR;
            }
            break;
        }
        case GET_SECTOR_SIZE: {
            // Returns sector size of the device into the WORD variable pointed by buff.
            // Valid return values for this command are 512, 1024, 2048 and 4096.
            // This command is required only if FF_MAX_SS > FF_MIN_SS.
            // When FF_MAX_SS == FF_MIN_SS, this command is never used and the device must
            // work at that sector size.
            WORD * res = buff;
            *res = 512;
            break;
        }
        case GET_BLOCK_SIZE: {
            // Returns erase block size of the flash memory media in unit of sector into
            // the DWORD variable pointed by buff. The allowable value is 1 to 32768 in
            // power of 2. Return 1 if the erase block size is unknown or non flash memory
            // media. This command is used by only f_mkfs function and it attempts to align
            // data area on the erase block boundary. Required if FF_USE_MKFS == 1.
            WORD * res = buff;
            *res = 1;
            break;
        }
        case CTRL_TRIM: {
            // Informs the device the data on the block of sectors is no longer needed and
            // it can be erased. The sector block is specified by a DWORD array
            // {<start sector>, <end sector>} pointed by buff. This is an identical command
            // to Trim of ATA device. Nothing to do for this command if this funcion is not
            // supported or not a flash memory device. FatFs does not check the result code
            // and the file function is not affected even if the sector block was not erased
            // well. This command is called on remove a cluster chain and in the f_mkfs function.
            // Required if FF_USE_TRIM == 1.
            break;
        }
        default: {
            return RES_PARERR;
        }
    }
    return RES_OK;
}