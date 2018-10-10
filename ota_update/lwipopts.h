#ifndef __OTA_UPDATE_LWIP_OPTS_H
#define __OTA_UPDATE_LWIP_OPTS_H

/** Max. length of TFTP filename
 */
#if defined(TFTP_MAX_FILENAME_LEN) && (TFTP_MAX_FILENAME_LEN < 32)
#undef TFTP_MAX_FILENAME_LEN
#endif

#ifndef TFTP_MAX_FILENAME_LEN
#define TFTP_MAX_FILENAME_LEN 32
#endif

#include_next <lwipopts.h>

#endif