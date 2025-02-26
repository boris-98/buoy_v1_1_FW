#include "DS3231M_util.h"

DS3231M_Class DS3231M;
DateTime currentTime;
uint8_t secs = 0;

void DS3231M_init()
{
  while (!DS3231M.begin())  // Initialize RTC communications
  {
    Serial.println(F("Unable to find DS3231MM. Checking again in 3s."));
    delay(3000);
  }                         // of loop until device is located
  DS3231M.pinSquareWave();  // Make INT/SQW pin toggle at 1Hz
  Serial.println(F("DS3231M initialized."));
  DS3231M.adjust();  // Set to library compile Date/Time
  Serial.print(F("DS3231M chip temperature is "));
  Serial.print(DS3231M.temperature() / 100.0, 1);  // Value is in 100ths of a degree
  Serial.println(
    "\xC2\xB0"
    "C");
}

void DS3231M_get_and_print_time()
{
  currentTime = DS3231M.now();          // get the current time from device
  if (secs != currentTime.second())     // Output if seconds have changed
  {
    // Use sprintf() to pretty print the date/time with leading zeros
    char output_buffer[32];  ///< Temporary buffer for sprintf()
    sprintf(output_buffer, "%04d-%02d-%02d %02d:%02d:%02d", currentTime.year(), currentTime.month(), currentTime.day(),
            currentTime.hour(), currentTime.minute(), currentTime.second());
    Serial.println(output_buffer);
    secs = currentTime.second();  // Set the counter variable
  }
}