#include "esp_intr_alloc.h"
#include "hal/gpio_types.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <bus.h>
#include <driver/gpio.h>
#include <esp_err.h>
#include <ext_gpio.h>
#include <gpio.h>
#include <mcp23017.h>
#include <esp_log.h>

static const char *TAG = "ext_gpio";

static uint8_t port_b = 0;
static uint8_t port_a = 0;
static mcp23017_handle_t i2c_device;
static ext_gpio_callback_t *gpio_cb_isr;

#define GET_LOW_BIT(pin) (MCP23027_IS_PORT_A(pin) ? MCP23017_PORT_A_BYTE(pin) : MCP23017_PORT_B_BYTE(pin))

#define SET_BIT(var, bit) (var) = ((var) | GET_LOW_BIT(bit))
#define CLEAR_BIT(var, bit) (var) = ((var) & ~(GET_LOW_BIT(bit)))
#define IS_BIT_SET(var, bit) (!!(var & (GET_LOW_BIT(bit))))

IRAM_ATTR static void gpio_isr_handler(void* arg)
{
    (void)arg;

    if (gpio_cb_isr)
        gpio_cb_isr();
}

void ext_gpio_fetch_int_captured(void)
{
    port_a = mcp23017_read_intcap(i2c_device, MCP23017_GPIOA);
}

void ext_gpio_init(void)
{
    static const gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<PIN_EXT_GPIO_INT),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_isr_handler_add(PIN_EXT_GPIO_INT, gpio_isr_handler, NULL));
    i2c_device = mcp23017_create(bus_i2c_get(), I2C_ADDR_MCP23017);
    assert(i2c_device);

    if (!ext_gpio_check_present()) {
        ESP_LOGE(TAG, "mcp23017 GPIO expander not found.");
        return;
    }

    // Configure PORT A
    ESP_ERROR_CHECK(mcp23017_set_io_dir(i2c_device, 0xff, MCP23017_GPIOA)); // all input
    ESP_ERROR_CHECK(mcp23017_set_input_polarity(i2c_device, 0x00ff)); // invert logic, because when button pushed, it grounded
    ESP_ERROR_CHECK(mcp23017_interrupt_en(i2c_device, 0x00ff, 0, 0)); // interrups only on GPIOA
    ESP_ERROR_CHECK(mcp23017_set_pullup(i2c_device, 0x00ff)); // Pullup on GPIOA

    // Configure PORT B
    ESP_ERROR_CHECK(mcp23017_set_io_dir(i2c_device, 0x00, MCP23017_GPIOB)); // all output

    port_a = mcp23017_read_io(i2c_device, MCP23017_GPIOA);
    //mcp23017_write_io(i2c_device, 0xff, MCP23017_GPIOB);

    ext_gpio_set_merus_en(true);
    ext_gpio_set_merus_mute(false);
}

void ext_gpio_set_int_callback(ext_gpio_callback_t *cb_isr)
{
    gpio_cb_isr = cb_isr;
}

bool ext_gpio_check_present(void)
{
    return mcp23017_check_present(i2c_device) == ESP_OK;
}

void ext_gpio_set_lcd(bool enable)
{
    if (enable)
        SET_BIT(port_b, PIN_EXT_GPIO_LCD);
    else
        CLEAR_BIT(port_b, PIN_EXT_GPIO_LCD);
    ESP_ERROR_CHECK(mcp23017_write_io(i2c_device, port_b, MCP23017_GPIOB));
}

bool ext_gpio_get_lcd(void)
{
    return IS_BIT_SET(port_b, PIN_EXT_GPIO_LCD);
}

void ext_gpio_set_i2s(enum ext_gpio_i2s_state state)
{
    CLEAR_BIT(port_b, PIN_EXT_GPIO_I2S_0 | PIN_EXT_GPIO_I2S_1);
    switch (state) {
        case I2S_SWITCH_ESP:
            // s0 - 0
            // s1 - 0
            break;
        case I2S_SWITCH_VS1053:
            // s0 - 0
            // s1 - 1
            SET_BIT(port_b, PIN_EXT_GPIO_I2S_1);
            break;
        case I2S_SWITCH_FM:
            // s0 - 1
            // s1 - x
            SET_BIT(port_b, PIN_EXT_GPIO_I2S_0);
            break;
    };

    ESP_ERROR_CHECK(mcp23017_write_io(i2c_device, port_b, MCP23017_GPIOB));
}

enum ext_gpio_i2s_state ext_gpio_get_i2s(void)
{
    const uint8_t mask = port_b & (MCP23017_PORT_B_BYTE(PIN_EXT_GPIO_I2S_0 | PIN_EXT_GPIO_I2S_1));
    if (mask == 0)
        return I2S_SWITCH_ESP;

    if (mask == MCP23017_PORT_B_BYTE(PIN_EXT_GPIO_I2S_1))
        return I2S_SWITCH_VS1053;

    return I2S_SWITCH_FM;
}

void ext_gpio_set_merus_en(bool enable)
{
    if (enable)
        CLEAR_BIT(port_b, PIN_EXT_GPIO_MERUS_EN);
    else
        SET_BIT(port_b, PIN_EXT_GPIO_MERUS_EN);
    ESP_ERROR_CHECK(mcp23017_write_io(i2c_device, port_b, MCP23017_GPIOB));
}

bool ext_gpio_get_merus_en(void)
{
    return !IS_BIT_SET(port_b, PIN_EXT_GPIO_MERUS_EN);
}

void ext_gpio_set_merus_mute(bool enable)
{
    if (enable)
        CLEAR_BIT(port_b, PIN_EXT_GPIO_MERUS_MUTE);
    else
        SET_BIT(port_b, PIN_EXT_GPIO_MERUS_MUTE);
    ESP_ERROR_CHECK(mcp23017_write_io(i2c_device, port_b, MCP23017_GPIOB));
}

bool ext_gpio_get_merus_mute(void)
{
    return !IS_BIT_SET(port_b, PIN_EXT_GPIO_MERUS_MUTE);
}

void ext_gpio_set_merus_chip_select(bool enable)
{
    if (enable)
        SET_BIT(port_b, PIN_EXT_GPIO_MERUS_CS);
    else
        CLEAR_BIT(port_b, PIN_EXT_GPIO_MERUS_CS);
    ESP_ERROR_CHECK(mcp23017_write_io(i2c_device, port_b, MCP23017_GPIOB));
}

void ext_gpio_set_fm_chip_select(bool enable)
{
    if (enable)
        SET_BIT(port_b, PIN_EXT_GPIO_FM_CS);
    else
        CLEAR_BIT(port_b, PIN_EXT_GPIO_FM_CS);
    ESP_ERROR_CHECK(mcp23017_write_io(i2c_device, port_b, MCP23017_GPIOB));
}

bool ext_gpio_get_enca(void)
{
    return IS_BIT_SET(port_a, PIN_EXT_GPIO_ENC_A);
}

bool ext_gpio_get_encb(void)
{
    return IS_BIT_SET(port_a, PIN_EXT_GPIO_ENC_B);
}

bool ext_gpio_get_enc_button(void)
{
    return IS_BIT_SET(port_a, PIN_EXT_GPIO_ENC_BTN);
}

bool ext_gpio_get_button_prev(void)
{
    return IS_BIT_SET(port_a, PIN_EXT_GPIO_PREV);
}

bool ext_gpio_get_button_play(void)
{
    return IS_BIT_SET(port_a, PIN_EXT_GPIO_PLAY);
}

bool ext_gpio_get_button_next(void)
{
    return IS_BIT_SET(port_a, PIN_EXT_GPIO_NEXT);
}