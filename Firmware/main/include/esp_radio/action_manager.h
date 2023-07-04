/**
 * @file action_manager.h
 * @author Vasily Dybala
 * @brief Functions to do main actions.
 * @version 0.1
 * @date 2023-06-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Current device control (webstation or fm station)
bool action_is_running(void);
void action_stop(void);
void action_switch(int delta);
void action_setstation(unsigned station_no);
unsigned action_getstation(void);

// Web radio station control.
bool action_webstation_is_running(void);
void action_webstation_stop(void);
void action_webstation_switch(int delta);
void action_webstation_set(unsigned station_no);
unsigned action_webstation_get(void);

// Fm stations control.
bool action_fmstation_is_running(void);
void action_fmstation_stop(void);
void action_fmstation_switch(int delta);
void action_fmstation_set(unsigned station_no);
unsigned action_fmstation_get(void);

// Volume control
void action_increase_volume(int delta);
void action_set_volume(uint8_t value);

// Power control
void action_power_toggle(void);
void action_power_on(void);
void action_power_off(void);

void action_toogle_time(void);
