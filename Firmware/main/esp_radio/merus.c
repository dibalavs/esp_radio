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
#include "i2s_redirector.h"

static const char *TAG = "merus";

void merus_init(void)
{
    ext_gpio_set_merus_chip_select(true);
    ext_gpio_set_merus_mute(true);
    ext_gpio_set_merus_en(true);

    bool is_running = i2s_redirector_is_running();
    i2s_redirector_stop();
    init_ma120();
    i2s_redirector_restore(is_running);

    ext_gpio_set_merus_mute(false);
    ext_gpio_set_merus_chip_select(false);
}

void merus_deinit(void)
{
    ext_gpio_set_merus_chip_select(false);
    ext_gpio_set_merus_mute(true);
    ext_gpio_set_merus_en(false);
}

bool merus_check_present(void)
{
    bool present = false;
    ext_gpio_set_merus_chip_select(true);
    bool is_running = i2s_redirector_is_running();
    i2s_redirector_stop();
    present = ma_check_present();
    i2s_redirector_restore(is_running);
    ext_gpio_set_merus_chip_select(false);

    return present;
}

void merus_set_volume(uint8_t volume)
{
    ext_gpio_set_merus_chip_select(true);
    bool is_running = i2s_redirector_is_running();
    i2s_redirector_stop();
    set_MA_vol_db_master(255 - volume);
    i2s_redirector_restore(is_running);
    ext_gpio_set_merus_chip_select(false);
}

void merus_get_status(void)
{
    int mute0 = 0;
    int mute1 = 0;
    int system_mute = 0;
    int err = 0;
    int err_acc = 0;
    int audio_proc_mute = 0;
    int lim_mon = 0;
    int lim_clip = 0;
    bool is_running = i2s_redirector_is_running();
    i2s_redirector_stop();
    ext_gpio_set_merus_chip_select(true);
    set_MA_eh_clear(0);             // clear errors
    set_MA_eh_clear(1);
    set_MA_eh_clear(0);

    mute0 = get_MA_dcu_mon0__mute();
    mute1 = get_MA_dcu_mon1__mute();
    err_acc = get_MA_error_acc();
    err = get_MA_error();
    system_mute = get_MA_system_mute();
    audio_proc_mute = get_MA_audio_proc_mute();
    lim_mon = get_MA_audio_proc_limiter_mon();
    lim_clip = get_MA_audio_proc_clip_mon();
    ext_gpio_set_merus_chip_select(false);
    i2s_redirector_restore(is_running);

    ESP_LOGI(TAG, "sys_mute:0x%x, audio_proc_mute:0x%x, audio_proc_lim_mon:0x%x, audio_proc_clip:0x%x, mute0:0x%x, mute1:0x%x, error:0x%x, error_acc:0x%x",
        system_mute, audio_proc_mute, lim_mon, lim_clip, mute0, mute1, err, err_acc);
}