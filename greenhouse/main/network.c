#include "network.h"
#include "wifi_credentials.h"
#include "esp_err.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_netif_ip_addr.h"
#include "esp_netif_types.h"
#include "esp_wifi_types_generic.h"
#include "nvs_flash.h"
#include "esp_wifi_default.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"

#define WIFI_MAX_RETRIES 5

static const char *TAG = "Hotspot from Arch";
static int retry_count = 0;

static void networkEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){


	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){

		ESP_LOGI(TAG, "WiFi Started; requesting connection");

		ESP_ERROR_CHECK(esp_wifi_connect());

	}else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){

		retry_count = 0;

		ip_event_got_ip_t *event = event_data;

		ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));

	}else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){

		wifi_event_sta_disconnected_t *event = event_data;

		ESP_LOGW(TAG, "Disconnected; Reason: %u", (unsigned)event->reason);

		if(retry_count < WIFI_MAX_RETRIES){

			retry_count++;

			ESP_LOGI(TAG, "Reconnect attempt %d/%d", retry_count, WIFI_MAX_RETRIES);

			esp_err_t err = esp_wifi_connect();

			if(err != ESP_OK){
				ESP_LOGE(TAG, "Connection request failed: %s", esp_err_to_name(err));
			}

		}else {

			ESP_LOGW(TAG, "Retry limit reached");
		}

	}

}




void networkInit(){

	esp_err_t result = nvs_flash_init();

	if(result != ESP_OK){

		ESP_LOGE(TAG, "NVS Init Failed: %s", esp_err_to_name(result));

		return;

	}

	ESP_LOGI(TAG, "Network init started");


	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_netif_t *station = esp_netif_create_default_wifi_sta();

	if(station == NULL){

		ESP_LOGE(TAG, "Failed to create WiFI network Interface");
		return;
	}


	ESP_LOGI(TAG, "Network Infrastructure ready");	


	wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));

	ESP_LOGI(TAG, "WiFi driver ready");

	wifi_config_t hotspot_config = { .sta = {

		.ssid = HOTSPOT_SSID,
		.password = HOTSPOT_PASSWORD,
		.threshold.authmode = WIFI_AUTH_WPA2_PSK,
		.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,

	},

	};


	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, networkEventHandler, NULL));

	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, networkEventHandler, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &hotspot_config));

	ESP_ERROR_CHECK(esp_wifi_start());
}


