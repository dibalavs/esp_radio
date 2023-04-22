/******************************************************************************
 *
 * Copyright 2017 karawin (http://www.karawin.fr)
 *
*******************************************************************************/

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "esp_sleep.h"
#include "buttons.h"
#include "app_main.h"
#include "gpio.h"
#include "webclient.h"
#include "webserver.h"
#include "interface.h"

#include "u8g2.h"
#include "ucg.h"

#include "addon.h"
#include "u8g2_esp32_hal.h"
#include "ucg_esp32_hal.h"
#include "ntp.h"

#include "eeprom.h"
#include "addonu8g2.h"
#include "addonucg.h"
#include <ext_gpio.h>

#define TAG  "addon"

static void evtClearScreen();
// second before time display in stop state
#define DTIDLE  60

#define isColor (lcd_type&LCD_COLOR)
const char *stopped = "STOPPED";

char irStr[4];
QueueHandle_t event_ir = NULL;
QueueHandle_t event_lcd = NULL;
ucg_t  ucg;
u8g2_t u8g2; // a structure which will contain all the data for one display
static uint8_t lcd_type;
static TaskHandle_t  pxTaskLcd;
// list of screen
typedef  enum typeScreen {smain,svolume,sstation,snumber,stime,snull} typeScreen ;
static typeScreen stateScreen = snull;
static typeScreen defaultStateScreen = smain;
// state of the transient screen
static uint8_t mTscreen = MTNEW; // 0 dont display, 1 display full, 2 display variable part

static bool playable = true;
static uint16_t volume;
static int16_t futurNum = 0; // the number of the wanted station

static unsigned timerScreen = 0;
static unsigned timerScroll = 0;
static unsigned timerLcdOut = 0;
static unsigned timer1s = 0;

static unsigned timein = 0;
static struct tm *dt;
time_t timestamp = 0;
static bool syncTime = false;
static bool itAskTime = true; // update time with ntp if true
static bool itAskStime = false; // start the time display
static uint8_t itLcdOut = 0;
//static bool itAskSsecond = false; // start the time display
static bool state = false; // start stop on Ok key

static int16_t currentValue = 0;
static bool dvolume = true; // display volume screen

// custom ir code init from hardware nvs
typedef enum {KEY_UP,KEY_LEFT,KEY_OK,KEY_RIGHT,KEY_DOWN,
		KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,
		KEY_STAR,KEY_DIESE,KEY_INFO, KEY_MAX} customKey_t;

static uint32_t customKey[KEY_MAX][2];
static bool isCustomKey = false;

void Screen(typeScreen st);
void drawScreen();
static void evtScreen(typelcmd value);

struct tm* addon_get_dt() { return dt;}

// Deep Sleep Power Save Input. https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html
gpio_num_t deepSleep_io; /** Enter Deep Sleep if pin is set to level defined in P_LEVELPINSLEEP. */
bool deepSleepLevel; /** Level to enter Deep Sleep / Wakeup if level is the opposite. */

static void ClearBuffer()
{
  if (lcd_type == LCD_NONE) return;
  if (isColor)
	ucg_ClearScreen(&ucg);
  else
	u8g2_ClearBuffer(&u8g2);
}

static int16_t DrawString(int16_t x, int16_t y,  const char *str)
{
  if (isColor)
	return ucg_DrawString(&ucg,x,y,0,str);
  else
	return u8g2_DrawUTF8(&u8g2,x,y,str);
}

static void DrawColor(uint8_t color, uint8_t r, uint8_t g, uint8_t b)
{
  if (isColor)
	ucg_SetColor(&ucg, 0,r,g,b);
  else
	u8g2_SetDrawColor(&u8g2, color);
}

static void DrawBox(ucg_int_t x, ucg_int_t y, ucg_int_t w, ucg_int_t h)
{
  if (isColor)
	ucg_DrawBox(&ucg, x,y,w,h);
  else
	u8g2_DrawBox(&u8g2, x,y,w,h);
}

uint16_t addon_get_width()
{
  if (isColor)
	  return ucg_GetWidth(&ucg);

  return u8g2.width;
}
uint16_t addon_get_height()
{
  if (isColor)
	  return ucg_GetHeight(&ucg);

  return u8g2.height;
}

void addon_wake_lcd()
{
	if ((iface_get_lcd_stop() != 0) && (!state))  timerLcdOut = iface_get_lcd_stop(); // rearm the tempo
	else timerLcdOut = iface_get_lcd_out(); // rearm the tempo
	if (itLcdOut==2)
	{
		mTscreen= MTNEW;
		evtScreen(stateScreen);
		itLcdOut = 0;  //0 not activated, 1 sleep requested, 2 in sleep ;
	}

	ext_gpio_set_lcd(true);
}

void sleepLcd()
{
	itLcdOut = 2;  // in sleep
	ext_gpio_set_lcd(false);
}

void addon_lcd_init(uint8_t Type)
{
	lcd_type = Type;

	if (lcd_type == LCD_NONE) return;

	if (isColor) // Color one
	{
		addonucg_lcd_init(&lcd_type);
	} else //B/W lcd
	{
		addonu8g2_lcd_init(&lcd_type);
	}
	vTaskDelay(1);

}

void in_welcome(const char* ip,const char*state,int y,char* Version)
{
	if (lcd_type == LCD_NONE) return;
	DrawString(2,2*y,Version);
	DrawColor(0,0,0,0);
	DrawBox(2, 4*y, addon_get_width()-2, y);
	DrawColor(1,255,255,255);
	DrawString(2,4*y,state);
	DrawString( DrawString(2,5*y,"IP:")+18,5*y,ip);
}

void addon_lcd_welcome(const char* ip,const char*state)
{
	char Version[20];
	sprintf(Version,"Version %s R%s\n",RELEASE,REVISION);
	if (lcd_type == LCD_NONE) return;
	if ((strlen(ip)==0)&&(strlen(state)==0)) ClearBuffer();
	if (isColor)
	{
		addonucg_setfont(2);
		int y = - ucg_GetFontDescent(&ucg)+ ucg_GetFontAscent(&ucg) +3; //interline
		DrawString(addon_get_width()/4,2,"KaRadio32");
		addonucg_setfont(1);
		in_welcome(ip,state,y,Version);
	} else
	{
		u8g2_FirstPage(&u8g2);
		do {
			addonu8g2_setfont(2);
			int y = (u8g2_GetAscent(&u8g2) - u8g2_GetDescent(&u8g2));
			DrawString(addon_get_width()/4,2,"KaRadio32");
			addonu8g2_setfont(1);
			in_welcome(ip,state,y,Version);
		} while ( u8g2_NextPage(&u8g2) );
	}
}

 // ----------------------------------------------------------------------------
// call this every 1 millisecond via timer ISR
//

IRAM_ATTR  void addon_service(void)
{
	timer1s++;
	timerScroll++;
	if (timer1s >=1000)
	{
		// Time compute
        timestamp++;  // time update
		if (timerLcdOut >0) timerLcdOut--; //
		timein++;
		if ((timestamp % (10*DTIDLE))==0){ itAskTime=true;} // synchronise with ntp every x*DTIDLE

		if (((timein % DTIDLE)==0)&&(!state)  ) {
			{itAskStime=true;timein = 0;} // start the time display when paused
        }
		if (timerLcdOut == 1) itLcdOut = 1; // ask to go to sleep
		if (!syncTime) itAskTime=true; // first synchro if not done

		timer1s = 0;
		// Other slow timers
        timerScreen++;
	}
}

////////////////////////////////////////
// futurNum
void addon_set_futur_num(int16_t new)
{
	futurNum = new;
}
int16_t addon_get_futur_num()
{
	return futurNum;
}

////////////////////////////////////////
// scroll each line
void scroll()
{
	isColor?addonucg_scroll():addonu8g2_scroll();
}


////////////////////////////
// Change the current screen
////////////////////////////
void Screen(typeScreen st){
//printf("Screen: st: %d, stateScreen: %d, mTscreen: %d, default: %d\n",st,stateScreen,mTscreen,defaultStateScreen);
  if (stateScreen != st)
  {
	mTscreen = MTNEW;
//	wakeLcd();
  }
  else
  {
    if (mTscreen == MTNODISPLAY) mTscreen = MTREFRESH;
  }

//  printf("Screenout: st: %d, stateScreen: %d, mTscreen: %d, default: %d, timerScreen: %d \n",st,stateScreen,mTscreen,defaultStateScreen,timerScreen);

  stateScreen = st;
  timein = 0;
  timerScreen = 0;
  drawScreen();
//printf("Screendis: st: %d, stateScreen: %d, mTscreen: %d, default: %d\n",st,stateScreen,mTscreen,defaultStateScreen);
//  vTaskDelay(1);
}


////////////////////////////////////////
// draw all lines
void drawFrame()
{
	dt=localtime(&timestamp);
	if (lcd_type == LCD_NONE) return;
	isColor?addonucg_draw_frame(mTscreen):addonu8g2_draw_frame(mTscreen);
}


//////////////////////////
void drawTTitle(char* ttitle)
{
	isColor?addonucg_draw_ttitle(ttitle):addonu8g2_draw_ttitle(ttitle);
}

////////////////////
// draw the number entered from IR
void drawNumber()
{
	if (strlen(irStr) >0)
		isColor?addonucg_draw_number(mTscreen,irStr):addonu8g2_draw_number(mTscreen,irStr);
}


////////////////////
// draw the station screen
void drawStation()
{
  char sNum[7] ;
  char* ddot;
  char* ptl ;
  struct shoutcast_info* si;

 //ClearBuffer();

  do {
	si = eeprom_get_station(futurNum);
	sprintf(sNum,"%d",futurNum);
	ddot = si->name;
	ptl = ddot;
	while ( *ptl == 0x20){ddot++;ptl++;}
	if (strlen(ddot)==0) // don't start an undefined station
	{
		playable = false;
		free(si);
		if (currentValue < 0) {
			futurNum--;
			if (futurNum <0) futurNum = 254;
		}
		else {
			futurNum++;
			if (futurNum > 254) futurNum = 0;
		}
	}
	else
		playable = true;
  } while (playable == false);

  //drawTTitle(ststr);
//printf ("drawStation: %s\n",sNum  );
  if (lcd_type != LCD_NONE)
	isColor?addonucg_draw_station(mTscreen,sNum,ddot):addonu8g2_draw_station(mTscreen,sNum,ddot);
  free (si);
}

////////////////////
// draw the volume screen
void drawVolume()
{
//  printf("drawVolume. mTscreen: %d, Volume: %d\n",mTscreen,volume);
  if (lcd_type == LCD_NONE) return;
  isColor?addonucg_draw_volume(mTscreen):addonu8g2_draw_volume(mTscreen);
}

void drawTime()
{
	dt=localtime(&timestamp);
	if (lcd_type == LCD_NONE) return;
	isColor?addonucg_draw_time(mTscreen,timein):addonu8g2_draw_time(mTscreen,timein);
}


////////////////////
// Display a screen on the lcd
void drawScreen()
{
//  if (lcd_type == LCD_NONE) return;
//  ESP_LOGD(TAG,"stateScreen: %d,defaultStateScreen: %d, mTscreen: %d, itLcdOut: %d",stateScreen,defaultStateScreen,mTscreen,itLcdOut);
  if ((mTscreen != MTNODISPLAY)&&(!itLcdOut))
  {
	switch (stateScreen)
	{
    case smain:  //
     drawFrame();
      break;
    case svolume:
      drawVolume();
      break;
    case sstation:
      drawStation();
      break;
    case stime:
      drawTime();
      break;
    case snumber:
      drawNumber();
      break;
    default:
	  Screen(defaultStateScreen);
//	  drawFrame();
	}
//	if (mTscreen == MTREFRESH)
		mTscreen = MTNODISPLAY;
  }
}


void stopStation()
{
//    irStr[0] = 0;
	webclient_disconnect("addon stop");
}
void startStation()
{
 //   irStr[0] = 0;
    webserver_play_station_int(futurNum); ;
}
void startStop()
{
	ESP_LOGD(TAG,"START/STOP State: %d",state);
    state?stopStation():startStation();
}
void stationOk()
{
	ESP_LOGD(TAG,"STATION OK");
       if (strlen(irStr) >0)
	   {
		  futurNum = atoi(irStr);
          webserver_play_station_int(futurNum);
	   }
        else
        {
            startStop();
        }
        irStr[0] = 0;
}
void changeStation(int16_t value)
{
	currentValue = value;
	ESP_LOGD(TAG,"changeStation val: %d, futurnum: %d",value,futurNum);
	if (value > 0) futurNum++;
	if (futurNum > 254) futurNum = 0;
	else if (value < 0) futurNum--;
	if (futurNum <0) futurNum = 254;
	ESP_LOGD(TAG,"futurnum: %d",futurNum);
	//else if (value != 0) mTscreen = MTREFRESH;
}
// IR
// a number of station in progress...
void nbStation(char nb)
{
  if (strlen(irStr)>=3) irStr[0] = 0;
  uint8_t id = strlen(irStr);
  irStr[id] = nb;
  irStr[id+1] = 0;
  evtScreen(snumber);
}

//
static void evtClearScreen()
{
//	isColor?ucg_ClearScreen(&ucg):u8g2_ClearDisplay(&u8g2);
	event_lcd_t evt;
	evt.lcmd = eclrs;
	evt.lline = NULL;
	if (lcd_type != LCD_NONE) xQueueSend(event_lcd,&evt, 0);
}

static void evtScreen(typelcmd value)
{
	event_lcd_t evt;
	evt.lcmd = escreen;
	evt.lline = (char*)((uint32_t)value);
	if (lcd_type != LCD_NONE) xQueueSend(event_lcd,&evt, 0);

}

static void evtStation(int16_t value)
{ // value +1 or -1
	event_lcd_t evt;
	evt.lcmd = estation;
	evt.lline = (char*)((uint32_t)value);
	if (lcd_type != LCD_NONE) xQueueSend(event_lcd,&evt, 0);
}

// toggle main / time
static void toggletime()
{
	event_lcd_t evt;
	evt.lcmd = etoggle;
	evt.lline = NULL;
	if (lcd_type != LCD_NONE) xQueueSend(event_lcd,&evt, 0);
}

void buttons_loop(void)
{
	button_event_t *event = buttons_get_event();
	if (event == NULL)
		return;

	switch (event->button) {
	case BTN_TYPE_PLAY:
		if (event->state == BTN_STATE_CLICKED)
			startStop();
		else if (event->state == BTN_STATE_DBLCLICKED)
			toggletime();
		break;

	case BTN_TYPE_PREV:
		evtStation(-1);
		break;

	case BTN_TYPE_NEXT:
		evtStation(+1);
		break;

	case BTN_TYPE_ENC_BTN:
	/*  TODO: implement it
		if (event->state == BTN_STATE_CLICKED)
			switch_fm_radio();
	*/
		if (event->state == BTN_STATE_HOLD)
			addon_deep_sleep_start();
		break;

	case BTN_TYPE_ENC_LESS:
		webserver_set_rel_volume(-1);
		break;

	case BTN_TYPE_ENC_MORE:
		webserver_set_rel_volume(+1);
		break;

	case BTN_TYPE_LAST:
		break;
	}

	buttons_release_event(event);
}

// compute custom IR
bool irCustom(uint32_t evtir, bool repeat)
{
	int i;
	for (i=KEY_UP;i < KEY_MAX;i++)
	{
		if ((evtir == customKey[i][0])||(evtir == customKey[i][1])) break;
	}
	if (i<KEY_MAX)
	{
		switch (i)
		{
			case KEY_UP: evtStation(+1);  break;
			case KEY_LEFT: webserver_set_rel_volume(-5);  break;
			case KEY_OK: if (!repeat ) stationOk();   break;
			case KEY_RIGHT: webserver_set_rel_volume(+5);   break;
			case KEY_DOWN: evtStation(-1);  break;
			case KEY_0: if (!repeat ) nbStation('0');   break;
			case KEY_1: if (!repeat ) nbStation('1');  break;
			case KEY_2: if (!repeat ) nbStation('2');  break;
			case KEY_3: if (!repeat ) nbStation('3');  break;
			case KEY_4: if (!repeat ) nbStation('4');  break;
			case KEY_5: if (!repeat ) nbStation('5');  break;
			case KEY_6: if (!repeat ) nbStation('6');  break;
			case KEY_7: if (!repeat ) nbStation('7');  break;
			case KEY_8: if (!repeat ) nbStation('8');  break;
			case KEY_9: if (!repeat ) nbStation('9');  break;
			case KEY_STAR: if (!repeat ) webserver_play_station_int(futurNum);  break;
			case KEY_DIESE: if (!repeat )  stopStation();  break;
			case KEY_INFO: if (!repeat ) toggletime();  break;
			default: ;
		}
		ESP_LOGV(TAG,"irCustom success, evtir %x, i: %d",evtir,i);
		return true;
	}
	return false;
}

 //-----------------------
 // Compute the ir code
 //----------------------

 void irLoop()
 {
// IR
event_ir_t evt;
	while (xQueueReceive(event_ir, &evt, 0))
	{
		addon_wake_lcd();
		uint32_t evtir = ((evt.addr)<<8)|(evt.cmd&0xFF);
		ESP_LOGI(TAG,"IR event: Channel: %x, ADDR: %x, CMD: %x = %X, REPEAT: %d",evt.channel,evt.addr,evt.cmd, evtir,evt.repeat_flag );

		if (isCustomKey){
			if (irCustom(evtir,evt.repeat_flag)) continue;
		}
		else{ // no predefined keys
		switch(evtir)
		{
		case 0xDF2047:
		case 0xDF2002:
		case 0xFF0046:
		case 0xF70812:  /*(" UP");*/  evtStation(+1);
		break;
		case 0xDF2049:
		case 0xDF2041:
		case 0xFF0044:
		case 0xF70842:
		case 0xF70815: /*(" LEFT");*/  webserver_set_rel_volume(-5);
		break;
		case 0xDF204A:
		case 0xFF0040:
		case 0xF7081E: /*(" OK");*/ if (!evt.repeat_flag ) stationOk();
		break;
		case 0xDF204B:
		case 0xDF2003:
		case 0xFF0043:
		case 0xF70841:
		case 0xF70814: /*(" RIGHT");*/ webserver_set_rel_volume(+5);
		break;
		case 0xDF204D:
		case 0xDF2009:
		case 0xFF0015:
		case 0xF70813: /*(" DOWN");*/ evtStation(-1);
		break;
		case 0xDF2000:
		case 0xFF0016:
		case 0xF70801: /*(" 1");*/ if (!evt.repeat_flag ) nbStation('1');
		break;
		case 0xDF2010:
		case 0xFF0019:
		case 0xF70802: /*(" 2");*/ if (!evt.repeat_flag ) nbStation('2');
		break;
		case 0xDF2011:
		case 0xFF000D:
		case 0xF70803: /*(" 3");*/ if (!evt.repeat_flag ) nbStation('3');
		break;
		case 0xDF2013:
		case 0xFF000C:
		case 0xF70804: /*(" 4");*/ if (!evt.repeat_flag ) nbStation('4');
		break;
		case 0xDF2014:
		case 0xFF0018:
		case 0xF70805: /*(" 5");*/ if (!evt.repeat_flag ) nbStation('5');
		break;
		case 0xDF2015:
		case 0xFF005E:
		case 0xF70806: /*(" 6");*/ if (!evt.repeat_flag ) nbStation('6');
		break;
		case 0xDF2017:
		case 0xFF0008:
		case 0xF70807: /*(" 7");*/ if (!evt.repeat_flag ) nbStation('7');
		break;
		case 0xDF2018:
		case 0xFF001C:
		case 0xF70808: /*(" 8");*/ if (!evt.repeat_flag ) nbStation('8');
		break;
		case 0xDF2019:
		case 0xFF005A:
		case 0xF70809: /*(" 9");*/ if (!evt.repeat_flag ) nbStation('9');
		break;
		case 0xDF2045:
		case 0xFF0042:
		case 0xF70817: /*(" *");*/   if (!evt.repeat_flag ) webserver_play_station_int(futurNum);
		break;
		case 0xDF201B:
		case 0xFF0052:
		case 0xF70800: /*(" 0");*/ if (!evt.repeat_flag ) nbStation('0');
		break;
		case 0xDF205B:
		case 0xFF004A:
		case 0xF7081D: /*(" #");*/ if (!evt.repeat_flag )  stopStation();
		break;
		case 0xDF2007: /*(" Info")*/ if (!evt.repeat_flag ) toggletime();
		break;
		default:;
		/*SERIALX.println(F(" other button   "));*/
		}// End Case
		}
	}
}

// custom ir code init from hardware nvs partition
#define hardware "hardware"
void customKeyInit()
{
	customKey_t index;
	nvs_handle handle;
	const char *klab[] = {"K_UP","K_LEFT","K_OK","K_RIGHT","K_DOWN","K_0","K_1","K_2","K_3","K_4","K_5","K_6","K_7","K_8","K_9","K_STAR","K_DIESE","K_INFO"};

	memset(&customKey,0,sizeof(uint32_t)*2*KEY_MAX); // clear custom
	if (open_partition(hardware, "custom_ir_space",NVS_READONLY,&handle)!= ESP_OK) return;

	for (index = KEY_UP; index < KEY_MAX;index++)
	{
		// get the key in the nvs
		isCustomKey |= gpio_get_ir_key(handle,klab[index],(uint32_t*)&(customKey[index][0]),(uint32_t*)&(customKey[index][1]));
		ESP_LOGV(TAG," isCustomKey is %d for %d",isCustomKey,index);
		taskYIELD();
	}
	close_partition(handle,hardware);
}

//--------------------
// LCD display task
//--------------------

void addon_task_lcd(void *pvParams)
{
	event_lcd_t evt ; // lcd event
	event_lcd_t evt1 ; // lcd event
	ESP_LOGD(TAG, "task_lcd Started, LCD Type %d",lcd_type);
	defaultStateScreen = (g_device->options32&T_TOGGLETIME)? stime:smain;
	if (lcd_type != LCD_NONE)  drawFrame();

	while (1)
	{
		if (itLcdOut==1) // switch off the lcd
		{
			sleepLcd();
		}

		if (timerScroll >= 500) //500 ms
		{
			if (lcd_type != LCD_NONE)
			{
				if (stateScreen == smain)
				{
					scroll();
				}
				if ((stateScreen == stime)||(stateScreen == smain)) {mTscreen = MTREFRESH; } // display time

				drawScreen();
			}
			timerScroll = 0;
		}
		if (event_lcd != NULL)
		while (xQueueReceive(event_lcd, &evt, 0))
		{
//			if (lcd_type == LCD_NONE) continue;
			if (evt.lcmd != lmeta)
				ESP_LOGV(TAG,"event_lcd: %x, %d, mTscreen: %d",(int)evt.lcmd,(int)evt.lline,mTscreen);
			else
				ESP_LOGV(TAG,"event_lcd: %x  %s, mTscreen: %d",(int)evt.lcmd,evt.lline,mTscreen);
			switch(evt.lcmd)
			{
				case lmeta:
					isColor?addonucg_meta(evt.lline):addonu8g2_meta(evt.lline);
					Screen(smain);
					addon_wake_lcd();
					break;
				case licy4:
					isColor?addonucg_icy4(evt.lline):addonu8g2_icy4(evt.lline);
					break;
				case licy0:
					isColor?addonucg_icy0(evt.lline):addonu8g2_icy0(evt.lline);
					break;
				case lstop:
					isColor?addonucg_status(stopped):addonu8g2_status(stopped);
					Screen(smain);
					addon_wake_lcd();
					break;
				case lnameset:
					isColor?addonucg_nameset(evt.lline):addonu8g2_nameset(evt.lline);
					isColor?addonucg_status("STARTING"):addonu8g2_status("STARTING");
					Screen(smain);
					addon_wake_lcd();
					break;
				case lplay:
					isColor?addonucg_playing():addonu8g2_playing();
					break;
				case lvol:
					// ignore it if the next is a lvol
					if(xQueuePeek(event_lcd, &evt1, 0))
						if (evt1.lcmd == lvol) break;
					isColor?addonucg_set_volume(volume):addonu8g2_set_volume(volume);
					if (dvolume)
					{	Screen(svolume);
						addon_wake_lcd();
					}
					dvolume = true;
					break;
				case lovol:
					dvolume = false; // don't show volume on start station
					break;
				case estation:
					if(xQueuePeek(event_lcd, &evt1, 0))
						if (evt1.lcmd == estation) {evt.lline = NULL;break;}
					ESP_LOGD(TAG,"estation val: %d",(uint32_t)evt.lline);
					changeStation((uint32_t)evt.lline);
					Screen(sstation);
					addon_wake_lcd();
					evt.lline = NULL;	// just a number
					break;
				case eclrs:
					isColor?ucg_ClearScreen(&ucg):u8g2_ClearDisplay(&u8g2);
					break;
				case escreen:
					Screen((uint32_t)evt.lline);
					addon_wake_lcd();
					evt.lline = NULL;	// just a number Don't free
					break;
				case etoggle:
					defaultStateScreen = (stateScreen==smain)?stime:smain;
					(stateScreen==smain)?Screen(stime):Screen(smain);
					g_device->options32 = (defaultStateScreen== smain)?g_device->options32&NT_TOGGLETIME:g_device->options32|T_TOGGLETIME;
					addon_wake_lcd();
//					saveDeviceSettings(g_device);
					break;
				default:;
			}
			if (evt.lline != NULL) free(evt.lline);
			vTaskDelay(4);
		 }
		 if ((event_lcd)&&(!uxQueueMessagesWaiting(event_lcd))) vTaskDelay(10);
		vTaskDelay(4);
	}
	vTaskDelete( NULL );
}

//-------------------
// Main task of addon
//-------------------
extern void rmt_nec_rx_task();

void addon_task(void *pvParams)
{
	TaskHandle_t pxCreatedTask;
	customKeyInit();

	futurNum = iface_get_current_station();

	//ir
	// queue for events of the IR nec rx
	event_ir = xQueueCreate(5, sizeof(event_ir_t));
	ESP_LOGD(TAG,"event_ir: %x",(int)event_ir);

	xTaskCreatePinnedToCore(rmt_nec_rx_task, "rmt_nec_rx_task", 2148, NULL, PRIO_RMT, &pxCreatedTask,CPU_RMT);
	ESP_LOGI(TAG, "%s task: %x","rmt_nec_rx_task",(unsigned int)pxCreatedTask);		;

	if (g_device->lcd_type!=LCD_NONE)
	{
		// queue for events of the lcd
		event_lcd = xQueueCreate(10, sizeof(event_lcd_t));
		ESP_LOGD(TAG,"event_lcd: %x",(int)event_lcd);

		xTaskCreatePinnedToCore (addon_task_lcd, "task_lcd", 2300, NULL, PRIO_LCD, &pxTaskLcd,CPU_LCD);
		ESP_LOGI(TAG, "%s task: %x","task_lcd",(unsigned int)pxTaskLcd);
	}

	// Configure Deep Sleep start and wakeup options
	addon_deep_sleep_conf(); // also called in app_main.c

	while (1)
	{
		buttons_loop();
		irLoop();  // compute the ir
		if (itAskTime) // time to ntp. Don't do that in interrupt.
		{
			if (ntp_get_time(&dt) )
			{
				timezone_apply_tz(dt);
				timestamp = mktime(dt);
				syncTime = true;
			}
			itAskTime = false;
		}

		if (timerScreen >= 3) //  sec timeout transient screen
		{
//			if ((stateScreen != smain)&&(stateScreen != stime)&&(stateScreen != snull))
//printf("timerScreen: %d, stateScreen: %d, defaultStateScreen: %d\n",timerScreen,stateScreen,defaultStateScreen);
			timerScreen = 0;
			if ((stateScreen != defaultStateScreen)&&(stateScreen != snull))
			{
				// Play the changed station on return to main screen
				// if a number is entered, play it.
				if (strlen(irStr) >0){
					futurNum = atoi (irStr);
					if (futurNum>254) futurNum = 0;
					playable = true;
					// clear the number
					irStr[0] = 0;
				}
				if ((strlen(isColor?addonucg_get_name_num_ucg():addonu8g2_get_name_num()) != 0 )
					&& playable
					&& ( futurNum!= atoi(  isColor?addonucg_get_name_num_ucg():addonu8g2_get_name_num()  )))
				{
					webserver_play_station_int(futurNum);
					vTaskDelay(2);
				}
				if (!itAskStime)
				{
					if ((defaultStateScreen == stime) && (stateScreen != smain))evtScreen(smain);
					else
					if ((defaultStateScreen == stime) && (stateScreen == smain))evtScreen(stime);
					else
					if 	(stateScreen != defaultStateScreen)
					evtScreen(defaultStateScreen); //Back to the old screen
				}
			}
			if (itAskStime&&(stateScreen != stime)) // time start the time display. Don't do that in interrupt.
				evtScreen(stime);
		}

		vTaskDelay(10);
	}
	vTaskDelete( NULL );
}

// force a new dt ntp fetch
void addon_dt() { itAskTime = true; }


////////////////////////////////////////
// parse the karadio received line and do the job
void addon_parse(const char *fmt, ...)
{
	event_lcd_t evt;
	char *line = NULL;
//	char* lfmt;
	int rlen;
	line = (char *)kmalloc(1024);
	if (line == NULL) return;
	line[0] = 0;
	strcpy(line,"ok\n");

	va_list ap;
	va_start(ap, fmt);
	rlen = vsprintf(line,fmt, ap);
	va_end(ap);
	line = realloc(line,rlen+1);
	if (line == NULL) return;
	ESP_LOGV(TAG,"LINE: %s",line);
	evt.lcmd = -1;
  char* ici;

 ////// Meta title  ##CLI.META#:
   if ((ici=strstr(line,"META#: ")) != NULL)
   {
		evt.lcmd = lmeta;
		evt.lline = kmalloc(strlen(ici)+1);
		strcpy(evt.lline,ici);
   } else
 ////// ICY4 Description  ##CLI.ICY4#:
    if ((ici=strstr(line,"ICY4#: ")) != NULL)
    {
		evt.lcmd = licy4;
		evt.lline = kmalloc(strlen(ici)+1);
		strcpy(evt.lline,ici);
    } else
 ////// ICY0 station name   ##CLI.ICY0#:
   if ((ici=strstr(line,"ICY0#: ")) != NULL)
   {
		evt.lcmd = licy0;
		evt.lline = kmalloc(strlen(ici)+1);
		strcpy(evt.lline,ici);
   } else
 ////// STOPPED  ##CLI.STOPPED#
   if (((ici=strstr(line,"STOPPED")) != NULL)&&(strstr(line,"C_HDER") == NULL)&&(strstr(line,"C_PLIST") == NULL))
   {
		state = false;
 		evt.lcmd = lstop;
		evt.lline = NULL;
   }
   else
 //////Nameset    ##CLI.NAMESET#:
   if ((ici=strstr(line,"MESET#: ")) != NULL)
   {
	  	evt.lcmd = lnameset;
		evt.lline = kmalloc(strlen(ici)+1);
		strcpy(evt.lline,ici);
   } else
 //////Playing    ##CLI.PLAYING#
   if ((ici=strstr(line,"YING#")) != NULL)
   {
		state = true;
		itAskStime = false;
 		evt.lcmd = lplay;
		evt.lline = NULL;
   } else
   //////Volume   ##CLI.VOL#:
   if ((ici=strstr(line,"VOL#:")) != NULL)
   {
	   if (*(ici+6) != 'x') // ignore help display.
	   {
		volume = atoi(ici+6);
 		evt.lcmd = lvol;
		evt.lline = NULL;//atoi(ici+6);
	   }
   } else
  //////Volume offset    ##CLI.OVOLSET#:
   if ((ici=strstr(line,"OVOLSET#:")) != NULL)
   {
	    evt.lcmd = lovol;
		evt.lline = NULL;
   }
   if (evt.lcmd != -1 && lcd_type !=LCD_NONE) xQueueSend(event_lcd,&evt, 0);
   free (line);
}

/** Configure Deep Sleep: source and wakeup options. */
bool addon_deep_sleep_conf(void)
{
	/** Configure Deep Sleep External wakeup (ext0). */
	/** Wake up (EXT0) when GPIO deepSleep_io pin level is opposite to deepSleepLevel. */
	esp_sleep_enable_ext0_wakeup(PIN_EXT_GPIO_INT, 1);
	return true;
}

/** Enter ESP32 Deep Sleep with the configured wakeup options, and powerdown peripherals, */
/** when P_SLEEP GPIO is set to P_LEVEL_SLEEP. */
/** https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html */
void addon_deep_sleep_start(void)
{
	/** 1. Enter peripherals sleep */
	sleepLcd();	// LCD
/** note that PCM5102 also enters powerdown because pins P_I2S_LRCK and P_I2S_BCLK are low. */

	/** 2. Enter ESP32 deep sleep with the configured wakeup options. */
/*	YMMV: rtc_gpio_isolate(deepSleep_io); // disconnect GPIO from internal circuits in deep sleep, to minimize leakage current. */
	esp_deep_sleep_start();
}
