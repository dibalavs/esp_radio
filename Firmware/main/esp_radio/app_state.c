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

#include <esp_attr.h>

static output_mode_t audio_output_mode;
static uint8_t clientIvol = 0;
static uint8_t clientIvolAddent = 0;

IRAM_ATTR uint8_t app_state_get_ivol(void)
{
    return clientIvol;
}

IRAM_ATTR void app_state_set_ivol(int vol)
{
    vol += clientIvolAddent;

    if (vol > 254)
        vol = 254;
    else if (vol < 0)
        vol = 0;
    clientIvol = vol;
};

void app_state_set_ivol_addent(uint8_t vol)
{
    clientIvolAddent = vol;
}

IRAM_ATTR output_mode_t app_state_get_audio_output_mode(void)
{
    return audio_output_mode;
}

void app_state_set_audio_output_mode(output_mode_t mode)
{
    audio_output_mode = mode;
}