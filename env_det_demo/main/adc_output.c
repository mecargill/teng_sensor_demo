#include "adc_output.h"
#include "adc.h"

#if ADC_OUTPUT_MODE == ADC_OUTPUT_UART

#include "driver/uart.h"
#include "uart.h"

esp_err_t adc_output_init(void)
{
    esp_log_level_set("*", ESP_LOG_NONE); //to not interfere with data streaming
    uart_init(921600);
    return ESP_OK;
}

esp_err_t adc_output_send(const void *data, size_t length)
{
    int written = uart_write_bytes(UART_NUM_0, data, length);
    return written == (int)length ? ESP_OK : ESP_FAIL;
}

#elif ADC_OUTPUT_MODE == ADC_OUTPUT_UDP

#include <errno.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "nvs_flash.h"

#define ADC_UDP_PORT 6000
#define ADC_UDP_DESTINATION "192.168.4.255"
#define ADC_WIFI_SSID "ESP32_ADC"

static const char *tag = "[ADC_OUTPUT]";
static int udp_socket = -1;
static struct sockaddr_in udp_destination;

esp_err_t adc_output_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ADC_WIFI_SSID,
            .ssid_len = sizeof(ADC_WIFI_SSID) - 1,
            .channel = 1,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 1,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (udp_socket < 0) {
        ESP_LOGE(tag, "Could not create UDP socket: errno %d", errno);
        return ESP_FAIL;
    }

    int broadcast = 1;
    if (setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        ESP_LOGE(tag, "Could not enable UDP broadcast: errno %d", errno);
        close(udp_socket);
        udp_socket = -1;
        return ESP_FAIL;
    }

    udp_destination.sin_family = AF_INET;
    udp_destination.sin_port = htons(ADC_UDP_PORT);
    udp_destination.sin_addr.s_addr = inet_addr(ADC_UDP_DESTINATION);
    ESP_LOGI(tag, "Sending ADC UDP packets to %s:%d", ADC_UDP_DESTINATION, ADC_UDP_PORT);
    return ESP_OK;
}

esp_err_t adc_output_send(const void *data, size_t length)
{
    if (udp_socket < 0) {
        return ESP_ERR_INVALID_STATE;
    }

    int sent = sendto(udp_socket, data, length, 0,
                      (struct sockaddr *)&udp_destination, sizeof(udp_destination));
    if (sent < 0) {
        ESP_LOGE(tag, "UDP send failed: errno %d", errno);
        return ESP_FAIL;
    }
    return sent == (int)length ? ESP_OK : ESP_FAIL;
}

#else
#error "ADC_OUTPUT_MODE must be ADC_OUTPUT_UART or ADC_OUTPUT_UDP"
#endif