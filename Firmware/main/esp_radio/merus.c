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

#include <ma120x0.h>
#include <MerusAudio.h>
#include <ext_gpio.h>
#include <esp_err.h>

void merus_init(void)
{

    ext_gpio_set_merus_chip_select(true);
    ext_gpio_get_merus_en();
    init_ma120(0x50);
    ext_gpio_set_merus_mute(false);
    ext_gpio_set_merus_chip_select(false);
}

bool merus_check_present(void)
{
    bool present = false;
    ext_gpio_set_merus_chip_select(true);
    present = ma_check_present();
    ext_gpio_set_merus_chip_select(false);

    return present;
}

void merus_set_volume(uint8_t volume)
{
    ext_gpio_set_merus_chip_select(true);
    ma_write_byte(MA_vol_db_master__a,volume);                // Set vol_db_master low
    ext_gpio_set_merus_chip_select(false);
}