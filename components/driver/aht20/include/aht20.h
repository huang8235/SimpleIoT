/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_types.h"
#include "esp_err.h"

#include "driver/i2c_master.h"

/* AHT20 address: CE pin low - 0x38, CE pin high - 0x39 */
#define AHT20_ADDRRES_0 (0x38)
#define AHT20_ADDRESS_1 (0x39)

/**
 * @brief Type of AHT20 device handle
 *
 */
typedef void *aht20_dev_handle_t;

/**
 * @brief Create new AHT20 device handle.
 *
 * @param[in]  bus_handle I2C bus instance handle used to communicate with the AHT20 device
 * @param[in]  slave_addr I2C address of AHT20 device
 * @param[out] handle_out New AHT20 device handle
 * @return
 *          - ESP_OK                  Device handle creation success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 *          - ESP_ERR_NO_MEM          Memory allocation failed.
 *
 */
esp_err_t aht20_new_sensor(const i2c_master_bus_handle_t bus_handle,
                           const uint16_t slave_addr,
                           aht20_dev_handle_t *handle_out);

/**
 * @brief Delete AHT20 device handle.
 *
 * @param[in] handle AHT20 device handle
 * @return
 *          - ESP_OK                  Device handle deletion success.
 *          - ESP_ERR_INVALID_ARG     Invalid device handle or argument.
 *
 */
esp_err_t aht20_del_sensor(aht20_dev_handle_t handle);

/**
 * @brief read the temperature and humidity data
 *
 * @param[in]  *handle points to an aht20 handle structure
 * @param[out] *temperature_raw points to a raw temperature buffer
 * @param[out] *temperature points to a converted temperature buffer
 * @param[out] *humidity_raw points to a raw humidity buffer
 * @param[out] *humidity points to a converted humidity buffer
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Fail
 */
esp_err_t aht20_read_temperature_humidity(aht20_dev_handle_t handle,
                                          uint32_t *temperature_raw, float *temperature,
                                          uint32_t *humidity_raw, float *humidity);
#ifdef __cplusplus
}
#endif
