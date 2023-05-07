/**
 * @file rda5807.h
 * @author Vasily Dybala (dibalavs@yandex.ru)
 * @brief Fm radio chip.
 * Original idea was taken from
 * Arduino library implementation: https://github.com/pu2clr/RDA5807
 * @version 0.1
 * @date 2023-04-23
 *
 * @copyright Copyright (c) 2023
 *
 */

#pragma once

#include <hal/gpio_types.h>
#include <stdint.h>
#include <stdbool.h>

void rda5807_init(uint8_t i2c_no, uint8_t i2c_addr, gpio_num_t int_gpio);

/**
 * @brief Callback is called when seek/tune operation is finished.
 * int_gpio interrupt pin shoulb be connected to ESP.
 *
 * @param cb
 */
void rda5807_set_complete_callback_isr(void (*cb)(void));

bool rda5807_check_present(void);

uint8_t rda5807_get_device_id(void);

typedef enum {
    RDA5807_CLK_CRYSTAL_32K   = 0x00,
    RDA5807_CLK_CRYSTAL_12M   = 0x01,
    RDA5807_CLK_CRYSTAL_13M   = 0x02,
    RDA5807_CLK_CRYSTAL_19_2M = 0x03,
    RDA5807_CLK_CRYSTAL_24M   = 0x05,
    RDA5807_CLK_CRYSTAL_26M   = 0x06,
    RDA5807_CLK_CRYSTAL_38_4M = 0x07,
    RDA5807_CLK_DIRECT_IN     = 0xff
} rda5807_clk_src_t;
void rda5807_set_clock_source(rda5807_clk_src_t clocks);

void rda5807_soft_reset(void);
void rda5807_power_on(bool enable);

void rda5807_softmute(bool set);
void rda5807_mute(bool set);

void rda5807_set_mono(bool mono);
void rda5807_set_rds(bool enable);
void rda5807_set_bass(bool enable);

/**
 * @brief Set frequency in Mhz.
 *  100.1 Mhz -> 100.1
 *  95.9 Mhz -> 95.9
 * @param frequency in Mhz
 */
void rda5807_set_frequency(float frequency_mhz);

/**
 * @brief Return current frequency in Mhz
 *  100.1 Mhz -> 100.1
 *  95.9 Mhz -> 95.9
 * @return uint16_t
 */
float rda5807_get_frequency(void);

void rda5807_set_frequency_up(void);
void rda5807_set_frequency_down(void);

typedef enum {
    RDA5807_DEEMPH_75US = 0, // Used in USA
    RDA5807_DEEMPH_50US = 1, // Used in Europe, Russia, Australia, Japan.
} rda5807_deemph_t;

void rda5807_set_fm_deemphasis(rda5807_deemph_t de);

#define RDA5807_SOFTBLEND_DISABLE 0xff

/**
 * @brief
 *
 * @param value 0 - 31. or 0xff - to disable
 */
void rda5807_set_soft_blend(uint8_t value);

/**
 * @brief Increase sensivity by 1dB.
 *
 * @param enable
 */
void rda5807_new_demodulate_method(bool enable);

typedef enum {
    RDA5807_SEEK_WRAP = 0,
    RDA5807_SEEK_STOP = 1
} rda5807_seek_mode_t;

typedef enum {
    RDA5807_SEEK_DOWN = 0,
    RDA5807_SEEK_UP = 1
} rda5807_seek_dir_t;

void rda5807_seek(rda5807_seek_mode_t seek_mode, rda5807_seek_dir_t direction);

/**
 * @brief
 *
 * @param value 0x00 .. 0x0F
 */
void rda5807_set_seek_threshold(uint8_t value);

typedef struct {
    float real_freq_mhz;
    bool is_stereo;
    bool seek_fail;
    bool tune_complete;
} rda5807_status1_t;
void rda5807_get_status1(rda5807_status1_t *stat);

typedef struct {
    uint8_t rssi; //!< 0b000000 = min; 0b111111 = max;
    bool fm_ready;
    bool is_fm_station;
} rda5807_status2_t;

void rda5807_get_status2(rda5807_status2_t *stat);

void rda5807_set_volume(uint8_t value);

void rda5807_i2s_on(bool enable);

typedef enum {
    RDA5807_I2S_WS_STEP_48     =  0b1000,
    RDA5807_I2S_WS_STEP_44_1   =  0b0111,
    RDA5807_I2S_WS_STEP_32     =  0b0110,
    RDA5807_I2S_WS_STEP_24     =  0b0101,
    RDA5807_I2S_WS_STEP_22_05  =  0b0100,
    RDA5807_I2S_WS_STEP_16     =  0b0011,
    RDA5807_I2S_WS_STEP_12     =  0b0010,
    RDA5807_I2S_WS_STEP_11_025 =  0b0001,
    RDA5807_I2S_WS_STEP_8      =  0b0000,
} rda5807_i2s_speed_t;

void rda5807_i2s_master_set_speed(rda5807_i2s_speed_t value);
void rda5807_i2s_set_signed(bool is_signed);

typedef struct {
    bool is_master;
    bool is_ws_left; // when WS==1 -> Left channel
    bool is_data_signed;
    bool is_sclk_inverted;
    bool is_ws_inverted;
    rda5807_i2s_speed_t master_speed;
    bool is_left_ch_delay;
    bool is_right_ch_delay;
} rda5807_i2s_config_t;

void rda5807_i2s_configure (const rda5807_i2s_config_t *cfg);

//******** RDS methods
/*
void setRDS(bool value);
void setRBDS(bool value);
void setRdsFifo(bool value);
void clearRdsFifo();

bool getRdsReady();
uint8_t getRdsFlagAB(void);
uint8_t getRdsVersionCode(void);
uint16_t getRdsGroupType();
uint8_t getRdsProgramType(void);
void getNext2Block(char *c);
void getNext4Block(char *c);
char *getRdsText(void);
char *getRdsText0A(void);
char *getRdsText2A(void);
char *getRdsText2B(void);
char *getRdsTime();
bool getRdsSync();
uint8_t getBlockId();
uint8_t getErrorBlockB();
bool hasRdsInfo();
*/

// I2S
