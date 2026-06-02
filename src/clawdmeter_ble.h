#pragma once

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialise the Clawdmeter BLE GATT service.
 * Must be called after BLE power-on (sifli_ble_enable).
 * Returns 0 on success.
 */
int cm_ble_init(void);

/**
 * Send a JSON string to the connected host via TX notification.
 * Returns 0 on success, -1 if not connected or notify disabled.
 */
int cm_ble_send_json(const char *json);

/**
 * Request the host to refresh data (sends a notification on the Request characteristic).
 */
void cm_ble_request_refresh(void);

/**
 * Returns true if a BLE client is connected and has TX notifications enabled.
 */
bool cm_ble_is_connected(void);

/**
 * Get the current BLE device name (set after advertising starts).
 */
const char *cm_ble_device_name(void);

/**
 * Get the BLE MAC address as "XX:XX:XX:XX:XX:XX" string.
 * Returns empty string if not yet available.
 */
const char *cm_ble_address(void);

/**
 * Process BLE events from the mailbox. Call periodically.
 */
void cm_ble_poll(void);
