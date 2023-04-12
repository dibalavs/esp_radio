/* (c)jp cocatrix May 2017
	quick and dirty telnet inplementation for wifi webradio
Inspirated by:
 *
 * This file is part of the WebSockets for Arduino.
 *
 * Copyright 2017 karawin (http://www.karawin.fr)
*/

#ifndef __TELNET_H__
#define __TELNET_H__
// max size of the WS Message Header
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define NBCLIENTT 5
//#define MAXDATAT	 256

//extern const char strtWELCOME[] ;
extern int telnetclients[NBCLIENTT];

// public:
// init some data
void telnet_init(void);
// a demand received, accept it
bool telnet_accept(int tsocket);
// a socket with a telnet .
bool telnet_new_client(int socket);
// a socket with a telnet closed
void telnet_remove_client(int socket);
// is socket a telnet?
bool is_telnet( int socket);

//write a txt data
void telnet_write(uint32_t len,const char *fmt, ...);
void telnet_vwrite(uint32_t lenb,const char *fmt, va_list ap);

int telnet_read(int tsocket);
// the telnet server task
void telnet_task(void* pvParams) ;

extern void* kmalloc(size_t memorySize);
extern void* kcalloc(size_t elementCount, size_t elementSize);
#endif