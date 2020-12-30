typedef void (*cbMqttHandler_t) (int32_t id, esp_mqtt_event_t *event);

void wifi_init(const char *ssid, const char *password);
void mqtt_init(const char *uri, cbMqttHandler_t callback);
void mqtt_publish(const char *id, const char *topic, const char *data, int qos, int retain);
void mqtt_subscribe(const char *topic, int qos);
