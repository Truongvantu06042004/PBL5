#pragma once

/**
 * Register the `ota-local <http-url>` serial command used by the local-host
 * OTA demonstration. The downloaded file must be an ESP32-C3 application
 * image produced by this project.
 */
void ota_local_register_command(void);
