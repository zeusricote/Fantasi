/* Fantasi / LilyGO T-Embed (ESP32-S3) - BLE stubs.
 *
 * The ESP32-S3 has an integrated 2.4 GHz radio, but BLE support is a
 * follow-up milestone (ESP-IDF NimBLE). For now every hal_ble_* entry
 * returns a sentinel so the shared CLI's scan/pair/radio commands
 * degrade gracefully at runtime.
 */

#include "hal.h"

int hal_ble_scan(hal_ble_scan_cb_t cb, uint32_t duration_ms)
{
    (void)cb; (void)duration_ms;
    return -1;
}

int hal_ble_pair_setup(uint8_t io_cap) { (void)io_cap; return -1; }
void hal_ble_pair_begin(void) {}
void hal_ble_pair_end(void) {}
int hal_ble_pair_connect(const uint8_t *addr, uint8_t addr_type)
{ (void)addr; (void)addr_type; return -1; }
void hal_ble_shutdown(void) {}
bool hal_ble_is_active(void) { return false; }
int hal_ble_pair_initiate(uint16_t conn_handle) { (void)conn_handle; return -1; }
int hal_ble_pair_passkey(uint16_t conn_handle, uint32_t passkey)
{ (void)conn_handle; (void)passkey; return -1; }
int hal_ble_pair_confirm(uint16_t conn_handle, bool accept)
{ (void)conn_handle; (void)accept; return -1; }
int hal_ble_pair_wait(hal_ble_evt_t *evt, uint32_t timeout_ms)
{ (void)evt; (void)timeout_ms; return -1; }
int hal_ble_disconnect(uint16_t conn_handle) { (void)conn_handle; return -1; }
uint32_t hal_ble_generate_passkey(void) { return 0; }
int hal_ble_connections(hal_ble_conn_info_t *out, int max)
{ (void)out; (void)max; return 0; }
int hal_ble_get_bonded(hal_ble_bonded_t *out, int max)
{ (void)out; (void)max; return 0; }
int hal_ble_remove_bond(const uint8_t *addr, uint8_t addr_type)
{ (void)addr; (void)addr_type; return -1; }
int hal_ble_clear_bonds(void) { return -1; }