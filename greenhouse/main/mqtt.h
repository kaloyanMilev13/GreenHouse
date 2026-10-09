#ifndef MQTT_H
#define MQTT_H


#include "esp_err.h"

esp_err_t mqttInit(void);

esp_err_t mqttQueueTelemetry(const char *json);

#endif
