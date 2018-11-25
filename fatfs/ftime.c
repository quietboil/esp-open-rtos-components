#include "ff.h"
#include <sys/time.h>
#include <time.h>

/**
 * \brief Get the current time
 */
DWORD get_fattime (void)
{
    struct timeval time_now;
    gettimeofday(&time_now, NULL);

    struct tm tm;
    localtime_r(&time_now.tv_sec, &tm);

    union {
        DWORD packed;
        struct {
            uint32_t second : 5; ///< Second / 2 (0..29, e.g. 25 for 50)
            uint32_t minute : 6; ///< Minute (0..59)
            uint32_t hour   : 5; ///< Hour (0..23)
            uint32_t day    : 5; ///< Day of the month (1..31)
            uint32_t month  : 4; ///< Month (1..12)
            uint32_t year   : 7; ///< Year origin from the 1980 (0..127, e.g. 37 for 2017)
        };
    } fattime;

    fattime.year   = tm.tm_year + 80;
    fattime.month  = tm.tm_mon + 1;
    fattime.day    = tm.tm_mday;
    fattime.hour   = tm.tm_hour;
    fattime.minute = tm.tm_min;
    fattime.second = tm.tm_sec / 2;

    return fattime.packed;
}