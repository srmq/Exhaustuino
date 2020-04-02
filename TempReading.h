#ifndef _TEMP_READING_H_
#define _TEMP_READING_H_

#include <Arduino.h>

enum DSChipType {DS18S20, DS18B20, DS1822, INVALID};


class TempReading {
  friend class DSTempReader;
private:
  byte addr[8];
  int16_t rawTemp;

  DSChipType getChipType();
  byte getTypeS();

public:
  static const int16_t INVALID_TEMP = 4096;
  TempReading();
  inline float TempReading::getCelsius() {
    if (rawTemp != INVALID_TEMP) {
      return (float)rawTemp / 16.0;
    } else {
      return (float)INVALID_TEMP;
    }
  }
  
  inline float getFahrenheit() {
    if (rawTemp != INVALID_TEMP) {
      return ((float)(rawTemp / 16.0))* 1.8f + 32.0f;;
    } else {
      return (float)INVALID_TEMP;
    }
  }
  
};

#endif
