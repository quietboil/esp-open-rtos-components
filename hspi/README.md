# Demux Aware User SPI (HSPI) Driver

This component targets environments where ESP8266 drives two or more SPI slave devices. While this driver is perfectly capable to work with only one SPI slave the API that it provides was designed to offer a convenient method to work with multiple SPI slaves.

## Hardware

Unless this component is used to drive a single slave SPI device some kind of demultiplexer for the CS0 line is needed to select which device HSPI will communicate with. For example, 74155 as a 2-to-4 decoder, 74237 as a 3-to-8 decoder, 74154 as a 4-to-16 decoder, or a simple 1-to-2 decoder built from NAND gates can be used to route CS signal to the appropriate slave device.

## Usage

### Makefile

List the component in the `EXTRA_COMPONENTS` list of the program's `Makefile`:
```makefile
EXTRA_COMPONENTS = \
	$(COMPONENTS_DIR)/hspi
```

### hspi_config.h

Implement `hspi_config.h` which would provide slave device descriptors and accessor functions for individual device specific properties.

See [sdcard_demo](https://github.com/quietboil/esp-open-rtos-components-demos/tree/master/sdcard_demo) for an example of one such implementation.

> :warning: **Note** that `sdcard_demo` drives only one SPI slave and thus the demuxing is not actually present in its `hspi_config.h`. More specifically it does not define `HSPI_CS_DEMUX_GPIO_PINS` and does not implement `hspi_dev_demux_cs` function that this driver would use to route CS line to the selected SPI slave.

