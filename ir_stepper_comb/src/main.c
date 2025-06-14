#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define STEP_PIN GPIO_NUM_14
#define DIR_PIN  GPIO_NUM_12
#define EN_PIN   GPIO_NUM_13

#define PROXIMITY_SENSOR_PIN GPIO_NUM_4

#define MICROSTEPS_PER_REV 6400
#define STEPS_FOR_60_DEG ((MICROSTEPS_PER_REV * 60) / 360)

#define TAG "SYSTEM"

void delay_us(uint32_t us) {
    int64_t start = esp_timer_get_time();
    while ((esp_timer_get_time() - start) < us);
}

void rotate_motor_60_degrees() {
    gpio_set_level(EN_PIN, 0); 
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_level(DIR_PIN, 1); 

    for (int i = 0; i < STEPS_FOR_60_DEG; i++) {
        gpio_set_level(STEP_PIN, 1);
        delay_us(500);
        gpio_set_level(STEP_PIN, 0);
        delay_us(500);
    }

    gpio_set_level(EN_PIN, 1);
    ESP_LOGI(TAG, "Motor rotated 60 degrees");
}

void app_main(void) {
    gpio_config_t motor_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << STEP_PIN) | (1ULL << DIR_PIN) | (1ULL << EN_PIN),
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    gpio_config(&motor_conf);

    gpio_config_t sensor_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PROXIMITY_SENSOR_PIN),
        .pull_down_en = 0,
        .pull_up_en = 1 
    };
    gpio_config(&sensor_conf);

    int detection_count = 0;
    bool motor_turned = false;

    while (1) {
        int sensor_value = gpio_get_level(PROXIMITY_SENSOR_PIN);

        if (sensor_value == 0) { 
            ESP_LOGI(TAG, "Object detected!");
            detection_count++;
        } else {
            ESP_LOGI(TAG, "No object detected");
        }

        if (detection_count >= 5 && !motor_turned) {
            rotate_motor_60_degrees();
            motor_turned = true;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
