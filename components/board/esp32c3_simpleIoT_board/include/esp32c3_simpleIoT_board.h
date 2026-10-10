#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "aht20.h"
#include "ssd1306.h"

void board_init(void);
aht20_dev_handle_t board_get_aht20_handle(void);
ssd1306_handle_t board_get_ssd1306_handle(void);

#ifdef __cplusplus
}
#endif