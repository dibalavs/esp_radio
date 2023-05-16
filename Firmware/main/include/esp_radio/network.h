/**
 * @file network.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Network management functions
 * @version 0.1
 * @date 2023-05-14
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
} wifi_ssid_t;

typedef void (netword_cb_t) (bool is_ap);

void network_set_cb(netword_cb_t *on_connected, netword_cb_t *on_disconnected);

void network_init(const wifi_ssid_t *sta, const wifi_ssid_t *ap, bool is_ap);

void network_deinit(void);

void network_wait(void);

const char* app_get_ip();