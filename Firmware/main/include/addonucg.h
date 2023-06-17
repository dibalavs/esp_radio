/******************************************************************************
 *
 * Copyright 2018 karawin (http://www.karawin.fr)
 *
*******************************************************************************/
#ifndef ADDONUCG_H_
#define ADDONUCG_H_

#include <stdint.h>

void addonucg_setfont(int size);
void addonucg_playing();
void addonucg_nameset(char* ici);
void addonucg_status(const char* label);
void addonucg_icy0(char* ici);
void addonucg_icy4(char* ici);
void addonucg_meta(char* ici);
void addonucg_scroll();
void addonucg_draw_frame(uint8_t mTscreen);
void addonucg_draw_ttitle(char* ttitle);
void addonucg_draw_number(uint8_t mTscreen,char* irStr);
void addonucg_draw_station(uint8_t mTscreen,char* snum,char* ddot);
//void drawVolumeUcg(uint8_t mTscreen,char* aVolume);
void addonucg_draw_volume(uint8_t mTscreen);
void addonucg_draw_time(uint8_t mTscreen,unsigned timein);
void addonucg_lcd_init(uint8_t* lcd_type);
void addonucg_set_volume(uint16_t vol);
void addonucg_draw_lines();

#endif /* ADDONUCG_H_ */