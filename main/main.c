// SPDX-License-Identifier: MIT
/*
 * Simple IoT System
 */
#include "esp_log.h"

#include "nvs_flash.h"
#include "wifi_service.h"
#include "mqtt_service.h"
#include "esp32c3_simpleIoT_board.h"
#include "iot_app.h"

static const char *TAG = "SIMPLE_IoT_MAIN";

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    board_init();
    wifi_init_sta();
    mqtt_app_start();
    app_start();

    ESP_LOGI(TAG, "Initialization complete.");
}
