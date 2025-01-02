#if (TARGET_MCU == atmega328P)
#include <Arduino.h>
#include "FrequencySensor.h"
#ifdef CAPTURE_SENSOR


/*
 * This does not currently work as a pattern. 
 */

volatile uint8_t isrState = 0x00;
volatile uint16_t counts[16];
volatile uint8_t counterIdx = 0;
volatile uint16_t overflows = 0;

#define COUNTER_OVERFLOW 0x01

ISR (TIMER1_OVF_vect) {
    isrState |= COUNTER_OVERFLOW;
    overflows++;
}
// save the counts in a ring buffer
// Only PIN PB0 can be used, which is D8, pin 12.
ISR (TIMER1_CAPT_vect) {
    counts[(counterIdx++)&0x0F] = ICR1; 
}




const double counterFrequencies[] = {
    F_CPU/1024,    // F_CPU/1024 
    F_CPU/256,    // F_CPU/256   
    F_CPU/64,   // F_CPU/64   
    F_CPU/8,  // F_CPU/8   
    F_CPU // F_CPU/1 
};
#define MAX_FREQUENCY 5
// CS12 | CS11 | CS10
// bit 7 ICNC1 == 1 cancel noise delay of 4/16MHz to edge
// bit 6 ICES1 == 0 Falling edge
// bit 5 0 reserved
// bit 4 WGM13 0  Mode 0 waveform generation disabled.
// bit 3 WGM12 0
// CS12 = varies
// CS11 = varies
// CS10 = varies
const uint8_t controlRegisterB[] = {
    0x85,
    0x84,
    0x83,
    0x82,
    0x81
};

bool FrequencySensor::begin() {
        Serial.println("Start Capture Sensor on PB0/D8");

    // reset the timer
    TCCR1A = 0;  // normal operation, , OC1A/OC1B disconnected, no waveform generation
    TCCR1B =  controlRegisterB[counterFrequencyIdx]; 
    TIMSK1 = bit(ICIE1) | bit (TOIE1);   // enable Timer1 Interrupt on overflow and interput on capture
    // zero the counter
    TCNT1 = 0;  
    return true;
 }
 

 // first reduce the frequency of the counter
// if the frequency is already at its lowest, then start 
// to reduce the number of pulses to count.
void FrequencySensor::slowDownCounter() {
    if ( counterFrequencyIdx > 0) {
        counterFrequencyIdx--;
        TCCR1B = controlRegisterB[counterFrequencyIdx]; 
        // clear the overflow state and 
        for(int i = 0; i < 16; i++) {
            counts[i] = 0xFFFE;
        }
        Serial.print("Frequency Down now :");
        Serial.print(counterFrequencyIdx);
        Serial.print(" ");
        Serial.println(counterFrequencies[counterFrequencyIdx]);
    }
    isrState &= ~COUNTER_OVERFLOW;
}
// first increase the number of pulses to count
// then reduce the count
void FrequencySensor::speedUpCounter() {
    if ( counterFrequencyIdx < MAX_FREQUENCY ) {
        counterFrequencyIdx++;
        TCCR1B = controlRegisterB[counterFrequencyIdx]; 
        for(int i = 0; i < 16; i++) {
            counts[i] = 0xFFFE;
        }
        Serial.println("Frequency Up now ");
        Serial.print(counterFrequencyIdx);
        Serial.print(" ");
        Serial.println(counterFrequencies[counterFrequencyIdx]);
    }
    isrState &= ~COUNTER_OVERFLOW;
}

int8_t FrequencySensor::autoScale() {
  if ((isrState&COUNTER_OVERFLOW) == COUNTER_OVERFLOW) {
    // counter has overflowed, cant produce reading until the timer is scaled suitably
    slowDownCounter();
    return -1;
  }
  uint16_t capturedCounts[16];
  noInterrupts();
  for(int i = 0; i < 0x0F; i++) {
    capturedCounts[i] = counts[i]; 
  }
  interrupts();
  // Values 0xFFFE have not been written and need to be ignored.
  uint32_t accumulate = 0;
  uint8_t samples = 0;
  for(int i = 0; i < 0x0F; i++) {
    if ( capturedCounts[i] != 0xFFFE ) {
        accumulate += capturedCounts[i]; 
        samples++;
    }
  }
  if ( samples < 8) {
    return 0;
  }
  uint32_t mean = accumulate/samples;
  if ( mean < 0x3FFF) {
    speedUpCounter();
    return 1;
  } else if ( mean > 0x7FFF ) {
    slowDownCounter();
    return -1;
  }
  return 0;

}





double FrequencySensor::readHz() {
  if ((isrState&COUNTER_OVERFLOW) == COUNTER_OVERFLOW) {
    // counter has overflowed, cant produce reading until the timer is scaled suitably
    Serial.print("Overflow ");
    Serial.println(overflows);
    slowDownCounter();
    return -1E9;
  }
  uint16_t capturedCounts[16];
  noInterrupts();
  for(int i = 0; i < 0x0F; i++) {
    capturedCounts[i] = counts[i]; 
  }
  interrupts();
  // Values 0xFFFE have not been written and need to be ignored.
  uint32_t accumulate = 0;
  uint8_t samples = 0;
  for(int i = 0; i < 0x0F; i++) {
    if ( capturedCounts[i] != 0xFFFE ) {
        accumulate += capturedCounts[i]; 
        samples++;
    }
  }
  if ( samples < 8) {
    // not enough samples yet
    return -1E9;
  }
  uint32_t mean = accumulate/samples;
  Serial.print("Mean ");
  Serial.println(mean);
  if ( mean < 0x3FFF) {
    speedUpCounter();
  } else if ( mean > 0x7FFF ) {
    slowDownCounter();
  }
  return (counterFrequencies[counterFrequencyIdx]*samples) / accumulate;
}


#endif
#endif



