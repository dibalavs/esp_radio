
#ifndef _MERUSAUDIO_H_
#define _MERUSAUDIO_H_

#include <esp_err.h>
#include <stdbool.h>

esp_err_t ma_write_byte(uint8_t address, uint8_t value);
esp_err_t ma_write(uint8_t address, uint8_t *wbuf, uint8_t n);


uint8_t ma_read_byte(uint8_t address);
esp_err_t ma_read(uint8_t address, uint8_t *rbuf, uint8_t n);

esp_err_t init_ma120(uint8_t vol);

bool ma_check_present(void);

#endif /* _MERUSAUDIO_H_  */


