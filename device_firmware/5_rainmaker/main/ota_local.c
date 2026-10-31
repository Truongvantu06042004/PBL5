/* Local-host OTA command for the OTA practice exercise.
 *
 * This is deliberately separate from RainMaker OTA: the board downloads a
 * .bin file from a HTTP server on the same LAN after the user enters
 * `ota-local http://<computer-ip>:8000/5_rainmaker.bin` in the serial console.
 * HTTP is suitable only for this trusted classroom/demo network.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_console.h"
#include "esp_err.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"

#include "ota_local.h"

static const char *TAG = "ota_local";
static bool s_ota_in_progress;

static void ota_local_task(void *arg)
{
    char *url = arg;
    esp_http_client_config_t http_config = {
        .url = url,
        .timeout_ms = 15000,
        .keep_alive_enable = true,
    };
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
    };

    ESP_LOGW(TAG, "Starting local-host OTA from %s", url);
    esp_err_t err = esp_https_ota(&ota_config);
    free(url);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Local-host OTA failed: %s", esp_err_to_name(err));
        s_ota_in_progress = false;
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGW(TAG, "Local-host OTA successful. Rebooting into the new firmware.");
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

static int ota_local_command(int argc, char **argv)
{
    if (argc != 2) {
        ESP_LOGI(TAG, "Usage: ota-local http://<computer-ip>:8000/5_rainmaker.bin");
        return ESP_ERR_INVALID_ARG;
    }
    if (strncmp(argv[1], "http://", strlen("http://")) != 0) {
        ESP_LOGE(TAG, "Only a local http:// URL is accepted by this demo command.");
        return ESP_ERR_INVALID_ARG;
    }
    if (s_ota_in_progress) {
        ESP_LOGW(TAG, "An OTA operation is already in progress.");
        return ESP_ERR_INVALID_STATE;
    }

    char *url = strdup(argv[1]);
    if (!url) {
        return ESP_ERR_NO_MEM;
    }

    s_ota_in_progress = true;
    BaseType_t task_created = xTaskCreate(ota_local_task, "ota_local", 8192,
            url, 5, NULL);
    if (task_created != pdPASS) {
        s_ota_in_progress = false;
        free(url);
        ESP_LOGE(TAG, "Could not create the local OTA task.");
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void ota_local_register_command(void)
{
    const esp_console_cmd_t command = {
        .command = "ota-local",
        .help = "Download a firmware image from a local HTTP server. Usage: ota-local <url>",
        .func = ota_local_command,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&command));
}
