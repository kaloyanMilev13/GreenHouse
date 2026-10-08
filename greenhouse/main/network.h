#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

esp_err_t networkInit(void);
bool networkWaitReady(TickType_t timeout_ticks);

#endif
