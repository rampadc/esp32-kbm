idf_component_register(
    SRCS "main.c" 
    "init_nvs.c" "init_bluetooth.c" "init_console.c" 
    "esp_hidd_prf_api.c"
    "hid_dev.c"
    "hid_device_le_prf.c"
    INCLUDE_DIRS "."
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable -Wno-error=switch)