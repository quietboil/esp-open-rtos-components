PROGRAM_OFFSET ?= 0x2000
USE_BEARSSL ?= 0

ota_update_SRC_DIR = $(ota_update_ROOT)
INC_DIRS += $(ota_update_ROOT)
EXTRA_CFLAGS += \
	-D ota_update_ROOT=$(ota_update_ROOT) \
	-D PROGRAM_OFFSET=$(PROGRAM_OFFSET) \
	-D USE_BEARSSL=$(USE_BEARSSL)
$(eval $(call component_compile_rules,ota_update))
