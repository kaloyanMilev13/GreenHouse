#include "mqtt.h"

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "mqtt_client.h"


static const char *TAG = "mqtt";

static esp_mqtt_client_handle_t mqtt_client;

static void mqttEventHandler(void *handler_args, esp_event_base_t event_base, int32_t event_id, void *event_data){


	(void)handler_args;
	(void)event_base;


	esp_mqtt_event_handle_t event = event_data;

	switch(event_id){

		case MQTT_EVENT_CONNECTED: { 
						   ESP_LOGI(TAG, "Connected to broker");

						   int message_id = esp_mqtt_client_publish(event->client, "greenhouse/01/test", "Hello from ESP", 0, 1, 0);

						   if(message_id < 0){

							ESP_LOGE(TAG, "Couldnt publish test message");

						   }else {
						   	
							   ESP_LOGI(TAG, "Test message submitted, id=%d", message_id);
						   }

						   break;
					   }

		case MQTT_EVENT_DISCONNECTED:

			ESP_LOGW(TAG, "Disconnected from broker!");

			break;


		case MQTT_EVENT_PUBLISHED:

		      	ESP_LOGI(TAG, "Broker acknowledged message, id=%d", event->msg_id);


		      	break;



		case MQTT_EVENT_ERROR:

			ESP_LOGE(TAG, "MQTT error occured");

			break;


		default: break;

	}



}




esp_err_t mqttInit(void){

	
	if(mqtt_client != NULL){

		return ESP_ERR_INVALID_STATE;

	}


	const esp_mqtt_client_config_t config = {

		.broker.address.uri = "mqtt://10.42.0.1:1883",
		.outbox.limit = 4096,
	
	};

	mqtt_client = esp_mqtt_client_init(&config);

	if(mqtt_client == NULL){

		ESP_LOGE(TAG, "Couldnt create MQTT client");

		return ESP_FAIL;

	}


	esp_err_t result = esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqttEventHandler, NULL);


	if(result != ESP_OK){

		esp_mqtt_client_destroy(mqtt_client);

		mqtt_client = NULL;

		return result;

	}

	result = esp_mqtt_client_start(mqtt_client);


	if(result != ESP_OK){

		esp_mqtt_client_destroy(mqtt_client);

		mqtt_client = NULL;

		return result;
	}


	return ESP_OK;



}


esp_err_t mqttQueueTelemetry(const char *json){


	if(json == NULL){

		return ESP_ERR_INVALID_ARG;

	}

	if(mqtt_client){

		return ESP_ERR_INVALID_STATE;

	}

	int message_id = esp_mqtt_client_enqueue(mqtt_client, "greenhouse/01/telemetry", json, 0, 1, 0, true);

	if(message_id == -2){

		return ESP_ERR_NO_MEM;
	}

	if(message_id < 0){

		return ESP_FAIL;

	}


	return ESP_OK;
}
