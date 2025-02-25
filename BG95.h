#ifndef __BG95
#define __BG95

#include <Arduino.h>

#define NBIOT_STREAM  Serial2
#define DEBUG_STREAM  Serial

#define BG95_PWRKEY 33

#define SERVER_IP "ENTER_SERVER_IP"
#define UDP_PORT  50070

//#define _TELENOR_SRB
#define _A1

#ifdef _TELENOR_SRB
#define APN      "internet"
#define APN_USER  "telenor"
#define APN_PASS  "gprs"
#endif

#ifdef _A1
#define APN     "iot"
#define APN_USER  ""
#define APN_PASS  ""
#endif

bool BG95_turnOn();
bool BG95_testIfAlive();
bool BG95_nwkRegister();
bool BG95_TxRxUDP(char payload[], char server_IP[], uint16_t port);

#endif
