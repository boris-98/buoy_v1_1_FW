#include "LoRa_util.h"

// Define LoRaWAN OTAA parameters
uint8_t devEui[8]  = { 0x70, 0xB3, 0xD5, 0x7E, 0xA1, 0x23, 0x45, 0xB6 };
uint8_t appEui[8]  = { 0x00, 0x1A, 0x79, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F };
uint8_t appKey[16] = { 0x1A, 0xCF, 0x45, 0x78, 0xB9, 0xE2, 0x33, 0xD4, 0x99, 0xAA, 0xCC, 0xFF, 0x10, 0x34, 0x67, 0x89 };

// Define LoRaWAN ABP parameters
uint8_t nwkSKey[16] = { 0x15, 0xb1, 0xd0, 0xef, 0xa4, 0x63, 0xdf, 0xbe, 0x3d, 0x11, 0x18, 0x1e, 0x1e, 0xc7, 0xda, 0x85 };
uint8_t appSKey[16] = { 0xd7, 0x2c, 0x78, 0x75, 0x8c, 0xdc, 0xca, 0xbf, 0x55, 0xee, 0x4a, 0x77, 0x8d, 0x16, 0xef, 0x67 };
uint32_t devAddr = 0x007e6ae1;

// Define LoRaWAN settings
uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 15000;
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

// Function to prepare LoRaWAN transmission frame
void prepareTxFrame(uint8_t port, char *payload) {
    // appData je definisan kao uint8_t tj. niz bajtova
    // njegova maks. velicina je definisana 242
    
    appDataSize = sizeof(payload);  // PROVERI KOLIKO ISPADNE
    memcpy(&appData, &payload, sizeof(payload));
/*
    uint8_t index = 0;
    memcpy(&appData[index], &timestamp, sizeof(timestamp));  index += sizeof(timestamp);
    memcpy(&appData[index], &orp_val, sizeof(orp_val));  index += sizeof(orp_val);
    memcpy(&appData[index], &ph_val, sizeof(ph_val));  index += sizeof(ph_val);
    memcpy(&appData[index], &do_val, sizeof(do_val));  index += sizeof(do_val);
    memcpy(&appData[index], &ec_val, sizeof(ec_val));  index += sizeof(ec_val);
    memcpy(&appData[index], &rtd_val, sizeof(rtd_val));  index += sizeof(rtd_val);
*/
}
