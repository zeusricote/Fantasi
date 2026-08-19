/* Fantasi / LilyGO T-Embed (ESP32-S3) - HAL glue.
 *
 * Wires the ESP32-S3 USB-OTG peripheral to TinyUSB (CDC serial for the
 * CLI) and exposes the ESP-IDF heap/flash/device identity through the
 * shared Fantasi HAL contract (hal/hal.h).
 *
 * Minimal bringup milestone: USB CDC + LittleFS storage partition + boot.
 * BLE, HID, MSC, buttons and the display are deliberately stubbed until
 * their follow-up milestones.
 */

#include "hal.h"
#include "hal_name.h"

#include "esp_log.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_efuse.h"

#include "tinyusb.h"
#include "esp_tinyusb.h"
#include "flash_storage.h"

#include "FreeRTOS.h"
#include "task.h"

static const char *TAG = "tembed_hal";

/* ---- USB CDC task (TinyUSB event pump) ----
 * ESP-IDF's tinyusb component starts its own tud_task after
 * tinyusb_driver_install(), so no extra task is needed here. Raw
 * tud_cdc_* calls below drive the shared CLI transport. */

/* ---- Boot / init ---- */

void hal_init(void)
{
    /* Bring up the LittleFS storage partition (platforms/tembed/partitions.csv). */
    if (storage_flash_init() != 0) {
        ESP_LOGE(TAG, "storage init failed - check partition table");
    }

    /* Start TinyUSB CDC. The descriptor in ESP-IDF's component already
     * provides CDC + MSC by default; we only need CDC for this milestone. */
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor = NULL,     /* ESP-IDF defaults (VID:PID configurable later) */
        .string_descriptor = NULL,
        .external_phy = false,
        .configuration_descriptor = NULL,
    };
    esp_err_t err = tinyusb_driver_install(&tusb_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "tinyusb_driver_install failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "TinyUSB CDC up");
    }
}

void hal_post_init(void)
{
    /* Nothing extra for this milestone. */
}

/* ---- Serial (CDC) ---- */

size_t hal_serial_write(const uint8_t *buf, size_t len)
{
    if (!tud_cdc_connected()) return 0;
    size_t wrote = tud_cdc_write(buf, len);
    tud_cdc_write_flush();
    return wrote;
}

size_t hal_serial_read(uint8_t *buf, size_t len)
{
    if (!tud_cdc_available()) return 0;
    return tud_cdc_read(buf, len);
}

bool hal_serial_connected(void)
{
    return tud_cdc_connected();
}

/* Poll-based wait: the shared core's CLI falls back to a 5 ms poll when
 * no RX-event transport is wired. Fine for this bring-up milestone. */
void hal_serial_wait(uint32_t timeout_ms)
{
    uint32_t ms = timeout_ms > 5 ? 5 : timeout_ms;
    if (ms) vTaskDelay(pdMS_TO_TICKS(ms));
}

/* ---- Heap ---- */

size_t hal_free_heap_bytes(void)
{
    return (size_t)xPortGetFreeHeapSize();
}

size_t hal_min_ever_free_heap_bytes(void)
{
    return (size_t)xPortGetMinimumEverFreeHeapSize();
}

/* ---- Device identity ---- */

const char *hal_device_id(void) { return "TMB"; }

const char *hal_device_name(void)
{
    static char name[16];
    if (name[0]) return name;

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    uint32_t u[2] = { (uint32_t)mac[0] << 24 | mac[1] << 16 | mac[2] << 8 | mac[3],
                      (uint32_t)mac[4] << 24 | mac[5] << 16 };
    hal_name_generate(u, 2, name, sizeof(name));
    return name;
}

/* ---- Flash ---- */

int32_t hal_flash_free_bytes(void)
{
    /* LittleFS partition size - firmware lives in a separate factory
     * partition, so the whole littlefs region is usable storage. */
    const esp_partition_t *p = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "littlefs");
    return p ? (int32_t)p->size : 0;
}

/* ---- Battery ---- */

int hal_battery_percent(void) { return -1; }   /* no battery sense in this milestone */

/* ---- Power / reboot ---- */

void hal_reboot(void) { esp_restart(); for (;;); }

void hal_set_dfu_magic(void) { /* ESP-IDF bootloader not hooked yet */ }

void hal_reboot_dfu(void) { esp_restart(); for (;;); }

int hal_shutdown(void) { return HAL_SHUTDOWN_UNSUPPORTED; }

bool hal_shutdown_button_held(void) { return false; }

/* ---- Memory regions ---- */

int hal_mem_regions(hal_mem_region_t *out, int max)
{
    int n = 0;
    if (n < max) {
        out[n].name  = "SRAM";
        out[n].total = (uint32_t)heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        out[n].free  = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        out[n].note  = NULL;
        n++;
    }
    return n;
}

int hal_test_regions(hal_test_region_t *out, int max)
{
    (void)out; (void)max;
    return 0;   /* not implemented for this milestone */
}

/* ---- Radio coprocessor / BLE (stub) ---- */

void hal_radio_info(hal_radio_info_t *info)
{
    if (info) {
        info->available = false;
        info->secure_flash_start = 0;
        info->secure_flash_kb = 0;
        info->fus_major = info->fus_minor = info->fus_sub = 0;
        info->ws_major = info->ws_minor = info->ws_sub = 0;
        info->ws_type = 0;
    }
}

void hal_ble_activate_fus(void) {}

/* ---- USB modes (composite isn't wired yet) ---- */

int hal_enter_msc_mode(void)    { return -1; }
int hal_enter_webusb_mode(void) { return -1; }
int hal_enter_cdc_mode(void)    { return -1; }

/* ---- HID / MSC toggles (stub) ---- */

int      hal_hid_enable(int on)      { (void)on; return -1; }
int      hal_hid_send(uint8_t m, const uint8_t *k, uint8_t n) { (void)m; (void)k; (void)n; return -1; }
uint32_t hal_hid_host(void)          { return 0; }
void     hal_hid_set_persistent(bool p) { (void)p; }
void     hal_msc_set_enabled(bool e) { (void)e; }
void     hal_usb_reenumerate(void)   { }