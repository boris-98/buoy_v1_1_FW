#ifndef SD_UTIL_H
#define SD_UTIL_H

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// SD module pins
#define SD_CS 32
#define SD_SCK 17
#define SD_MOSI 33
#define SD_MISO 13

extern SPIClass sd_spi; // zasebna SPI klasa za SD, kako bi se zauzeo HSPI za nju, koji LoRa ne koristi (svi uzmu VSPI po defaultu)
extern char filePath[64];

void SD_module_init();
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void createDir(fs::FS &fs, const char *path);
void removeDir(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void testFileIO(fs::FS &fs, const char *path);

#endif