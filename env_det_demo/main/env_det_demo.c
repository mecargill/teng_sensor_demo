
#include "freertos/FreeRTOS.h" 
#include <stdio.h>
#include "uart.h"
#include "adc.h"
#include "adc_output.h"
#include <esp_log.h>

static const char *tag = "[MAIN]";

void app_main(void)
{
    //Initialize uart and the ADC
    ESP_LOGI(tag, "Starting up. Logs disabled in 15 seconds...");
    ESP_ERROR_CHECK(adc_output_init());
    
    adc_init();
}
