/**
 * \file  tftp_vfs.h
 * \brief TFTP server with virtual file system
 */
#ifndef __TFTP_VFS_H
#define __TFTP_VFS_H

#include <lwip/apps/tftp_server.h>

/**
 * \brief Starts TFTP server
 * 
 * \param vfs_contexts  NULL terminated array of pointers to VFS TFTP context
 * 
 * When TFTP is requested to open a file it will ask each VFS to open it.
 * The first one that returns a non-NULL handle will handle the rest of the
 * file IO.
 */
void tftp_vfs_init(struct tftp_context const * vfs_contexts[]);

#endif