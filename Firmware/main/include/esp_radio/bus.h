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

#include <esp_err.h>

void bus_init_spi(void);
void bus_init_i2c(void);
void bus_init_i2s(void);