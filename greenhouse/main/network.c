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
#include "esp_timer.h"


#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"


#define WIFI_RETRY_DELAY_US 3000000ULL // = 3 secs
#define NETWORK_READY_BIT (1U << 0)

static const char *TAG = "Hotspot from Arch";
static esp_timer_handle_t retry_timer;
static EventGroupHandle_t network_events;

static void scheduleRetry(void){


	esp_err_t err = esp_timer_start_once(retry_timer, WIFI_RETRY_DELAY_US);

	if(err != ESP_OK && err != ESP_ERR_INVALID_STATE){ //INVALID_STATE means that the timer is already runnign, 
	
		ESP_LOGE(TAG, "Cannot schedule retry: %s", esp_err_to_name(err));	

	}

}


static void retryConnection(void *arg){


	(void)arg;
	ESP_LOGI(TAG, "Trying to reconnect");


	esp_err_t err = esp_wifi_connect();

	if(err != ESP_OK){

		ESP_LOGW(TAG, "Connection request failed: %s", esp_err_to_name(err));

		scheduleRetry();

	}

}

static void networkEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data){


	if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START){

		ESP_LOGI(TAG, "WiFi Started; requesting connection");

		retryConnection(NULL);

	}else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){

		(void)esp_timer_stop(retry_timer);// dawa statusa na operciqta, no iztriwa resulta, zashtoto realo ne se interesuwame ot rezultata, a prosto iskame da sprem timera

		ip_event_got_ip_t *event = event_data;

		ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));

		xEventGroupSetBits(network_events, NETWORK_READY_BIT);

	}else if(event_base == IP_EVENT && event_id == IP_EVENT_STA_LOST_IP){

		xEventGroupClearBitsFromISR(network_events, NETWORK_READY_BIT);
	
	}else if(event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){

		wifi_event_sta_disconnected_t *event = event_data;

		ESP_LOGW(TAG, "Disconnected; Reason: %u", (unsigned)event->reason);

		xEventGroupClearBits(network_events, NETWORK_READY_BIT);

		scheduleRetry();

	}

}


bool networkWaitReady(TickType_t timeout_ticks){

	if(network_events == NULL){
		return false;
	}


	EventBits_t bits = xEventGroupWaitBits(network_events, NETWORK_READY_BIT, pdFALSE, pdTRUE, timeout_ticks);


	return (bits & NETWORK_READY_BIT) != 0;


}




esp_err_t networkInit(void){

	esp_err_t result = nvs_flash_init();

	if(result != ESP_OK){

		ESP_LOGE(TAG, "NVS Init Failed: %s", esp_err_to_name(result));

		return result;

	}

	ESP_LOGI(TAG, "Network init started");


	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	esp_netif_t *station = esp_netif_create_default_wifi_sta();

	if(station == NULL){

		ESP_LOGE(TAG, "Failed to create WiFI network Interface");
		return ESP_ERR_NO_MEM;
	}


	ESP_LOGI(TAG, "Network Infrastructure ready");	


	wifi_init_config_t wifi_init = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init));

	ESP_LOGI(TAG, "WiFi driver ready");

	const esp_timer_create_args_t retry_config = {
		
		.callback = retryConnection,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "wifi_retry",

	};

	ESP_ERROR_CHECK(esp_timer_create(&retry_config, &retry_timer));

	wifi_config_t hotspot_config = { .sta = {

		.ssid = HOTSPOT_SSID,
		.password = HOTSPOT_PASSWORD,
		.threshold.authmode = WIFI_AUTH_WPA2_PSK,
		.sae_pwe_h2e = WPA3_SAE_PWE_BOTH,

		},

	};


	network_events = xEventGroupCreate();
	
	if(network_events == NULL){

		ESP_LOGE(TAG, "Error creating network event group");

		return ESP_ERR_NO_MEM;

	}


	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, networkEventHandler, NULL));

	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, networkEventHandler, NULL));

	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP, networkEventHandler, NULL));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &hotspot_config));

	ESP_ERROR_CHECK(esp_wifi_start());

	
	return ESP_OK;
}



