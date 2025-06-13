#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define PROXIMITY_SENSOR_PIN GPIO_NUM_4
#define TAG "PROXIMITY_SENSOR"

void app_main(void)
{
    gpio_set_direction(PROXIMITY_SENSOR_PIN, GPIO_MODE_INPUT);

    while (1) {
        int sensor_value = gpio_get_level(PROXIMITY_SENSOR_PIN);

        if (sensor_value == 0) {
            ESP_LOGI(TAG, "Object detected!");
        } else {
            ESP_LOGI(TAG, "No object detected");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
