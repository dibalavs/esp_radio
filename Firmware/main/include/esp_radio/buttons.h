/**
 * @file keyboard.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief
 * @version 0.1
 * @date 2023-04-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdint.h>

#define ENC_DEBOUCE_MS         10
#define BTN_DOUBLECLICKTIME_MS 500
#define BTN_HOLDTIME_MS        1000

enum button_type {
    BTN_TYPE_PREV,
    BTN_TYPE_NEXT,
    BTN_TYPE_PLAY,
    BTN_TYPE_ENC_BTN,
    BTN_TYPE_ENC_LESS,
    BTN_TYPE_ENC_MORE,
    BTN_TYPE_LAST
};

enum button_state {
    BTN_STATE_PRESSED,
    BTN_STATE_RELEASED,
    BTN_STATE_CLICKED,
    BTN_STATE_HOLD,
    BTN_STATE_DBLCLICKED, // TODO: not implemented yet
};

typedef struct _button_event_t {
    enum button_type button;
    enum button_state state;
} button_event_t;

void buttons_init();
void buttons_service(void);

button_event_t *buttons_get_event();
void buttons_release_event(button_event_t *evt);
uint8_t buttons_get_encoder_value(void);