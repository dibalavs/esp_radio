/******************************************************************************
 *
 * Copyright 2017 karawin (http://www.karawin.fr)
 *
*******************************************************************************/
#pragma once
#ifndef __GPIO_H__
#define __GPIO_H__
#include "nvs_flash.h"
#include "driver/spi_master.h"
#include "app_main.h"
#include <hal/gpio_types.h>

#define GPIO_NONE 255

// i2c devices addrs
//------------------
#define I2C_ADDR_MCP23017  0b00100111
#define I2C_ADDR_RDA5807FP 0b00100000
#define I2C_ADDR_MERUS     0b01000000

//-------------------------------//
// Define Bus pins               //
//-------------------------------//

// KSPI pins of the SPI bus
//-------------------------

#define SPI_NO VSPI_HOST            // Must be HSPI or VSPI
#define PIN_SPI_MISO GPIO_NUM_35 	// Master Input, Slave Output
#define PIN_SPI_MOSI GPIO_NUM_32	// Master Output, Slave Input   Named Data or SDA or D1 for oled
#define PIN_SPI_CLK  GPIO_NUM_33 	// Master clock  Named SCL or SCK or D0 for oled

// I2C lcd (and rda5807 if lcd is i2c or LCD_NONE)
//------------------------------------------------
#define I2C_NO I2C_NUM_0    //  I2C port number for master dev
#define PIN_I2C_SCL GPIO_NUM_23
#define PIN_I2C_SDA GPIO_NUM_22

// I2S DAC or PDM output
//-----------------------
#define I2S_OUT_NO I2S_NUM_0
#define PIN_I2S_OUT_LRCK GPIO_NUM_12 	// or Channel1
#define PIN_I2S_OUT_BCLK GPIO_NUM_13	// or channel2
#define PIN_I2S_OUT_DATA GPIO_NUM_14	//
#define PIN_I2S_OUT_MCLK GPIO_NUM_NC

#define I2S_IN_NO I2S_NUM_1
#define PIN_I2S_IN_LRCK GPIO_NUM_5 	// or Channel1
#define PIN_I2S_IN_BCLK GPIO_NUM_19	// or channel2
#define PIN_I2S_IN_DATA GPIO_NUM_18	//
#define PIN_I2S_IN_MCLK GPIO_NUM_0	//

// SPI lcd
//---------
#define PIN_LCD_CS	GPIO_NUM_21		//CS
#define PIN_LCD_A0	GPIO_NUM_4		//A0 or D/C
#define PIN_LCD_RST	GPIO_NUM_2		//Reset RES RST or not used

// GPIO expander interrupt input
//------------------------------
#define PIN_EXT_GPIO_INT        GPIO_NUM_36

// Port A
#define PIN_EXT_GPIO_PREV       MCP23017_PIN0
#define PIN_EXT_GPIO_PLAY       MCP23017_PIN1
#define PIN_EXT_GPIO_NEXT       MCP23017_PIN2
#define PIN_EXT_GPIO_ENC_A      MCP23017_PIN3
#define PIN_EXT_GPIO_ENC_B      MCP23017_PIN4
#define PIN_EXT_GPIO_ENC_BTN    MCP23017_PIN5

// Port B
#define PIN_EXT_GPIO_LCD        MCP23017_PIN8
#define PIN_EXT_GPIO_I2S_0      MCP23017_PIN9
#define PIN_EXT_GPIO_I2S_1      MCP23017_PIN10
#define PIN_EXT_GPIO_MERUS_EN   MCP23017_PIN11
#define PIN_EXT_GPIO_MERUS_MUTE MCP23017_PIN12
#define PIN_EXT_GPIO_MERUS_CS   MCP23017_PIN13
#define PIN_EXT_GPIO_FM_CS      MCP23017_PIN14

// gpio of the vs1053
//-------------------
#define PIN_NUM_XCS  GPIO_NUM_26
#define PIN_NUM_RST  GPIO_NUM_27
#define PIN_NUM_XDCS GPIO_NUM_25
#define PIN_NUM_DREQ GPIO_NUM_34
// + KSPI pins

// gpio of FM radio interrupt
#define PIN_FM_INT GPIO_NUM_39

// IR Signal
//-----------
#define PIN_IR_SIGNAL GPIO_NUM_15	// Remote IR source

// get the hardware partition infos
esp_err_t open_partition(const char *partition_label, const char *namespace,nvs_open_mode open_mode,nvs_handle *handle);
void close_partition(nvs_handle handle,const char *partition_label);
void gpio_get_label(char** label);
void gpio_get_comment(char** label);
void gpio_get_ir_signal(gpio_num_t *ir);
bool gpio_get_ir_key(nvs_handle handle,const char *key, uint32_t *out_value1 , uint32_t *out_value2);
void option_get_lcd_info(uint8_t *enca,uint8_t* rt);
void option_set_lcd_info(uint8_t enca, uint8_t rt);
void option_get_ddmm(uint8_t *enca);
void option_set_ddmm(uint8_t enca);
void option_get_lcd_out(uint32_t *enca,uint32_t *encb);
void option_set_lcd_stop(uint32_t enca);
void option_set_lcd_out(uint32_t enca);
uint8_t gpioToChannel(uint8_t gpio);

#endif
