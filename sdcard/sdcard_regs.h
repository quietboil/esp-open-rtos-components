/**
 * \file  sdcard_regs.h
 * \brief CID and CSD registers definitions.
 */
#ifndef __SDCARD_REGS_H
#define __SDCARD_REGS_H

#include <stdint.h>

/**
 * \brief Card Identification Register
 * \note  #sdcard_read_cid saves CID in little-endian byte order.
 *        The struct layout reflects that.
 */
typedef struct {
    uint8_t mid;                ///< Manufacturer ID
    uint8_t oid[2];             ///< OEM/Application ID (2 ASCII characters)
    uint8_t pnm[5];             ///< Product Name (5 ASCII characters)
    uint8_t rev_min       : 4;  ///< Product Revision (minor), sometimes FW revision
    uint8_t rev_maj       : 4;  ///< Product Revision (major), sometimes HW revision
    uint8_t serial[4];          ///< 32-bit big endian integer
    uint8_t mdt_year_high : 4;  ///< High nibble of the year in the manufacture date code yym
    uint8_t reserved      : 4;
    uint8_t mdt_month     : 4;  ///< Month number in the manufacture date code yym
    uint8_t mdt_year_low  : 4;  ///< Low nibble of the year in the manufacture date code yym
    uint8_t stop_bit      : 1;  ///< Always 1
    uint8_t crc           : 7;
} sdcard_cid_t;

_Static_assert(sizeof(sdcard_cid_t) == 16, "CID struct is not packed correctly");

/**
 * \brief Card Specific Data Register
 * \note  While #sdcard_read_csd receives CSD in little-endian byte format,
 *        it swaps received bytes to the big-endian order as some of the fields
 *        cross bytes and interpretation otheriwse would be quite cumbersome.
 */
typedef struct {
    uint32_t stop_bit               : 1; ///< Always 1
    uint32_t crc                    : 7;
    uint32_t reserved_bits_8_9	    : 2;
    // FILE_FORMAT [10:11]
    uint32_t file_format            : 2; ///< V1: card’s file format:
                                         ///<   FILE_FORMAT_GROUP 0:
                                         ///<       0: Hard disk-like file system with partition table
                                         ///<       1: DOS FAT (floppy-like) w/boot sector only
                                         ///<       2: Universal file format
                                         ///<       3: Others/unknown.
                                         ///<   FILE_FORMAT_GROUP 1:
                                         ///<       0,1,2,3: reserved
                                         ///< V2: This field is set to 0. Host should not use this field.
    // TMP_WRITE_PROTECT [12] R/W
    uint32_t temp_write_protect     : 1; ///< temporarily protects the entire card contents against overwriting or erasing
                                         ///< This bit can be set and reset.
    // PERM_WRITE_PROTECT [13] R/W
    uint32_t perm_write_protect     : 1; ///< permanently protects the entire card contents against overwriting or erasing
                                         ///< (all write and erase commands for this card are permanently disabled).
    // COPY [14] R/W
    uint32_t copy                   : 1; ///< 0: original, 1: non-original.
                                         ///< Once set to non-original, this bit cannot be reset to original.
                                         ///< The definition of "original" and "non-original" is implementation dependent.
    // FILE_FORMAT_GROUP [15] R/W
    uint32_t file_format_group	    : 1; ///< v1: The selected group of file formats
                                         ///< v2: This field is set to 0. Host should not use this field.
    uint32_t reserved_bits_16_20	: 5;
    // WRITE_BLK_PARTIAL [21]
    uint32_t write_block_partial    : 1; ///< 0: Only the WRITE_BL_LEN block size, and its partial derivatives in
                                         ///< units of 512-byte blocks, can be used for block data write.
                                         ///< 1: Smaller blocks can be used as well. The minimum block size == minimum addressable unit
                                         ///< For SDHC and SDXC cards this field is fixed to 0, which indicates partial
                                         ///< block read is inhibited and only unit of block access is allowed
    // WRITE_BL_LEN [22:25]
    uint32_t write_block_length	    : 4; ///< 0..8=reserved, 9=2^9=512, 10=1024, 11=2048, 12..15=reserved.
                                         ///< A 512-byte write block length is always supported
                                         ///< In the SD Memory Card the WRITE_BL_LEN is always equal to READ_BL_LEN
                                         ///< For SDHC and SDXC cards this field is fixed to 9h, which indicates
                                         ///< WRITE_BL_LEN=512 Bytes.
    // R2W_FACTOR [26:28]
    uint32_t write_speed_factor	    : 3; ///< The typical block program time as a multiple of the read access time
                                         ///< 0=1, 1=2 (write half as fast as read), 2=4, ... 5=32, 6..7 - reserved
                                         ///< For SDHC and SDXC cards this field is fixed to 2h, which indicates 4
                                         ///< multiples.
    uint32_t reserved_bits_29_30	: 2;
    // WP_GRP_ENABLE [31]
    uint32_t write_protect_enabled  : 1; ///< A value of “0” means group write protection is not possible
    union {
        struct {
            // WP_GRP_SIZE [32:38]
            uint32_t write_protect_size     : 7; ///< The number of Erase Groups (see SECTOR_SIZE). Actual number is this number + 1.
            // SECTOR_SIZE [39:45]
            uint32_t erase_sector_size      : 7; ///< The number of write blocks (see WRITE_BL_LEN). Actual number is this number + 1.
            // ERASE_BLK_EN [46]
            uint32_t erase_block_enabled    : 1; ///< 0=Host can erase a SECTOR_SIZE unit,
                                                 ///< 1=Host can erase either a SECTOR_SIZE unit or a WRITE_BLK_LEN unit.
            // C_SIZE_MULT [47:49]
            uint32_t device_size_multiplier : 3; ///< 0=4, 1=8, 2=16, ..., 6=256, 7=512
            // VDD_W_CURR_MAX [50:52]
            uint32_t max_write_current      : 3; ///< 0=0.5ma, 1=1, 2=5, 3=10, 4=25, 5=35, 6=60, 7=100ma
            // VDD_W_CURR_MIN [53:55]
            uint32_t min_write_current      : 3; ///< 0=0.5ma, 1=1, 2=5, 3=10, 4=25, 5=35, 6=60, 7=100ma
            // VDD_R_CURR_MAX [56:58]
            uint32_t max_read_current       : 3; ///< 0=0.5ma, 1=1, 2=5, 3=10, 4=25, 5=35, 6=60, 7=100ma
            // VDD_R_CURR_MIN [59:61]
            uint32_t min_read_current       : 3; ///< 0=0.5ma, 1=1, 2=5, 3=10, 4=25, 5=35, 6=60, 7=100ma
            // C_SIZE [62:63 of 62:73]
            uint32_t device_size_low        : 2;
            // C_SIZE [64:73 of 62:73]
            uint32_t device_size_high       : 10;
            uint32_t reserved_bits_74_75    : 2;
            // DSR_IMP [76]
            uint32_t dsr_implemented        : 1; ///< if set, then configurable driver state (and register) is implemented
            // READ_BLK_MISALLIGN [77]
            uint32_t read_block_misalign    : 1; ///< if set, data block read by one command can be spread over more then one physical block
            // WRITE_BLK_MISALLIGN [78]
            uint32_t write_block_misalign   : 1; ///< if set, data block written by one command can be spread over more then one physical block
            // READ_BLK_PARTIAL [79]
            uint32_t read_block_partial     : 1; ///< 0: Only the READ_BL_LEN block size can be used for block data transfers
                                                ///< 1: Smaller blocks can be used. The minimum block size will be equal to minimum addressable unit
            // READ_BL_LEN [80:83]
            uint32_t max_read_block_len     : 4; ///< 0..8=reserved, 9=2^9=512, 10=1024, 11=2048, 12..15=reserved
                                                ///< In SD memory card write block length always == read block length
            // CCC [84:95]
            uint32_t card_command_classes   : 12; ///< bit 0 = Class 0, ..., bit 11 = Class 11
        } v1;
        struct {
            // WP_GRP_SIZE [32:38]
            uint32_t write_protect_size     : 7; ///< This field is fixed to 00h. SDHC and SDXC Cards do not support write protected groups.
            // SECTOR_SIZE [39:45]
            uint32_t erase_sector_size      : 7; ///< This field is fixed to 7Fh, which indicates 64 KBytes.
                                                 ///< This value is not related to erase operation. This field should not be used
                                                 ///< for SDHC and SDXC cards.
            // ERASE_BLK_EN [46]
            uint32_t erase_block_enabled    : 1; ///< This field is fixed to 1, which means the host can erase one or multiple units of 512 bytes
            uint32_t reserved_bit_47        : 1;
            // C_SIZE [48:63 of 48:69]
            uint32_t device_size_low        : 16;
            // C_SIZE [64:69 of 48:69]
            uint32_t device_size_high       : 6; ///< The user data area capacity = (C_SIZE + 1) * 512 KB
            uint32_t reserved_bits_70_75    : 2;
            // DSR_IMP [76]
            uint32_t dsr_implemented        : 1; ///< if set, then configurable driver state (and register) is implemented
            // READ_BLK_MISALLIGN [77]
            uint32_t read_block_misalign    : 1; ///< This field is fixed to 0, which indicates that read access crossing physical
                                                 ///< block boundaries is always disabled in SDHC and SDXC Cards
            // WRITE_BLK_MISALLIGN [78]
            uint32_t write_block_misalign   : 1; ///< This field is fixed to 0, which indicates that write access crossing physical
                                                 ///< block boundaries is always disabled in SDHC and SDXC Cards
            // READ_BLK_PARTIAL [79]
            uint32_t read_block_partial     : 1; ///< This field is fixed to 0, which indicates partial block read is inhibited
                                                 ///< and only unit of block access is allowed
            // READ_BL_LEN [80:83]
            uint32_t max_read_block_len     : 4; ///< This field is fixed to 9h, which indicates READ_BL_LEN=512 Bytes
            // CCC [84:95]
            uint32_t card_command_classes   : 12; ///< bit 0 = Class 0, ..., bit 11 = Class 11
        } v2;
    };
    // TRANS_SPEED [96:103]
    uint32_t transfer_rate_exponent : 3; ///< 0=100kb/s, 1=1Mb/s, 2=10Mb/s, 3=100Mb/s, 4..7=reserved
    uint32_t transfer_rate_mantissa : 4; ///< 0=reserved, 1=1.0, 2=1.2, 3=1.3, 4=1.5, 5=2.0, 6=2.5, 7=3.0, 8=3.5, 9=4.0, A=4.5, B=5.0, C=5.5, D=6.0, E=7.0, F=8.0
    uint32_t reserved_bit_103       : 1;
    // NSAC [104:111]
    uint32_t nsac                   : 8; ///< V1: Data read access time 2 (CLK cycles NSAC*100)
                                         ///<     Typical delay for the first data bit of the data block from the end bit on the read command
                                         ///< V2: For SDHC and SDXC cards this field is fixed to 00h.
                                         ///<     NSAC should not be used to calculate time-out values
    // TAAC [112:119]
    uint32_t taac_time_exponent     : 3; ///< V1: 0=1ns, 1=10ns, 2=100ns, 3=1us, 4=10us, 5=100us, 6=1ms, 7=10ms
    uint32_t taac_time_value        : 4; ///< V1: 0=reserved, 1=1.0, 2=1.2, 3=1.3, 4=1.5, 5=2.0, 6=2.5, 7=3.0, 8=3.5, 9=4.0, A=4.5, B=5.0, C=5.5, D=6.0, E=7.0, F=8.0
    uint32_t reserved_taac_bit_119  : 1; ///< V2: For SDHC and SDXC cards this field is fixed to 0Eh, which indicates 1 ms
    uint32_t reserved_bits_120_125  : 6;
    // CSD_STRUCTURE [126:127]
    uint32_t version                : 2; ///< 0=V1, 1=V2, 2..3=reserved
} sdcard_csd_t;

_Static_assert(sizeof(sdcard_csd_t) == 16, "CSD struct is not packed correctly");

#endif