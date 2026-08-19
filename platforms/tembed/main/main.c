/* Fantasi / LilyGO T-Embed (ESP32-S3) - main entry point.
 *
 * Minimal bringup milestone: initialise the HAL (USB CDC + LittleFS
 * storage partition) and start a simple interactive CLI over the USB
 * CDC console. The full Fantasi core (VFS, RAMFS, app runner, proto)
 * is wired in a follow-up milestone.
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "hal.h"

static const char *TAG = "fantasi";

/* Simple line-based CLI over the USB CDC transport. */
static void cli_task(void *arg)
{
    (void)arg;
    char line[128];
    size_t len = 0;

    for (;;) {
        uint8_t b;
        if (hal_serial_read(&b, 1) == 0) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        if (b == '\r' || b == '\n') {
            if (len > 0) {
                line[len] = '\0';
                len = 0;
                /* Echo + handle a few basic commands. */
                hal_serial_write((const uint8_t *)"\r\n", 2);
                if (strcmp(line, "help") == 0) {
                    const char *msg =
                        "Fantasi T-Embed (ESP32-S3) - minimal bringup\r\n"
                        "  help     this message\r\n"
                        "  device   show device id\r\n"
                        "  name     show device name\r\n"
                        "  free     show free heap\r\n"
                        "  reboot   restart the device\r\n";
                    hal_serial_write((const uint8_t *)msg, strlen(msg));
                } else if (strcmp(line, "device") == 0) {
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "device: %s\r\n", hal_device_id());
                    hal_serial_write((const uint8_t *)buf, n);
                } else if (strcmp(line, "name") == 0) {
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "name: %s\r\n", hal_device_name());
                    hal_serial_write((const uint8_t *)buf, n);
                } else if (strcmp(line, "free") == 0) {
                    char buf[64];
                    int n = snprintf(buf, sizeof(buf), "free heap: %u bytes\r\n",
                                     (unsigned)hal_free_heap_bytes());
                    hal_serial_write((const uint8_t *)buf, n);
                } else if (strcmp(line, "reboot") == 0) {
                    hal_serial_write((const uint8_t *)"rebooting...\r\n", 14);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    hal_reboot();
                } else {
                    const char *msg = "unknown command (try 'help')\r\n";
                    hal_serial_write((const uint8_t *)msg, strlen(msg));
                }
                const char *prompt = "fantasi> ";
                hal_serial_write((const uint8_t *)prompt, strlen(prompt));
            }
        } else if (b >= 0x20 && b <= 0x7E && len < sizeof(line) - 1) {
            line[len++] = (char)b;
            hal_serial_write(&b, 1);   /* echo */
        }
    }
}

void app_main(void)
{
    esp_err_t err;

    /* Initialise NVS (used by the flash storage layer). */
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Fantasi starting on LilyGO T-Embed (ESP32-S3)");

    /* Initialise the platform HAL (USB CDC + storage partition). */
    hal_init();

    /* Start the interactive CLI over USB CDC. */
    xTaskCreate(cli_task, "fantasi_cli", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Fantasi ready - connect over USB CDC");
}