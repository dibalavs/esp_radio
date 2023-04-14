/**
 * @file bus.c
 * @author Vasily Dybala
 * @brief
 * @version 0.1
 * @date 2023-04-12
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <bus.h>
#include <driver/spi_master.h>
#include <driver/i2c.h>
#include <i2c_bus.h>
#include <driver/i2s_std.h>
#include <esp_chip_info.h>
#include <esp_log.h>

#include "../gpio.h"
#include "esp_err.h"

static const char *TAG = "bus";

#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE   0      //  I2C master do not need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0      //  I2C master do not need buffer

#define I2S_DMA_BUFFER_NUMBER 12
#define I2S_DMA_BUFFER_SIZE 256

static i2c_bus_handle_t i2c_bus;
static i2s_chan_handle_t i2s_tx_chan;

void bus_init_spi(void)
{
    esp_err_t ret;
	static const spi_bus_config_t buscfg = {
        .miso_io_num = PIN_SPI_MISO,
        .mosi_io_num = PIN_SPI_MOSI,
        .sclk_io_num = PIN_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
		.flags = SPICOMMON_BUSFLAG_MASTER
	};

	ret = spi_bus_initialize(SPI_NO, &buscfg, SPI_DMA_CH1);
    ESP_LOGI(TAG, "Init SPI bus. (mosi:%d, miso:%d, clk:%s) = %d", buscfg.mosi_io_num, buscfg.miso_io_num, buscfg.sclk_io_num, esp_err_to_name(ret));
	ESP_ERROR_CHECK(ret);
}

void bus_init_i2c(void)
{
    static const i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = PIN_I2C_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        .clk_flags = 0
    };

    ESP_LOGI(TAG, "Init I2C bus. (sda:%d, scl:%d)", conf.sda_io_num, conf.scl_io_num);
    i2c_bus = i2c_bus_create(I2C_NO, &conf);
    assert(i2c_bus);
}

void bus_init_i2s(void)
{
    static const i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_tx_chan, NULL));

    static const i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
            .bclk = PIN_I2S_BCLK,
            .ws   = PIN_I2S_LRCK,
            .dout = PIN_I2S_DATA,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    ESP_LOGI(TAG, "Init I2S bus. (bclk:%d, lrclk:%d, data:%d)", std_cfg.gpio_cfg.bclk, std_cfg.gpio_cfg.ws, std_cfg.gpio_cfg.dout);
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_disable(i2s_tx_chan));
}

i2c_bus_handle_t bus_i2c_get(void)
{
    return i2c_bus;
}

i2s_chan_handle_t bus_i2s_get(void)
{
    return i2s_tx_chan;
}