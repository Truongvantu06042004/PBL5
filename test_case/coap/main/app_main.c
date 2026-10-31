/* ESP32-C3 HTTP and CoAP light controller */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "esp_http_server.h"

#include "coap3/coap.h"

#include "app_priv.h"
#include "app_storage.h"

#define LIGHT_ESP_WIFI_SSID     "Tan"
#define LIGHT_ESP_WIFI_PASS     "123456789"
#define LIGHT_ESP_MAXIMUM_RETRY 5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "coap";

static EventGroupHandle_t s_wifi_event_group = NULL;
static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < LIGHT_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_initialize(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
}

static void wifi_station_initialize(void)
{
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = LIGHT_ESP_WIFI_SSID,
            .password = LIGHT_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_station_initialize finished.");

    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 LIGHT_ESP_WIFI_SSID, LIGHT_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 LIGHT_ESP_WIFI_SSID, LIGHT_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

static const char WEB_PAGE[] =
    "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<title>ESP32 LIGHT CONTROL</title></head>"
    "<body style=\"font-family:Arial;text-align:center;margin-top:60px\">"
    "<h1>ESP32 LIGHT CONTROL</h1>"
    "<p><a href=\"/on\"><button style=\"font-size:24px;padding:16px 32px\">BẬT ĐÈN</button></a></p>"
    "<p><a href=\"/off\"><button style=\"font-size:24px;padding:16px 32px\">TẮT ĐÈN</button></a></p>"
    "</body></html>";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, WEB_PAGE, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t light_on_get_handler(httpd_req_t *req)
{
    app_driver_set_state(true);
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t light_off_get_handler(httpd_req_t *req)
{
    app_driver_set_state(false);
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    return httpd_resp_send(req, NULL, 0);
}

static void web_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    static const httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = NULL,
    };

    static const httpd_uri_t light_on_uri = {
        .uri = "/on",
        .method = HTTP_GET,
        .handler = light_on_get_handler,
        .user_ctx = NULL,
    };

    static const httpd_uri_t light_off_uri = {
        .uri = "/off",
        .method = HTTP_GET,
        .handler = light_off_get_handler,
        .user_ctx = NULL,
    };

    ESP_ERROR_CHECK(httpd_start(&server, &config));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &root_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &light_on_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &light_off_uri));

    ESP_LOGI(TAG, "Web server started");
}

static void coap_light_put_handler(coap_resource_t *resource,
                                   coap_session_t *session,
                                   const coap_pdu_t *request,
                                   const coap_string_t *query,
                                   coap_pdu_t *response)
{
    size_t size = 0;
    const uint8_t *data = NULL;

    if (!coap_get_data(request, &size, &data)) {
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
        return;
    }

    if (size == 2 && memcmp(data, "ON", 2) == 0) {
        ESP_LOGI(TAG, "CoAP command: ON");
        app_driver_set_state(true);
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
    } else if (size == 3 && memcmp(data, "OFF", 3) == 0) {
        ESP_LOGI(TAG, "CoAP command: OFF");
        app_driver_set_state(false);
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);
    } else {
        ESP_LOGW(TAG, "CoAP command is invalid");
        coap_pdu_set_code(response, COAP_RESPONSE_CODE_BAD_REQUEST);
    }
}

static void coap_server_task(void *arg)
{
    coap_address_t server_address;
    coap_context_t *context;
    coap_endpoint_t *endpoint;
    coap_resource_t *light_resource;

    coap_startup();

    coap_address_init(&server_address);
    server_address.addr.sin.sin_family = AF_INET;
    server_address.addr.sin.sin_port = htons(COAP_DEFAULT_PORT);
    server_address.addr.sin.sin_addr.s_addr = htonl(INADDR_ANY);

    context = coap_new_context(NULL);
    if (context == NULL) {
        ESP_LOGE(TAG, "coap_new_context() failed");
        vTaskDelete(NULL);
        return;
    }

    endpoint = coap_new_endpoint(context, &server_address, COAP_PROTO_UDP);
    if (endpoint == NULL) {
        ESP_LOGE(TAG, "coap_new_endpoint() failed");
        coap_free_context(context);
        coap_cleanup();
        vTaskDelete(NULL);
        return;
    }

    light_resource = coap_resource_init(coap_make_str_const("light"), 0);
    if (light_resource == NULL) {
        ESP_LOGE(TAG, "coap_resource_init() failed");
        coap_free_context(context);
        coap_cleanup();
        vTaskDelete(NULL);
        return;
    }

    coap_register_handler(light_resource, COAP_REQUEST_PUT, coap_light_put_handler);
    coap_add_resource(context, light_resource);

    ESP_LOGI(TAG, "CoAP server started on port %d", COAP_DEFAULT_PORT);

    while (true) {
        if (coap_run_once(context, COAP_RESOURCE_CHECK_TIME * 1000) < 0) {
            ESP_LOGE(TAG, "coap_run_once() failed");
            break;
        }
    }

    coap_free_context(context);
    coap_cleanup();
    vTaskDelete(NULL);
}

void app_main(void)
{
    int i = 0;

    ESP_LOGI(TAG, "NVS Flash initialization");
    app_storage_init();

    ESP_LOGI(TAG, "Application driver initialization");
    app_driver_init();

    ESP_LOGI(TAG, "Wi-Fi initialization");
    wifi_initialize();

    ESP_LOGI(TAG, "Wi-Fi Station initialization");
    wifi_station_initialize();

    web_server_start();

    BaseType_t task_created = xTaskCreate(
        coap_server_task,
        "coap_server",
        8192,
        NULL,
        5,
        NULL
    );
    ESP_ERROR_CHECK(task_created == pdPASS ? ESP_OK : ESP_FAIL);

    while (true) {
        ESP_LOGI(TAG, "[%02d] Hello world!", i++);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
