#include <LowPower.h>

#include <OneWire.h>
#include "DSTempReader.h"
#include "TempReading.h"

/**
 *   Copyright (C) 2019--2020  Sergio Queiroz <srmq@srmq.org>
 *   This file is part of Exhaustuino.
 *
 *   Exhaustuino is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Exhaustuino is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Exhaustuino.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "Globals.h"

#define IC12_COLD 6 //Cold sensor data
#define IC14_HOT 8 //Hot sensor data
#define IC5_LED 3 //status LED 
#define IC2_FANPWR 0 //Turn fan
#define IC4_FANMON 2 //Fan monitor
    
DSTempReader  dsHot(IC14_HOT);  
DSTempReader  dsCold(IC12_COLD);

bool fanOn = false;
bool fanNotResponding = false;
#define TEMP_DIFF 1.5f

#define MIN_FAN_PULSES 15

#define FAN_GRACE_ON 5*60*1000
#define FAN_GRACE_OFF 5*60*1000

struct HotColdTemps {
  float hotTemp;
  float coldTemp;
  bool hotOk;
  bool coldOk;
};

int ledBrightness = 0;
int ledFadeAmount = 5;

unsigned long fanPulseCount = 0;

void setup(void) {
  pinMode(IC12_COLD, INPUT);
  
  pinMode(IC14_HOT, INPUT);
  
  pinMode(IC5_LED, OUTPUT);
  digitalWrite(IC5_LED, LOW);
  
  pinMode(IC2_FANPWR, OUTPUT);
  digitalWrite(IC2_FANPWR, LOW);
  fanOn = false; 

  pinMode(IC14_HOT, INPUT);  

 #if DEBUG 
  Serial.begin(9600);
  Serial.println(F("Finished SETUP"));
 #endif
}

void myDelayPulsing(unsigned long ms) {
  const int delaystep = 30;
  
  const unsigned long startedAt = millis();
  
  do {
    ledBrightness += ledFadeAmount;  
        
    if(ledBrightness <= 0 || ledBrightness >= 255) {
      ledFadeAmount = -ledFadeAmount;
    } else {
      analogWrite(IC5_LED, ledBrightness);       
    }
    delay(delaystep);
  } while ((millis() - startedAt) < ms);
}

void flashPulses(const unsigned number) {
  for (unsigned n = 0; n < number; n++) {
    digitalWrite(IC5_LED, HIGH);
    delay(300);
    digitalWrite(IC5_LED, LOW);
    delay(300);
  }
}

bool tempDiffFire(const HotColdTemps &temps) {
  bool result = false;

  if (temps.coldOk && temps.hotOk) {
    float highTemp, lowTemp;
    if (temps.hotTemp > temps.coldTemp) {
      highTemp = temps.hotTemp;
      lowTemp = temps.coldTemp;
    } else {
      highTemp = temps.coldTemp;
      lowTemp = temps.hotTemp;
    }
    if ((highTemp - lowTemp) > TEMP_DIFF) {
      result = true;
    }
  }
  
  return result;
}

void readTemps(HotColdTemps &temps) {
  TempReadStatus readStatus;
  TempReading reading;
  int numRead = 0;
  temps.hotOk = false;
  temps.coldOk = false;
  while((readStatus = dsCold.next(reading)) == READ_OK) {
    ++numRead;
    temps.coldTemp = reading.getCelsius();
    
    #if DEBUG
    Serial.print(F("Cold Reading: "));
    Serial.println(numRead);
    Serial.print(F("Celsius: "));
    Serial.println(reading.getCelsius());
    Serial.print(F("Fahrenheit: "));
    Serial.println(reading.getFahrenheit());
    Serial.println("");
    #endif
  }
  if (numRead > 0 && temps.coldTemp != (float) TempReading::INVALID_TEMP) {
    temps.coldOk = true;
  }
  numRead = 0;
  while((readStatus = dsHot.next(reading)) == READ_OK) {
    ++numRead;
    temps.hotTemp = reading.getCelsius();
    
    #if DEBUG
    Serial.print(F("Hot Reading: "));
    Serial.println(numRead);
    Serial.print(F("Celsius: "));
    Serial.println(reading.getCelsius());
    Serial.print(F("Fahrenheit: "));
    Serial.println(reading.getFahrenheit());
    Serial.println("");
    #endif
  }
  if (numRead > 0 && temps.hotTemp != (float) TempReading::INVALID_TEMP) {
    temps.hotOk = true;
  }
}

void turnOnFan() {
  digitalWrite(IC2_FANPWR, HIGH);
  fanNotResponding = false;
  fanOn = true;
}

void turnOffFan() {
  digitalWrite(IC2_FANPWR, LOW);
  fanOn = false;  
}

void flashIfTempsProblem(const HotColdTemps &temps) {
  if (!temps.coldOk) {
    flashPulses(2);
    delay(1000);
  }
  if (!temps.hotOk) {
    flashPulses(3);
    delay(1000);    
  }  
}

void waitFanGraceOn(const HotColdTemps &temps) {
  const unsigned long startedAt = millis();
  while((millis() - startedAt) < FAN_GRACE_ON) {
    if(!temps.coldOk || !temps.hotOk) {
       flashIfTempsProblem(temps);
    } else {
      myDelayPulsing(300);
    }
  }
}

#if MONITOR_FAN
static void handleFanInterrupt() {
  fanPulseCount++;
}

void fanMonitor() {
  fanPulseCount = 0;  
  attachInterrupt(digitalPinToInterrupt(IC4_FANMON), handleFanInterrupt, FALLING);
  myDelayPulsing(1000);
  detachInterrupt(digitalPinToInterrupt(IC4_FANMON));

  fanNotResponding = (fanPulseCount < MIN_FAN_PULSES);

  #if DEBUG 
    Serial.print(F("Fan monitor, number of pulses read: "));
    Serial.println(fanPulseCount);
    if(fanNotResponding) {
      Serial.println(F("ERROR: Fan is not responding!"));
    } else {
      Serial.println(F("INFO: Fan is ok!"));      
    }
  #endif    
}
#else //will not monitor fan. Assumes always working
void fanMonitor() {
  #if DEBUG
    Serial.println(F("INFO: Fan is not monitored"));
  #endif
  myDelayPulsing(1000);
  fanNotResponding = false;
}
#endif

void loop(void) {
  HotColdTemps temps;

  readTemps(temps);

  flashIfTempsProblem(temps);
  
  if (temps.coldOk && temps.hotOk && tempDiffFire(temps)) {
    turnOnFan();
    const unsigned long fanOnAt = millis();    
    myDelayPulsing(2000);
    while(!fanNotResponding) {
      fanMonitor();
      if ((millis() - fanOnAt) > FAN_GRACE_OFF) {
        readTemps(temps);
        for (int tryn = 1; (!temps.coldOk || !temps.hotOk) && tryn < 10; tryn++) {
          myDelayPulsing(60);
          readTemps(temps);
        }
        if(!temps.coldOk || !temps.hotOk || !tempDiffFire(temps)) {
          turnOffFan();
          waitFanGraceOn(temps);
          break;
        } 
      }
    }
    if(fanNotResponding) {
      turnOffFan();
      for(;;) {
        flashPulses(4);
        delay(1000);
      }
    }
  } else if(temps.coldOk && temps.hotOk){
    myDelayPulsing(2000);
  }
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
}
