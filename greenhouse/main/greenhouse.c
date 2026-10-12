//C libraries
#include <complex.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <inttypes.h>

//ESP libraries
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/adc_types.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include "dht.h"
#include "esp_adc/adc_oneshot.h"
#include "ssd1306.h"

#include "esp_log.h"

#include "esp_timer.h"


//User libraries
#include "network.h"
#include "mqtt.h"

#define SENSOR_READ_INTERVAL_MS 500


//Oled
#define OLED_SDA_GPIO GPIO_NUM_21
#define OLED_SCL_GPIO GPIO_NUM_22

//INPUT ONLY
#define MOISTURE_GPIO GPIO_NUM_33
#define WATER_LEVEL_GPIO GPIO_NUM_35
#define LIGHT_SENSOR_GPIO GPIO_NUM_34
#define DHT_GPIO GPIO_NUM_17 


//ADC
#define LIGHT_ADC_CHANNEL ADC_CHANNEL_6
#define WATER_LEVEL_ADC_CHANNEL ADC_CHANNEL_7
#define MOISTURE_ADC_CHANNEL ADC_CHANNEL_5

#define ADC_LOWER_BOUND 0
#define ADC_UPPER_BOUND 4095

//Lower Bounds
#define MARK_LIGHT_LOW 0 //tested
#define MARK_WATER_LOW 1200 //tested
#define MARK_MOISTURE_LOW 0 

//Upper Bounds
#define MARK_LIGHT_HIGH 2400 //tested
#define MARK_MOISTURE_HIGH 4095
#define MARK_WATER_MAX 1700 //tested


//PUMP
#define PUMP_GPIO GPIO_NUM_16
#define PUMP_ON_LEVEL 1
#define PUMP_OFF_LEVEL 0

typedef struct {
	float temperature;
	float humidity;

	int light;
	float light_pct;

	int water;
	bool water_low;

	int moisture;

	esp_err_t dht_status;
	esp_err_t light_status;
	esp_err_t water_status;
	esp_err_t moisture_status;

	bool air_valid;
	bool light_valid;
	bool water_valid;
	bool moist_valid;

	uint64_t sampled_at_us;

} LatestReadings;


static bool pump_on = false;


adc_oneshot_unit_handle_t initSensors(void){

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


	return adc_handle;

}


void initOLED(SSD1306_t *oled){

	i2c_master_init(oled, OLED_SDA_GPIO, OLED_SCL_GPIO, -1);
	ESP_ERROR_CHECK(ssd1306_init(oled, 128, 64));

	ssd1306_clear_screen(oled, false);
	ssd1306_display_text(oled, 0, "Hello", 5, false);

}

void initPump(){

	gpio_config_t config = {

		.pin_bit_mask = (1ULL << PUMP_GPIO),//64-bit gpio mask, (not needed in my case, but a goood pracitce)
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE

	};


	ESP_ERROR_CHECK(gpio_set_level(PUMP_GPIO, PUMP_OFF_LEVEL));

	ESP_ERROR_CHECK(gpio_config(&config));

	ESP_ERROR_CHECK(gpio_set_level(PUMP_GPIO, PUMP_OFF_LEVEL));

	pump_on = false;
	

}

void setPump(bool on){

	if(on){
		ESP_ERROR_CHECK(gpio_set_level(PUMP_GPIO, PUMP_ON_LEVEL));	
	}else{
		ESP_ERROR_CHECK(gpio_set_level(PUMP_GPIO, PUMP_OFF_LEVEL));

	}

	pump_on = on;


}



void readSensors(LatestReadings *readings, adc_oneshot_unit_handle_t adc_handle){

	int light_raw;
	int water_raw;
	int moist_raw;
	float temp_raw;
	float hum_raw;

	readings->dht_status = dht_read_float_data(DHT_TYPE_DHT11, DHT_GPIO, &hum_raw, &temp_raw);
	readings->light_status = adc_oneshot_read(adc_handle, LIGHT_ADC_CHANNEL, &light_raw);
	readings->water_status = adc_oneshot_read(adc_handle, WATER_LEVEL_ADC_CHANNEL, &water_raw);
	readings->moisture_status = adc_oneshot_read(adc_handle, MOISTURE_ADC_CHANNEL, &moist_raw);


	if(readings->dht_status == ESP_OK){
		readings->temperature = temp_raw;
		readings->humidity = hum_raw;
	}


	if(readings->light_status == ESP_OK){
		readings->light = light_raw;
	}

	if(readings->water_status == ESP_OK){
		readings->water = water_raw;
	}

	if(readings->moisture_status == ESP_OK){
		readings->moisture = moist_raw;
	}


	readings->sampled_at_us = (uint64_t)esp_timer_get_time();



}


void verifyReadings(LatestReadings *readings){

	readings->air_valid = (readings->dht_status == ESP_OK) && isfinite(readings->temperature) && isfinite(readings->humidity) && readings->humidity >= 0.0f && readings->humidity <= 100.0f;

	readings->light_valid = (readings->light_status == ESP_OK) && readings->light >= ADC_LOWER_BOUND && readings->light <= ADC_UPPER_BOUND;

	readings->water_valid = (readings->water_status == ESP_OK) && readings->water >= ADC_LOWER_BOUND && readings->water <= MARK_WATER_MAX;

	readings->moist_valid = (readings->moisture_status == ESP_OK) && readings->moisture >= ADC_LOWER_BOUND && readings->moisture <= ADC_UPPER_BOUND;	

}


void validateReadings(LatestReadings *readings){

	verifyReadings(readings);

	if(readings->light_valid){

		//translate light adc to percentage
		float percentage = 100.0f * (MARK_LIGHT_HIGH - readings->light) /
		       	(MARK_LIGHT_HIGH - MARK_LIGHT_LOW);

		readings->light_pct = fminf(100.0f, fmaxf(0.0f, percentage));

	}

	if(readings->water_valid){
			readings->water_low = readings->water <= MARK_WATER_LOW;
	}else{
			readings->water_low = true;	
	}

	

}

static bool formatTelemetry(LatestReadings *readings, char *output, size_t output_size){


	char temperature[32] = "null";
	char humidity[32] = "null";
	char light[32] = "null";
	char water[32] = "null";
	char moisture[32] = "null";

	if(readings->air_valid){

		snprintf(temperature, sizeof(temperature), "%.1f", readings->temperature);

		snprintf(humidity, sizeof(humidity), "%.1f", readings->humidity);

	}

	if(readings->light_valid){

		snprintf(light, sizeof(light), "%.1f", readings->light_pct);

	}

	if(readings->water_valid){

		snprintf(water, sizeof(water), "%s", readings->water_low ? "true" : "false");

	}

	if(readings->moist_valid){

		snprintf(moisture, sizeof(moisture), "%d", readings->moisture);

	}


	uint64_t sampled_at_ms = readings->sampled_at_us / 1000ULL;

	int length = snprintf(output, output_size,
			"{\"test\":false"
			",\"sampled_at_ms\":%" PRIu64
			",\"temperature_c\":%s"
			",\"humidity_pct\":%s"
			",\"water_low\":%s"
			",\"light_pct\":%s"
			",\"moisture_pct\":%s"
			",\"pump_on\":%s}",
			sampled_at_ms,
			temperature,
			humidity,
			water,
			light,
			moisture,
			pump_on ? "true" : "false"
			);


	return length >= 0 && (size_t)length < output_size;

}

void displayResults(SSD1306_t *oled, const LatestReadings *readings){

	char readingsBuffer[17];

	ssd1306_clear_screen(oled, false);

	//Display on the OLED
	if(readings->air_valid){
		snprintf(readingsBuffer, sizeof(readingsBuffer), "Temp: %3.2f C" , readings->temperature);
		ssd1306_display_text(oled, 0, readingsBuffer, strlen(readingsBuffer), false);

		snprintf(readingsBuffer, sizeof(readingsBuffer), "Hum: %.1f %%", readings->humidity);
		ssd1306_display_text(oled, 1, readingsBuffer, strlen(readingsBuffer), false);

	}else{
		ssd1306_display_text(oled, 0, "Temp: ERROR", strlen("Temp: ERROR"), false);
		ssd1306_display_text(oled, 1, "Hum: ERROR", strlen("Hum: ERROR"), false);
	}

	if(readings->light_valid){

		snprintf(readingsBuffer, sizeof(readingsBuffer), "Light: %d", readings->light);
		ssd1306_display_text(oled, 2, readingsBuffer, strlen(readingsBuffer), false);

	}else{

		ssd1306_display_text(oled, 2, "Light: ERROR", strlen("Light: ERROR"), false);
	}


	if(readings->water_valid){
		snprintf(readingsBuffer, sizeof(readingsBuffer), "Water: %d", readings->water);
		ssd1306_display_text(oled, 3, readingsBuffer, strlen(readingsBuffer), false);
	}else {

		ssd1306_display_text(oled, 3, "Water: ERROR", strlen("Water: ERROR"), false);
	}

	if(readings->moist_valid){
		snprintf(readingsBuffer, sizeof(readingsBuffer), "Soil: %d", readings->moisture);
		ssd1306_display_text(oled, 4, readingsBuffer, strlen(readingsBuffer), false);
	}else{

		ssd1306_display_text(oled, 4, "Soil: ERROR", strlen("Soil: ERROR"), false);
	}

}

void app_main(void){


	initPump();

	adc_oneshot_unit_handle_t adc_handle = initSensors();

	LatestReadings readings = {0};

/*	ESP_ERROR_CHECK(networkInit());

	ESP_LOGI("test", "Waiting for Wi-Fi");

	while (!networkWaitReady(pdMS_TO_TICKS(5000))) {
		ESP_LOGW("test", "Still waiting for Wi-Fi");
	}

	ESP_ERROR_CHECK(mqttInit()); */

	while (1) {

		readSensors(&readings, adc_handle);
		
		validateReadings(&readings);

		if (readings.water_status == ESP_OK) {
			ESP_LOGI("calibration", "light_raw=%d", readings.light);
			ESP_LOGI("calibration", "light_pct = %.1f", readings.light_pct);
		} else {
			ESP_LOGW("calibration", "water read failed: %s",
					esp_err_to_name(readings.water_status));
		}



/*		char payload[384];

		uint64_t sampled_at_ms = (uint64_t)esp_timer_get_time() / 1000ULL;

		int length = snprintf(
				payload,
				sizeof(payload),
				"{\"test\":true"
				",\"sampled_at_ms\":%" PRIu64
				",\"temperature_c\":23.5"
				",\"humidity_pct\":48.0"
				",\"water_low\":false"
				",\"light_pct\":65.0"
				",\"moisture_pct\":42.0"
				",\"pump_on\":false}",
				sampled_at_ms
				);

		if (length < 0 || (size_t)length >= sizeof(payload)) {
			ESP_LOGE("test", "Payload formatting failed");
		} else {
			esp_err_t result = mqttQueueTelemetry(payload);

			if (result == ESP_OK) {
				ESP_LOGI("test", "Queued: %s", payload);
			} else {
				ESP_LOGW(
						"test",
						"Could not queue: %s",
						esp_err_to_name(result)
					);
			}
		}

*/

		vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
	}
}

/*
   void app_main(void){

   SSD1306_t oled = {0};
//initOLED(&oled);

//adc_oneshot_unit_handle_t adc_handle = initSensors();

LatestReadings readings = {0};
bool mqtt_started = false;

ESP_ERROR_CHECK(networkInit());

while (1) {
if (!mqtt_started && networkWaitReady(0)) {
ESP_ERROR_CHECK(mqttInit());
mqtt_started = true;
}

//readSensors(&readings, adc_handle);
//checkValidity(&readings);
//displayResults(&oled, &readings);

if (mqtt_started) {
char payload[256];

if (formatTelemetry(&readings, payload, sizeof(payload))) {

esp_err_t result = mqttQueueTelemetry(payload);

if (result != ESP_OK) {
ESP_LOGW("greenhouse", "Telemetry not queued: %s", esp_err_to_name(result));
}
} else {
ESP_LOGE("greenhouse", "Telemetry formatting failed");
}
}

vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
}
}
*/

/*
   void app_main(void){
//ESP32-WROOM-32D


SSD1306_t oled = {0};

initOLED(&oled);

adc_oneshot_unit_handle_t adc_handle = initSensors();

LatestReadings readings = {0};



while(1){

readSensors(&readings, adc_handle);

checkValidity(&readings);

displayResults(&oled, &readings);

//Delay
vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));

}

}

*/



