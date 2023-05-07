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

#include "rda5807.h"
#include "esp_attr.h"

#include <assert.h>
#include <endian.h>

#include <driver/gpio.h>
#include <driver/i2c.h>
#include <stdint.h>

static uint8_t _i2c_no;
static uint8_t _i2c_addr;
static void (*_complete_callback)(void);

static uint16_t _regs[16]; // copy of registers;

static const uint32_t _band_lower_freq_khz[4] = {87000, 76000, 76000, 65000};
static const uint32_t _spacing_freq_khz[4] = {100, 200, 50, 25};

typedef struct {
    uint16_t UNUSED : 8;
    uint16_t CHIPID : 8;
} reg_00_t;

typedef struct {
    uint16_t ENABLE : 1;         //!< Power Up Enable; 0 = Disabled; 1 = Enabled
    uint16_t SOFT_RESET : 1;     //!< Soft reset; If 0, not reset; If 1, reset.
    uint16_t NEW_METHOD : 1;     //!< New Demodulate Method Enable, can improve 0 the receive sensitivity about 1dB.
    uint16_t RDS_EN : 1;         //!< RDS/RBDS enable; If 1, rds/rbds enable
    uint16_t CLK_MODE : 3;       //!< Clock source. See rda5807_clk_src_t enum.
    uint16_t SKMODE : 1;         //!< Seek Mode; 0 = wrap at the upper or lower band limit and continue seeking; 1 = stop seeking at the upper or lower band limit
    uint16_t SEEK : 1;           //!< Seek; 0 = Disable stop seek; 1 = Enable;
    uint16_t SEEKUP : 1;         //!< Seek Up; 0 = Seek down; 1 = Seek up
    uint16_t RCLK_DIRECT_IN : 1; //!< RCLK clock use the directly input mode. 1 = enable
    uint16_t NON_CALIBRATE : 1;  //!< 0=RCLK clock is always supply; 1=RCLK clock is not always supply when FM work
    uint16_t BASS : 1;           //!< Bass Boost; 0 = Disabled; 1 = Bass boost enabled
    uint16_t MONO : 1;           //!< Mono Select; 0 = Stereo; 1 = Force mono
    uint16_t DMUTE : 1;          //!< Mute Disable; 0 = Mute; 1 = Normal operation
    uint16_t DHIZ : 1;           //!< Audio Output High-Z Disable; 0 = High impedance; 1 = Normal operation
} reg_02_t;

/**
 * Channel space table
 *
 * | Value | Description |
 * | ----- | ----------- |
 * | 00    | 100KHz      |
 * | 01    | 200KHz      |
 * | 10    | 50KHz       |
 * | 11    | 25KHz       |
 *
 *
 * FM band table
 *
 * | Value | Description                 |
 * | ----- | --------------------------- |
 * | 00    | 87–108 MHz (US/Europe)      |
 * | 01    | 76–91 MHz (Japan)           |
 * | 10    | 76–108 MHz (world wide)     |
 * | 11    | 65 –76 MHz (East Europe) or 50-65MHz (see bit 9 of gegister 0x06) |
 *
 *  Channel select table
 *
 * | BAND   | Description                                         |
 * | ------ | --------------------------------------------------  |
 * |  0     | Frequency = Channel Spacing (kHz) x CHAN + 87.0 MHz |
 * | 1 or 2 | Frequency = Channel Spacing (kHz) x CHAN + 76.0 MHz |
 * | 3      | Frequency = Channel Spacing (kHz) x CHAN + 65.0 MHz |
 * IMPORTANT: CHAN is updated after a seek operation.
 *
 */
typedef struct {
    uint16_t SPACE: 2;        //!< See Channel space table above
    uint16_t BAND: 2;         //!< Seet band table above
    uint16_t TUNE : 1;        //!< Tune; 0 = Disable; 1 = Enable
    uint16_t DIRECT_MODE : 1; //!< Directly Control Mode, Only used when test
    uint16_t CHAN : 10;       //!< Channel Select.
} reg_03_t;

typedef struct {
    uint16_t GPIO1 : 2;        //!< General Purpose I/O 1. when gpio_sel=01; 00 = High impedance; 01 = Reserved; 10 = Low; 11 = High
    uint16_t GPIO2 : 2;        //!< General Purpose I/O 2. when gpio_sel=01; 00 = High impedance; 01 = Reserved; 10 = Low; 11 = High
    uint16_t GPIO3 : 2;        //!< General Purpose I/O 1. when gpio_sel=01; 00 = High impedance; 01 = Mono/Stereo indicator (ST); 10 = Low; 11 = High
    uint16_t I2S_ENABLE : 1;   //!< I2S enable; 0 = disabled; 1 = enabled.
    uint16_t RSVD1 : 1;
    uint16_t AFCD : 1;         //!< AFC disable; If 0, afc work; If 1, afc disabled.
    uint16_t SOFTMUTE_EN  : 1; //!< If 1, softmute enable.
    uint16_t RDS_FIFO_CLR : 1; //!< 1 = clear RDS fifo
    uint16_t DE : 1;           //!< De-emphasis; 0 = 75 μs; 1 = 50 μs
    uint16_t RDS_FIFO_EN : 1;  //!< 1 = RDS fifo mode enable.
    uint16_t RBDS : 1;         //!< 1 = RBDS mode enable; 0 = RDS mode only
    uint16_t STCIEN : 1;       //!< Seek/Tune Complete Interrupt Enable; 0 = Disable Interrupt; 1 = Enable Interrupt;
    uint16_t RSVD2 : 1;
} reg_04_t;

typedef struct {
    uint16_t VOLUME : 4;         //!< DAC Gain Control Bits (Volume); 0000 = min volume; 1111 = max volume.
    uint16_t LNA_ICSEL_BIT : 2;  //!< Lna working current bit: 00=1.8mA; 01=2.1mA; 10=2.5mA; 11=3.0mA.
    uint16_t LNA_PORT_SEL : 2;   //!< LNA input port selection bit: 00: no input; 01: LNAN; 10: LNAP; 11: dual port input
    uint16_t SEEKTH : 4;         //!< Seek SNR Threshold value
    uint16_t RSVD2  : 1;
    uint16_t SEEK_MODE : 2;      //!< Default value is 00; When = 10, will add the RSSI seek mode
    uint16_t INT_MODE : 1;       //!< If 0, generate 5ms interrupt; If 1, interrupt last until read reg0CH action occurs.
} reg_05_t;

typedef struct {
    uint16_t R_DELY : 1;       //!< If 1, R channel data delay 1T.
    uint16_t L_DELY : 1;       //!< If 1, L channel data delay 1T.
    uint16_t SCLK_O_EDGE : 1;  //!< If 1, invert sclk output when as master.
    uint16_t SW_O_EDGE : 1;    //!< If 1, invert ws output when as master.
    uint16_t I2S_SW_CNT : 4;   //!< Only valid in master mode. See rda5807_i2s_speed_t enum.
    uint16_t WS_I_EDGE : 1;    //!< If 0, use normal ws internally; If 1, inverte ws internally.
    uint16_t DATA_SIGNED : 1;  //!< If 0, I2S output unsigned 16-bit audio data. If 1, I2S output signed 16-bit audio data.
    uint16_t SCLK_I_EDGE : 1;  //!< If 0, use normal sclk internally;If 1, inverte sclk internally.
    uint16_t WS_LR : 1;        //!< Ws relation to l/r channel; If 0, ws=0 ->r, ws=1 ->l; If 1, ws=0 ->l, ws=1 ->r.
    uint16_t SLAVE_MASTER : 1; //!< I2S slave or master; 1 = slave; 0 = master.
    uint16_t OPEN_MODE : 2;    //!< Open reserved register mode;  11=open behind registers writing function others: only open behind registers reading function.
    uint16_t RSVD : 1;
} reg_06_t;

typedef struct {
    uint16_t FREQ_MODE : 1;    //!< If 1, then freq setting changed. Freq = 76000(or 87000) kHz + freq_direct (08H) kHz.
    uint16_t SOFTBLEND_EN : 1; //!< If 1, Softblend enable
    uint16_t SEEK_TH_OLD : 6;  //!< Seek threshold for old seek mode, Valid when Seek_Mode=001
    uint16_t RSVD1 : 1;
    uint16_t MODE_50_60 : 1;   //!< 1 = 65~76 MHz;  0 = 50~76MHz
    uint16_t TH_SOFRBLEND : 5; //!< Threshold for noise soft blend setting, unit 2dB (default 0b10000).
    uint16_t RSVD2 : 1;
} reg_07_t;

// Next registers are readonly registers.

typedef struct {
    uint16_t READCHAN : 10;  //!< See Channel table . See table above
    uint16_t ST : 1;         //!< Stereo Indicator; 0 = Mono; 1 = Stereo
    uint16_t BLK_E : 1;      //!< When RDS enable: 1 = Block E has been found; 0 = no Block E has been found
    uint16_t RDSS : 1;       //!< RDS Synchronization; 0 = RDS decoder not synchronized(default); 1 = RDS decoder synchronized; Available only in RDS Verbose mode
    uint16_t SF : 1;         //!< Seek Fail. 0 = Seek successful; 1 = Seek failure;
    uint16_t STC : 1;        //!< Seek/Tune Complete. 0 = Not complete; 1 = Complete;
    uint16_t RDSR : 1;       //!< RDS ready; 0 = No RDS/RBDS group ready(default); 1 = New RDS/RBDS group ready.
} reg_0a_t;

typedef struct {
    uint16_t BLERB : 2;      //!< Block Errors Level of RDS_DATA_1
    uint16_t BLERA : 2;      //!< Block Errors Level of RDS_DATA_0
    uint16_t ABCD_E : 1;     //!< 1 = the block id of register 0cH,0dH,0eH,0fH is E;  0 = the block id of register 0cH, 0dH, 0eH,0fH is A, B, C, D
    uint16_t RSVD1  : 2;
    uint16_t FM_READY : 1;   //!< 1=ready; 0=not ready.
    uint16_t FM_TRUE : 1;    //!< 1 = the current channel is a station; 0 = the current channel is not a station.
    uint16_t RSSI : 7;       //!< RSSI; 000000 = min; 111111 = max; RSSI scale is logarithmic.
} reg_0b_t;

#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

#define DEFAULT_TIMEOUT (500 / portTICK_PERIOD_MS)
#define REG(r) ((reg_ ##r ##_t *)&_regs[0x ##r])
#define REG_VAL(r, v) ((reg_ ##r ##_t *)&v)

static void write_reg(uint8_t reg_no)
{
    uint16_t value = _regs[reg_no];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    assert(cmd);
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, _i2c_addr | WRITE_BIT , ACK_CHECK_EN));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, reg_no, ACK_VAL));

    htobe16(value); // Chip has big endian format
    ESP_ERROR_CHECK(i2c_master_write(cmd, (const uint8_t *)&value, sizeof(value), ACK_CHECK_EN));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ESP_ERROR_CHECK(i2c_master_cmd_begin(_i2c_no, cmd, DEFAULT_TIMEOUT));
    i2c_cmd_link_delete(cmd);
}

static uint16_t read_reg(uint8_t reg_no)
{
    uint16_t value = 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    assert(cmd);
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, _i2c_addr | WRITE_BIT, ACK_CHECK_EN ));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, reg_no, ACK_VAL));
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, _i2c_addr | READ_BIT, ACK_CHECK_EN));

    ESP_ERROR_CHECK(i2c_master_read(cmd, (uint8_t *)&value, sizeof(value), NACK_VAL));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ESP_ERROR_CHECK(i2c_master_cmd_begin(_i2c_no, cmd, DEFAULT_TIMEOUT));
    i2c_cmd_link_delete(cmd);

    be16toh(value); // Chip has big endian format
    _regs[reg_no] = value;
    return value;
}

static void read_all_regs(void)
{
    for (int i = 0; i < 16; i++) {
        if (i== 0x01 || i == 0x08 || i == 0x09)
            continue;

        _regs[i] = read_reg(i);
    }
}

static IRAM_ATTR void rda5807_isr_handler(void *arg)
{
    (void)arg;

    if (_complete_callback)
        _complete_callback();
}

void rda5807_init(uint8_t i2c_no, uint8_t i2c_addr, gpio_num_t int_gpio)
{
    _i2c_no = i2c_no;
    _i2c_addr = i2c_addr;

    const gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<int_gpio),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_isr_handler_add(int_gpio, rda5807_isr_handler, NULL));
}

void rda5807_set_complete_callback_isr(void (*cb)(void))
{
    _complete_callback = cb;
}

bool rda5807_check_present(void)
{
    return rda5807_get_device_id() == 0x58;
}

uint8_t rda5807_get_device_id(void)
{
    _regs[00] = read_reg(0x00);
    return REG(00)->CHIPID;
}

void rda5807_soft_reset(void)
{
    REG(02)->SOFT_RESET = 1;
    write_reg(0x02);
    vTaskDelay(DEFAULT_TIMEOUT);
    read_all_regs();
}

void rda5807_power_on(bool enable)
{
    REG(02)->ENABLE = enable ? 1 : 0;
    write_reg(0x02);

    if (enable)
        read_all_regs();
}

void rda5807_softmute(bool set)
{
    REG(04)->SOFTMUTE_EN = set ? 1 : 0;
    write_reg(0x04);
}

void rda5807_mute(bool set)
{
    REG(02)->DMUTE = set ? 0 : 1;
    write_reg(0x02);
}

void rda5807_set_mono(bool mono)
{
    REG(02)->MONO = mono ? 1 : 0;
    write_reg(0x02);
}

void rda5807_set_rds(bool enable)
{
    REG(02)->RDS_EN = enable ? 1 : 0;
    write_reg(0x02);
}

void rda5807_set_bass(bool enable)
{
    REG(02)->BASS = enable ? 1 : 0;
    write_reg(0x02);
}

void rda5807_set_clock_source(rda5807_clk_src_t clocks)
{
    if (clocks == RDA5807_CLK_DIRECT_IN) {
        REG(02)->RCLK_DIRECT_IN = 1;
    } else {
        REG(02)->RCLK_DIRECT_IN = 0;
        REG(02)->CLK_MODE = clocks;
    }

    write_reg(0x02);
}

static uint8_t get_band(float freq)
{
    if (freq >= 76.0)
        return 0b10; // worldwide

    return 0b11; // East Europe
}

void rda5807_set_frequency(float frequency_mhz)
{
    if (frequency_mhz > 108.0)
        frequency_mhz = 108.0;

    if (frequency_mhz < 65.0)
        frequency_mhz = 65.0;

    const uint32_t freq_khz = frequency_mhz * 1000;
    const uint8_t band = get_band(frequency_mhz);
    const uint8_t chan = (freq_khz - _band_lower_freq_khz[band]) / _spacing_freq_khz[REG(03)->SPACE];

    REG(03)->BAND = band;
    REG(03)->CHAN = chan;
    REG(03)->TUNE = 1;
    REG(03)->DIRECT_MODE = 0;
    write_reg(0x03);
}

float rda5807_get_frequency(void)
{
    float res_khz = REG(03)->CHAN * _spacing_freq_khz[REG(03)->SPACE] + _band_lower_freq_khz[REG(03)->BAND];
    return res_khz / 1000.0; // convert Khz -> Mhz
}

void rda5807_set_frequency_up(void)
{
    rda5807_set_frequency(rda5807_get_frequency() + _spacing_freq_khz[REG(03)->SPACE] / 1000.0);
}

void rda5807_set_frequency_down(void)
{
    rda5807_set_frequency(rda5807_get_frequency() - _spacing_freq_khz[REG(03)->SPACE] / 1000.0);
}

void rda5807_set_fm_deemphasis(rda5807_deemph_t de)
{
    REG(04)->DE = de;
    write_reg(0x04);
}

void rda5807_set_soft_blend(uint8_t value)
{
    if (value == RDA5807_SOFTBLEND_DISABLE) {
        REG(07)->SOFTBLEND_EN = 0;
    } else {
        if (value > 31)
            value = 31;

        REG(07)->SOFTBLEND_EN = 1;
        REG(07)->TH_SOFRBLEND = value;
    }

    write_reg(0x07);
}

void rda5807_new_demodulate_method(bool enable)
{
    REG(02)->NEW_METHOD = enable ? 1 : 0;
    write_reg(0x02);
}

void rda5807_seek(rda5807_seek_mode_t seek_mode, rda5807_seek_dir_t direction)
{
    REG(02)->SEEK = 1;
    REG(02)->SKMODE = seek_mode;
    REG(02)->SEEKUP = direction;
    write_reg(0x02);
}

void rda5807_set_seek_threshold(uint8_t value)
{
    if (value > 0x0f)
        value = 0x0f;

    REG(05)->SEEKTH = value;
    write_reg(0x05);
}

void rda5807_get_status1(rda5807_status1_t *stat)
{
    read_reg(0x0a);
    stat->is_stereo = REG(0a)->ST;
    stat->seek_fail = REG(0a)->SF;
    stat->tune_complete = REG(0a)->STC;

    float res_khz = REG(0a)->READCHAN * _spacing_freq_khz[REG(03)->SPACE] + _band_lower_freq_khz[REG(03)->BAND];
    stat->real_freq_mhz = res_khz / 1000.0;
}

void rda5807_get_status2(rda5807_status2_t *stat)
{
    read_reg(0x0b);
    stat->fm_ready = REG(0b)->FM_READY;
    stat->is_fm_station = REG(0b)->FM_TRUE;
    stat->rssi = REG(0b)->RSSI;
}

void rda5807_set_volume(uint8_t value)
{
    if (value > 0x0f)
        value = 0x0f;

    REG(05)->VOLUME = value;
    write_reg(0x05);
}

void rda5807_i2s_on(bool enable)
{
    REG(04)->I2S_ENABLE = enable ? 1 : 0;
    write_reg(0x04);
}

void rda5807_i2s_master_set_speed(rda5807_i2s_speed_t value)
{
    REG(06)->I2S_SW_CNT = value;
    write_reg(0x06);
}

void rda5807_i2s_set_signed(bool is_signed)
{
    REG(06)->DATA_SIGNED = is_signed ? 1 : 0;
    write_reg(0x06);
}

void rda5807_i2s_configure (const rda5807_i2s_config_t *cfg)
{
    REG(06)->SLAVE_MASTER = cfg->is_master ? 0 : 1;
    REG(06)->WS_LR = cfg->is_ws_left;
    REG(06)->DATA_SIGNED = cfg->is_data_signed;
    REG(06)->SCLK_O_EDGE = cfg->is_sclk_inverted;
    REG(06)->SCLK_I_EDGE = cfg->is_sclk_inverted;
    REG(06)->SW_O_EDGE = cfg->is_ws_inverted;
    REG(06)->WS_I_EDGE = cfg->is_ws_inverted;
    REG(06)->I2S_SW_CNT = cfg->master_speed;
    REG(06)->L_DELY = cfg->is_left_ch_delay;
    REG(06)->R_DELY = cfg->is_right_ch_delay;
    write_reg(0x06);
}
