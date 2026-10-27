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
#include "esp_random.h"
#include "esp_timer.h"


//User libraries
#include "network.h"
#include "mqtt.h"

#define SENSOR_READ_INTERVAL_MS 5000

#define WATERING_DURATION_MS 2000ULL

#define WATERING_COOLDOWN_MS 60000ULL //timer for spreading the water in the soil



//Oled
#define OLED_SDA_GPIO GPIO_NUM_21
#define OLED_SCL_GPIO GPIO_NUM_22

//INPUT ONLY
#define MOISTURE_GPIO GPIO_NUM_33
#define WATER_LEVEL_GPIO GPIO_NUM_35
#define LIGHT_SENSOR_GPIO GPIO_NUM_34
#define DHT_GPIO GPIO_NUM_17 


#define MOISTURE_SAMPLE_COUNT 32


//ADC
#define LIGHT_ADC_CHANNEL ADC_CHANNEL_6
#define WATER_LEVEL_ADC_CHANNEL ADC_CHANNEL_7
#define MOISTURE_ADC_CHANNEL ADC_CHANNEL_5

#define ADC_LOWER_BOUND 0
#define ADC_UPPER_BOUND 4095

#define MOISTURE_DRY_ADC 1883.0f
#define MOISTURE_WET_ADC 1822.0f

//Lower Bounds
#define MARK_LIGHT_LOW 0 //tested
#define MARK_WATER_LOW 1200 //tested
#define MARK_MOISTURE_DRY 1880 

//Upper Bounds
#define MARK_LIGHT_HIGH 2400 //tested
#define MARK_WATER_MAX 1700 //tested
#define MARK_MOISTURE_WET 1860


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
	float moisture_pct;
	bool activate_pump;

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



typedef struct {

	const char *boot_id;
	uint32_t cycle_id;
	bool started;
	uint64_t occurred_at_ms;
	uint64_t duration_ms;
	const char *reason;
	bool test;

} Watering;

static bool pump_on = false;
static char watering_boot_id[17];

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



void readSensors(LatestReadings *readings, adc_oneshot_unit_handle_t adc_handle, bool read_air){

	int light_raw;
	int water_raw;
	int moist_raw;
	float temp_raw;
	float hum_raw;

	if(read_air){

		readings->dht_status = dht_read_float_data(DHT_TYPE_DHT11, DHT_GPIO, &hum_raw, &temp_raw);

		if(readings->dht_status == ESP_OK){
			readings->temperature = temp_raw;
			readings->humidity = hum_raw;
		}


	}

	readings->light_status = adc_oneshot_read(adc_handle, LIGHT_ADC_CHANNEL, &light_raw);
	readings->water_status = adc_oneshot_read(adc_handle, WATER_LEVEL_ADC_CHANNEL, &water_raw);

	int moisture_sum = 0;
	readings->moisture_status = ESP_OK;


	for(int i = 0; i < MOISTURE_SAMPLE_COUNT; i++){
		
		int sample = 0;

		esp_err_t result = adc_oneshot_read(adc_handle, MOISTURE_ADC_CHANNEL, &sample);

		if(result != ESP_OK){

			readings->moisture_status = result;
			break;

		}

		moisture_sum += sample;

	}


	if(readings->light_status == ESP_OK){
		readings->light = light_raw;
	}

	if(readings->water_status == ESP_OK){
		readings->water = water_raw;
	}

	if(readings->moisture_status == ESP_OK){
		
		moist_raw = (moisture_sum + MOISTURE_SAMPLE_COUNT / 2) / MOISTURE_SAMPLE_COUNT; //Round

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



	if(readings->moist_valid){

		float percentage = 100.0f * (MOISTURE_DRY_ADC - readings->moisture) / (MOISTURE_DRY_ADC - MOISTURE_WET_ADC);

		readings->moisture_pct = fminf(100.0f, fmaxf(0.0f, percentage));

		

		readings->activate_pump = readings->moist_valid && readings->water_valid && !readings->water_low && readings->moisture >= MARK_MOISTURE_DRY;//Higher readings are drier soil

	}else {
		readings->moisture_pct = NAN;
		readings->activate_pump = false;
	}



}

static esp_err_t sendWateringEvent(const Watering *event);


void updateWatering(LatestReadings *readings, bool *pump_on){

	static uint32_t cycle_id = 0;

	static uint64_t started_at_ms = 0;
	static uint64_t stopped_at_ms = 0;
	static bool cooling_down = false; //retain the value between calls

	uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000ULL;


	bool watering_safe = readings->moist_valid && readings->water_valid && !readings->water_low;


	bool should_start = watering_safe && (readings->moisture >= MARK_MOISTURE_DRY);

	if(*pump_on){

		bool time_expired = now_ms - started_at_ms >= WATERING_DURATION_MS;

		bool soil_wet = readings->moisture <= MARK_MOISTURE_WET;

		if(!watering_safe || soil_wet || time_expired){ // enought time watering or shouldnt water now

			setPump(false);

			stopped_at_ms = (uint64_t)esp_timer_get_time() / 1000ULL;

			cooling_down = true; 

			const char *reason;

			if(!readings->moist_valid || !readings->water_valid){
				reason = "invalid_sensor";
			}else if(readings->water_low){
				reason = "low_water";
			}else if(time_expired){
				reason = "time_limit";
			}else{
				reason = "soil_wet";
			}

			Watering event = {
				
				.boot_id = watering_boot_id,
				.cycle_id = cycle_id,
				.started = false,
				.occurred_at_ms = stopped_at_ms,
				.duration_ms = stopped_at_ms - started_at_ms,
				.reason = reason,
				.test = false

			};

			esp_err_t result = sendWateringEvent(&event);

			if(result != ESP_OK){
				ESP_LOGW("Watering", "Couldnt queue stop event: %s", esp_err_to_name(result));
			
			}

			ESP_LOGI("Watering", "Pump stopped, cooldown started");

		}

		return;
	}


	if(cooling_down){

		bool cooldown_finished = now_ms - stopped_at_ms >= WATERING_COOLDOWN_MS;

		if(!cooldown_finished){

			return;

		}	


		cooling_down = false;
	}

	if(should_start){

		setPump(true);

		started_at_ms = (uint64_t)esp_timer_get_time() / 1000ULL;

		cycle_id++;

		Watering event = {
			
			.boot_id = watering_boot_id,
			.cycle_id = cycle_id,
			.started = true,
			.occurred_at_ms = started_at_ms,
			.duration_ms = 0,
			.reason = "dry_soil",
			.test = false,
		};

		esp_err_t result = sendWateringEvent(&event);

		if(result != ESP_OK){
			ESP_LOGW("Watering", "Couldnt queue start event: %s", esp_err_to_name(result));
			
		}

		ESP_LOGI("Watering", "Pump started");

	}

}


static esp_err_t sendWateringEvent(const Watering *event){

	if(event == NULL || event->boot_id == NULL || event->reason == NULL){
		return ESP_ERR_INVALID_ARG;
	}

	char payload[384];
	char duration[32];

	if(event->started){
		snprintf(duration, sizeof(duration), "null");
	}else{
		snprintf(duration, sizeof(duration), "%" PRIu64,event->duration_ms);
	}


	int length = snprintf(payload, sizeof(payload), 

			"{\"device_id\":\"01\","
			"\"boot_id\":\"%s\","
			"\"cycle_id\":%" PRIu32 ","
			"\"event\":\"%s\","
			"\"occurred_at_ms\":%" PRIu64 ","
			"\"duration_ms\":%s,"
			"\"reason\":\"%s\","
			"\"test\":%s}",
			event->boot_id,
			event->cycle_id,
			event->started ? "started" : "stopped",
			event->occurred_at_ms,
			duration,
			event->reason,
			event->test ? "true" : "false"
			);

	if(length < 0 || (size_t)length >= sizeof(payload)){
		return ESP_ERR_INVALID_SIZE;
	}

	return mqttQueueWateringEvent(payload);

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

		snprintf(moisture, sizeof(moisture), "%.1f", readings->moisture_pct);

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

		snprintf(readingsBuffer, sizeof(readingsBuffer), "Light: %.1f", readings->light_pct);
		ssd1306_display_text(oled, 2, readingsBuffer, strlen(readingsBuffer), false);

	}else{

		ssd1306_display_text(oled, 2, "Light: ERROR", strlen("Light: ERROR"), false);
	}


	if(!readings->water_valid){
		snprintf(readingsBuffer, sizeof(readingsBuffer), "Water: ERROR");
		ssd1306_display_text(oled, 3, readingsBuffer, strlen(readingsBuffer), false);
	}else {
		snprintf(readingsBuffer, sizeof(readingsBuffer), "Water: %s", readings->water_low ? "LOW" : "OK");
		ssd1306_display_text(oled, 3, readingsBuffer, strlen(readingsBuffer), false);
	}

	
	const char* soil_status;

	if(!readings->moist_valid){
		soil_status = "ERROR";
	}else if (readings->moisture >= MARK_MOISTURE_DRY){
		soil_status = "DRY";
	}else if (readings->moisture <= MOISTURE_WET_ADC){
		soil_status = "WET";	
	}else {
		soil_status = "OK";
	}


	snprintf(readingsBuffer, sizeof(readingsBuffer), "Soil: %s", soil_status);
	ssd1306_display_text(oled, 4, readingsBuffer, strlen(readingsBuffer), false);


	snprintf(readingsBuffer, sizeof(readingsBuffer), "Pump: %s", pump_on ? "ON" : "OFF");
	ssd1306_display_text(oled, 5, readingsBuffer, strlen(readingsBuffer), false);


	const char *status;
	const char *action;

	if (!readings->water_valid || !readings->moist_valid) {
		status = "SENSOR FAULT";
		action = "CHECK WATER/SOIL";
	} else if (readings->water_low) {
		status = "LOW WATER";
		action = "REFILL RESERVOIR";
	} else if (!readings->air_valid || !readings->light_valid) {
		status = "SENSOR FAULT";
		action = "CHECK AIR/LIGHT";
	} else if (pump_on) {
		status = "WATERING";
		action = "";
	} else {
		status = "MONITORING";
		action = "";
	}

	ssd1306_display_text(oled, 6, status, strlen(status), false);

	ssd1306_display_text(oled, 7, action, strlen(action), false);

}

/*
void app_main(void)
{
    // Keep the pump OFF throughout calibration.
    initPump();

    adc_oneshot_unit_handle_t adc_handle = initSensors();

    printf("\nSoil moisture calibration — pump OFF\n");
    printf("Record readings in dry soil and watered, drained soil.\n\n");

    while (1) {
        int sum = 0;
        int minimum = 4095;
        int maximum = 0;
        int successful_reads = 0;

        // Average several samples to reduce noise.
        for (int i = 0; i < 32; i++) {
            int raw = 0;
            esp_err_t result = adc_oneshot_read(
                adc_handle,
                MOISTURE_ADC_CHANNEL,
                &raw
            );

            if (result == ESP_OK) {
                sum += raw;

                if (raw < minimum) minimum = raw;
                if (raw > maximum) maximum = raw;

                successful_reads++;
            } else {
                printf("ADC read failed: %s\n",
                       esp_err_to_name(result));
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }

        if (successful_reads > 0) {
            float average = (float)sum / successful_reads;

            printf(
                "Soil ADC | average: %.1f | min: %d | max: %d"
                " | valid samples: %d/32\n",
                average,
                minimum,
                maximum,
                successful_reads
            );
        } else {
            printf("ERROR: no successful soil readings\n");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

*/


void app_main(void)
{
	//initialize the pump
	initPump();

	//init sensors
	adc_oneshot_unit_handle_t adc_handle = initSensors();

	//init the oled
	SSD1306_t oled = {0};
	initOLED(&oled);

	//init the struct for readings data
	LatestReadings readings = {
		.water_low = true,
		.activate_pump = false
	};

	//flag checking if mqtt is successful
	bool mqtt_started = false;

	//init network
	ESP_ERROR_CHECK(networkInit());
	ESP_LOGI("greenhouse", "Local monitoring started");

	//create 2 random digits
	uint32_t random_parts[2];
	esp_fill_random(random_parts, sizeof(random_parts));
	snprintf(watering_boot_id, sizeof(watering_boot_id), "%08" PRIx32 "%08" PRIx32, random_parts[0], random_parts[1]);

	while (1) {

		//start mqtt client
		if(!mqtt_started) {
			esp_err_t result = mqttInit();

			if(result == ESP_OK){
				mqtt_started = true;
				ESP_LOGI("greenhouse", "MQTT client started");
			}else{
				ESP_LOGW("greenhouse","MQTT initialization failed: %s",esp_err_to_name(result));
			}
		}


		//independant local monitoring
		readSensors(&readings, adc_handle, true);
		validateReadings(&readings);
		
		//display stats on the local oled
		displayResults(&oled, &readings);


		//display the pump checking
		if(readings.activate_pump){
			ssd1306_display_text(&oled, 5, "Pump: checking  ", 16, false);
		}


		//check if should wter
		updateWatering(&readings, &pump_on);
		
		if(pump_on){
			//show if the pump is on
			displayResults(&oled, &readings);
		}
				
		//while watering
		while(pump_on){

			//check if it shpuld stop (time expired)
			updateWatering(&readings, &pump_on);

			//stop
			if(!pump_on)
				break;


			//check only moisture, water level, without another DHT request
			readSensors(&readings, adc_handle, false);
			validateReadings(&readings);

			//check again but because of moisture and water level
			updateWatering(&readings, &pump_on);


			//pump water
			if(pump_on)
				vTaskDelay(pdMS_TO_TICKS(50));


		}

		//display results on the oled again, this time with pump off and new measurements
		displayResults(&oled, &readings);



		//if mqtt is successful, try to publish the data
		if(mqtt_started) {
			char payload[512];

			if(!formatTelemetry(&readings, payload, sizeof(payload))) {
				ESP_LOGE("greenhouse", "Telemetry formatting failed");
			}else{
				esp_err_t result = mqttQueueTelemetry(payload);

				if(result != ESP_OK){
					ESP_LOGW("greenhouse", "Telemetry queue failed: %s", esp_err_to_name(result));
				}
			}
		}

		//this long wait between sensor readings is reache only when the pump is off so that it doesnt interfere with the watering time. 
		vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
	}
}

