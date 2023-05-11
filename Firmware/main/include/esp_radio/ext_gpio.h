/**
 * @file ext_gpio.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Extended GPIO via i2c extender.
 *   Currenlty next devices are connected to expander:
 *   - i2S switcher
 *   - Merus amplifier (En and Mute signals)
 *   - Tactile buttons (Prev, Play, Next)
 *   - Rotary encoder with push button
 *   - LCD screen power control
 * @version 0.1
 * @date 2023-04-13
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdbool.h>

typedef void ext_gpio_callback_t(void);

enum ext_gpio_i2s_state {
    I2S_SWITCH_ESP,
    I2S_SWITCH_VS1053,
    I2S_SWITCH_FM
};

void ext_gpio_init(void);

void ext_gpio_fetch_int_captured(void);

void ext_gpio_set_int_callback(ext_gpio_callback_t *cb_isr);

bool ext_gpio_check_present(void);

void ext_gpio_set_lcd(bool enable);
bool ext_gpio_get_lcd(void);

void ext_gpio_set_i2s(enum ext_gpio_i2s_state state);
enum ext_gpio_i2s_state ext_gpio_get_i2s(void);

void ext_gpio_set_merus_en(bool enable);
bool ext_gpio_get_merus_en(void);

void ext_gpio_set_merus_mute(bool enable);
bool ext_gpio_get_merus_mute(void);

void ext_gpio_set_merus_chip_select(bool enable);
void ext_gpio_set_fm_chip_select(bool enable);

bool ext_gpio_get_enca(void);
bool ext_gpio_get_encb(void);
bool ext_gpio_get_enc_button(void);

bool ext_gpio_get_button_prev(void);
bool ext_gpio_get_button_play(void);
bool ext_gpio_get_button_next(void);