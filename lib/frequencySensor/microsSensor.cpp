#include "FrequencySensor.h"
#ifdef MICROS_SENSOR 

// ISR for frequency measurements.
volatile int8_t pulseCount = 0;
volatile unsigned long lastPulse=0;
volatile unsigned long thisPulse=0;


// generally at 0-31KHz this works with minimal errors
// and is simple and portable, however the time spend in the ISR is larger than
// ideal.


void measurePeriod() {
  pulseCount++;
  if ( pulseCount == 10 ) {
    lastPulse = thisPulse;
    thisPulse = micros();
    pulseCount = 0;
  }
}



bool FrequencySensor::begin() {
  Serial.println("Start Micros Sensor on D2");

  // should not be necessary to setup the ADC on either as they will both be defaulting
  // to VDD as a reference, however, where it matters the reading 
  // will need to be scaled by adcScale before use.

// PULLUP is required with the LMV393 which is open collector.

  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), measurePeriod, FALLING);
  Serial.println(F("ISR enabled"));
  return true;
}


double FrequencySensor::readHz() {
  noInterrupts();
  unsigned long lastP = lastPulse;
  unsigned long thisP = thisPulse;
  interrupts();
  unsigned long period = thisP - lastP;
  if ( (period == 0)  ) {
    return 1E9;
  } else if ((micros() - thisP) > 1000000) {
    return 0;
  } else {
    // over 10 periods, so multiply top by 10.
    return 10000000.0/((double)period);
  }
}

#endif
