# OTA Firmware Update

This module, which is implemented as a [TFTP VFS](../tftp_vfs) "file handler", provides OTA update capabilities to systems that run firmware booted with the [Bootloader](../bootloader).

## Configuration

By default the module is using boot ROM's functions to compute MD5 hash of the firmware image. This can be switched to use BearSSL implementation instead. To do so create `local.mk` file and define `USE_BEARSSL` variables in it like this:
```makefile
USE_BEARSSL = 1
```

## Usage

Let's say the program - `hello_world` - added `esp-open-rtos-components` as a submodule:
```sh
$ git submodule add https://github.com/quietboil/esp-open-rtos-components components
```
Add this module together with `bootloader` and `tftp_vfs` to the list of `EXTRA_COMPONENTS` in the program `Makefile`:
```makefile
-include local.mk
# where ESP_OPEN_RTOS_DIR is defined
PROGRAM = hello_world
COMPONENTS_DIR = $(realpath $(PROGRAM_DIR)/components)
EXTRA_COMPONENTS = $(COMPONENTS_DIR)/bootloader \
    $(COMPONENTS_DIR)/tftp_vfs \
    $(COMPONENTS_DIR)/ota_update
include $(ESP_OPEN_RTOS_DIR)/common.mk
```
Then in `user_init` register OTA VFS handler with TFTP VFS:
```c
#include <tftp_vfs.h>
#include <ota.h>

void user_init(void)
{
    // ...
    static struct tftp_context const * vfs[] = {
        &OTA_VFS,
        NULL
    };
    tftp_vfs_init(vfs);
}
```
The very first program with OTA update capabilities still need to be flashed using `esptool`. Once it is running subsequent updates can be delivered via TFTP. For example, assuming ESP IP is 10.0.0.11:
```sh
$ tftp -m binary 10.0.0.11 -c put firmware.bin d41d8cd98f00b204e9800998ecf8427e
```
That last part is the MD5 hash of the firmware BIN. OTA update uses it to ensure that the data it received and the image it flashed was not at any point corrupted.

You could also add an OTA target to the `Makefile` if you do this frequently enough:
```makefile
-include local.mk
# where DEVICE_IP and ESP_OPEN_RTOS_DIR are defined

include $(ESP_OPEN_RTOS_DIR)/common.mk

ota_update: $(FW_FILE)
	tftp -m binary $(DEVICE_IP) -c put $(FW_FILE) $$(md5sum $(FW_FILE) | cut -d ' ' -f 1)
```
> :warning: Do not try to push OTA update right after flashing ESP. Either reset or power cycle it after it was flashed with esptool. Otherwise when OTA issues a software reset the ESP will reboot into the last hardware selected mode, i.e. the ROM serial bootloader.