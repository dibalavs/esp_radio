/******************************************************************************
 *
 * Copyright 2018 karawin (http://www.karawin.fr)
 *
*******************************************************************************/

#include "app_state.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "u8g2_esp32_hal.h"
#include "addon.h"
#include "app_main.h"
#include "gpio.h"
#include <time.h>
#include "esp_log.h"
#include "logo.h"
#include "interface.h"
#include "eeprom.h"
#include "addoncommon.h"
#include "u8g2.h"

/*==========================================*/
//#include "u8g2-karadio32_fonts.h"
extern const uint8_t u8g2_font_4x6_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_4x6_t_latcyr");
extern const uint8_t u8g2_font_5x7_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_5x7_t_latcyr");
extern const uint8_t u8g2_font_5x8_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_5x8_t_latcyr");
extern const uint8_t u8g2_font_6x12_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_6x12_t_latcyr");
extern const uint8_t u8g2_font_6x13_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_6x13_t_latcyr");
extern const uint8_t u8g2_font_7x13_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_7x13_t_latcyr");
extern const uint8_t u8g2_font_8x13_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_8x13_t_latcyr");
extern const uint8_t u8g2_font_9x15_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_9x15_t_latcyr");
extern const uint8_t u8g2_font_10x20_t_latcyr[] U8G2_FONT_SECTION("u8g2_font_10x20_t_latcyr");
#define TAG  "addonu8g2"

extern u8g2_t u8g2; // a structure which will contain all the data for one display

// nams <--> num of line
#define STATIONNAME 0
#define STATION1  1
#define STATION2  2
#define IP        3
#define GENRE     2
#define TITLE1    3
#define TITLE2    4
#define VOLUME    5
#define TIME      6

#undef LINES
#define LINES	5


////////////////////////////////////////
typedef enum sizefont  {small, text,middle,large} sizefont;
void addonu8g2_setfont(sizefont size)
{
//	printf("setfont8 size: %d, yy: %d\n",size,yy);
	switch(size)
	{
		case small:
		switch(yy)
		{
			case 200:
			u8g2_SetFont(&u8g2,u8g2_font_6x12_t_latcyr);
			break;
			case 128:
			u8g2_SetFont(&u8g2,u8g2_font_6x12_t_latcyr);
			break;
			case 32:
			u8g2_SetFont(&u8g2,u8g2_font_4x6_t_latcyr);
			break;
			case 64:
			default: //
			u8g2_SetFont(&u8g2, u8g2_font_5x8_t_latcyr);
			;
		}
		break;
		case text:
		switch(yy)
		{
			/*
			case 200:
			charset?u8g2_SetFont(&u8g2,u8g2_font_7x13_t_cyrillic):u8g2_SetFont(&u8g2,u8g2_font_7x14_tf);//
			break;
			case 128:
			charset?u8g2_SetFont(&u8g2,u8g2_font_7x13_t_cyrillic):u8g2_SetFont(&u8g2, u8g2_font_7x14_tf);//
			break;
			case 32:
			charset?u8g2_SetFont(&u8g2,u8g2_font_5x7_t_cyrillic):u8g2_SetFont(&u8g2,u8g2_font_5x7_tf);//
			break;
			case 64:
			default: //
			charset?u8g2_SetFont(&u8g2, u8g2_font_haxrcorp4089_t_cyrillic ):u8g2_SetFont(&u8g2, u8g2_font_6x12_tf);
			*/
			case 200:
			u8g2_SetFont(&u8g2, u8g2_font_7x13_t_latcyr);//
			break;
			case 128:
			u8g2_SetFont(&u8g2, u8g2_font_7x13_t_latcyr);//
			break;
			case 32:
			u8g2_SetFont(&u8g2, u8g2_font_5x7_t_latcyr);//
			break;
			case 64:
			default: //
			u8g2_SetFont(&u8g2, u8g2_font_6x12_t_latcyr);
		}
		break;
		case middle:
		switch(yy)
		{
			case 200:
			u8g2_SetFont(&u8g2, u8g2_font_9x15_t_latcyr);
			break;
			case 128:
			u8g2_SetFont(&u8g2, u8g2_font_9x15_t_latcyr);
			break;
			case 32:
			u8g2_SetFont(&u8g2, u8g2_font_5x8_t_latcyr);
			break;
			case 64:
			default: //
			u8g2_SetFont(&u8g2, u8g2_font_7x13_t_latcyr);
			;

		}
		break;
		case large:
		switch(yy)
		{
			case 200:
			u8g2_SetFont(&u8g2, u8g2_font_10x20_t_latcyr);
			break;
			case 128:
			if (x == 296)
				u8g2_SetFont(&u8g2, u8g2_font_inr46_mn );
			else
				u8g2_SetFont(&u8g2, u8g2_font_10x20_t_latcyr);
			break;
			case 32:
			u8g2_SetFont(&u8g2,  u8g2_font_8x13_t_latcyr);
			break;
			case 64:
			default: //
			u8g2_SetFont(&u8g2, u8g2_font_10x20_t_latcyr);
			;
		}
		break;
		default:
		ESP_LOGE(TAG,"Default for size %d",size);
	}
}




////////////////////////////////////////
char* addonu8g2_get_name_num()
{
	return nameNum;
}

static uint8_t getFontLineSpacing()
{
	return (u8g2_GetAscent(&u8g2) - u8g2_GetDescent(&u8g2));
}
////////////////////////////////////////
// Clear all buffers and indexes
void clearAllU8g2()
{
      title[0] = 0;
      station[0]=0;
    for (int i=1;i<LINES;i++) {lline[i] = NULL;iline[i] = 0;tline[i] = 0;;mline[i]=1;}
}
////////////////////////////////////////
void cleartitleU8g2(uint8_t froml)
{
     title[0] = 0;
     for (int i = froml;i<LINES;i++)  // clear lines
     {
		lline[i] = NULL;
		iline[i] = 0;
		tline[i] = 0;
		mline[i] = 1;
     }
}

// Mark the lines to draw
void markDrawResetU8g2(int i)
{
  mline[i] = 1;
  iline[i] = 0;
  tline[i] = 0;
}

////////////////////////////////////////
// scroll each line
void addonu8g2_scroll()
{
unsigned len ;
	addonu8g2_setfont(text);
	for (int i = 0;i < LINES;i++)
	{
	   if (tline[i]>0)
	   {
	     if (tline[i] == 4) {iline[i]= 0;
		 mline[i] = 1;;}
	     tline[i]--;
	   }
	   else
	   {
		   if ((lline[i] != NULL))
		   {
				len = u8g2_GetUTF8Width(&u8g2,lline[i]+iline[i]);
				if (i == 0)	 len += u8g2_GetUTF8Width(&u8g2,nameNum);
				if (len >= x)
				{
					iline[i]++;
					//Max
					while (((*(lline[i]+iline[i])) & 0xc0) == 0x80) {
						iline[i]++;
					}
					mline[i] = 1;;
				}
				else
					tline[i] = 6;
		   }
	   }
	}
}

// Bottom of screens
static void screenBottomU8g2()
{
//VOLUME
    u8g2_DrawFrame(&u8g2,0,yy-3,x-1,3);
    u8g2_DrawHLine(&u8g2,0,yy-2,((uint16_t)(x*app_state_get_ivol()/255)));
//TIME
	if (yy != 32) // not enough room
	{
		char strsec[36];
		if (iface_get_ddmm())
			sprintf(strsec,"%02d-%02d-%04d  %02d:%02d:%02d",dt->tm_mday,dt->tm_mon+1,dt->tm_year+1900, dt->tm_hour, dt->tm_min,dt->tm_sec);
		else
			sprintf(strsec,"%02d-%02d-%04d  %02d:%02d:%02d",dt->tm_mon+1,dt->tm_mday,dt->tm_year+1900, dt->tm_hour, dt->tm_min,dt->tm_sec);
		addonu8g2_setfont(small);
		u8g2_DrawUTF8(&u8g2,x/2-(u8g2_GetUTF8Width(&u8g2,strsec)/2),yy-y-3,strsec);
	}
}

// draw the screen from buffer
void addonu8g2_draw_lines()
{
//u8g2_SendBuffer(&u8g2);
}


//Thanks to Max
void eraseSlashes(char * str) {
	//Symbols: \" \' \\ \? \/
	char * sym = str, * sym1;
	if (str != NULL) {
		while (*sym != 0) {
			if (*sym == 0x5c) {
				sym1 = sym + 1;
				if (*sym1 == 0x22 || *sym1 == 0x27 || *sym1 == 0x5c || *sym1 == 0x3f || *sym1 == 0x2f) {
					*sym = 0x1f; //Erase \ to non-printable symbol
					sym++;
				}
			}
			sym++;
		}
	}
}
//-Max


////////////////////////////////////////
// draw all lines
void addonu8g2_draw_frame(uint8_t mTscreen)
{
	if (dt == NULL) {dt = addon_get_dt();}
	u8g2_ClearBuffer(&u8g2);
	u8g2_FirstPage(&u8g2);
	do {
		addonu8g2_setfont(text);
		u8g2_SetDrawColor(&u8g2, 1);
		y = getFontLineSpacing();
		u8g2_SetFontRefHeightText(&u8g2);
		{
			u8g2_DrawHLine(&u8g2,0,((yy==32)?3:4*y) - (y/2)-1,x);
			u8g2_DrawBox(&u8g2,0,0,x-1,y+1);
		}
		for (int i = 0;i < LINES;i++)
		{
			if (i == 0){
				u8g2_SetDrawColor(&u8g2, 0);
			}
			else {
				if (i >=3) z = y/2+2 ; else z = 1;
				u8g2_SetDrawColor(&u8g2, 1);
			}
			int zz = y*i;
			if (yy==32)
			{
				if ((i==STATION2)||(i==TITLE2)) continue;
				if (i==TITLE1) zz = y*GENRE;
			}
			if (lline[i] != NULL)
			{
				//Max
				eraseSlashes(lline[i]);
				if (i == 0)
				{
//					printf("DRAW: len= %d,STR= %s\n",u8g2_GetUTF8Width(&u8g2,lline[i]+iline[i]),lline[i]+iline[i]);
					if (nameNum[0] ==0)  u8g2_DrawUTF8(&u8g2,1,0,lline[i]+iline[i]);
					else
					{
						u8g2_DrawUTF8(&u8g2,1,0,nameNum);
						u8g2_DrawUTF8(&u8g2,u8g2_GetUTF8Width(&u8g2,nameNum)-1,0,lline[i]+iline[i]);
					}
				}
				else u8g2_DrawUTF8(&u8g2,0,zz+z,lline[i]+iline[i]);
			}
			vTaskDelay(1);
		}
		screenBottomU8g2();
	} while ( u8g2_NextPage(&u8g2) );
}


//////////////////////////
void addonu8g2_draw_ttitle(char* ttitle)
{
  char strIp[23];
	addonu8g2_setfont(middle);
    uint16_t xxx = (x/2)-(u8g2_GetUTF8Width(&u8g2,ttitle)/2);
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_DrawBox(&u8g2,0,0,x,getFontLineSpacing()+1);
    u8g2_SetDrawColor(&u8g2, 0);
    u8g2_DrawUTF8(&u8g2,xxx,1,ttitle);
    u8g2_SetDrawColor(&u8g2, 1);

    // draw ip
	addonu8g2_setfont(small);
	sprintf(strIp,"IP: %s", app_get_ip());
//	u8g2_DrawUTF8(&u8g2,(x/2)-(u8g2_GetUTF8Width(&u8g2,strIp)/2),yy-getFontLineSpacing(),strIp);
	u8g2_DrawUTF8(&u8g2,(x/2)-(u8g2_GetUTF8Width(&u8g2,strIp)/2),yy-(2*getFontLineSpacing()),strIp);
	// Rssi
	sprintf(strIp,"%d dBm", iface_get_rssi());
	u8g2_DrawUTF8(&u8g2,4,yy-(getFontLineSpacing()),strIp);
}
//////////////////////////
void addonu8g2_draw_number(uint8_t mTscreen,char* irStr)
{
  char ststr[] = {"Number"};
  u8g2_ClearBuffer(&u8g2);
  u8g2_FirstPage(&u8g2);
  do {
	addonu8g2_draw_ttitle(ststr);
	addonu8g2_setfont(large);
	uint16_t xxx = (x/2)-(u8g2_GetUTF8Width(&u8g2,irStr)/2);
	u8g2_DrawUTF8(&u8g2,xxx,yy/3, irStr);
	vTaskDelay(1);
  } while ( u8g2_NextPage(&u8g2) );
}
//////////////////////////
void addonu8g2_draw_station(uint8_t mTscreen,char* snum,char* ddot)
{
  int16_t len;
  char ststr[] = {"Station"};
  u8g2_ClearBuffer(&u8g2);
  u8g2_FirstPage(&u8g2);
  do {
	addonu8g2_draw_ttitle(ststr);
	if (ddot != NULL)
	{
		addonu8g2_setfont(middle);
		u8g2_DrawUTF8(&u8g2,(x/2)-(u8g2_GetUTF8Width(&u8g2,snum)/2),yy/3-2, snum);
		len = (x/2)-(u8g2_GetUTF8Width(&u8g2,ddot)/2);
		if (len <0) len = 0;
        u8g2_DrawUTF8(&u8g2,len,yy/3+4+y, ddot);
		vTaskDelay(1);
	}
  } while ( u8g2_NextPage(&u8g2) );
}


//void drawVolumeU8g2(uint8_t mTscreen,char* aVolume)
void addonu8g2_draw_volume(uint8_t mTscreen)
{
  char vlstr[] = {"Volume"};
  char aVolume[4];
//  volume = atoi(aVolume);
  sprintf(aVolume,"%d",app_state_get_ivol());

  u8g2_ClearBuffer(&u8g2);
  u8g2_FirstPage(&u8g2);
  do {
	addonu8g2_draw_ttitle(vlstr) ;
	addonu8g2_setfont(large);
	uint16_t xxx = (x/2)-(u8g2_GetUTF8Width(&u8g2,aVolume)/2);
	u8g2_DrawUTF8(&u8g2,xxx,(yy/3)+6,aVolume);
	vTaskDelay(1);
  } while ( u8g2_NextPage(&u8g2) );
}

void addonu8g2_draw_time(uint8_t mTscreen,unsigned timein)
{
  char strdate[23];
  char strtime[20];
//  printf("DRAW TIME U8G2  mtscreen : %d\n",mTscreen);
	u8g2_ClearBuffer(&u8g2);
  u8g2_FirstPage(&u8g2);
  do {
	if (iface_get_ddmm())
		sprintf(strdate,"%02d-%02d-%04d", dt->tm_mday, dt->tm_mon+1,  dt->tm_year+1900);
    else
		sprintf(strdate,"%02d-%02d-%04d", dt->tm_mon+1, dt->tm_mday, dt->tm_year+1900);
    sprintf(strtime,"%02d:%02d:%02d", dt->tm_hour, dt->tm_min,dt->tm_sec);
    addonu8g2_draw_ttitle(strdate);
    addonu8g2_setfont(large);
    u8g2_DrawUTF8(&u8g2,(x/2)-(u8g2_GetUTF8Width(&u8g2,strtime)/2),(yy/3)+4,strtime);
	vTaskDelay(1);
  } while ( u8g2_NextPage(&u8g2) );
}


////////////////////////////////////////
void separatorU8g2(char* from)
{
    char* interp;
    while (from[strlen(from)-1] == ' ') from[strlen(from)-1] = 0; // avoid blank at end
    while ((from[0] == ' ') ){ strcpy( from,from+1); }
    interp=strstr(from," - ");
  if (from == nameset) {lline[0] = nameset;lline[1] = NULL;lline[2] = NULL;return;}
  if ((interp != NULL) &&  (yy != 32))
  {
    from[interp-from]= 0;
    lline[(from==station)?1:3] = from;
    lline[(from==station)?2:4] = interp+3;
  } else
  {
    lline[(from==station)?1:3] = from;
  }
}

//cli.meta
void addonu8g2_meta(char* ici)
{
     cleartitleU8g2(3);
     strcpy(title,ici+7);
     separatorU8g2(title);
}

//cli.icy4
void addonu8g2_icy4(char* ici)
{
	char newstation[BUFLEN];
	 //move the STATION2 to STATION1S
	 if (lline[STATION2] != NULL)
	 {  strcpy(newstation,lline[STATION1]);strcat(newstation," - ");  strcat(newstation,lline[STATION2]);
		strcpy(lline[STATION1],newstation);
		markDrawResetU8g2(STATION1);
	 }

     strcpy(genre,ici+7);
     lline[GENRE] = genre;
}
//cli.icy0
void addonu8g2_icy0(char* ici)
{
      clearAllU8g2();
      if (strlen(ici+7) == 0) strcpy (station,nameset);
      else strcpy(station,ici+7);
      separatorU8g2(station);
}

//cli.stopped or label
void addonu8g2_status( const char* label)
{
     cleartitleU8g2(3);
     strcpy(title,label);
     lline[TITLE1] = title;
}
//cli.nameset
void addonu8g2_nameset(char* ici)
{
	strcpy(nameset,ici+8);
    ici = strstr(nameset," ");
    if (ici != NULL)
    {
       clearAllU8g2();
       strncpy(nameNum,nameset,ici-nameset+1);
       nameNum[ici - nameset+1] = 0;
	   addon_set_futur_num(atoi(nameNum));
    }
	char nameseti[BUFLEN];
	strcpy(nameseti,nameset+strlen(nameNum));
    strcpy(nameset,nameseti);
    lline[STATIONNAME] = nameset;
}

// cli.playing
void addonu8g2_playing()
{
	if (strcmp(title,"STOPPED") == 0)
    {
        cleartitleU8g2(3);
        separatorU8g2(title);
    }
}



void addonu8g2_lcd_init(uint8_t *lcd_type)
{
	const u8g2_cb_t *rotat;
	if (*lcd_type == LCD_NONE) return;

	u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
	if (*lcd_type & LCD_SPI) // BW SPI
	{
		u8g2_esp32_hal.spi_no = SPI_NO;
		u8g2_esp32_hal.cs    = PIN_LCD_CS;
		u8g2_esp32_hal.dc    = PIN_LCD_A0;
		u8g2_esp32_hal.reset = PIN_LCD_RST;
	}

	u8g2_esp32_hal_init(u8g2_esp32_hal);

	if (iface_get_rotat())
		rotat = U8G2_R2;
	else
		rotat = U8G2_R0;

	switch (*lcd_type){
	case LCD_I2C_SH1106:
		u8g2_Setup_sh1106_i2c_128x64_noname_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_I2C_SSD1306NN:
		u8g2_Setup_ssd1306_i2c_128x64_noname_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_I2C_SSD1306:
		u8g2_Setup_ssd1306_i2c_128x64_vcomh0_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_I2C_SSD1309:
		u8g2_Setup_ssd1309_i2c_128x64_noname2_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_I2C_SSD1325:
		u8g2_Setup_ssd1325_i2c_nhd_128x64_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure

		break;
	case LCD_I2C_SSD1309NN:
		u8g2_Setup_ssd1309_i2c_128x64_noname0_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_I2C_SSD1306UN:
		u8g2_Setup_ssd1306_i2c_128x32_univision_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_I2C_ST7567:
		u8g2_Setup_st7567_i2c_64x32_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
//B/W spi
	case LCD_SPI_SSD1306NN:
		u8g2_Setup_ssd1306_128x64_noname_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_SSD1306:
		u8g2_Setup_ssd1306_128x32_univision_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_SSD1309:
		u8g2_Setup_ssd1309_128x64_noname2_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_SSD1309NN:
		u8g2_Setup_ssd1309_128x64_noname0_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_ST7565_ZOLEN:
		u8g2_Setup_st7565_zolen_128x64_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_SSD1322_NHD:
		u8g2_Setup_ssd1322_nhd_256x64_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_IL3820_V2:	//E Paper
		u8g2_Setup_il3820_v2_296x128_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_SSD1607:	//E Paper
		u8g2_Setup_ssd1607_200x200_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_LS013B7DH03:
		u8g2_Setup_ls013b7dh03_128x128_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_ST7920:
		u8g2_Setup_st7920_s_128x64_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_ST7565_NHD_C12864:
		u8g2_Setup_st7565_nhd_c12864_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;	case LCD_SPI_ST7567_pi:
		u8g2_Setup_st7567_pi_132x64_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_ST7567:
		u8g2_Setup_st7567_64x32_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;
	case LCD_SPI_ST7565_64128N:
		u8g2_Setup_st7565_64128n_2(
			&u8g2,
			rotat,
			u8g2_esp32_spi_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
		break;

		default:
		ESP_LOGE(TAG,"UNKNOWN LCD lcd_type %d. Fall back to type \"LCD_NONE\"",*lcd_type);
		*lcd_type = LCD_NONE;
/*		u8g2_esp32_hal_t u8g2_esp32_hal = U8G2_ESP32_HAL_DEFAULT;
		u8g2_esp32_hal.sda  = sda;
		u8g2_esp32_hal.scl  = scl;
		u8g2_esp32_hal_init(u8g2_esp32_hal);
		u8g2_Setup_sh1106_128x64_noname_2(
			&u8g2,
			rotat,
			u8g2_esp32_i2c_byte_cb,
			u8g2_esp32_gpio_and_delay_cb); // init u8g2 structure
*/
	}
	if (*lcd_type != LCD_NONE)
	{
		ESP_LOGD(TAG,"lcd init BW type: %d",*lcd_type);
		if (*lcd_type < LCD_SPI) u8x8_SetI2CAddress(&u8g2.u8x8,0x78);
		u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this,
		u8g2_SetPowerSave(&u8g2, 0); // wake up display
		u8g2_ClearBuffer(&u8g2);
		u8g2_ClearDisplay(&u8g2);
		yy = u8g2.height;
		x  = u8g2.width;
		z = 0;
  u8g2_FirstPage(&u8g2);
  do {
		u8g2_SetFontPosTop(&u8g2);
		addonu8g2_setfont(text);
		y = getFontLineSpacing();
		if (yy>= logo_height)
			 u8g2_DrawXBM( &u8g2,x/2-logo_width/2, yy/2-logo_height/2, logo_width, logo_height, logo_bits);
		else u8g2_DrawXBM( &u8g2,x/2-logo_width/2, 0, logo_width, logo_height, logo_bits);
 } while ( u8g2_NextPage(&u8g2) );
// u8g2_SendBuffer(&u8g2);
		ESP_LOGI(TAG,"X: %d, YY: %d, Y: %d\n",x,yy,y);
		vTaskDelay(100);
//		z = 0;
	}
}

