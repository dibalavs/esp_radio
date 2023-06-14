/*
 * Copyright 2016 karawin (http://www.karawin.fr)
*/
#ifndef __WEBCLIENT_H__
#define __WEBCLIENT_H__
#include "esp_system.h"
#include "esp_log.h"
//#include "websocket.h"

#define METADATA 9
#define METAINT 8
#define BITRATE 5
#define METANAME 0
#define METAGENRE 4
#define ICY_HEADERS_COUNT 9
#define ICY_HEADER_COUNT 10

// audio buffer size in k
// default to 30
#define DEFAULTRAM	40
// for https
#define HTTPSRAM	25
#define HTTPSVSRAM	40
// for vs1053 output on woover
#define SMALLRAM 	50
// for wrover
#define BIGRAM		400


typedef enum
{
    KMIME_UNKNOWN = 1, KOCTET_STREAM, KAUDIO_AAC, KAUDIO_MP4, KAUDIO_MPEG, KAUDIO_OGG
} contentType_t;


struct icyHeader
{
	union
	{
		struct
		{
			char* name;
			char* notice1;
			char* notice2;
			char* url;
			char* genre;
			char* bitrate;
			char* description;
			char* audioinfo;
			int metaint;
			char* metadata;
		} single;
		char* mArr[ICY_HEADER_COUNT];
	} members;
};


enum clientStatus {C_HEADER0, C_HEADER, C_HEADER1,C_METADATA, C_DATA, C_PLAYLIST, C_PLAYLIST1 };

void webclient_init();
uint8_t webclient_is_connected();
bool webclient_parse_playlist(char* s);
void webclient_set_url(char* url);
void webclient_set_name(const char* name,uint16_t index);
void webclient_set_path(char* path);
void webclient_set_port(uint16_t port);
bool webclient_print_headers();
void webclient_print_state();
bool webclient_get_state();
char* webclient_get_meta();


struct icyHeader* webclient_get_header();
void webclient_connect();
void webclient_silent_connect();
void webclient_connect_once();
void webclient_disconnect(const char* from);
void webclient_silent_disconnect();
bool webclient_save_one_header(const char* t, uint16_t len, uint8_t header_num);
void webclient_task(void *pvParams);
void webclient_ws_vol(uint8_t vol);
void webclient_ws_monitor();
void webclient_ws_station_next();
void webclient_ws_station_prev();
uint8_t webclient_wolfssl_get_log_state();


#endif
