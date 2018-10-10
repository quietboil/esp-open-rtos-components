# SNTP Timekeeper

This module implements time keeping functions. The 2 main ones are:
- `sntp_set_system_time_us` that is used by LWIP SNTP to set system time and
- `_gettimeofday_r` a newlib syscall implementation that reports current system time by joining the most recent NTP time and current system clock.

## Configration

LWIP SNTP implementation might need to be configured to change its default settings. This is usually accomplished by creating `lwipopts.h` file in your project. The useful bits that can be set there are:
- `SNTP_MAX_SERVERS` - The maximum number of SNTP servers that can be set. Default is 1. If you are planning to use NTP pool or if you have you own NTP server on your local network, the default is what you need.
- `SNTP_SERVER_DNS` - Set to 1 to use DNS names (or IP address strings) to set SNTP servers. Default is 0 and then SNTP servers are configured via `ip_addr_t`.
- `SNTP_SERVER_ADDRESS` - One server address/name can be defined as default if SNTP_SERVER_DNS == 1.

For example, `lwipopts.h` with these values will not require manual configuration of time sources:
```h
#define SNTP_SERVER_DNS 1
#define SNTP_SERVER_ADDRESS "us.pool.ntp.org"

#include_next <lwipopts.h>
``` 

## Usage

Reference `sntp_time` in `EXTRA_COMPONENTS`. Assuming your program added `esp-open-rtos-components` as a submodule:
```sh
$ git submodule add https://github.com/quietboil/esp-open-rtos-components components
```
`Makefile` for `hello_world` might look like this:
```makefile
-include local.mk
# where ESP_OPEN_RTOS_DIR is defined
PROGRAM = hello_world
COMPONENTS_DIR = $(realpath $(PROGRAM_DIR)/components)
EXTRA_COMPONENTS = $(COMPONENTS_DIR)/sntp_time
include $(ESP_OPEN_RTOS_DIR)/common.mk
```
Then in `user_init` you need to call `sntp_time_init` which creates mutex to protect system time:
```c
#include <sntp_time.h>

void user_init(void)
{
    // ...
    sntp_time_init();
    // ...
}
```
If you have not configuired `SNTP_SERVER_ADDRESS`, you need to configure your time source(s). Let's say, for example, that there is an NTP server on the local network at 10.0.0.10. It could be configured, in the same `user_init`, as:
```c
void user_init(void)
{
    // ...
    ip_addr_t ntp[] = {
        IPADDR4_INIT_BYTES(10,0,0,10)
    };
    sntp_setserver(0, &ntp[0]);
    // ...
}
```
The final task is to start LWIP SNTP periodic synchronization process. The only caveat here is that you need to do that after ESP acquired an IP from the AP. One of the good places for this is somewhere at the start up section of the main task:
```c
static void main_task(void * arg)
{
    // wait until WiFi gets connected
    while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    // Start SNTP
    LOCK_TCPIP_CORE();
    sntp_init();
    UNLOCK_TCPIP_CORE();
    // ...
}
```
