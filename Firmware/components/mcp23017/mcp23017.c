// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <stdio.h>
#include "mcp23017.h"
#include "i2c_bus.h"
#include "esp_log.h"

static const char *TAG = "mcp23017";

#define I2C_NO I2C_NUM_0    //  I2C port number for master dev

#define MCP23017_CHECK(a, str, ret) if(!(a)) { \
        ESP_LOGE(TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        return (ret); \
    }

#define MCP23017_CHECK_GOTO(a, str, lable) if(!(a)) { \
        ESP_LOGE(TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        goto lable; \
    }

/**
 * @brief register address when iocon.bank == 0 (default)
 *
 */
typedef enum {
    MCP23017_REG_IODIRA = 0, /*!< DIRECTION REGISTER A */
    MCP23017_REG_IODIRB,     /*!< DIRECTION REGISTER B */
    MCP23017_REG_IPOLA, /*!< INPUT POLARITY REGISTER A */
    MCP23017_REG_IPOLB, /*!< INPUT POLARITY REGISTER B */
    MCP23017_REG_GPINTENA, /*!< NTERRUPT-ON-CHANGE CONTROL REGISTER A */
    MCP23017_REG_GPINTENB,  /*!< NTERRUPT-ON-CHANGE CONTROL REGISTER B */
    MCP23017_REG_DEFVALA,  /*!< DEFAULT COMPARE VALUE A */
    MCP23017_REG_DEFVALB,  /*!< DEFAULT COMPARE VALUE B */
    MCP23017_REG_INTCONA,  /*!< INTERRUPT-ON-CHANGE CONTROL REGISTER A */
    MCP23017_REG_INTCONB,  /*!< INTERRUPT-ON-CHANGE CONTROL REGISTER B */
    MCP23017_REG_IOCONA,  /*!< I/O EXPANDER CONFIGURATION REGISTER */
    MCP23017_REG_IOCONB,  /*!< I/O EXPANDER CONFIGURATION REGISTER */
    MCP23017_REG_GPPUA,  /*!< PULL-UP RESISTOR REGISTER A */
    MCP23017_REG_GPPUB,  /*!< PULL-UP RESISTOR REGISTER B */
    MCP23017_REG_INTFA,  /*!< INTERRUPT FLAG REGISTER A */
    MCP23017_REG_INTFB,  /*!< INTERRUPT FLAG REGISTER B */
    MCP23017_REG_INTCAPA,  /*!< INTERRUPT CAPTURED VALUE FOR PORT REGISTER A */
    MCP23017_REG_INTCAPB,  /*!< INTERRUPT CAPTURED VALUE FOR PORT REGISTER B */
    MCP23017_REG_GPIOA,  /*!<  GENERAL PURPOSE I/O PORT REGISTER A */
    MCP23017_REG_GPIOB,  /*!<  GENERAL PURPOSE I/O PORT REGISTER B */
    MCP23017_REG_OLATA,  /*!< OUTPUT LATCH REGISTER 0 A */
    MCP23017_REG_OLATB,  /*!< OUTPUT LATCH REGISTER 0 B */
} mcp23017_reg_t;

typedef enum {
    MCP23017_IOCON_UNIMPLEMENTED = 0x01,
    MCP23017_IOCON_INTPOL = 0x01 << 1,
    MCP23017_IOCON_ODR = 0x01 << 2,
    MCP23017_IOCON_HAEN = 0x01 << 3,
    MCP23017_IOCON_DISSLW = 0x01 << 4,
    MCP23017_IOCON_SEQOP = 0x01 << 5,
    MCP23017_IOCON_MIRROR = 0x01 << 6,
    MCP23017_IOCON_BANK = 0x01 << 7,
} mcp23017_reg_iocon_t;

typedef struct {
    i2c_bus_device_handle_t i2c_dev;
    uint8_t dev_addr;
    uint16_t intEnabledPins;//pin of interrupt
} mcp23017_dev_t;

static uint8_t i2caddr;

static esp_err_t my_i2c_bus_write_byte(i2c_bus_device_handle_t dev_handle, uint8_t mem_address, uint8_t data)
{
    (void)dev_handle;

    esp_err_t ret=0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (i2caddr<<1) | WRITE_BIT , ACK_CHECK_EN));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, mem_address, ACK_VAL));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, data, ACK_VAL));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ret = i2c_master_cmd_begin(I2C_NO, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t my_i2c_bus_read_byte(i2c_bus_device_handle_t dev_handle, uint8_t mem_address, uint8_t *data)
{
    (void)dev_handle;
    esp_err_t ret=0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));								// Send i2c start on bus
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (i2caddr<<1) | WRITE_BIT, ACK_CHECK_EN ));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, mem_address, ACK_VAL));         // Send address to start read from
    ESP_ERROR_CHECK(i2c_master_start(cmd));							    // Repeated start
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (i2caddr<<1) | READ_BIT, ACK_CHECK_EN));
    ESP_ERROR_CHECK(i2c_master_read_byte(cmd, data, NACK_VAL));
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    ret = i2c_master_cmd_begin(I2C_NO, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

mcp23017_handle_t mcp23017_create(i2c_bus_handle_t bus, uint8_t dev_addr)
{
    i2caddr = dev_addr;
    // if (bus == NULL) {
    //     return NULL;
    // }

    mcp23017_dev_t *p_device = (mcp23017_dev_t *) calloc(1, sizeof(mcp23017_dev_t));

    if (p_device == NULL) {
        return NULL;
    }

    // p_device->i2c_dev = i2c_bus_device_create(bus, dev_addr, 0);

    // if (p_device->i2c_dev == NULL) {
    //     free(p_device);
    //     return NULL;
    // }

    p_device->dev_addr = dev_addr;
    return (mcp23017_handle_t)p_device;
}

esp_err_t mcp23017_delete(mcp23017_handle_t *p_dev)
{
    MCP23017_CHECK(p_dev != NULL && *p_dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *)(*p_dev);
   // i2c_bus_device_delete(&p_device->i2c_dev);
    free(p_device);
    *p_dev = NULL;
    return ESP_OK;
}

esp_err_t mcp23017_write(mcp23017_handle_t dev, uint8_t reg_start_addr,
                         uint8_t reg_num, uint8_t *data_buf)
{
    MCP23017_CHECK(dev != NULL && data_buf != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    esp_err_t ret = ESP_FAIL;
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    for (size_t i = 0; i < reg_num; i++) {
        ret = my_i2c_bus_write_byte(p_device->i2c_dev, reg_start_addr + i, data_buf[i]);

        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t mcp23017_read(mcp23017_handle_t dev, uint8_t reg_start_addr,
                        uint8_t reg_num, uint8_t *data_buf)
{
    MCP23017_CHECK(dev != NULL && data_buf != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    esp_err_t ret = ESP_FAIL;
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    for (size_t i = 0; i < reg_num; i++) {
        ret = my_i2c_bus_read_byte(p_device->i2c_dev, reg_start_addr + i, &data_buf[i]);

        if (ret != ESP_OK) {
            return ret;
        }
    }

    return ESP_OK;
}

esp_err_t mcp23017_set_pullup(mcp23017_handle_t dev, uint16_t pins)
{
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;
    esp_err_t ret = ESP_FAIL;
    ret = my_i2c_bus_write_byte(p_device->i2c_dev, MCP23017_REG_GPPUA, MCP23017_PORT_A_BYTE(pins));
    if (ret != ESP_OK)
        return ret;

    ret = my_i2c_bus_write_byte(p_device->i2c_dev, MCP23017_REG_GPPUB, MCP23017_PORT_B_BYTE(pins));

    return ret;
}

esp_err_t mcp23017_set_input_polarity(mcp23017_handle_t dev, uint16_t bits)
{
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;
    esp_err_t ret = ESP_FAIL;
    ret = my_i2c_bus_write_byte(p_device->i2c_dev, MCP23017_REG_IPOLA, MCP23017_PORT_A_BYTE(bits));
    if (ret != ESP_OK)
        return ret;

    ret = my_i2c_bus_write_byte(p_device->i2c_dev, MCP23017_REG_IPOLB, MCP23017_PORT_B_BYTE(bits));

    return ret;
}

esp_err_t mcp23017_interrupt_en(mcp23017_handle_t dev, uint16_t pins,
                                uint16_t intr_mode, uint16_t defaultValue)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;
    //write register REG_GPINTENA(pins) REG_GPINTENB(pins) DEFVALA(0) DEFVALB(0) INTCONA(0) INTCONB(0)
    uint8_t data[] = { MCP23017_PORT_A_BYTE(pins), MCP23017_PORT_B_BYTE(pins) };

    uint8_t data1[] = { MCP23017_PORT_A_BYTE(defaultValue),
                        MCP23017_PORT_B_BYTE(defaultValue), MCP23017_PORT_A_BYTE(intr_mode),
                        MCP23017_PORT_B_BYTE(intr_mode) };

    if (mcp23017_write(dev, MCP23017_REG_DEFVALA, sizeof(data1),
                        data1) == ESP_FAIL) {
        return ESP_FAIL;
    }

    if (mcp23017_write(dev, MCP23017_REG_GPINTENA, sizeof(data),
                       data) == ESP_FAIL) {
        return ESP_FAIL;
    }

    p_device->intEnabledPins = p_device->intEnabledPins | pins;
    return ESP_OK;
}

esp_err_t mcp23017_interrupt_disable(mcp23017_handle_t dev, uint16_t pins)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;
    //write register REG_GPINTENA(pins) REG_GPINTENB(pins) DEFVALA(0) DEFVALB(0) INTCONA(0) INTCONB(0)
    uint8_t data[] = { MCP23017_PORT_A_BYTE(p_device->intEnabledPins & ~pins),
                       MCP23017_PORT_B_BYTE(p_device->intEnabledPins & ~pins)
                     };

    if (mcp23017_write(dev, MCP23017_REG_GPINTENA, sizeof(data),
                       data) == ESP_FAIL) {
        return ESP_FAIL;
    }

    p_device->intEnabledPins = p_device->intEnabledPins & ~pins;
    return ESP_OK;
}

esp_err_t mcp23017_set_interrupt_polarity(mcp23017_handle_t dev,
        mcp23017_gpio_port_t gpio, uint8_t chLevel)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    uint8_t getIOCON = {
        (gpio == MCP23017_GPIOA) ?
        MCP23017_REG_IOCONA : MCP23017_REG_IOCONB
    };
    uint8_t ioCONValue[] = { 0, 0 };

    if (mcp23017_read(dev, getIOCON, sizeof(ioCONValue),
                      ioCONValue) == ESP_FAIL) {
        return ESP_FAIL;
    }

    uint8_t setIOCON[] = {
        (gpio == MCP23017_GPIOA) ?
        MCP23017_REG_IOCONA : MCP23017_REG_IOCONB, 0
    };

    if (chLevel) {
        setIOCON[1] = *ioCONValue | MCP23017_IOCON_INTPOL;
    } else {
        setIOCON[1] = *ioCONValue & ~MCP23017_IOCON_INTPOL;
    }

    return my_i2c_bus_write_byte(p_device->i2c_dev, setIOCON[0], setIOCON[1]);
}

esp_err_t mcp23017_set_seque_mode(mcp23017_handle_t dev, uint8_t isSeque)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    uint8_t getIOCON = { MCP23017_REG_IOCONA };
    uint8_t ioCONValue[] = { 0, 0 };

    if (mcp23017_read(dev, getIOCON, sizeof(ioCONValue),
                      ioCONValue) == ESP_FAIL) {
        return ESP_FAIL;
    }

    uint8_t setIOCON[] = { MCP23017_REG_IOCONA, 0 };

    if (isSeque) {
        setIOCON[1] = *ioCONValue | MCP23017_IOCON_SEQOP;
    } else {
        setIOCON[1] = *ioCONValue & ~MCP23017_IOCON_SEQOP;
    }

    return my_i2c_bus_write_byte(p_device->i2c_dev, setIOCON[0], setIOCON[1]);
}

esp_err_t mcp23017_mirror_interrupt(mcp23017_handle_t dev, uint8_t mirror,
                                    mcp23017_gpio_port_t gpio)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    uint8_t getIOCON = {
        (gpio == MCP23017_GPIOA) ?
        MCP23017_REG_IOCONA : MCP23017_REG_IOCONB
    };
    uint8_t ioCONValue[] = { 0, 0 };

    if (mcp23017_read(dev, getIOCON, sizeof(ioCONValue),
                      ioCONValue) == ESP_FAIL) {
        return ESP_FAIL;
    }

    // Now munge the MIRROR bit and write IOCON back out
    uint8_t setIOCON[] = {
        (gpio == MCP23017_GPIOA) ?
        MCP23017_REG_IOCONA : MCP23017_REG_IOCONB, 0
    };

    if (mirror) {
        setIOCON[1] = *ioCONValue | MCP23017_IOCON_MIRROR;
    } else {
        setIOCON[1] = *ioCONValue & ~MCP23017_IOCON_MIRROR;
    }

    return my_i2c_bus_write_byte(p_device->i2c_dev, setIOCON[0], setIOCON[1]);
}

esp_err_t mcp23017_set_io_dir(mcp23017_handle_t dev, uint8_t value,
                              mcp23017_gpio_port_t gpio)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    return my_i2c_bus_write_byte(p_device->i2c_dev,
                               (gpio == MCP23017_GPIOA) ?
                               MCP23017_REG_IODIRA : MCP23017_REG_IODIRB, value);
}

esp_err_t mcp23017_write_io(mcp23017_handle_t dev, uint8_t value,
                            mcp23017_gpio_port_t gpio)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    return my_i2c_bus_write_byte(p_device->i2c_dev,
                               (gpio == MCP23017_GPIOA) ? MCP23017_REG_GPIOA : MCP23017_REG_GPIOB,
                               value);
}

uint8_t mcp23017_read_io(mcp23017_handle_t dev, mcp23017_gpio_port_t gpio)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", 0)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    uint8_t data = 0;
    my_i2c_bus_read_byte(p_device->i2c_dev,
                      (gpio == MCP23017_GPIOA) ? MCP23017_REG_GPIOA : MCP23017_REG_GPIOB,
                      &data);
    return data;
}

uint8_t mcp23017_read_intcap(mcp23017_handle_t dev, mcp23017_gpio_port_t gpio)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", 0)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;

    uint8_t data = 0;
    my_i2c_bus_read_byte(p_device->i2c_dev,
                      (gpio == MCP23017_GPIOA) ? MCP23017_REG_INTCAPA : MCP23017_REG_INTCAPB,
                      &data);
    return data;
}

uint16_t mcp23017_get_int_pin(mcp23017_handle_t dev)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", 0)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;
    uint16_t pinValues = 0;

    if (p_device->intEnabledPins != 0) {
        uint8_t getIntPins[] = { MCP23017_REG_INTCAPA };
        uint8_t intPins[2] = { 0 };
        mcp23017_read(p_device, getIntPins[0], sizeof(intPins), intPins);
        pinValues = MCP23017_PORT_AB_WORD(intPins);
    }

    uint8_t getGPIOPins[] = { MCP23017_REG_GPIOA };
    uint8_t gpioPins[2] = { 0 };

    if (mcp23017_read(p_device, getGPIOPins[0], sizeof(gpioPins),
                      gpioPins) == ESP_FAIL) {
        return ESP_FAIL;
    }

    uint16_t gpioValue = MCP23017_PORT_AB_WORD(gpioPins);
    pinValues |= (gpioValue & ~p_device->intEnabledPins); // Don't let current gpio values overwrite the intcap values

    return pinValues;
}

uint16_t mcp23017_get_int_flag(mcp23017_handle_t dev)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", 0)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;
    uint8_t intfpins[2] = { 0 };
    uint8_t getIntPins[] = { MCP23017_REG_INTFA };
    uint16_t pinIntfValues = 0;
    mcp23017_read(dev, getIntPins[0], sizeof(intfpins), intfpins);
    pinIntfValues = MCP23017_PORT_AB_WORD(intfpins);
    return pinIntfValues & p_device->intEnabledPins;
}

esp_err_t mcp23017_check_present(mcp23017_handle_t dev)
{
    MCP23017_CHECK(dev != NULL, "invalid arg", ESP_ERR_INVALID_ARG)
    mcp23017_dev_t *p_device = (mcp23017_dev_t *) dev;
    uint8_t lastregValue = 0x00;
    uint8_t regValue = 0x00;
    my_i2c_bus_read_byte(p_device->i2c_dev, MCP23017_REG_INTCONA, &lastregValue);
    my_i2c_bus_write_byte(p_device->i2c_dev, MCP23017_REG_INTCONA, 0xAA);
    my_i2c_bus_read_byte(p_device->i2c_dev, MCP23017_REG_INTCONA, &regValue);
    my_i2c_bus_write_byte(p_device->i2c_dev, MCP23017_REG_INTCONA, lastregValue);
    return (regValue == 0xAA) ? ESP_OK : ESP_FAIL;
}
