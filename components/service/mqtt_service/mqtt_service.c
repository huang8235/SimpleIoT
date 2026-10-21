#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "esp_system.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "wifi_service.h"
#include "mqtt_service.h"

#define MQTT_TOPIC_MAX_LEN 64
#define MAX_CALLBACKS 8

ESP_EVENT_DEFINE_BASE(MQTT_SERVICE_EVENT);

typedef struct {
    char topic_pattern[MQTT_TOPIC_MAX_LEN];
    mqtt_msg_cb_t cb;
    void *arg;
    bool used;
} mqtt_callback_entry_t;

static const char *TAG = "mqtt_service";
static esp_mqtt_client_handle_t client = NULL;
static bool mqtt_connected = false;
static mqtt_callback_entry_t callbacks[MAX_CALLBACKS];
static SemaphoreHandle_t cb_mutex;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void handle_mqtt_data(const char *topic, int topic_len, const char *data, int data_len)
{
    xSemaphoreTake(cb_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (callbacks[i].used && strncmp(callbacks[i].topic_pattern, topic, topic_len) == 0) {
            callbacks[i].cb(topic, data, data_len, callbacks[i].arg);
            break;
        }
    }
    xSemaphoreGive(cb_mutex);
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        mqtt_connected = true;
        esp_event_post(MQTT_SERVICE_EVENT, MQTT_SERVICE_EVENT_CONNECTED,
                        NULL, 0, portMAX_DELAY);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        mqtt_connected = false;
        esp_event_post(MQTT_SERVICE_EVENT, MQTT_SERVICE_EVENT_DISCONNECTED,
                        NULL, 0, portMAX_DELAY);
        break;

    case MQTT_EVENT_SUBSCRIBED:
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        handle_mqtt_data(event->topic, event->topic_len, event->data, event->data_len);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void wifi_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    switch (event_id) {
    case WIFI_SERVICE_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WI-FI connected, start mqtt client");
        esp_mqtt_client_start(client);
        break;
    case WIFI_SERVICE_EVENT_DISCONNECTED:
        ESP_LOGE(TAG, "WI-FI disconnected, stop mqtt client");
        esp_mqtt_client_stop(client);
        break;
    default:
        break;
    }
}

void mqtt_service_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
        .credentials.client_id = CONFIG_MQTT_CLIENT_ID,
        .credentials.username = CONFIG_MQTT_USERNAME,
        .credentials.authentication.password = CONFIG_MQTT_PASSWORD,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_event_handler_register(WIFI_SERVICE_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);

    cb_mutex = xSemaphoreCreateMutex();
}

bool mqtt_service_is_connected(void)
{
    return mqtt_connected;
}

void mqtt_service_publish(const char *topic, const char *data, int qos)
{
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, data, 0, qos, 0);
    ESP_LOGI(TAG, "sent publish successful, topic=%s, msg_id=%d", topic, msg_id);
}

/* Publish a message with retain flag set.
   The broker will retain the last message sent to this topic */
void mqtt_service_publish_retain(const char *topic, const char *data, int qos)
{
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return;
    }

    int msg_id = esp_mqtt_client_publish(client, topic, data, 0, qos, 1);
    ESP_LOGI(TAG, "sent publish successful, topic=%s, msg_id=%d", topic, msg_id);
}

void mqtt_service_subscribe(const char *topic, int qos, mqtt_msg_cb_t cb, void *arg)
{
    if (client == NULL) {
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return;
    }

    int msg_id = esp_mqtt_client_subscribe(client, topic, qos);
    ESP_LOGI(TAG, "sent subscribe successful, topic=%s, msg_id=%d", topic, msg_id);

    xSemaphoreTake(cb_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_CALLBACKS; i++) {
        if (!callbacks[i].used) {
            strncpy(callbacks[i].topic_pattern, topic, MQTT_TOPIC_MAX_LEN);
            callbacks[i].cb = cb;
            callbacks[i].arg = arg;
            callbacks[i].used = true;
            break;
        }
    }
    xSemaphoreGive(cb_mutex);
}
