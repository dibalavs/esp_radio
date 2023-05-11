/**
 * @file merus.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Control of MerusAudio amplifier.
 * NOTE: since MerusAudio has the same I2C address as
 *  Fm chip, it is used chip select emulation via
 *  disabling SCL signal.
 *  Need to enable chip before any communication.
 * @version 0.1
 * @date 2023-04-22
 *
 * @copyright Copyright (c) 2023
 *
 */

 #pragma once

#include <stdbool.h>
#include <stdint.h>

void merus_init(void);

bool merus_check_present(void);

void merus_set_volume(uint8_t volume);