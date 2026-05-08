#ifndef E220_H
#define E220_H

#include <Arduino.h>

void setupE220();
bool receiveData(char* buffer, size_t len);

#endif