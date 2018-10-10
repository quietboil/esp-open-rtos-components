-include local.mk

# Use esptool defaults for FLASH attributes there weren't specified
FLASH_SIZE  ?= detect
FLASH_SPEED ?= 40m
FLASH_MODE  ?= qio

# Assume stock esp-open-rtos linker script
PROGRAM_OFFSET ?= 0x2000

# Use the safe flashing speed
ESPBAUD ?= 115200

ESPTOOL_ARGS ?= -fs $(FLASH_SIZE) -ff $(FLASH_SPEED) -fm $(FLASH_MODE)

CC = xtensa-lx106-elf-gcc
LD = xtensa-lx106-elf-gcc
AR = xtensa-lx106-elf-ar

CFLAGS  = -std=c99 -O2 -mlongcalls -D IMG_OFFSET=$(PROGRAM_OFFSET)
LDFLAGS = -L $(CURDIR) -nostdlib
LDLIBS  =

all: boot-0x00000.bin

boot-0x00000.bin: boot
	esptool.py elf2image $(ESPTOOL_ARGS) $^

boot: boot.o
	$(LD) $(LDFLAGS) -T boot.ld $^ -o $@

flash: boot-0x00000.bin
	esptool.py -b $(ESPBAUD) write_flash $(ESPTOOL_ARGS) 0x00000 boot-0x00000.bin

clean:
	rm -f boot *.o *.bin
