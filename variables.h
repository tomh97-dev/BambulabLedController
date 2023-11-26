#ifndef VARIABLES_H
#define VARIABLES_H

#include <Arduino.h>

const int Max_ipLength = 15;
const int Max_accessCode = 8;
const int Max_DeviceId = 15;
const int Max_EspPassword = 8;

const int Ip_Adress = 0;
const int Accesscode_Adress = 15;
const int DeviceId_Adress = 23;
const int EspPassword_Adress = 38;
const int LedType_Address = 46;
const int LedCount_Address = 48;

enum LedState {
  IDLE,
  PREHEATING,
  PRINTING,
  ERROR,
  ERROR_RESOLVED,
  PRINT_COMPLETE,
  OFF
};

#endif