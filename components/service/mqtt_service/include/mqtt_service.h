#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_event.h"

ESP_EVENT_DECLARE_BASE(MQTT_SERVICE_EVENT);

typedef enum {
    MQTT_SERVICE_EVENT_CONNECTED,
    MQTT_SERVICE_EVENT_DISCONNECTED,
} mqtt_service_event_t;

typedef void (*mqtt_msg_cb_t)(const char *topic, const char *data, int data_len, void *arg);

void mqtt_service_init(void);
bool mqtt_service_is_connected(void);
void mqtt_service_publish(const char *topic, const char *data, int qos);
void mqtt_service_publish_retain(const char *topic, const char *data, int qos);
void mqtt_service_subscribe(const char *topic, int qos, mqtt_msg_cb_t cb, void *arg);

#ifdef __cplusplus
}
#endif