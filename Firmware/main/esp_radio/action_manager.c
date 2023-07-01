/**
 * @file action_manager.c
 * @author Vasily Dybala
 * @brief Functions to do main actions.
 * @version 0.1
 * @date 2023-06-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_radio/action_manager.h"

#include "esp_radio/app_state.h"
#include "esp_radio/ext_gpio.h"
#include "esp_radio/i2s_redirector.h"
#include "esp_radio/helpers.h"
#include "esp_radio/merus.h"

#include "addon.h"
#include "audio_player.h"
#include "eeprom.h"
#include "gpio.h"
#include "webclient.h"
#include "websocket.h"

#include <rda5807.h>

#include <esp_debug_helpers.h>

#include <esp_err.h>
#include <esp_log.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "action";

static bool webstation_is_running;
static bool fmstation_is_running;
static bool is_sleeping = true;

bool action_webstation_is_running(void)
{
    return webstation_is_running;
}

void action_webstation_stop(void)
{
    webstation_is_running = false;

	xSemaphoreGive(sDisconnect);
    event_lcd_t evt;
    evt.lcmd = lstop;
    evt.lline = NULL;
    xQueueSend(event_lcd, &evt, 0);

    ESP_LOGI(TAG,"Webstation stopped.");

    //esp_backtrace_print(10);  // Uncomment, to see backtrace of caller of this function.

	if (get_player_status() != STOPPED)
		audio_player_stop();

    i2s_redirector_stop();
    merus_deinit();
}

void action_webstation_switch(int delta)
{
    unsigned curr = app_state_get_curr_webstation();
    action_webstation_set(curr + delta);
}

void action_webstation_set(unsigned station_no)
{
	struct shoutcast_info* si;
	char answer[32];

    event_lcd_t evt;
	evt.lcmd = estation;
	evt.lline = (char*)((uintptr_t)station_no);
	xQueueSend(event_lcd, &evt, 0);

    app_state_set_curr_webstation(station_no);
    station_no = app_state_get_curr_webstation(); // can be truncated.

    webclient_silent_disconnect();

	si = eeprom_get_webstation(station_no);
    if (si == NULL)
        return;

    webstation_is_running = true;
    fmstation_is_running = false;
    app_state_set_fm(false);

    ESP_LOGI(TAG,"Webstation set: %d, Name: %s", station_no, si->name);
    webclient_set_name(si->name, station_no);
    webclient_set_url(si->domain);
    webclient_set_path(si->file);
    webclient_set_port(si->port);
    app_state_set_ivol_addent(si->ovol);
    action_increase_volume(0); // Just re-new if addent was changed.

    webclient_connect();

	free(si);
	sprintf(answer,"{\"wsstation\":\"%u\"}",station_no);
	websocket_broadcast(answer, strlen(answer));

    merus_init();
    i2s_redirector_start();
    ext_gpio_set_i2s(I2S_SWITCH_VS1053);
}

unsigned action_webstation_get(void)
{
    return app_state_get_curr_webstation();
}

bool action_fmstation_is_running(void)
{
    return fmstation_is_running;
}

void action_fmstation_stop(void)
{
    fmstation_is_running = false;

    ext_gpio_set_fm_chip_select(true);
    rda5807_i2s_on(false);
    rda5807_mute(true);
    ext_gpio_set_fm_chip_select(true);

    i2s_redirector_stop();
    merus_deinit();

    // TODO: Power down FM chip
}

void action_fmstation_switch(int delta)
{
    unsigned curr = app_state_get_curr_fmstation();
    action_fmstation_set(curr + delta);
}

void action_fmstation_set(unsigned station_no)
{
    fmstation_is_running = true;
    webstation_is_running = false;

    ext_gpio_set_fm_chip_select(true);
    rda5807_i2s_on(true);
    rda5807_mute(false);
    rda5807_set_frequency(96.8);
    ext_gpio_set_fm_chip_select(true);

    app_state_set_fm(true);
    merus_init();
    i2s_redirector_start();
    ext_gpio_set_i2s(I2S_SWITCH_FM);
    // TODO setup FM chip
}

unsigned action_fmstation_get(void)
{
    return app_state_get_curr_fmstation();
}

void action_set_volume(uint8_t value)
{
    event_lcd_t evt;
    uint8_t prev_volume = app_state_get_ivol();

    // Uncomment, to see backtrace of caller of this function.
    // if (value < 2) {
    //     ESP_LOGE(TAG, "prev:%d, now:%d", (int)prev_volume, (int)value);
    //     esp_backtrace_print(10);
    // }

    // Master volume not changed, no need to show on LCD.
    if (prev_volume != value) {
        ESP_LOGI(TAG, "Set volume:%u", (unsigned)value);
        app_state_set_ivol(value);
        evt.lcmd = lvol;
        evt.lline = NULL;
        xQueueSend(event_lcd, &evt, 0);
    }

    const uint8_t vol = CLIP_VOLUME(value + app_state_get_ivol_addent());
	//renderer_volume(vol);   // Need only for SW decoder
    //VS1053_SetVolume(vol);  // Need only for analog output on VS1053
    merus_set_volume(vol);
	webclient_ws_vol(vol);
}

void action_increase_volume(int delta)
{
    action_set_volume(CLIP_VOLUME(delta + app_state_get_ivol()));
}

void action_power_toggle(void)
{
    if (is_sleeping) {
        // TODO: wifi wake?
        rda5807_power_on(true);
        rda5807_init(I2C_NO, I2C_ADDR_RDA5807FP, PIN_FM_INT);
        addon_wake_lcd();
        action_switch(0);
    } else {
        action_stop();
        addon_sleep_lcd();
        rda5807_power_on(false);
        // TODO: wifi sleep?
    }

    is_sleeping = !is_sleeping;
}

void action_toogle_time(void)
{

}

bool action_is_running(void)
{
    if (app_state_is_fm())
        return action_fmstation_is_running();
    else
        return action_webstation_is_running();
}

void action_stop(void)
{
    if (app_state_is_fm())
        action_fmstation_stop();
    else
        action_webstation_stop();
}

void action_switch(int delta)
{
    if (app_state_is_fm())
        action_fmstation_switch(delta);
    else
        action_webstation_switch(delta);
}

void action_setstation(unsigned station_no)
{
    if (app_state_is_fm())
        return action_fmstation_set(station_no);
    else
        return action_webstation_set(station_no);
}

unsigned action_getstation(void)
{
    if (app_state_is_fm())
        return action_fmstation_get();
    else
        return action_webstation_get();
}
