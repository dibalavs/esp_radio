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

#include <stdint.h>

typedef enum {
    I2S, I2S_MERUS, DAC_BUILT_IN, PDM, VS1053, SPDIF, BTOOTH
} output_mode_t;

typedef struct {
    int type;               /*!< event type */
    int i1;                 /*!< TIMER_xxx timer group */
    int i2;                 /*!< TIMER_xxx timer number */
} queue_event_t;

output_mode_t app_state_get_audio_output_mode(void);
void app_state_set_audio_output_mode(output_mode_t mode);

uint8_t app_state_get_ivol(void);
void app_state_set_ivol(int vol);
void app_state_set_ivol_addent(uint8_t vol);
