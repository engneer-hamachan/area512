idf_component_register(SRCS "bootloader_start.c"
                    REQUIRES bootloader bootloader_support)

idf_build_get_property(scripts BOOTLOADER_LINKER_SCRIPT)
target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")
