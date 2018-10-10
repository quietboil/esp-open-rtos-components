sntp_time_SRC_DIR = $(sntp_time_ROOT)
INC_DIRS += $(sntp_time_ROOT)
$(eval $(call component_compile_rules,sntp_time))
