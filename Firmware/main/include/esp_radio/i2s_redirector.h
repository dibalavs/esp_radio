/**
 * @file i2s_redirector.h
 * @author Vasily Dybala
 * @brief Task to redirect i2s streams with fixing it format.
 *  Original idea - is fix incompatibility VS1053 output and MA12070P input.
 * @version 0.1
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <stdbool.h>

void i2s_redirector_init(void);

bool i2s_redirector_is_running(void);

void i2s_redirector_start(void);
void i2s_redirector_stop(void);
void i2s_redirector_restore(bool run);