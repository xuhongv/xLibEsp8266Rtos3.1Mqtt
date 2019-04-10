#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "string.h"

extern int connect_socket;


int mqtt_tcp_connect(const char*host,const int port); 
int mqtt_tcp_disconnect(void); 
//int tcp_send(int connect_socket,uint8_t*buff,int len);
//int tcp_receive(int connect_socket,uint8_t*buff,int len);

int tcp_send(int connect_socket,uint8_t*buff,int len,unsigned int timeout_ms);
int tcp_receive(int connect_socket,uint8_t*buff,int len,unsigned int timeout_ms);

#endif