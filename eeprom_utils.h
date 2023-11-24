#ifndef EEPROM_UTILS_H
#define EEPROM_UTILS_H

#include <Arduino.h>

void readFromEEPROM(char* Printerip, char* Printercode, char* PrinterID, char* EspPassword, int* LedType);

void writeToEEPROM(char* Printerip, char* Printercode, char* PrinterID, char* EspPassword, int* LedType);

void clearEEPROM();

#endif // EEPROM_UTILS_H