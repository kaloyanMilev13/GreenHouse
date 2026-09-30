#include "driver/gpio.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"


void app_main(void){
//ESP32-WROOM-32D


	gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

	while(1){


		gpio_set_level(GPIO_NUM_2, 1);

		vTaskDelay(pdMS_TO_TICKS(1000));

	       	gpio_set_level(GPIO_NUM_2, 0);
		
		vTaskDelay(pdMS_TO_TICKS(1000));



	}

}
