idf_component_register(SRCS "env_det_demo.c"
                            "adc.c"
                            "uart.c"
                            "adc_output.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_adc esp_driver_uart esp_wifi esp_event esp_netif nvs_flash lwip)


