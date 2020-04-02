#include "TempReading.h"
#include <string.h>

TempReading::TempReading() : rawTemp(INVALID_TEMP) {
  memset(this->addr, 0, sizeof(this->addr));
}

DSChipType TempReading::getChipType() {
  switch (this->addr[0]) {
    case 0x10:
      return DS18S20;
    case 0x28:
      return DS18B20;
    case 0x22:
      return DS1822;
    default:
      return INVALID;
  }
}

byte TempReading::getTypeS() {
  switch(this->getChipType()) {
    case DS18S20:
      return 1;
    case DS18B20:
    case DS1822:
      return 0;
    default:
      return -1;
  }
}
