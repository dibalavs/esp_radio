#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

/**
 * @file i2s_redirector.c
 * @author Vasily Dybala
 * @brief Task to redirect i2s streams with fixing it format.
 *  Original idea - is fix incompatibility VS1053 output and MA12070P input.
 * @version 0.1
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "i2s_redirector.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <string.h>
#include <sys/time.h>
#include <esp_err.h>

#include <driver/i2s.h>
#include <driver/i2s_types_legacy.h>
#include "esp_log.h"
#include "gpio.h"
#include "freertos_err.h"
#include "kmalloc.h" //kmalloc

static const size_t buf_samples = 512;
static const size_t src_buf_size = buf_samples * sizeof(uint16_t);
static const size_t dst_buf_size = buf_samples * sizeof(uint32_t);

static const char *TAG = "redirector";
static bool is_running = false;

static void redirector_task(void* p)
{
    uint16_t *src_buf = kmalloc(src_buf_size);
    uint32_t *dst_buf = kmalloc(dst_buf_size);
    size_t read;
    size_t written;

    is_running = true;

    while (true) {
        read = buf_samples * sizeof(uint16_t);
        ESP_ERROR_CHECK(i2s_read(I2S_IN_NO, src_buf, src_buf_size, &read, portMAX_DELAY));

        output_mode_t mode = app_state_get_audio_output_mode();
        if (mode == I2S || mode == I2S_MERUS)
            continue;

        for (int i = 0; i < buf_samples; i++) {
            dst_buf[i] = src_buf[i] << 16;
        }

        ESP_ERROR_CHECK(i2s_write(I2S_OUT_NO, dst_buf, dst_buf_size, &written, portMAX_DELAY));
        if (written / sizeof(uint32_t) != read / sizeof(uint16_t)) {
            ESP_LOGE(TAG, "Not all data was written: %u", written);
        }
    }
}

void i2s_redirector_init(void)
{
    static TaskHandle_t task_handle;
    FREERTOS_ERROR_CHECK(xTaskCreate(redirector_task, "i2s_redirector", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, &task_handle));
}

bool i2s_redirector_is_running(void)
{
    return is_running;
}

void i2s_redirector_start(void)
{
    ESP_ERROR_CHECK(i2s_start(I2S_OUT_NO));
    ESP_ERROR_CHECK(i2s_start(I2S_IN_NO));
    is_running = true;
}

void i2s_redirector_stop(void)
{
    ESP_ERROR_CHECK(i2s_stop(I2S_OUT_NO));
    ESP_ERROR_CHECK(i2s_stop(I2S_IN_NO));
    is_running = false;
}

void i2s_redirector_restore(bool run)
{
    if (run)
        i2s_redirector_start();
    else
        i2s_redirector_stop();
}