#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/adc_types.h"
#include "soc/gpio_num.h"
#include "dht.h"
#include "esp_adc/adc_oneshot.h"
#include <complex.h>
#include <stdint.h>
#include <stdio.h>

#define SENSOR_READ_INTERVAL_MS 2000

#define DHT_GPIO GPIO_NUM_17 

#define PUMP_GPIO GPIO_NUM_16

#define OLED_SDA_GPIO GPIO_NUM_21
#define OLED_SCL_GPIO GPIO_NUM_22

//INPUT ONLY
#define MOISTURE_GPIO GPIO_NUM_36
#define WATER_LEVEL_GPIO GPIO_NUM_39
#define LIGHT_SENSOR_GPIO GPIO_NUM_34


//ADC
#define LIGHT_ADC_CHANNEL ADC_CHANNEL_6



typedef struct{

	float temperature;
	float humidity;
	float moisture;
	float waterLevel;
	float light;

	uint64_t sampled_at_us;//idk about that
	
	
} LatestReadings;


typedef struct{
       

	float humidity ;
	float temperature;
	esp_err_t dht_stats;

} Readings_DHT;


int read_DHT(Readings_DHT *sensorDHT){

		sensorDHT->dht_stats = dht_read_float_data(DHT_TYPE_DHT11, DHT_GPIO, &sensorDHT->humidity, &sensorDHT->temperature);


		if(sensorDHT->dht_stats == ESP_OK){
			return 0;
		}else {
			return 1;
		}

}



void app_main(void){
//ESP32-WROOM-32D


	Readings_DHT sensorDHT = {0, 0, 0};


	adc_oneshot_unit_handle_t adc_handle;
	
	adc_oneshot_unit_init_cfg_t unit_config = {.unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));



	adc_oneshot_chan_cfg_t channel_config = {.bitwidth = ADC_BITWIDTH_12, .atten = ADC_ATTEN_DB_12};

	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LIGHT_ADC_CHANNEL, &channel_config));

	while(1){


		int raw;
		esp_err_t err = adc_oneshot_read(adc_handle, LIGHT_ADC_CHANNEL, &raw);

		//read light
		if(err == ESP_OK){
			
			printf("Light raw: %d\n", raw);
			

		}else {
		
			printf("ADC error: %s\n", esp_err_to_name(err));
		}

		//read temp and hum
		if(read_DHT(&sensorDHT) == 0){
		
			printf("Temperature: %.1f C, Humidity: %.1f %%\n", sensorDHT.temperature, sensorDHT.humidity);

		}else {
		
			printf("Error reading: %s\n", esp_err_to_name(sensorDHT.dht_stats));
		}




		vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));

	}

}
