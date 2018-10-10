# Modular Components for the esp-open-rtos

This is a collection of [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) components I created for use in my own projects.

- [Bootloader](bootloader) is a tiny and simple bootloader for ESP8266 that can load firmware from one of the two predefined locations in flash leaving the other one to be used for OTA updates.
- [OTA Update](ota_update) handles TFTP requests to update firmware. It interacts with the [bootloader](bootloader) to figure out which firmware slot in flash is available and which is active and need to be deactivated after successful firmware update.
- [SNTP Time](sntp_time) is a system timekeeper. It defines functions needed by LWIP SNTP module to maintain system time in sync with NTP.
- [Sunriset](sunriset) is an adaptation of Paul Schlyter's `sunriset.c` that caculates sunrise and sunset times at any date and latitude.
- [TFTP VFS](tftp_vfs) is a sort of VFS for TFTP. It allows program to attach multiple handles to LWIP TFTP server.
