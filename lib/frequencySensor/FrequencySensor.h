#pragma once
#include <Arduino.h>

#define MICROS_SENSOR 1
//#define TIMER_SENSOR 1
//#define CAPTURE_SENSOR 1

class FrequencySensor {
public:
    FrequencySensor() {};
    bool begin();
    double readHz();
    int8_t autoScale();
private:
    int8_t counterFrequencyIdx;
    void slowDownCounter();
    void speedUpCounter();
};