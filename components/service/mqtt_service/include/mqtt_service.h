#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void mqtt_service_init(void);
void mqtt_service_publish(const char *topic, const char *data, int qos);
void mqtt_service_publish_retain(const char *topic, const char *data, int qos);

#ifdef __cplusplus
}
#endif