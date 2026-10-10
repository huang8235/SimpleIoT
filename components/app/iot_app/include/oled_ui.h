#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "ssd1306.h"

void ui_clear_area(ssd1306_handle_t ssd1306, int x, int y, int width, int height);
void ui_draw_bitmap(ssd1306_handle_t dev, int x, int y,
                         const uint8_t *bitmap, int w, int h);
void ui_draw_static(ssd1306_handle_t ssd1306);
void ui_draw_dynamic(ssd1306_handle_t ssd1306, float temperature, float humidity);


#ifdef __cplusplus
}
#endif