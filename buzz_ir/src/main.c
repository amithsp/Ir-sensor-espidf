#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define IR_SENSOR_PIN GPIO_NUM_5
#define BUZZER_PIN GPIO_NUM_2

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT

// Detection threshold and continuous beep settings
#define MAX_DETECTIONS          10
#define CONTINUOUS_BEEP_FREQ    2000    // 2kHz for different tone
#define CONTINUOUS_BEEP_DURATION 5000   // 5 seconds continuous beep

static const char *TAG = "OBSTACLE_DETECTION";

// Global counter for obstacle detections
static int detection_count = 0;
static bool continuous_mode = false;

// Function to play a beep with specific frequency and duration
void play_beep(uint32_t frequency, uint32_t duration_ms)
{
    // Set frequency
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency);
    
    // Turn on buzzer (50% duty cycle)
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 4096);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    
    // Wait for duration
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    // Turn off buzzer
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// Function to start continuous beep
void start_continuous_beep(uint32_t frequency)
{
    // Set frequency
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequency);
    
    // Turn on buzzer (50% duty cycle)
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 4096);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// Function to stop continuous beep
void stop_continuous_beep(void)
{
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

// Function to play obstacle alert pattern
void play_obstacle_alert(void)
{
    for (int i = 0; i < 3; i++) {
        play_beep(1000, 200);  // 1kHz beep for 200ms
        vTaskDelay(pdMS_TO_TICKS(100)); // 100ms silence
    }
}

// Function to play continuous beep for specified duration
void play_continuous_alert(void)
{
    ESP_LOGI(TAG, "Playing continuous beep for %d seconds...", CONTINUOUS_BEEP_DURATION/1000);
    
    start_continuous_beep(CONTINUOUS_BEEP_FREQ);
    vTaskDelay(pdMS_TO_TICKS(CONTINUOUS_BEEP_DURATION));
    stop_continuous_beep();
    
    ESP_LOGI(TAG, "Continuous beep finished. Resetting detection count.");
    
    // Reset counter and mode after continuous beep
    detection_count = 0;
    continuous_mode = false;
}

void buzzer_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = 1000,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BUZZER_PIN,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void ir_sensor_init(void)
{
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << IR_SENSOR_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
}

void obstacle_detection_task(void *pvParameters)
{
    bool previous_state = false;
    
    while (1) {
        int sensor_state = gpio_get_level(IR_SENSOR_PIN);
        bool obstacle_detected = (sensor_state == 0);
        
        if (obstacle_detected && !previous_state && !continuous_mode) {
            detection_count++;
            ESP_LOGI(TAG, "Obstacle detected! Count: %d/%d", detection_count, MAX_DETECTIONS);
            
            if (detection_count >= MAX_DETECTIONS) {
                continuous_mode = true;
                ESP_LOGI(TAG, "Maximum detections reached! Switching to continuous mode.");
                play_continuous_alert();
            } else {
                play_obstacle_alert();
            }
        }
        
        previous_state = obstacle_detected;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Enhanced Obstacle Detection System");
    ESP_LOGI(TAG, "Normal beep: 1kHz, Continuous beep after %d detections: %dHz", 
             MAX_DETECTIONS, CONTINUOUS_BEEP_FREQ);
    
    ir_sensor_init();
    buzzer_init();
    
    xTaskCreate(obstacle_detection_task, "obstacle_detection", 2048, NULL, 10, NULL);
    
    ESP_LOGI(TAG, "System ready. Place object near IR sensor to test...");
}
