/******************************************************************************
 *
 * Copyright 2018 karawin (http://www.karawin.fr)
 *
*******************************************************************************/
#ifndef INTERFACE_H
#define INTERFACE_H
#include "esp_log.h"
#include "telnet.h"
#include "addon.h"

// need this for ported soft to esp32
#define ESP32_IDF

#define PSTR(s) (s)
#define MAXDATAT	 256


#define RELEASE "2.3"
#define REVISION "0"

uint32_t iface_check_uart(uint32_t speed);
extern unsigned short adcdiv;
void iface_switch_command(void );
void iface_check_command(int size, char* s);
esp_log_level_t iface_get_log_level();
void iface_set_log_level(esp_log_level_t level);
void iface_wifi_connect_mem();
char* iface_web_info();
char* iface_web_list(int id);
uint16_t iface_get_current_station();
void iface_set_current_station( uint16_t vol);
void iface_client_vol(char *s);
uint32_t iface_get_lcd_out();
uint32_t iface_get_lcd_stop();
bool iface_get_auto_wifi(void);
void iface_set_auto_wifi();
int8_t iface_get_rssi(void);
void iface_set_ddmm(uint8_t dm);
uint8_t iface_get_ddmm();
void iface_set_rotat(uint8_t dm);
uint8_t iface_get_rotat();
void iface_set_hostname(char* s);

#define kprintf(fmt, ...) do {    \
		telnet_write(printf(fmt, ##__VA_ARGS__),fmt, ##__VA_ARGS__); \
		addon_parse(fmt, ##__VA_ARGS__);\
	} while (0)



int lkprintf(const char *format, va_list ap);

#endif
