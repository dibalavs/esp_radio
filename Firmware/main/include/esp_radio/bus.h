/**
 * @file bus.h
 * @author Vasily Dybala
 * @brief Bus initialization code
 * @version 0.1
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <i2c_bus.h>
#include <driver/i2s_types.h>
#include <esp_err.h>

void bus_init_spi(void);
void bus_init_i2c(void);
void bus_init_i2s(void);

i2c_bus_handle_t bus_i2c_get(void);
i2s_chan_handle_t bus_i2s_get(void);