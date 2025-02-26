#ifndef LORA_UTIL_H
#define LORA_UTIL_H

#include "LoRaWan_APP.h"

extern uint8_t devEui[8];
extern uint8_t appEui[8];
extern uint8_t appKey[16];

/* LoraWan ABP para*/
extern uint8_t nwkSKey[16];
extern uint8_t appSKey[16];
extern uint32_t devAddr;

/*LoraWan channelsmask, default channels 0-7*/ 
extern uint16_t userChannelsMask[6];

/*LoraWan region, select in arduino IDE tools*/
extern LoRaMacRegion_t loraWanRegion;

/*LoraWan Class, Class A and Class C are supported*/
extern DeviceClass_t  loraWanClass;

/*the application data transmission duty cycle.  value in [ms].*/
extern uint32_t appTxDutyCycle;

/*OTAA or ABP*/
extern bool overTheAirActivation;

/*ADR enable*/
extern bool loraWanAdr;

/* Indicates if the node is sending confirmed or unconfirmed messages */
extern bool isTxConfirmed;

/* Application port */
extern uint8_t appPort;

extern uint8_t confirmedNbTrials;

/* Prepares the payload of the frame */
void prepareTxFrame( uint8_t port, char *timestamp, float orp_val, float ph_val, float do_val, float ec_val, float rtd_val );

#endif