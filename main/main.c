// SPDX-License-Identifier: MIT
/*
 * Simple IoT System
 */
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "wifi_service.h"
#include "mqtt_service.h"
#include "ota_service.h"
#include "iot_board.h"
#include "iot_app.h"

static const char *TAG = "SIMPLE_IoT_MAIN";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_netif_init());

    board_init();
    wifi_service_init();
    mqtt_service_init();
    ota_service_init();
    app_start();

    ESP_LOGI(TAG, "Initialization complete.");
}
