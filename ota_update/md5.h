#ifndef __OTA_MD5_H
#define __OTA_MD5_H

#if (USE_BEARSSL)
#include <bearssl.h>
typedef br_md5_context  md5_ctx_t;
#define MD5_SIZE        br_md5_SIZE
#define md5_init        br_md5_init
#define md5_update      br_md5_update
#define md5_out         br_md5_out
#else
#include <stdint.h>
typedef struct {
    uint32_t _[22];
} md5_ctx_t;
#define MD5_SIZE        16
#define md5_init        ((void (*)(md5_ctx_t*))                 0x40009818)
#define md5_update      ((void (*)(md5_ctx_t*,void*,uint32_t))  0x40009834)
#define MD5Final        ((void (*)(uint8_t*,md5_ctx_t*))        0x40009900)
static inline void md5_out(md5_ctx_t * ctx, uint8_t * hash)
{
    MD5Final(hash, ctx); 
}
#endif

#endif