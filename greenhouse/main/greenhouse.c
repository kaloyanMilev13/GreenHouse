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
#define MOISTURE_GPIO GPIO_NUM_33
#define WATER_LEVEL_GPIO GPIO_NUM_35
#define LIGHT_SENSOR_GPIO GPIO_NUM_34


//ADC
#define LIGHT_ADC_CHANNEL ADC_CHANNEL_6
#define WATER_LEVEL_ADC_CHANNEL ADC_CHANNEL_7
#define MOISTURE_ADC_CHANNEL ADC_CHANNEL_5


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

	//Init ADC1
	adc_oneshot_unit_handle_t adc_handle;
	
	adc_oneshot_unit_init_cfg_t unit_config = {.unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_config, &adc_handle));


	//Init LIGHT_ADC_CHANNEL
	adc_oneshot_chan_cfg_t light_channel_config = {.bitwidth = ADC_BITWIDTH_12, .atten = ADC_ATTEN_DB_12};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LIGHT_ADC_CHANNEL, &light_channel_config));

	//Init WATER_LEVEL_ADC_CHANNEL
	adc_oneshot_chan_cfg_t water_channel_config = {.bitwidth = ADC_BITWIDTH_12, .atten = ADC_ATTEN_DB_12};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, WATER_LEVEL_ADC_CHANNEL, &water_channel_config));

	//Init MOISTURE_ADC_CHANNEL
	adc_oneshot_chan_cfg_t moist_channel_config = {.bitwidth = ADC_BITWIDTH_12, .atten = ADC_ATTEN_DB_12};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, MOISTURE_ADC_CHANNEL, &moist_channel_config));




	while(1){


		int light_raw;
		esp_err_t light_err = adc_oneshot_read(adc_handle, LIGHT_ADC_CHANNEL, &light_raw);
		

		int water_raw;
		esp_err_t water_err = adc_oneshot_read(adc_handle, WATER_LEVEL_ADC_CHANNEL, &water_raw);


		int moist_raw;
		esp_err_t moist_err = adc_oneshot_read(adc_handle, MOISTURE_ADC_CHANNEL, &moist_raw);

		//read light
		if(light_err == ESP_OK){
			
			printf("Light raw: %d\n", light_raw);
			

		}else {
		
			printf("ADC error: %s\n", esp_err_to_name(light_err));
		}


		//read water level
		if(water_err == ESP_OK){
			
			printf("Water raw: %d\n", water_raw);
			

		}else {
		
			printf("ADC error: %s\n", esp_err_to_name(water_err));
		}



		//read moisture level
		if(moist_err == ESP_OK){
			
			printf("Moisture raw: %d\n", moist_raw);
			

		}else {
		
			printf("ADC error: %s\n", esp_err_to_name(moist_err));
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
