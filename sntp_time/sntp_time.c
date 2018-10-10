#include <sys/time.h>
#include <sys/reent.h>
#include <lwip/sys.h>
#include <lwip/apps/sntp_opts.h>
#include <esplibs/libmain.h>

static sys_mutex_t sntp_time_mutex;

#define MAX_CLOCK_SKEW  250000
#define CLOCK_SLEW_RATE 2000

static uint32_t sys_time_sec;
static uint32_t sys_time_usec;
static uint32_t sys_time_clock;
static  int32_t sys_clock_skew;

void sntp_set_system_time_us(uint32_t ntp_time_sec, uint32_t ntp_time_usec)
{
    gettimeofday(NULL, NULL);
    
    sys_mutex_lock(&sntp_time_mutex);
    int32_t diff = ntp_time_sec - sys_time_sec;
    if (diff < -2147 || 2147 < diff) {
        diff = INT32_MIN;
    } else {
        diff = diff * 1000000 + ntp_time_usec - sys_time_usec;
    }
    if (diff < -MAX_CLOCK_SKEW || MAX_CLOCK_SKEW < diff) {
        sys_time_sec   = ntp_time_sec;
        sys_time_usec  = ntp_time_usec;
        sys_clock_skew = 0;
    } else {
        sys_clock_skew = diff;
    }
    sys_mutex_unlock(&sntp_time_mutex);
}

int _gettimeofday_r(struct _reent * r, struct timeval * t, void * tz)
{
    sys_mutex_lock(&sntp_time_mutex);
    uint32_t elapsed;
    uint32_t sys_clock = sdk_system_relative_time(0);
    if (sys_clock > sys_time_clock) {
        elapsed = sys_clock - sys_time_clock;
    } else { // system clock has wrapped
        elapsed = (UINT32_MAX - sys_time_clock) + 1 + sys_clock;
    }
    if (sys_clock_skew) {
        int32_t slew = elapsed / CLOCK_SLEW_RATE;
        if (sys_clock_skew < 0) {
            slew = (sys_clock_skew < -slew ?  -slew : sys_clock_skew);
        } else if (sys_clock_skew < slew) {
            slew = sys_clock_skew;
        }
        elapsed += slew;
        sys_clock_skew -= slew;
    }
    sys_time_clock = sys_clock;
    sys_time_sec  += elapsed / 1000000;
    sys_time_usec += elapsed % 1000000;
    if (sys_time_usec > 1000000) {
        sys_time_sec++;
        sys_time_usec -= 1000000;
    }
    if (t) {
        t->tv_sec = sys_time_sec;
        if (sys_time_sec < 1539000000) {
            // Start up condition
            // See SNTP_COMP_ROUNDTRIP: "for the round-trip calculation to work,
            // the difference between the local clock and the NTP server clock
            // must not be larger than about 34 years"
            // Pretend system clock started ticking on 2018-10-08 at 12:00:00 UT
            // TODO (maybe): use stored (as sysparam?) time offset
            t->tv_sec += 1539000000;
        }
        t->tv_usec = sys_time_usec;
    }
    sys_mutex_unlock(&sntp_time_mutex);
    return 0;
}

void sntp_time_init(void)
{
    sys_mutex_new(&sntp_time_mutex);
}

bool sntp_time_is_set(void)
{
    return sys_time_sec > 1539000000;
}