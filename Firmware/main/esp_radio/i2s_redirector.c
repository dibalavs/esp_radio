#include "driver/i2s_common.h"
#include "freertos/portmacro.h"
#include "lwip/err.h"
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
#include "esp_log.h"
#include "freertos_err.h"
#include "gpio.h"
#include "kmalloc.h" //kmalloc
#include "math.h"
#include <esp_err.h>
#include <string.h>
#include <sys/time.h>
#include <test_i2s_song.h>

static const size_t buf_samples = 256;
static const size_t src_buf_size = buf_samples * sizeof(uint16_t);
static const size_t dst_buf_size = buf_samples * sizeof(uint32_t);

static const char *TAG = "redirector";
static bool is_running = false;

static TaskHandle_t task_handle;

static void redirector_task(void* p)
{
    int16_t *src_buf = calloc(1, src_buf_size);
    int16_t *dst_buf = calloc(1, dst_buf_size);
    size_t read;
    size_t written;
    esp_err_t rc;
    size_t samples;

    is_running = true;

    //test_twinkle_twinkle_little_star();

    while (true) {
        read = buf_samples * sizeof(uint16_t);
        ESP_ERROR_CHECK(i2s_channel_read(i2s_rx_chan, src_buf, src_buf_size, &read, portMAX_DELAY));

        output_mode_t mode = app_state_get_audio_output_mode();
        if (mode == I2S || mode == I2S_MERUS)
            continue;

        samples = read / sizeof(uint16_t);
        for (int i = 0; i < samples; i++) {
            dst_buf[i * 2 + 1] = src_buf[i];
        }

        rc = i2s_channel_write(i2s_tx_chan, dst_buf, samples * sizeof(uint32_t), &written, 1);
        if (rc == ESP_ERR_TIMEOUT || rc == ESP_ERR_INVALID_STATE)
            continue;

        ESP_ERROR_CHECK(rc);
        if (samples != written / sizeof(uint32_t)) {
            ESP_LOGE(TAG, "Not all data was written: %u", written);
        }
    }
}

void i2s_redirector_init(void)
{
    FREERTOS_ERROR_CHECK(xTaskCreate(redirector_task, "i2s_redirector", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, &task_handle));
    i2s_redirector_stop();
}

bool i2s_redirector_is_running(void)
{
    return is_running;
}

void i2s_redirector_start(void)
{
    i2s_channel_enable(i2s_tx_chan);
    vTaskResume(task_handle);
    is_running = true;
}

bool i2s_redirector_stop(void)
{
    bool prev_is_running = is_running;
    if (is_running) {
        vTaskSuspend(task_handle);
        i2s_channel_disable(i2s_tx_chan);
        is_running = false;
    }

    return prev_is_running;
}

void i2s_redirector_restore(bool run)
{
    if (run)
        i2s_redirector_start();
    else
       i2s_redirector_stop();
}