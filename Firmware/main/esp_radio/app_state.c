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

#include <esp_debug_helpers.h>

#include <assert.h>
#include <esp_attr.h>
#include <limits.h>

static output_mode_t audio_output_mode;
static uint8_t ivol = 0;
static int8_t ivol_addent = 0;
static unsigned webstation_no = UINT_MAX;
static unsigned fmstation_no = UINT_MAX;

IRAM_ATTR uint8_t app_state_get_ivol(void)
{
    return ivol;
}

IRAM_ATTR void app_state_set_ivol(uint8_t vol)
{
    ivol = vol;
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
}

void app_state_set_curr_webstation(unsigned sta_no)
{
    webstation_no = sta_no;
}

unsigned app_state_get_curr_webstation(void)
{
    return webstation_no;
}

void app_state_set_curr_fmstation(unsigned sta_no)
{
    fmstation_no = sta_no;
}

unsigned app_state_get_curr_fmstation(void)
{
    return fmstation_no;
}
