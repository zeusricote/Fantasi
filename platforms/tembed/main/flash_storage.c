/* Fantasi / LilyGO T-Embed (ESP32-S3) flash storage driver.
 *
 * The storage region is the `littlefs` partition in the ESP-IDF partition
 * table (platforms/tembed/partitions.csv): 256 KB of SPI flash. We service
 * it through ESP-IDF's esp_partition API - the ESP32-S3 flash subsystem
 * handles erase/program/read and caching internally.
 */

#include "flash_storage.h"

#include "esp_partition.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "tembed_storage";

/* The LittleFS partition - found at init. */
static const esp_partition_t *s_part;

int storage_flash_init(void)
{
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "littlefs");
    if (!s_part) {
        ESP_LOGE(TAG, "littlefs partition not found - check partitions.csv");
        return -1;
    }

    ESP_LOGI(TAG, "littlefs partition at 0x%lx, size %lu",
             (unsigned long)s_part->address, (unsigned long)s_part->size);
    return 0;
}

uint32_t storage_flash_base(void)
{
    return s_part ? s_part->address : 0;
}

int storage_flash_read(uint32_t offset, void *buf, size_t len)
{
    if (!s_part || offset + len > s_part->size) return -1;
    esp_err_t err = esp_partition_read(s_part, offset, buf, len);
    return err == ESP_OK ? 0 : -1;
}

int storage_flash_erase(uint32_t page_index)
{
    if (!s_part) return -1;
    /* page_index is a 4 KB page index; erase that 4 K region. */
    uint32_t off = page_index * STORAGE_PAGE_SIZE;
    if (off + STORAGE_PAGE_SIZE > s_part->size) return -1;
    esp_err_t err = esp_partition_erase_range(s_part, off, STORAGE_PAGE_SIZE);
    return err == ESP_OK ? 0 : -1;
}

int storage_flash_program(uint32_t offset, const void *buf, size_t len)
{
    if (!s_part || offset + len > s_part->size) return -1;
    /* ESP32 flash programming is byte-granular; no word alignment needed. */
    esp_err_t err = esp_partition_write(s_part, offset, buf, len);
    return err == ESP_OK ? 0 : -1;
}