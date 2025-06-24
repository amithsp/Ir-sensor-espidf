#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_SWITCH1 GPIO_NUM_34
#define GPIO_SWITCH2 GPIO_NUM_35

static const char *TAG = "GPIO_INPUT";

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_SWITCH1) | (1ULL << GPIO_SWITCH2),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    while (1)
    {
        int switch1_state = gpio_get_level(GPIO_SWITCH1);
        int switch2_state = gpio_get_level(GPIO_SWITCH2);

        ESP_LOGI(TAG, "Switch1 (GPIO34): %s | Switch2 (GPIO35): %s",
                 switch1_state ? "PRESSED" : "RELEASED",
                 switch2_state ? "PRESSED" : "RELEASED");

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
