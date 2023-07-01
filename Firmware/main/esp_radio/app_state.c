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
static const char *master_volume_name = "volume";          // u8
static const char *audio_output_mode_name = "output_mode"; // u8
static const char *web_station_no_name = "web_station_no"; // u16
static const char *fm_station_no_name = "fm_station_no";   // u16
static const char *is_current_fm_name = "is_current_fm";   // u8

static TimerHandle_t update_timer;

// Cache of setting, to fast access.
static int8_t ivol_addent = 0;
static uint8_t master_volume = 0;
static uint8_t is_current_fm = false;
static output_mode_t audio_output_mode = 0;
static uint16_t web_station_no = NO_STATION;
static uint16_t fm_station_no = NO_STATION;
static struct device_settings *settings;

IRAM_ATTR uint8_t app_state_get_ivol(void)
{
    return master_volume;
}

IRAM_ATTR void app_state_set_ivol(uint8_t vol)
{
    master_volume = vol;
    eeprom_settings_set_int8(master_volume_name, vol);
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
    return audio_output_mode;
}

void app_state_set_audio_output_mode(output_mode_t mode)
{
    audio_output_mode = mode;
    eeprom_settings_set_int8(audio_output_mode_name, (int8_t)mode);
    eeprom_settings_commit();
}

void app_state_set_curr_webstation(unsigned sta_no)
{
    web_station_no = sta_no;
    eeprom_settings_set_int16(web_station_no_name, (int16_t)sta_no);
    xTimerStart(update_timer, portMAX_DELAY);
}

unsigned app_state_get_curr_webstation(void)
{
    return web_station_no;
}

void app_state_set_curr_fmstation(unsigned sta_no)
{
    fm_station_no = sta_no;
    eeprom_settings_set_int16(fm_station_no_name, (int16_t)sta_no);
    xTimerStart(update_timer, portMAX_DELAY);
}

unsigned app_state_get_curr_fmstation(void)
{
    return fm_station_no;
}

void app_state_set_fm(bool is_fm)
{
    is_current_fm = is_fm;
    eeprom_settings_set_int8(is_current_fm_name, (int8_t)is_fm);
    xTimerStart(update_timer, portMAX_DELAY);
}

bool app_state_is_fm(void)
{
    return is_current_fm;
}

struct device_settings *app_state_get_settings(void)
{
    return settings;
}

static void update_settings_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    eeprom_settings_commit();
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
            settings->trace_level = ESP_LOG_INFO; //default
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

    // Get current values of settings or use default
    eeprom_settings_get_int8(master_volume_name, (int8_t *)&master_volume, 100);
    eeprom_settings_get_int8(audio_output_mode_name, (int8_t *)&audio_output_mode, VS1053);
    eeprom_settings_get_int16(web_station_no_name, (int16_t *)&web_station_no, NO_STATION);
    eeprom_settings_get_int16(fm_station_no_name, (int16_t *)&fm_station_no, NO_STATION);
    eeprom_settings_get_int8(is_current_fm_name, (int8_t *)&is_current_fm, false);
}