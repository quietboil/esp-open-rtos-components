/**
 * \file  hspi_config.h
 * \brief HSPI configuration and device descriptors
 * 
 * This file provides definitions of functions that have to be
 * implemented by the program to create functional HSPI device
 * descriptors.
 * 
 * \note This file will not be used to compile firmware.
 *       The program has to provide the actual implementation for that.
 */

#include <esp/spi_regs.h>


/**
 * \def   HSPI_CS_DEMUX_GPIO_PINS
 * \brief A list of pins that are used for HSPI CS demux output selection.
 *
 * Becuse GPIO 6..11 are used by system SPI and GPIO 12..15 are used by
 * the user SPI, only GPIO 0..5 and 16 remain available for demux output
 * selection.
 *
 * \note If #HSPI_WITHOUT_MISO is defined the driver will not mux GPIO12 to
 *       the SPI1 and that pin would also become available for CS demuxing
 *       (of for any other GPIO needs).
 *
 * For example, if GPIO4 and GPIO5 are used to demux SPI CS for up to 4 devices
 * a program would use:
 * \code
 * #define HSPI_CS_DEMUX_GPIO_PINS (BIT(4) | BIT(5))
 * \endcode
 *
 * \note If this "list" is not defined a driver will only work with a single
 *       device which CS pin is directly connected to the CS0 ESP pin.
 */

/**
 * \def   HSPI_WITHOUT_MISO
 * \brief Configures the output only version of the driver.
 *
 * \note  This is a global setting. It is used to relinquish MISO pin
 *        if none of the attached devices need it.
 *
 *        When defined, the driver will support only output (MOSI) and
 *        input only for devices that use a single/shared I/O line.
 *        GPIO12 will remain in its default configuration unless it is
 *        listed in #HSPI_CS_DEMUX_GPIO_PINS.
 */

/**
 * \brief Slave device descriptor
 *
 * From the driver point of view the device descriptor is an opaque entity
 * and is viewed as a trait. The application is free to define it any way
 * it sees fit as long as it implements the "trait's" methods (those
 * listed in hspi_dev_impl.h).
 *
 * \note hspi_config.h only defines methods relevant for the SPI communication.
 *       The drivers for actual slave devices would require additional methods
 *       implemented (i.o.w. require `hspi_dev_t` to provide functionality for
 *       the traits they need).
 */
typedef uintptr_t hspi_dev_t;
/**
 * \brief State of demultiplexer select lines.
 * \param dev Slave device descriptor
 * \return a set of pins that should be set high to demux CS for this device.
 *
 * Driver masks the returned value with #HSPI_CS_DEMUX_GPIO_PINS and then changes
 * the state of the select lines during #hspi_select using the masked set of pins.
 * 
 * \note  this function does not need to be implemented when #HSPI_CS_DEMUX_GPIO_PINS
 *        is undefined.
 */
static inline uint32_t hspi_dev_demux_cs(hspi_dev_t dev)
{
    return 0;
}

/**
 * \brief SPI clock settings.
 * \param dev Slave device descriptor
 * \return a value that #hspi_select will write into the CLOCK register
 *
 * \note If the return value has `SPI_CLOCK_EQU_SYS_CLOCK` bit set, then
 *       #hspi_select will configure SPI clock to equal system clock.
 */
static inline uint32_t hspi_dev_clock(hspi_dev_t dev)
{
    // return 10MHz clock with equal high and low pulse width
    return VAL2FIELD(SPI_CLOCK_DIV_PRE,0) | VAL2FIELD(SPI_CLOCK_COUNT_NUM,7) | VAL2FIELD(SPI_CLOCK_COUNT_HIGH,3);
}

/**
 * \brief SPI transfer mode
 * \param dev Slave device descriptor
 * \return 0..3
 *   0 - clock idle state is low, data are captured on the clock's leading edge (low-to-high transition)
 *   1 - clock idle state is low, data are captured on the clock's trailing edge (high-to-low transition)
 *   2 - clock idle state is high, data are captured on the clock's leading edge (high-to-low transition)
 *   3 - clock idle state is high, data are captured on the clock's trailing edge (low-to-high transition)
 * \see for example, http://dlnware.com/theory/SPI-Transfer-Modes for details
 */
static inline uint32_t hspi_dev_transfer_mode(hspi_dev_t dev)
{
    return 0;
}

/**
 * \brief Bit order used by the slave device
 * \param dev Slave device descriptor
 * \return `true` if device expects MSB bit order and `false` for an LSB device.
 *
 * \note While ESP SPI allows different bit order settings for input and output
 *       the driver configures them to be the same. This might change one day
 *       if there is a use case for it.
 */
static inline bool hspi_dev_is_msb(hspi_dev_t dev)
{
    // use MSB bit order
    return true;
}

/**
 * \brief Hardware vs software CS0 pin control
 * \param dev Slave device descriptor
 * \return `true` if the application will control CS0 pin via software
 *
 * Used by devices that have atypical CS usage patterns like SD/MMC.
 */
static inline bool hspi_dev_software_cs(hspi_dev_t dev)
{
    return false;
}

/**
 * \brief Whether device uses the same pin for both input and output
 * \param dev Slave device descriptor
 * \return `true` if device has only one pin that used for both input and putput.
 *
 * \see ST7735 for an example
 */
static inline bool hspi_dev_shared_io(hspi_dev_t dev)
{
    return false;
}
