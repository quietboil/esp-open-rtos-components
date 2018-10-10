#ifndef __SNTP_TIME_LWIP_OPTS_H
#define __SNTP_TIME_LWIP_OPTS_H

#include <stdint.h>

/** Enable round-trip delay compensation.
 * Compensate for the round-trip delay by calculating the clock offset from
 * the originate, receive, transmit and destination timestamps, as per RFC.
 */
#if defined(SNTP_COMP_ROUNDTRIP) && (SNTP_COMP_ROUNDTRIP != 1)
#undef SNTP_COMP_ROUNDTRIP
#endif

#ifndef SNTP_COMP_ROUNDTRIP
#define SNTP_COMP_ROUNDTRIP 1
#endif

/** SNTP update delay - in milliseconds
 * Default is 1 hour. Must not be beolw 60 seconds by specification (i.e. 60000)
 */
#if defined(SNTP_UPDATE_DELAY) && (SNTP_UPDATE_DELAY > 4200000)
#undef SNTP_UPDATE_DELAY
#endif

#ifndef SNTP_UPDATE_DELAY
#define SNTP_UPDATE_DELAY 4200000
#endif

/** SNTP macro to get system time, used for round-trip delay compensation 
 */
#define SNTP_GET_SYSTEM_TIME(S, F) do { \
    struct timeval t;                   \
    gettimeofday(&t, NULL);             \
    S = t.tv_sec;                       \
    F = t.tv_usec;                      \
} while (0)

void sntp_set_system_time_us(uint32_t sec, uint32_t usec);
#define SNTP_SET_SYSTEM_TIME_US(S, F) sntp_set_system_time_us(S, F)

#include_next <lwipopts.h>

#endif