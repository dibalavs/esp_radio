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
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/i2c.h>
#include <i2c_bus.h>
#include "driver/i2s.h"
//#include <driver/i2s_std.h>
#include <esp_chip_info.h>
#include <esp_log.h>

#include "../gpio.h"
#include "driver/i2s_types_legacy.h"
#include "esp_err.h"
#include "hal/i2c_types.h"

static const char *TAG = "bus";

#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_TX_BUF_DISABLE   0      //  I2C master do not need buffer
#define I2C_MASTER_RX_BUF_DISABLE   0      //  I2C master do not need buffer

#define I2S_DMA_BUFFER_NUMBER 12
#define I2S_DMA_BUFFER_SIZE 256

static i2c_bus_handle_t i2c_bus;
static i2s_chan_handle_t i2s_tx_chan;

void bus_init_gpio(void)
{
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
}

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
    ESP_ERROR_CHECK(i2c_param_config(I2C_NO, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NO, I2C_MODE_MASTER, 10, 10, 0));

    //i2c_bus = i2c_bus_create(I2C_NO, &conf);
    //assert(i2c_bus);
}

// void bus_init_i2s(void)
// {
//     static const i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NO, I2S_ROLE_MASTER);
//     ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &i2s_tx_chan, NULL));

//     static const i2s_std_config_t std_cfg = {
//         .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
//         .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
//         .gpio_cfg = {
//             .mclk = I2S_GPIO_UNUSED,    // some codecs may require mclk signal, this example doesn't need it
//             .bclk = PIN_I2S_BCLK,
//             .ws   = PIN_I2S_LRCK,
//             .dout = PIN_I2S_DATA,
//             .din  = I2S_GPIO_UNUSED,
//             .invert_flags = {
//                 .mclk_inv = false,
//                 .bclk_inv = false,
//                 .ws_inv   = false,
//             },
//         },
//     };

//     ESP_LOGI(TAG, "Init I2S bus. (bclk:%d, lrclk:%d, data:%d)", std_cfg.gpio_cfg.bclk, std_cfg.gpio_cfg.ws, std_cfg.gpio_cfg.dout);
//     ESP_ERROR_CHECK(i2s_channel_init_std_mode(i2s_tx_chan, &std_cfg));
//     ESP_ERROR_CHECK(i2s_channel_disable(i2s_tx_chan));
// }

void  bus_init_i2s(/*renderer_config_t *config*/)
{
    const i2s_config_t i2s_out_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,          // Only TX
        .sample_rate = 48000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .bits_per_chan = I2S_BITS_PER_CHAN_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,   // 2-channels
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, // I2S_COMM_FORMAT_STAND_MSB,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        .dma_desc_num = 12,                            // number of buffers, 128 max.  16
        .dma_frame_num = 512,                          // size of each buffer 128
        .intr_alloc_flags = 0 ,        // default
        .tx_desc_auto_clear = true,
        .use_apll = 1,
        .fixed_mclk = 0,	// avoiding I2S bug
    };

	const i2s_pin_config_t pin_out_config = {
        .bck_io_num = PIN_I2S_OUT_BCLK,
        .ws_io_num = PIN_I2S_OUT_LRCK,
        .data_out_num = PIN_I2S_OUT_DATA,
        .data_in_num = I2S_PIN_NO_CHANGE,
        .mck_io_num = PIN_I2S_OUT_MCLK
	};

    ESP_ERROR_CHECK(i2s_driver_install(I2S_OUT_NO, &i2s_out_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_OUT_NO, &pin_out_config));

    const i2s_config_t i2s_in_config = {
        .mode = I2S_MODE_SLAVE | I2S_MODE_RX,          // Only TX
        .sample_rate = 48000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .bits_per_chan = I2S_BITS_PER_CHAN_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,   // 2-channels
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, // I2S_COMM_FORMAT_STAND_MSB,
        .mclk_multiple = I2S_MCLK_MULTIPLE_256,
        .dma_desc_num = 12,                            // number of buffers, 128 max.  16
        .dma_frame_num = 512,                          // size of each buffer 128
        .intr_alloc_flags = 0 ,        // default
        .tx_desc_auto_clear = true,
        .use_apll = 0,
        .fixed_mclk = 0,	// avoiding I2S bug
    };

	const i2s_pin_config_t pin_in_config = {
        .bck_io_num = PIN_I2S_IN_BCLK,
        .ws_io_num = PIN_I2S_IN_LRCK,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = PIN_I2S_IN_DATA,
        .mck_io_num = PIN_I2S_IN_MCLK
	};

    ESP_ERROR_CHECK(i2s_driver_install(I2S_IN_NO, &i2s_in_config, 0, NULL));
    ESP_ERROR_CHECK(i2s_set_pin(I2S_IN_NO, &pin_in_config));
}

i2c_bus_handle_t bus_i2c_get(void)
{
    return i2c_bus;
}

i2s_chan_handle_t bus_i2s_get(void)
{
    return i2s_tx_chan;
}