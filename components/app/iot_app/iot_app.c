#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"

#include "esp32c3_simpleIoT_board.h"
#include "mqtt_service.h"
#include "oled_ui.h"
#include "iot_app.h"

static const char *TAG = "IoT_APP";

typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

static volatile sensor_data_t sensor_data;
static TaskHandle_t display_task_handle = NULL;
static TaskHandle_t cloud_task_handle = NULL;

static void aht20_sensor_read(aht20_dev_handle_t aht20, float *temperature, float *humidity)
{
    uint32_t temperature_raw = 0;
    uint32_t humidity_raw = 0;

    ESP_ERROR_CHECK(aht20_read_temperature_humidity(
        aht20, &temperature_raw, temperature, &humidity_raw, humidity));
    ESP_LOGI(TAG, "%-20s: %.1f %%", "humidity is", *humidity);
    ESP_LOGI(TAG, "%-20s: %.1f degC", "temperature is", *temperature);
}

void sensor_task(void *pvParameters)
{
    aht20_dev_handle_t aht20 = (aht20_dev_handle_t)pvParameters;
    float temperature = 0;
    float humidity = 0;

    for (;;) {
        aht20_sensor_read(aht20, &temperature, &humidity);
        sensor_data.temperature = temperature;
        sensor_data.humidity = humidity;
        xTaskNotifyGive(display_task_handle); // Notify the display task to update
        xTaskNotifyGive(cloud_task_handle);   // Notify the cloud task to publish data
        vTaskDelay(pdMS_TO_TICKS(2000));      // Read every 2 seconds
    }
}

void display_task(void *pvParameters)
{
    ssd1306_handle_t ssd1306 = (ssd1306_handle_t)pvParameters;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait for notification from sensor task
        ssd1306_draw_dynamic(ssd1306, sensor_data.temperature, sensor_data.humidity);
    }
}

void cloud_task(void *pvParameters)
{
    for (;;) {
        char payload[64];
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        snprintf(payload, sizeof(payload), "{\"temperature\": %.1f, \"humidity\": %.1f}",
                 sensor_data.temperature, sensor_data.humidity);
        mqtt_service_publish("simpleiot/sensor/data", payload, 0);
    }
}

void app_start(void)
{
    ui_draw_static();

    xTaskCreate(sensor_task, "sensor", 2048, board_get_aht20_handle(), 5, NULL);
    xTaskCreate(display_task, "display", 2048, board_get_ssd1306_handle(), 5, &display_task_handle);
    xTaskCreate(cloud_task, "cloud", 2048, NULL, 5, &cloud_task_handle);
}
