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

#include "esp_radio/action_manager.h"
#include "esp_radio/app_state.h"
#include "esp_radio/merus.h"

#include "addon.h"
#include "webclient.h"

#include <stdint.h>
#include <stdio.h>

void action_webstation_playpause(void)
{

}

void action_webstation_switch(int delta)
{

}

void action_webstation_set(unsigned station_no)
{

}

void action_fmstation_playpause(void)
{

}

void action_fmstation_switch(int delta)
{

}

void action_fmstation_set(unsigned station_no)
{

}

void action_set_volume(uint8_t value)
{
    action_increase_volume(value - app_state_get_ivol());
}

void action_increase_volume(int delta)
{
    event_lcd_t evt;

	app_state_set_ivol(app_state_get_ivol() + delta);
    const uint8_t vol = app_state_get_ivol();

    evt.lcmd = lvol;
    evt.lline = NULL;
    xQueueSend(event_lcd, &evt, 0);

	//renderer_volume(vol);   // Need only for SW decoder
    //VS1053_SetVolume(vol);  // Need only for analog output on VS1053
    merus_set_volume(vol);
	webclient_ws_vol(vol);
}

void action_toogle_time(void)
{

}