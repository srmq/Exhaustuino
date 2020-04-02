#include "DSTempReader.h"
#include <string.h>

DSTempReader::DSTempReader(uint8_t pin) : ds(pin) {
}

TempReadStatus DSTempReader::next(TempReading& temp) {
  if ( !ds.search(temp.addr)) {
    ds.reset_search();
    delay(250);
    temp.rawTemp = TempReading::INVALID_TEMP;
    return NO_MORE_ADDR;
  }

  if (OneWire::crc8(temp.addr, 7) != temp.addr[7]) {
      temp.rawTemp = TempReading::INVALID_TEMP;
      return INVALID_CRC8;
  }

  ds.reset();
  ds.select(temp.addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  ds.reset();
  ds.select(temp.addr);    
  ds.write(0xBE);         // Read Scratchpad
  byte data[12];
  memset(data, 0, sizeof(data));

  for (int i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  temp.rawTemp = (data[1] << 8) | data[0];
  byte typeS = temp.getTypeS();
  if (typeS == -1) {
    temp.rawTemp = TempReading::INVALID_TEMP;
    return INVALID_TYPE_S;
  }
  if (typeS) {
    temp.rawTemp = temp.rawTemp << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      temp.rawTemp = (temp.rawTemp & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) temp.rawTemp = temp.rawTemp & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) temp.rawTemp = temp.rawTemp & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) temp.rawTemp = temp.rawTemp & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

  return READ_OK;
}
