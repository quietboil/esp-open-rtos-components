/**
 * \file  ota.h
 * \brief TFTP VFS for OTA firmware updates
 * 
 * The OTA VFS expects that the client will provide an MD5 hash of the
 * firmware via the "remote name" part of the put command. For example:
 * \code
 *   tftp -m binary 10.0.0.11 -c put firmware.bin d41d8cd98f00b204e9800998ecf8427e
 * \endcode
 * The OTA uses this hash to validate that the firmware was not corrupted
 * while in transmission or during flashing.
 */
#ifndef __OTA_H
#define __OTA_H

#include <lwip/apps/tftp_server.h>

/**
 * \brief TFTP context for OTA firmware updates
 */
extern struct tftp_context const OTA_VFS;

#endif