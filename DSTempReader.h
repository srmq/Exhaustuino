#ifndef _DSTEMP_READER_H_
#define _DSTEMP_READER_H_

#include <Arduino.h>
#include "TempReading.h"

#include <OneWire.h>

enum TempReadStatus {
  READ_OK = 0,
  INVALID_CRC8 = -1,
  NO_MORE_ADDR = -2,
  INVALID_TYPE_S = -3
};

class DSTempReader {
private:
  OneWire ds;

public:
  DSTempReader(uint8_t pin);
  TempReadStatus next(TempReading& temp);
  
};

#endif
