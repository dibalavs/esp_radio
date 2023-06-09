/*
 * Copyright 2016 karawin (http://www.karawin.fr)
*/
#pragma once

#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "websocket.h"

extern SemaphoreHandle_t semclient;
extern SemaphoreHandle_t semfile;

void webserver_client_task(void *pvParams);
void webserver_play_station_int(int sid);
void webserver_websocket_handle(int socket, wsopcode_t opcode, uint8_t * payload, size_t length);
uint16_t webserver_get_volume(void);
void webserver_set_volume(char* vol);
void webserver_set_volumei(int16_t vol);
void webserver_set_rel_volume(int8_t vol);

#endif