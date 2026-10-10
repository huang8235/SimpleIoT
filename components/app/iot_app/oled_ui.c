#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "esp_log.h"

#include "oled_ui.h"

#define OLED_TEXT_WIDTH 6
#define OLED_TEXT_HEIGHT 8

static const char *TAG = "OLED_UI";

void ui_clear_area(ssd1306_handle_t ssd1306, int x, int y, int width, int height)
{
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            ssd1306_draw_pixel(ssd1306, x + i, y + j, false);
        }
    }
}

void ui_draw_bitmap(ssd1306_handle_t dev, int x, int y,
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

void ui_draw_static(ssd1306_handle_t ssd1306)
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

void ui_draw_dynamic(ssd1306_handle_t ssd1306, float temperature, float humidity)
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
