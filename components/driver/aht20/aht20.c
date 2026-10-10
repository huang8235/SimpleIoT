/*
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "aht20.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"

#include "aht20_reg.h"

const static char *TAG = "AHT20";

static esp_err_t aht20_start_measurment(aht20_dev_handle_t handle)
{
    i2c_master_dev_handle_t dev_handle = (i2c_master_dev_handle_t)handle;

    const uint8_t cmd = AHT20_START_MEASURMENT_CMD;
    const uint8_t data0 = 0x33;
    const uint8_t data1 = 0x00;
    const uint8_t data_wr[3] = {cmd, data0, data1};

    return i2c_master_transmit(dev_handle, data_wr, sizeof(data_wr), -1);
}

static esp_err_t aht20_read_data(aht20_dev_handle_t handle, uint8_t *data, size_t len)
{
    i2c_master_dev_handle_t dev_handle = (i2c_master_dev_handle_t)handle;
    return i2c_master_receive(dev_handle, data, len, -1);
}

static uint8_t aht20_calc_crc(uint8_t *data, uint8_t len)
{
    uint8_t i;
    uint8_t byte;
    uint8_t crc = 0xFF;

    for (byte = 0; byte < len; byte++) {
        crc ^= data[byte];
        for (i = 8; i > 0; --i) {
            if ((crc & 0x80) != 0) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

esp_err_t aht20_read_temperature_humidity(aht20_dev_handle_t handle,
                                          uint32_t *temperature_raw, float *temperature,
                                          uint32_t *humidity_raw, float *humidity)
{
    uint8_t status;
    uint8_t buf[7];
    uint32_t raw_data;

    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    ESP_RETURN_ON_ERROR(aht20_start_measurment(handle), TAG, "I2C write error");

    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_RETURN_ON_ERROR(aht20_read_data(handle, &status, 1), TAG, "I2C read error");

    if ((status & BIT(AT581X_STATUS_Calibration_Enable)) &&
            (status & BIT(AT581X_STATUS_CRC_FLAG)) &&
            ((status & BIT(AT581X_STATUS_BUSY_INDICATION)) == 0)) {
        ESP_RETURN_ON_ERROR(aht20_read_data(handle, buf, 7), TAG, "I2C read error");
        ESP_RETURN_ON_ERROR((aht20_calc_crc(buf, 6) != buf[6]), TAG, "crc is error");

        raw_data = buf[1];
        raw_data = raw_data << 8;
        raw_data += buf[2];
        raw_data = raw_data << 8;
        raw_data += buf[3];
        raw_data = raw_data >> 4;
        *humidity_raw = raw_data;
        *humidity = (float)raw_data * 100 / 1048576;

        raw_data = buf[3] & 0x0F;
        raw_data = raw_data << 8;
        raw_data += buf[4];
        raw_data = raw_data << 8;
        raw_data += buf[5];
        *temperature_raw = raw_data;
        *temperature = (float)raw_data * 200 / 1048576 - 50;
        return ESP_OK;
    } else {
        ESP_LOGI(TAG, "data is not ready");
        return ESP_ERR_NOT_FINISHED;
    }
}

esp_err_t aht20_new_sensor(const i2c_master_bus_handle_t bus_handle,
                           const uint16_t slave_addr,
                           aht20_dev_handle_t *handle_out)
{
    ESP_LOGI(TAG, "%-15s: %1.1f - %1.1fV", "SUPPLY_VOLTAGE", SUPPLY_VOLTAGE_MIN, SUPPLY_VOLTAGE_MAX);
    ESP_LOGI(TAG, "%-15s: %.2f - %.2f℃", "TEMPERATURE", TEMPERATURE_MIN, TEMPERATURE_MAX);

    ESP_RETURN_ON_FALSE(bus_handle, ESP_ERR_INVALID_ARG, TAG, "invalid master handle pointer");
    ESP_RETURN_ON_FALSE(handle_out, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = slave_addr,
        .scl_speed_hz = 100000,
    };

    i2c_master_dev_handle_t dev_handle;
    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed");
        return ret;
    }

    *handle_out = dev_handle;
    return ESP_OK;
}

esp_err_t aht20_del_sensor(aht20_dev_handle_t handle)
{
    ESP_RETURN_ON_FALSE(handle, ESP_ERR_INVALID_ARG, TAG, "invalid device handle pointer");
    i2c_master_dev_handle_t dev_handle = (i2c_master_dev_handle_t)handle;
    esp_err_t ret = i2c_master_bus_rm_device(dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_rm_device failed");
        return ret;
    }

    return ESP_OK;
}
