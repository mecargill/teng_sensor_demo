#pragma once

#include <stddef.h>
#include "esp_err.h"

esp_err_t adc_output_init(void);
esp_err_t adc_output_send(const void *data, size_t length);