# ESP8266 Bootloader

A simple bootloader that supports OTA firmware updates. This bootloader will load firmware from the 1st or the 2nd megabyte of flash. It tries both slots and loads firmware from the first active - the one where the first byte of the image has the valid magic value. It expects that OTA update will deactivate the current firmware image after it saved the new one into the currently inactive slot.

## Motivation

Initially I have implemented this bootloader while I was learning the ESP platform to further *my* understanding of the inner workings of the system. However, I have come to realization that what it does satisfies 100% of my needs, so I started using it exclusively. One of the benefits is its tiny source code, so it is very simple to change it when or if there is a need.

## Limitations

The limitations stem from the trade offs selected between simpler implementation and richer functionality. Depending on the capabilities of the target system and project goals and limits these can be either inconsequentional or crippling:
- The bootloader expects that the system has at least 2MB of flash. This makes it unsuitable for instance for ESP-01 modules.
- The bootloader reserves first 2MB of flash for firmware images. Those first 2MB are divided into 2 slots of 1MB each - one for the currently running image and another for the OTA update. The update process swaps these 2 slots. This leaves on ESP-12, for example, only 2MB (minus 36KB for system parameters areas) to store data.
On the other hand, if you can afford it, this flash layout makes development a lot easier as there is no need to keep separate linker scripts for different images and select the appropriate one for the update.

## Configuration

From the ESP boot ROM point of view the bootloader **is** the program that will be running on a system. It, the boot ROM, will use flash configuration stored in the header of the bootloader image to configure flash speed and mode. Therefore it might be very important to change the default configration to match your system's capabilities. For instance, for some NodeMCU you need to set flash mode to `dio` from the default `qio`.

To configure the build you need to create `local.mk` script with one or more of the following variables:
```makefile
FLASH_SPEED    = 40m
FLASH_MODE     = qio
FLASH_SIZE     = detect
ESPBAUD        = 115200
```
The values above are the defaults. In my experiments I noticed that all my ESPs would work just fine with `FLASH_SPEED` set to `80m`. NodeMCU needed `FLASH_MODE` set to `dio`. ESP-12 can be programmed with `ESPBAUD` at `921600` (that's as fast as CP2102 that I use to program it would go) and NodeMCU needed it at `460800` for reliable flashing even though CH340 on that board can go as fast as 2000000.

## Installation

To compile and install the bootloader execute `make` and then `make flash`.

> **Note** that after flashing `esptool` will reboot the system and without the actual program the bootloader will end up complaining that there are "No active programs" in flash.

## Source Component

The second part of this module is an actual source component that **must** be added to the list of `EXTRA_COMPONENTS` to compile the program that will run on the system with this bootloader. The component provides implementation of the `Cache_Read_Enable` function that is called during program startup to enable instruction cache. This implementation queries the bootloader to find which slot (megabyte) of flash was used to load the current firmware image and activates caching for that megabyte.

Let's say the program - `hello_world` - added `esp-open-rtos-components` as a submodule:
```sh
$ git submodule add https://github.com/quietboil/esp-open-rtos-components components
```
The `Makefile` for this program then might look like this:
```makefile
-include local.mk
# where ESP_OPEN_RTOS_DIR is defined
PROGRAM = hello_world
COMPONENTS_DIR = $(realpath $(PROGRAM_DIR)/components)
EXTRA_COMPONENTS = $(COMPONENTS_DIR)/bootloader
include $(ESP_OPEN_RTOS_DIR)/common.mk
```
