/**
 * @file app_state.h
 * @author Vasily Dybala
 * @brief Hold current application state
 * @version 0.1
 * @date 2023-06-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    I2S, I2S_MERUS, DAC_BUILT_IN, PDM, VS1053, SPDIF, BTOOTH
} output_mode_t;

typedef struct {
    int type;               /*!< event type */
    int i1;                 /*!< TIMER_xxx timer group */
    int i2;                 /*!< TIMER_xxx timer number */
} queue_event_t;

struct device_settings;

#define NO_STATION 0xFFFF

void app_state_init(void);

output_mode_t app_state_get_audio_output_mode(void);
void app_state_set_audio_output_mode(output_mode_t mode);

uint8_t app_state_get_ivol(void);
int8_t app_state_get_ivol_addent(void);
void app_state_set_ivol(uint8_t vol);
void app_state_set_ivol_addent(int8_t vol); // minor addent, specific for each station.

void app_state_set_curr_webstation(unsigned sta_no);
unsigned app_state_get_curr_webstation(void);

void app_state_set_curr_fmstation(unsigned sta_no);
unsigned app_state_get_curr_fmstation(void);

void app_state_set_fm(bool is_fm);
bool app_state_is_fm(void);

struct device_settings *app_state_get_settings(void); // TODO: remove it.