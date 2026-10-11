idf_component_register(SRCS "mqtt_service.c"
                    PRIV_REQUIRES mqtt wifi_service
                    INCLUDE_DIRS "include")
