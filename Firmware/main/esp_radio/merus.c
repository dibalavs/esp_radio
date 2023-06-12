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

#include <merus.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <ma120x0.h>
#include <MerusAudio.h>
#include <ext_gpio.h>
#include <esp_err.h>
#include <esp_log.h>

#include <driver/i2s.h>

#include "gpio.h"

static const char *TAG = "merus";

void merus_init(void)
{
    ext_gpio_set_merus_chip_select(true);
    ext_gpio_set_merus_mute(true);
    ext_gpio_set_merus_en(true);

    ESP_ERROR_CHECK(i2s_stop(I2S_OUT_NO));
    init_ma120();
    ESP_ERROR_CHECK(i2s_start(I2S_OUT_NO));

    ext_gpio_set_merus_mute(false);
    ext_gpio_set_merus_chip_select(false);
}

bool merus_check_present(void)
{
    bool present = false;
    ext_gpio_set_merus_chip_select(true);
    ESP_ERROR_CHECK(i2s_stop(I2S_OUT_NO));
    present = ma_check_present();
    ESP_ERROR_CHECK(i2s_start(I2S_OUT_NO));
    ext_gpio_set_merus_chip_select(false);

    return present;
}

void merus_set_volume(uint8_t volume)
{
    ext_gpio_set_merus_chip_select(true);
    ESP_ERROR_CHECK(i2s_stop(I2S_OUT_NO));
    set_MA_vol_db_master(volume);
    ESP_ERROR_CHECK(i2s_start(I2S_OUT_NO));
    ext_gpio_set_merus_chip_select(false);
}