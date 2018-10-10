tftp_vfs_SRC_DIR = $(tftp_vfs_ROOT)
INC_DIRS += $(tftp_vfs_ROOT)
$(eval $(call component_compile_rules,tftp_vfs))
