/*   Interface library for Merus-Audio audio amplifier

	 Provide functions for:
	  Setup i2c bus
	  i2c read and write functions to ma12040 and ma12070 analog/digital input device

	 The include file ma120x0.h provide macros for register access usign literal symbols by
	 read-modify-write register access using this library.

     Written by Jorgen Kragh Jakobsen, Merus-Audio, April, 2017
                                                                                             */

// Todo : Use esp log macros for errors / info


#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "gpio.h"

#include "MerusAudio.h"                 // Provides i2c read/write to audio amplifier
#include "ma120x0.h"                    // Register map and macros

#define I2C_CHECK(a, str, ret)  if(!(a)) {                                             \
        ESP_LOGE(I2C_TAG,"%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str);      \
        return (ret);                                                                   \
        }

#define MA12040_ADDR  0x20   /*!< slave address for MA12040 amplifier */

#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

esp_err_t ma_write(uint8_t address, uint8_t *wbuf, uint8_t n)
{
  bool ack = ACK_VAL;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, MA12040_ADDR<<1 | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, address, ACK_VAL);

  for (int i=0 ; i<n ; i++)
  { if (i==n-1) ack = NACK_VAL;
    i2c_master_write_byte(cmd, wbuf[i], ack);
  }
  i2c_master_stop(cmd);
  int ret = i2c_master_cmd_begin(I2C_NO, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_FAIL) { return ret; }
  return ESP_OK;
}

esp_err_t ma_write_byte(uint8_t address, uint8_t value)
{ esp_err_t ret=0;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (MA12040_ADDR<<1) | WRITE_BIT , ACK_CHECK_EN);
  i2c_master_write_byte(cmd, address, ACK_VAL);
  i2c_master_write_byte(cmd, value, ACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_NO, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_FAIL) {
     printf("ESP_I2C_WRITE ERROR : %d\n",ret);
	 return ret;
  }
  return ESP_OK;
}

esp_err_t ma_read(uint8_t address, uint8_t *rbuf, uint8_t n)
{ esp_err_t ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  if (cmd == NULL ) { printf("ERROR handle null\n"); }
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (MA12040_ADDR<<1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, address, ACK_VAL);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (MA12040_ADDR<<1) | READ_BIT, ACK_CHECK_EN);

  i2c_master_read(cmd, rbuf, n-1 ,ACK_VAL);
 // for (uint8_t i = 0;i<n;i++)
 // { i2c_master_read_byte(cmd, rbuf++, ACK_VAL); }
  i2c_master_read_byte(cmd, rbuf + n-1 , NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_NO, cmd, 100 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_FAIL) {
      printf("i2c Error read - readback\n");
	  return ESP_FAIL;
  }
  return ret;
}


uint8_t ma_read_byte(uint8_t address)
{
  uint8_t value = 0;
  esp_err_t ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);								// Send i2c start on bus
  i2c_master_write_byte(cmd, (MA12040_ADDR<<1) | WRITE_BIT, ACK_CHECK_EN );
  i2c_master_write_byte(cmd, address, ACK_VAL);         // Send address to start read from
  i2c_master_start(cmd);							    // Repeated start
  i2c_master_write_byte(cmd, (MA12040_ADDR<<1) | READ_BIT, ACK_CHECK_EN);
  i2c_master_read_byte(cmd, &value, NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_NO, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_FAIL) {
      printf("i2c Error read - readback\n");
	  return ESP_FAIL;
  }

  return value;
}

bool ma_check_present(void)
{
    const uint8_t MA_hw_version__a = 127;
    uint8_t res = ma_read_byte(MA_hw_version__a);
    return res != (uint8_t)ESP_FAIL;
}

/*
 * Output audio data to I2S and setup MerusAudio digital power amplifier
 */
esp_err_t init_ma120(uint8_t vol)
{
    printf("Setup MA120x0\n");

    const uint8_t MA_hw_version__a = 127;
    uint8_t res = ma_read_byte(MA_hw_version__a);
    printf("Hardware version: 0x%02x\n",res);

    ma_write_byte(MA_i2s_format__a,8);                     // Set i2s_std_format, set audio_proc_enable
    ma_write_byte(MA_vol_db_master__a,vol);                // Set vol_db_master low

    res = ma_read_byte(MA_error__a);
    printf("Errors : 0x%02x\n",res);

    res = ma_read_byte(116);
    printf("Audio in mode : 0x%02x\n",res);

    ma_write_byte(45,0x34);                                // Clean any errors on device
    if (!ma_write_byte(45,0x30)) return ESP_FAIL;

    printf("Init done\n");
	return ESP_OK;
}
