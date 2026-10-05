#include <string.h>
#include <esp_log.h>
#include <esp_adc/adc_continuous.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h> // The OS kernel
#include <freertos/task.h>  
#include <esp_attr.h>
#include "driver/uart.h"

#include "adc.h"
#include "adc_output.h"

#define ADC_READ_LEN_SAMPLES 64 //in samples!! not bytes

#define ADC_WRITE_LEN 128 + 1 //+1 for the length field. in samples!!
#define ADC_DOWNSAMPLE_FACTOR 10

//packet format: [start bytes][length of data in 16-bit words][data, little-endian][end bytes]

static const char *tag = "[ADC]";

static TaskHandle_t adc_task_handle;


static bool IRAM_ATTR conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) 
{
    BaseType_t high_task_wakeup = pdFALSE;
    
    // Notify the task (using the handle passed via user_data)
    vTaskNotifyGiveFromISR((TaskHandle_t)user_data, &high_task_wakeup);
    
    // Force an immediate context switch if the task is higher priority
    portYIELD_FROM_ISR(high_task_wakeup);
    
    return (high_task_wakeup == pdTRUE);
}



static void adc_proc_stream(adc_continuous_data_t *parsed_data, uint32_t num_samples)
{
    static const uint8_t start_bytes[2] = {0xAB, 0xAB};
    static const uint8_t end_bytes[2] = {0xCD, 0xCD};
    static uint16_t out_buf[ADC_WRITE_LEN];//Pack 12 bits into 16 bits, one extra for the length
    static uint16_t out_buf_idx = 1;//to keep track of how many samples we actually wrote
    static int ds_count = 0;
    static uint8_t packet[2 + sizeof(out_buf) + 2];

    for (int i = 0; i < num_samples; i++) 
    {
        ds_count++;
        if (parsed_data[i].valid && (ds_count >= ADC_DOWNSAMPLE_FACTOR) && (out_buf_idx < ADC_WRITE_LEN)) 
        {
            ds_count = 0;
            out_buf[out_buf_idx] = (uint16_t) parsed_data[i].raw_data;
            out_buf_idx++;
        }
    }
    //only write once out_buf fils up enough (space left is less than what could be written)
    if (ADC_WRITE_LEN - out_buf_idx < ADC_READ_LEN_SAMPLES)
    {
        //because I increment index after write, it is also a count of how many samples written
        //including the lenth itself, so subtract one
        out_buf[0] = out_buf_idx-1;
        
        
        memcpy(packet, start_bytes, sizeof(start_bytes));
        memcpy(packet + sizeof(start_bytes), out_buf, out_buf_idx * sizeof(out_buf[0]));
        memcpy(packet + sizeof(start_bytes) + out_buf_idx * sizeof(out_buf[0]), end_bytes, sizeof(end_bytes));
        adc_output_send(packet, sizeof(start_bytes) + out_buf_idx * sizeof(out_buf[0]) + sizeof(end_bytes));
      
        
        out_buf_idx = 1;
    }
}


//we are passed the adc handle
static void adc_proc_task(void *pvParameters)
{
    
    uint32_t num_samples = 0;
    adc_continuous_data_t parsed_data[ADC_READ_LEN_SAMPLES];//number of samples to read - 12 bit resolution
    
    adc_continuous_handle_t handle = (adc_continuous_handle_t)pvParameters;

    while (1) {
        // Go to sleep until the ISR calls vTaskNotifyGiveFromISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        esp_err_t ret = adc_continuous_read_parse(handle, parsed_data, ADC_READ_LEN_SAMPLES, &num_samples, 100);
        if (ret == ESP_OK) {

            adc_proc_stream(parsed_data, num_samples);
  
        }
        else
        {
            ESP_LOGE(tag, "ADC reading failed: %s", esp_err_to_name(ret));
        }

    }
    
}

void adc_init(void)
{
    //Range is ~150mV to ~2450mV with this setting of 12dB attenuation

    ESP_LOGI(tag, "Configuring ADC");
     //get handle to adc and initialize driver
    adc_continuous_handle_t adc_handle;
    adc_continuous_handle_cfg_t adc_handle_cfg = {
        .max_store_buf_size = 1024,
        .conv_frame_size = ADC_READ_LEN_SAMPLES*2, //2 bytes per sample
        };
    //ESP_LOGI("CHECKPOINT", "intitializing and getting adc handle ");
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_handle_cfg, &adc_handle));

    adc_digi_pattern_config_t pattern_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bit_width = ADC_BITWIDTH_12,
        .channel = ADC_CHANNEL_3,
        .unit = ADC_UNIT_1
    };
    adc_continuous_config_t adc_cfg = {
        .pattern_num = 1,
        .adc_pattern = &pattern_cfg,
        .sample_freq_hz = 20000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1
    };
    //ESP_LOGI("CHECKPOINT", "configuring adc");
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &adc_cfg));
    
    

    xTaskCreate(adc_proc_task, "adc_task", 4096, adc_handle, 6, &adc_task_handle);

   

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = conv_done_cb,
    };
    ESP_LOGI(tag, "Registering ADC callback");
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, adc_task_handle));
    //the adc task handle is passed as input to conv_done_cb


    //ESP_LOGI("CHECKPOINT", "starting adc");
    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));
}
