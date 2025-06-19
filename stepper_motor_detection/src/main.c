#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define STEP_PIN GPIO_NUM_14
#define DIR_PIN  GPIO_NUM_27
#define EN_PIN   GPIO_NUM_26
#define PROXIMITY_SENSOR_PIN GPIO_NUM_4

#define MICROSTEPS_PER_REV 6400
#define STEPS_FOR_60_DEG ((MICROSTEPS_PER_REV * 60) / 360)
#define TAG "SYSTEM"

void init_gpio() {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << STEP_PIN) | (1ULL << DIR_PIN) | (1ULL << EN_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level(EN_PIN, 1);

    gpio_config_t sensor_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PROXIMITY_SENSOR_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&sensor_conf);
}

void motor_disable_task(void *arg) {
    int delay_ms = (int)(uintptr_t)arg;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    gpio_set_level(EN_PIN, 1);
    ESP_LOGI(TAG, "Motor disabled after %d ms", delay_ms);
    vTaskDelete(NULL);
}

void generate_step_pulses(int steps) {
    gpio_set_level(DIR_PIN, 1);
    gpio_set_level(EN_PIN, 0);

    const int pulse_freq = 10660;

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_1_BIT,
        .freq_hz = pulse_freq,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = STEP_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 1,
        .hpoint = 0,
        .flags.output_invert = 0
    };
    ledc_channel_config(&ledc_channel);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    int duration_ms = (steps * 1000) / pulse_freq;

    xTaskCreate(motor_disable_task, "disable_motor", 2048, (void*)(uintptr_t)duration_ms, 5, NULL);
    ESP_LOGI(TAG, "Motor rotating 60 degrees in %d ms", duration_ms);
}

void app_main() {
    init_gpio();
    int detection_count = 0;

    while (1) {
        int sensor = gpio_get_level(PROXIMITY_SENSOR_PIN);

        if (sensor == 0) {
            ESP_LOGI(TAG, "Object detected");
            detection_count++;
        } else {
            ESP_LOGI(TAG, "No object");
        }

        if (detection_count >= 5) {
            generate_step_pulses(STEPS_FOR_60_DEG);
            detection_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
