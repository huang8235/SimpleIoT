#pragma once

#ifdef __cplusplus
extern "C" {
#endif

ESP_EVENT_DECLARE_BASE(WIFI_SERVICE_EVENT);

typedef enum {
    WIFI_SERVICE_EVENT_IDLE,
    WIFI_SERVICE_EVENT_CONNECTED,
    WIFI_SERVICE_EVENT_DISCONNECTED,
} wifi_service_event_t;

void wifi_service_init(void);

#ifdef __cplusplus
}
#endif
