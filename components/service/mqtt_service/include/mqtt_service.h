void mqtt_app_start(void);
void mqtt_service_publish(const char *topic, const char *data, int qos);
void mqtt_service_publish_retain(const char *topic, const char *data, int qos);
