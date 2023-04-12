
/******************************************************************************
 *
 * Copyright 2018 karawin (http://www.karawin.fr)
 *
*******************************************************************************/
#ifndef ADDONU8G2_H_
#define ADDONU8G2_H_
void addonu8g2_setfont(int size);
void addonu8g2_playing();
void addonu8g2_nameset(char* ici);
void addonu8g2_status(const char* label);
void addonu8g2_icy0(char* ici);
void addonu8g2_icy4(char* ici);
void addonu8g2_meta(char* ici);
char* addonu8g2_get_name_num();
void addonu8g2_scroll();
void addonu8g2_draw_frame(uint8_t mTscreen);
void addonu8g2_draw_ttitle(char* ttitle);
void addonu8g2_draw_number(uint8_t mTscreen,char* irStr);
void addonu8g2_draw_station(uint8_t mTscreen,char* snum,char* ddot);
void addonu8g2_draw_volume(uint8_t mTscreen);
void addonu8g2_draw_time(uint8_t mTscreen,unsigned timein);
void addonu8g2_lcd_init(uint8_t* lcd_type);
void addonu8g2_set_volume(uint16_t vol);
void addonu8g2_draw_lines();

#endif /* ADDONU8G2_H_ */