#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "control.pb.h"
#include <pb_decode.h>

#define TAG "PROTO"
#define PORT 3333

#ifndef WIFI_SSID
#define WIFI_SSID  "Mangifera Indica"
#define WIFI_PASS  "azbe50000"
#endif

static void wifi_init_sta(void)
{
    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Configure WiFi credentials
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());
    
    ESP_LOGI(TAG, "WiFi connecting to %s...", WIFI_SSID);
}

static void server_task(void *arg)
{
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (s < 0) {
        ESP_LOGE(TAG, "Failed to create socket: %d", s);
        return;
    }
    
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY)
    };
    
    int result = bind(s, (struct sockaddr *)&addr, sizeof(addr));
    if (result < 0) {
        ESP_LOGE(TAG, "Failed to bind socket: %d", result);
        close(s);
        return;
    }
    
    result = listen(s, 1);
    if (result < 0) {
        ESP_LOGE(TAG, "Failed to listen on socket: %d", result);
        close(s);
        return;
    }
    
    ESP_LOGI(TAG, "Server listening on port %d", PORT);

    while (true) {
        int client = accept(s, NULL, NULL);
        if (client < 0) continue;

        // Read message length (2 bytes)
        uint8_t len_buf[2];
        if (recv(client, len_buf, 2, MSG_WAITALL) != 2) {
            close(client);
            continue;
        }
        
        uint16_t msg_len = (len_buf[0] << 8) | len_buf[1];
        if (msg_len > 256) {
            close(client);
            continue;
        }

        // Read protobuf message
        uint8_t msg_buf[256];
        if (recv(client, msg_buf, msg_len, MSG_WAITALL) != msg_len) {
            close(client);
            continue;
        }

        // Decode protobuf message
        ControlCommand cmd = ControlCommand_init_default;
        pb_istream_t stream = pb_istream_from_buffer(msg_buf, msg_len);

        if (pb_decode(&stream, ControlCommand_fields, &cmd)) {
            ESP_LOGI(TAG, "Received - ID: %" PRIu32 ", Speed: %.2f, Steering: %.2f, Enable: %s",
                     cmd.id, cmd.speed, cmd.steering, cmd.enable ? "true" : "false");
        } else {
            ESP_LOGW(TAG, "Failed to decode message: %s", PB_GET_ERROR(&stream));
        }
        
        close(client);
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    
    // Wait for WiFi connection to be established
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds
    
    xTaskCreatePinnedToCore(server_task, "server", 4096, NULL, 5, NULL, 0);
}