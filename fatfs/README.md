# FatFs - Generic FAT Filesystem Module

This component is an adaptation of ChaN's [FatFs](http://elm-chan.org/fsw/ff/00index_e.html), specifically R0.13c.

The component is mostly verbatim copy of the original with a few changes:
- `diskio.c` is removed. The version that implements FatFs I/O driver for [sdcard](../sdcard) is available as a separate component - [fatfs_sdcard_io](../fatfs_sdcard_io).
- `ffconf.h` has all defintions wrapped in `ifndef` to allow a program to change the defaults and bring in the rest via `include_next`.

## Usage

For a program to use this component to access FAT filesystem it has to:
- Include this component in the list of `EXTRA_COMPONENTS`.
- Either include an existing component that provides `diskio.c` driver (like [fatfs_sdcard_io](../fatfs_sdcard_io)) or create its own version of it.
- Optionally create `ffconf.h` that changes the FatFs configuration if the defaults provides by this component are not adequate.

Check [fatfs_sdcard_demo](https://github.com/quietboil/esp-open-rtos-components-demos/fatfs_sdcard_demo) for an example.
