#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "string.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_wifi.h"

#include "mqtt_service.h"
#include "ota_service.h"

typedef struct {
    char version[32];
    char url[256];
} ota_info_t;

static const char *TAG = "ota_service";
static volatile bool ota_running = false;

static esp_err_t parse_ota_json(const char *json, int len, ota_info_t *out)
{
    cJSON *root = cJSON_ParseWithLength(json, len);
    if (root == NULL)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }

    esp_err_t ret = ESP_FAIL;
    cJSON *ver = cJSON_GetObjectItemCaseSensitive(root, "version");
    cJSON *url = cJSON_GetObjectItemCaseSensitive(root, "url");

    if (cJSON_IsString(ver) && cJSON_IsString(url))
    {
        strncpy(out->version, ver->valuestring, sizeof(out->version) - 1);
        strncpy(out->url, url->valuestring, sizeof(out->url) - 1);
        ret = ESP_OK;
    }

    cJSON_Delete(root);
    return ret;
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_HEADERS_COMPLETE:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADERS_COMPLETE");
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void start_https_ota(char *url)
{
    ESP_LOGI(TAG, "Starting OTA task");
    /* Ensure to disable any WiFi power save mode */
    esp_wifi_set_ps(WIFI_PS_NONE);

    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .event_handler = _http_event_handler,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    ESP_LOGI(TAG, "Attempting to download update from %s", config.url);
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "Firmware upgrade failed");
    }

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

void ota_task(void *pvParameters)
{
    ota_info_t *ota_info = (ota_info_t *)pvParameters;

    // 1. Check if OTA is already running
    if (ota_running)
    {
        ESP_LOGW(TAG, "OTA already running, ignoring new request");
        free(ota_info);
        vTaskDelete(NULL);
        return;
    }

    // 2. Check if the version is different from the current version
    const esp_app_desc_t *app_desc = esp_app_get_description();
    if (strcmp(app_desc->version, ota_info->version) == 0)
    {
        ESP_LOGI(TAG, "OTA version is the same as current version, ignoring request");
        free(ota_info);
        vTaskDelete(NULL);
        return;
    }

    // 3. Start OTA
    ota_running = true;
    start_https_ota(ota_info->url);
    free(ota_info);
    ota_running = false;

    vTaskDelete(NULL);
}

/* Callback only parse json and create new task */
void ota_update_callback(const char *topic, const char *data, int data_len, void *arg)
{
    if (data_len <= 0 || data_len > 512)
    {
        ESP_LOGW(TAG, "OTA message invalid length: %d", data_len);
        return;
    }

    ota_info_t *info = malloc(sizeof(ota_info_t));
    if (parse_ota_json(data, data_len, info) != ESP_OK)
    {
        ESP_LOGE(TAG, "invalid OTA json");
        free(info);
        return;
    }

    xTaskCreate(&ota_task, "ota", 8192, (void *)info, 5, NULL);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    switch (event_id) {
    case MQTT_SERVICE_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected, subscribe to ota topic");
        mqtt_service_subscribe("simpleiot/ota/update", 1, ota_update_callback, NULL);
        break;
    case MQTT_SERVICE_EVENT_DISCONNECTED:
        ESP_LOGE(TAG, "MQTT disconnected, do nothing");
        break;
    default:
        break;
    }
}

void ota_service_init(void)
{
    mqtt_service_subscribe("simpleiot/ota/update", 1, ota_update_callback, NULL);
    esp_event_handler_register(MQTT_SERVICE_EVENT, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    ESP_LOGI(TAG, "OTA service initialized");
}
