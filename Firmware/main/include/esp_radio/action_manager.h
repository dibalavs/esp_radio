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

void action_webstation_stop(void);
void action_webstation_switch(int delta);
void action_webstation_set(unsigned station_no);
unsigned action_webstation_get(void);

void action_fmstation_stop(void);
void action_fmstation_switch(int delta);
void action_fmstation_set(unsigned station_no);
unsigned action_fmstation_get(void);

void action_increase_volume(int delta);
void action_set_volume(uint8_t value);
void action_toogle_time(void);