# SD Card Access

This component provides an API of sorts that a program would use to access an SD card via SPI protocol. It does not even try to be a comprehensive implementation by ignoring capabilities of some cards providing instead a more or less uniform API that might be useful for most applications. For instance it:
- accesses non-high capacity cards (usually those that are 2GB or smaller) using 512-byte blocks as if they are SDHC (where I/O is only possible in 512-byte blocks),
- provides only read/write/erase finctions and only reads access to the CSD register.

## Usage

The component is dependent and in many ways tightly coupled with the [hspi](../hspi) component. To use this component it and the `hspi` both have to be added to the list of `EXTRA_COMPONENTS`:
```makefile
EXTRA_COMPONENTS = \
	$(COMPONENTS_DIR)/sdcard \
	$(COMPONENTS_DIR)/hspi
```

To be accessible as an SPI slave device SD card specific trait functions have to be implemented by the program. The component needs access - set and get - to the "it is an SDHC card" property. The program is expected to allocate a storage for it - one bit is enough really :smile: - in the SPI slave device descriptor and implement `sdcard_set_sdhc_flag` and `sdcard_is_sdhc` functions. See `hspi_config.h` in this module for the description of the trait and [sdcard_demo](https://github.com/quietboil/esp-open-rtos-components-demos/tree/master/sdcard_demo) for an example of the implementation.

> :warning: **Note** that `hspi_config.h` in this component is used only to document the SPI slave device descriptor SD-Card trait. The file itself should not be included (for instance via `include_next`) in the program sources. Instead a program would use this and the `hspi_config.h` from the [hspi](../hspi) component as a template to create its own header for device descriptors.
