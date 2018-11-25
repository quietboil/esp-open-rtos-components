/**
 * \file  hspi.h
 * \brief Demux aware user SPI (HSPI) driver
 *
 * Targets environments where ESP8266 drives two or more
 * SPI devices.
 */
#ifndef __HSPI_H
#define __HSPI_H

#include <hspi_config.h>
#include <esp/spi_regs.h>

#define HSPI SPI(1)
/**
 * \brief A "list" of GPIO pins that can be used to select output for CS demux.
 */
#define HSPI_CS_DEMUX_GPIO_PINS_M (BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4) | BIT(5) | BIT(12) | BIT(16))

#if defined(HSPI_CS_DEMUX_GPIO_PINS) && (HSPI_CS_DEMUX_GPIO_PINS & ~HSPI_CS_DEMUX_GPIO_PINS_M)
#error "CS demux selectors list includes pins that cannot be used for CS demuxing"
#endif

#if !defined(HSPI_WITHOUT_MISO) && defined(HSPI_CS_DEMUX_GPIO_PINS) && (HSPI_CS_DEMUX_GPIO_PINS & BIT(12))
#error "GPIO12 cannot be used as demux output selector when MISO is enabled"
#endif

/**
 * \brief Configures HSPI driver.
 * 
 * Creates a mutex to control shared hardware access,
 * configures #HSPI_CS_DEMUX_GPIO_PINS for GPIO output
 * and routes the following IO pins to HSPI:
 *  - MISO = GPIO 12 (unless HSPI_WITHOUT_MISO is defined)
 *  - MOSI = GPIO 13
 *  - SCK  = GPIO 14
 *  - CS0  = GPIO 15
 */
void hspi_init();

/**
 * \brief Configures HSPI to comminicate with the specified device.
 * \param device Device descriptor.
 * 
 * This function also locks an HSPI mutex and thus may not return
 * immediately if at the time of an attemped device selection another
 * device was still actively using HSPI.
 */
void hspi_select(hspi_dev_t device);

/**
 * \brief Releases HSPI for use by other tasks.
 * 
 * Unlocks the HSPI mutex when the task that locked it is done using HSPI.
 */
void hspi_release();

/**
 * \brief Returns current HSPI clock configuration
 */
static inline uint32_t hspi_get_clock()
{
    return HSPI.CLOCK;
}

/**
 * \brief Builds HSPI clock configuration
 * \param div System clock pre-divider (0..8191)
 * \param cnt Number of post-divided ticks (0..63) in an HSPI clock cycle
 * \return value that can be passed to #hspi_set_clock to change the HSPI clock
 * \note  This function does not check the arguments.
 */
static inline uint32_t hspi_new_clock(uint32_t div, uint32_t cnt)
{
    uint32_t clock;
    if (!div && !cnt) {
        clock = SPI_CLOCK_EQU_SYS_CLOCK;
    } else {
        clock = VAL2FIELD(SPI_CLOCK_DIV_PRE, div)
              | VAL2FIELD(SPI_CLOCK_COUNT_NUM, cnt)
              | VAL2FIELD(SPI_CLOCK_COUNT_HIGH, (cnt + 1) / 2)
              ;
    }
    return clock;
}

#define HSPI_CLOCK(div,cnt) hspi_new_clock((div)-1,(cnt)-1)

/**
 * \brief Changes HSPI clock frequency.
 * \param clock Preconfigured content of the HSPI CLOCK register
 */
void hspi_set_clock(uint32_t clock);

/**
 * \brief Starts new SPI transaction
 *
 * Resets settings - command, address, dummy, MISO and MOSI - set by the
 * previous transaction.
 */
void hspi_reset();

/**
 * \brief Sets "command" that will be sent to the device in the current transaction.
 *
 * \param cmd_len Length of the command in bits - 1..16.
 * \param cmd     Command
 */
void hspi_set_command(uint32_t cmd_len, uint16_t cmd);

/**
 * \brief Removes command from the transaction
 */
static inline void hspi_clear_command()
{
    HSPI.USER0 &= ~SPI_USER0_COMMAND;
}

/**
 * \brief Sets "address" that will be sent to the device in the current transaction.
 *
 * \param addr_len Length of the address in bits - 1..32.
 * \param addr     Address
 */
void hspi_set_address(uint32_t addr_len, uint32_t addr);

/**
 * \brief Removes address from the transaction
 */
static inline void hspi_clear_address()
{
    HSPI.USER0 &= ~SPI_USER0_ADDR;
}

/**
 * \brief Sets output (MOSI) data that will be sent to the device in the current transaction.
 *
 * \param data_len Length of the data in bits - 1..512.
 * \param data     Payload
 */
void hspi_set_data(uint32_t data_len, const void * data);

/**
 * \brief Removes data from the transaction
 */
static inline void hspi_clear_data()
{
    HSPI.USER0 &= ~SPI_USER0_MOSI;
}

/**
 * \brief Sets data with a repeated 32-bit pattern
 * \param num_bits Length of the message (1..512)
 * \param pattern  Data
 */
void hspi_set_pattern(uint32_t num_bits, uint32_t pattern);

/**
 * \brief Transaction parameters
 *
 * These are the remaining transaction parameters. Command, address and
 * output data are established using dedicated methods.
 */
typedef struct _hspi_tx {
    uint32_t    big_endian_output :  1; ///< 0: Little, 1: Big endian byte ordering
    uint32_t    dummy_cycles      :  9; ///< Number of dummy cycles (1..256)
    uint32_t    recv_bits         : 10; ///< Number of input bits   (1..512)
    uint32_t    big_endian_input  :  1; ///< 0: Little, 1: Big endian byte ordering
} hspi_tx_t;

/**
 * \brief Sets transaction parameters
 *
 * Caller is expected to set command and/or address and/or data before calling this function.
 *
 * \param tx Transaction parameters.
 */
void hspi_config_exec(hspi_tx_t tx);

/**
 * \brief Starts data transfer.
 */
static inline void hspi_exec()
{
    HSPI.CMD |= SPI_CMD_USR;
}

/**
 * \brief Checks whether HSPI is still executing the previous transaction
 * \return true if HSPI transaction is still in progress
 */
static inline bool hspi_is_busy()
{
    return (HSPI.CMD & SPI_CMD_USR) != 0;
}

/**
 * \brief Waits until HSPI finished the transaction
 */
static inline void hspi_wait()
{
    while (hspi_is_busy()) {
        // spin
    }
}

/**
 * \brief Reads data sent by a slave during in the last transaction.
 *
 * This function just copies data from the SPI work registers into a specified buffer.
 * The actual number of bits sent by a slave was specified in the `hspi_exec` function.
 * It is up to the caller to get all of them, less than received or more than recieved and
 * interpret the copied data accordingly.
 *
 * \param buf_len Length of the buffer in **bytes** (1..64).
 * \param buf     Buffer to copy data to.
 */
void hspi_get_data(uint32_t buf_len, void * buf);

/**
 * \brief Reads a specific word of the slave's response.
 * \param i Index of the 32-bit word to read (0..15)
 */
static inline uint32_t hspi_read(int i)
{
    hspi_wait();
    return HSPI.W[i & 0xf];
}

#endif