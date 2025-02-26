#ifndef DS3231M_UTIL
#define DS3231M_UTIL

#include <DS3231M.h>

extern DS3231M_Class DS3231M;
extern DateTime currentTime;
extern uint8_t secs;

void DS3231M_init();
void DS3231M_get_and_print_time();

#endif