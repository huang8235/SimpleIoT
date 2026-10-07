// SPDX-License-Identifier: MIT
/*
 * Simple IoT System
 */
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ssd1306.h"
#include "aht20.h"
#include "wifi_manager.h"
#include "nvs_flash.h"

#define I2C_MASTER_SCL_IO   CONFIG_I2C_MASTER_SCL   /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO   CONFIG_I2C_MASTER_SDA   /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM      I2C_NUM_0               /*!< I2C port number for master dev */

#define SPI_BUS_MOSI_IO     CONFIG_SPI_BUS_MOSI
#define SPI_BUS_SCLK_IO     CONFIG_SPI_BUS_SCLK
#define SPI_BUS_CLK_HZ      8000000

#define SSD1306_SPI_CS_IO   CONFIG_SSD1306_SPI_CS
#define SSD1306_DC_IO       CONFIG_SSD1306_DC
#define SSD1306_RESET_IO    CONFIG_SSD1306_RESET
#define SSD1306_OLED_WIDTH  128
#define SSD1306_OLED_HEIGHT 64

#define OLED_TEXT_WIDTH 6
#define OLED_TEXT_HEIGHT 8

static const char *TAG = "SIMPLE_IoT_MAIN";

typedef struct {
    float temperature;
    float humidity;
} sensor_data_t;

static i2c_master_bus_handle_t i2c_bus_handle;
static volatile sensor_data_t sensor_data;
static TaskHandle_t display_task_handle = NULL;

static void i2c_master_init(void)
{
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &i2c_bus_handle));
}

static void spi_bus_init(void)
{
    const spi_bus_config_t buscfg = {
        .mosi_io_num     = SPI_BUS_MOSI_IO,
        .miso_io_num     = -1,
        .sclk_io_num     = SPI_BUS_SCLK_IO,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 0,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));
}

static void ssd1306_init(ssd1306_handle_t *handle)
{
    ssd1306_config_t cfg = {
        .bus    = SSD1306_SPI,
        .width  = SSD1306_OLED_WIDTH,
        .height = SSD1306_OLED_HEIGHT,
        .iface.spi =
            {
                .host     = SPI2_HOST,
                .cs_gpio  = SSD1306_SPI_CS_IO,     // Chip-select pin
                .dc_gpio  = SSD1306_DC_IO,         // Data/Command pin
                .rst_gpio = SSD1306_RESET_IO,      // Reset pin (GPIO_NUM_NC if tied high)
                .clk_hz   = SPI_BUS_CLK_HZ,        // 8 MHz
            },
        .fb     = NULL,                            // Let driver allocate framebuffer
        .fb_len = 0,
    };

    ESP_ERROR_CHECK(ssd1306_new_spi(&cfg, handle));
}

static void aht20_init(aht20_dev_handle_t *handle)
{
    ESP_ERROR_CHECK(aht20_new_sensor(i2c_bus_handle, AHT20_ADDRRES_0, handle));
}

static void ssd1306_clear_area(ssd1306_handle_t ssd1306, int x, int y, int width, int height)
{
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            ssd1306_draw_pixel(ssd1306, x + i, y + j, false);
        }
    }
}

/*
void ssd1306_draw_bitmap(ssd1306_handle_t dev, int x, int y,
                 const uint8_t *bitmap, int w, int h) {
    int pages = (h + 7) / 8;
    for (int p = 0; p < pages; p++) {
        for (int col = 0; col < w; col++) {
            uint8_t byte = bitmap[p * w + col];
            for (int bit = 0; bit < 8; bit++) {
                if (byte & (1 << bit)) {
                    int px = x + col;
                    int py = y + p * 8 + bit;
                    if (py < y + h) {
                        ssd1306_draw_pixel(dev, px, py, true);
                    }
                }
            }
        }
    }
}
*/

static void ssd1306_draw_static(ssd1306_handle_t ssd1306)
{
    // Clear the display
    ESP_ERROR_CHECK(ssd1306_clear(ssd1306));

    // Draw static text
    ESP_ERROR_CHECK(
        ssd1306_draw_text_scaled(ssd1306, 0, 0, "Temp: ", true, 2));
    ESP_ERROR_CHECK(
        ssd1306_draw_text_scaled(ssd1306, 0, 20, "Humi: ", true, 2));

    ESP_ERROR_CHECK(
        ssd1306_draw_text_scaled(ssd1306,
            SSD1306_OLED_WIDTH - OLED_TEXT_WIDTH * 4,
            SSD1306_OLED_HEIGHT - OLED_TEXT_HEIGHT,
            "V0.1", true, 1));

    // Update the display
    ESP_ERROR_CHECK(ssd1306_display(ssd1306));
}

static void ssd1306_draw_dynamic(ssd1306_handle_t ssd1306, float temperature, float humidity)
{
    char temperature_str[8];
    char humidity_str[8];

    snprintf(temperature_str, sizeof(temperature_str), "%.1f", temperature);
    snprintf(humidity_str, sizeof(humidity_str), "%.1f", humidity);

    ssd1306_clear_area(ssd1306, 60, 0, 68, 20);  // Clear area for temperature
    ssd1306_clear_area(ssd1306, 60, 20, 68, 20); // Clear area for humidity

    ESP_ERROR_CHECK(
        ssd1306_draw_text_scaled(ssd1306, 60, 0, temperature_str, true, 2));
    ESP_ERROR_CHECK(
        ssd1306_draw_text_scaled(ssd1306, 60, 20, humidity_str, true, 2));

    ESP_ERROR_CHECK(ssd1306_display(ssd1306));
}

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

void app_main(void)
{
    spi_bus_init();
    i2c_master_init();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    ssd1306_handle_t ssd1306 = NULL;
    ssd1306_init(&ssd1306);

    aht20_dev_handle_t aht20 = NULL;
    aht20_init(&aht20);

    ssd1306_draw_static(ssd1306);

    xTaskCreate(sensor_task, "sensor", 2048, aht20, 5, NULL);
    xTaskCreate(display_task, "display", 2048, ssd1306, 5, &display_task_handle);

    ESP_LOGI(TAG, "Initialization complete. System is running.");
}
