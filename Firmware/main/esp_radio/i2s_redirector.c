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
#include <sys/time.h>
#include <esp_err.h>

#include <driver/i2s.h>
#include <driver/i2s_types_legacy.h>
#include "esp_log.h"
#include "gpio.h"
#include "freertos_err.h"
#include "kmalloc.h" //kmalloc

static const size_t buf_size = 512 * 4;
static const char *TAG = "redirector";
static bool is_running = false;

static void redirector_task(void* p)
{
    void *buf = kmalloc(buf_size);
    size_t read = buf_size;
    size_t written;

    is_running = true;

    while (true) {
        ESP_ERROR_CHECK(i2s_read(I2S_IN_NO, buf, buf_size, &read, portMAX_DELAY));
        ESP_ERROR_CHECK(i2s_write_expand(I2S_OUT_NO, buf, read, I2S_BITS_PER_SAMPLE_16BIT, I2S_BITS_PER_SAMPLE_32BIT, &written, portMAX_DELAY));
        if (written != buf_size) {
            ESP_LOGE(TAG, "Not all data was written: %u", written);
        }
    }
}

void i2s_redirector_init(void)
{
    static TaskHandle_t task_handle;
    FREERTOS_ERROR_CHECK(xTaskCreatePinnedToCore(redirector_task, "i2s_redirector", configMINIMAL_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, &task_handle, 1));
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