# Fantasi T-Embed main component.
#
# This component is the platform HAL + glue. It wires the shared Fantasi
# core (CLI, commands, VFS, storage, log) onto the ESP32-S3 via ESP-IDF's
# FreeRTOS, TinyUSB, and SPI flash stack.

idf_component_register(
    SRCS
        "main.c"
        "hal.c"
        "flash_storage.c"
        "ble_stubs.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS
        "${PROJECT_DIR}/../../core"
        "${PROJECT_DIR}/../../hal"
        "${PROJECT_DIR}/../../hal/storage"
        "${PROJECT_DIR}/../../hal/tinyusb"
        "${PROJECT_DIR}/../../proto"
        "${PROJECT_DIR}/../../apps"
    REQUIRES
        esp_system
        esp_common
        esp_rom
        esp_hw_support
        freertos
        esp_tinyusb
        driver
        nvs_flash
        esp_partition
        heap
        log
        soc
)
