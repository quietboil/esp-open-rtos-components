# FatFs DiskI/O Driver for SD Card

This component implements `diskio.c` driver for the [fatfs](../fatfs) component that allows the latter to access FAT filesystem on SD cards.

## Usage

As this component is just a driver that bridges [sdcard](../sdcard) and [fatfs](../fatfs) components all that is needed to use it is to list it as one of the `EXTRA_COMPONENTS`:
```makefile
EXTRA_COMPONENTS = \
	$(COMPONENTS_DIR)/fatfs \
	$(COMPONENTS_DIR)/fatfs_sdcard_io \
	$(COMPONENTS_DIR)/sdcard \
	$(COMPONENTS_DIR)/hspi
```
