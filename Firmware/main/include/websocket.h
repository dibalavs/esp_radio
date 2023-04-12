/* (c)jp cocatrix May 2017
	quick and dirty websocket inplementation for wifi webradio
Inspirated by:
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the WebSockets for Arduino.
 *
 * Copyright 2017 karawin (http://www.karawin.fr)
*/

#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <stdbool.h>

// max size of the WS Message Header
#define WEBSOCKETS_MAX_HEADER_SIZE  (14)

typedef  uint32_t u32;
//#include "crypto/sha1_i.h"


#define NBCLIENT 5
#define MAXDATA	 528



typedef enum {
    WSop_continuation = 0x00, ///< %x0 denotes a continuation frame
    WSop_text = 0x01,         ///< %x1 denotes a text frame
    WSop_binary = 0x02,       ///< %x2 denotes a binary frame
                              ///< %x3-7 are reserved for further non-control frames
    WSop_close = 0x08,        ///< %x8 denotes a connection close
    WSop_ping = 0x09,         ///< %x9 denotes a ping
    WSop_pong = 0x0A          ///< %xA denotes a pong
                              ///< %xB-F are reserved for further control frames
} wsopcode_t;

typedef struct {
        bool fin;
//        bool rsv1;
//        bool rsv2;
//        bool rsv3;
		wsopcode_t opCode;
        bool mask;
		size_t payloadLen;
        uint8_t * maskKey;
} wsMessageHeader_t;


typedef struct {
	int socket;
} client_t;

extern client_t webserverclients[NBCLIENT];

// public:
// init some data
void websocket_init(void);
// a demand received, accept it
void websocket_accept(int wsocket,char* bufin,int buflen);
// a socket with a websocket .
bool websocket_new_client(int socket);
// a socket with a websocket closed
void websocket_remove_client(int socket);
// is socket a websocket?
bool is_websocket( int socket);
//read a message. close the connection if error
void websocket_parse_data(int socket,char* buf, int recbytes);
//write a txt data
void websocket_write(int socket,char* buf, int len);
//read a txt data
int websocket_read(int conn);
//broadcast a txt data to all clients
void websocket_broadcast(char* buf, int len);
//broadcast a txt data to all clients but the sender
void websocket_limited_broadcast(int socket,char* buf, int len);
// the websocket server task
void websocket_task(void* pvParams) ;
//private:
bool websocket_send_frame(int socket, wsopcode_t opcode, uint8_t * payload , size_t length );
// parse a new client request and prepare the answer
uint32_t websocket_decode_http_message (char * inputMessage, char * outputMessage);
void websocket_client_disconnect(int socket, uint16_t code, char * reason , size_t reasonLen );
#endif