#include "FrequencySensor.h"
#if (TARGET_MCU == atmega328P)
#ifdef TIMER_SENSOR


// Written for an 328p
// Uses a faster running time to measure the ticks between falling edges.
// Unlike the capture sensor, the counter is captured when a set number of pulses
// have been captured, this allows more pins to be used to capture the edges.
// the ISR is longer.  

volatile uint8_t state = 0x00;
volatile uint8_t lastState = 0x00;
volatile uint16_t lastCount = false;
volatile uint8_t pulses = 0;
volatile uint8_t npulses = 10;
volatile uint8_t pinMask = 0x00;

#define COUNTER_OVERFLOW 0x01
#define PULSE_COUNT_VALID 0x02
#define NPULSES_UPDATED 0x04

#define MIN_COUNTER_FREQUENCY  


ISR (TIMER1_OVF_vect) {
    state |= COUNTER_OVERFLOW;
}

// D2, Pin 32
ISR (INT0_vect) {
    // may have to save state and restore.
    // need this here to save the counter as close to the edge as possible.
    uint16_t captureCount = TCNT1;
    pulses--;
    if ( (state&NPULSES_UPDATED) == NPULSES_UPDATED ) {
        TCNT1 = 0;
        lastState = state;
        lastCount = 0;
        pulses = npulses;
        state = 0x00;
    } else if ( pulses == 0 ) {
        TCNT1 = 0;
        // if there was no overflow, the period is valid
        lastState = state | PULSE_COUNT_VALID;
        // save the last time period.
        lastCount = captureCount;
        // restart the counter and reset overflow
        pulses = npulses;
        state = 0x00;
    } 
}



const double counterFrequencies[] = {
    F_CPU/1024,    // F_CPU/1024 
    F_CPU/256,    // F_CPU/256   
    F_CPU/64,   // F_CPU/64   
    F_CPU/8,  // F_CPU/8   
    F_CPU // F_CPU/1 
};
// CS12 | CS11 | CS10
// 
const uint8_t clockDivisors[] = {
    0x05,
    0x04,
    0x03,
    0x02,
    0x01,
};

#define MAX_FREQUENCY 5

bool FrequencySensor::begin() {
        Serial.println("Start Timer sensor on D2");
    // reset the timer
    TCCR1A = 0;
    TCCR1B = clockDivisors[counterFrequencyIdx];
    TIMSK1 = bit (TOIE1);   // enable Timer1 Interrupt on overflow
    // zero the counter
    TCNT1 = 0;  

    // Enable falling edge on D2/32 pin.
    EICRA = 0x02;
    // enable INT0 interrupt
    EIMSK = 0x01;
    // enable interrupts
    SREG |= 0x80;
    return true;
}

double FrequencySensor::readHz() {
  noInterrupts();
  unsigned long lastRecordedCount = lastCount;
  uint8_t lastRecordedState = lastState;
  interrupts();
  if ((lastRecordedState & COUNTER_OVERFLOW) == COUNTER_OVERFLOW) {
    // overflowed unit16_t, need to divide clock scaler by 2 or decrese the puse count.
  } else if ((lastRecordedState & PULSE_COUNT_VALID) == PULSE_COUNT_VALID 
    && (lastRecordedState & NPULSES_UPDATED) == 0 
    && (lastRecordedState & COUNTER_OVERFLOW) == 0) {
    double countPerPulse = lastRecordedCount/npulses;
    double frequency = counterFrequencies[counterFrequencyIdx] / countPerPulse;
    autoScale();
    return frequency;
  }
  autoScale();
  return -1E9;
}
// first reduce the frequency of the counter
// if the frequency is already at its lowest, then start 
// to reduce the number of pulses to count.
void FrequencySensor::slowDownCounter() {
    if ( counterFrequencyIdx > 0) {
        counterFrequencyIdx--;
        TCCR1B = clockDivisors[counterFrequencyIdx]; 
        state |= NPULSES_UPDATED;
    } else if ( npulses > 1 ) {
        npulses--;
        state |= NPULSES_UPDATED;
    }
}
// first increase the number of pulses to count
// then reduce the count
void FrequencySensor::speedUpCounter() {
    if ( npulses < 10) {
        npulses++;
        state |= NPULSES_UPDATED;
    } else if ( counterFrequencyIdx < MAX_FREQUENCY ) {
        counterFrequencyIdx++;
        TCCR1B = clockDivisors[counterFrequencyIdx]; 
        state |= NPULSES_UPDATED;
    }
}

int8_t FrequencySensor::autoScale() {
  noInterrupts();
  unsigned long lastRecordedCount = lastCount;
  uint8_t lastRecordedState = lastState;
  interrupts();
  if ((lastRecordedState & COUNTER_OVERFLOW) == COUNTER_OVERFLOW) {
    slowDownCounter();
    return -1;
  } else if (lastRecordedCount > 0x7FFF) {
    slowDownCounter();
    return -1;
  } else if (lastRecordedCount < 0x3FFF) {
    speedUpCounter();
    return 1;
  }
  return 0;
}

#endif
#endif
