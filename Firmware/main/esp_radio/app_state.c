/**
 * @file app_state.c
 * @author Vasily Dybala
 * @brief Hold current application state
 * @version 0.1
 * @date 2023-06-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "esp_radio/app_state.h"
#include "eeprom.h"

#include <esp_debug_helpers.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/timers.h"

#include <assert.h>
#include <esp_attr.h>
#include <esp_log.h>
#include <limits.h>
#include <string.h>

static const char *TAG = "appstate";

static TimerHandle_t update_timer;
static int8_t ivol_addent = 0;
static struct device_settings *settings;

IRAM_ATTR uint8_t app_state_get_ivol(void)
{
    return settings->vol;
}

IRAM_ATTR void app_state_set_ivol(uint8_t vol)
{
    settings->vol = vol;
    xTimerStart(update_timer, portMAX_DELAY);
};

void app_state_set_ivol_addent(int8_t vol)
{
    ivol_addent = vol;
}

int8_t app_state_get_ivol_addent(void)
{
    return ivol_addent;
}

IRAM_ATTR output_mode_t app_state_get_audio_output_mode(void)
{
    return settings->audio_output_mode;
}

void app_state_set_audio_output_mode(output_mode_t mode)
{
    settings->audio_output_mode = mode;
    xTimerStart(update_timer, portMAX_DELAY);
}

void app_state_set_curr_webstation(unsigned sta_no)
{
    settings->web_station = sta_no;
    xTimerStart(update_timer, portMAX_DELAY);
}

unsigned app_state_get_curr_webstation(void)
{
    return settings->web_station;
}

void app_state_set_curr_fmstation(unsigned sta_no)
{
    settings->fm_station = sta_no;
    xTimerStart(update_timer, portMAX_DELAY);
}

unsigned app_state_get_curr_fmstation(void)
{
    return settings->fm_station;
}

struct device_settings *app_state_get_settings(void)
{
    return settings;
}

static void update_settings_cb(TimerHandle_t xTimer)
{
    (void)xTimer;

    struct device_settings *old = eeprom_get_device_settings();
    if (old->web_station != settings->web_station
     || old->fm_station != settings->web_station
     || old->vol != settings->vol
     || old->audio_output_mode != settings->audio_output_mode) {

        old->web_station = settings->web_station;
        old->fm_station = settings->fm_station;
        old->vol = settings->vol;
        old->audio_output_mode = settings->audio_output_mode;
        ESP_LOGI(TAG, "Found settings changes. Update device settings.");
        eeprom_save_device_settings(old);
    }

    free(old);
}

void app_state_init(void)
{
    //init hardware
    eeprom_partitions_init();
    ESP_LOGI(TAG, "Partition init done...");

    settings = eeprom_get_device_settings();
    assert(settings);

    if (settings->cleared != 0xAABB)
    {
        ESP_LOGE(TAG,"Device config not ok. Try to restore");
        free(settings);
        eeprom_restore_device_settings(); // try to restore the config from the saved one
        settings = eeprom_get_device_settings();
        if (settings->cleared != 0xAABB)
        {
            ESP_LOGE(TAG,"Device config not cleared. Clear it.");
            free(settings);
            eeprom_erase_all();
            settings = eeprom_get_device_settings();
            settings->cleared = 0xAABB; //marker init done
            settings->uartspeed = 115200; // default
			settings->audio_output_mode = VS1053; // default
            settings->trace_level = ESP_LOG_INFO; //default
            settings->vol = 100; //default
            settings->led_gpio = -1;
            eeprom_save_device_settings(settings);
        } else
            ESP_LOGE(TAG,"Device config restored");
    }

    //set hostname and instance name
    if ((strlen(settings->hostname) == 0) || (strlen(settings->hostname) > HOSTLEN))
        strcpy(settings->hostname, "esp_radio");

    update_timer = xTimerCreate("DelayedSettingsUpdate", pdMS_TO_TICKS(30000), pdFALSE, NULL, update_settings_cb);
    assert(update_timer);
}