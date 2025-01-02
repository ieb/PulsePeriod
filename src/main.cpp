#include <Arduino.h>
#include "FrequencySensor.h"


// Ok upto 31KHz then starts to fail, 
// has errors before indicating sensitivity to pulse edges.

FrequencySensor frequencySensor;

uint8_t counter = 255;
uint8_t range = 6;
double frequency = 0;
#define NRANGES 7
// change the prescaler TCCR2B with WGM22(0x08) set. 
const uint8_t ranges[] = {
    0x09, // /1
    0x0A, // /8
    0x0B, // /32
    0x0C, // 64
    0x0D, // 128
    0x0E, // 256
    0x0F, // 1024
};
const uint16_t prescale[] = {
    1,
    8,
    32,
    64,
    128,
    256,
    1024
};



void showHelp() {
  Serial.println(F("Test Frequency measurements"));
  Serial.println(F("Frequency Output is on pin MOSI/D11 "));
  Serial.println(F("Measurement pin depends on the method, see boot message "));
  Serial.println(F("  - Send 'h' to show this message"));
  Serial.println(F("  - Send 'U' increase range"));
  Serial.println(F("  - Send 'D' decrease range"));
  Serial.println(F("  - Send 'u' increase period"));
  Serial.println(F("  - Send 'd' decrease period"));
  Serial.println(F("  - Send 'm' measure frequency once"));
  Serial.println(F("  - Send 'R' to restart"));
}

void setFrequency() {
    TCCR2B = ranges[range];
    OCR2A = counter;
    frequency = 16000000.0/((double)prescale[range]*2*(1.0+(double)counter));
    double lowerFrequency = 16000000.0/((double)prescale[range]*2*255);
    double upperFrequency = 16000000.0/((double)prescale[range]*2*1);
    Serial.print("Range ");
    Serial.print(range);
    Serial.print(":");
    Serial.print(lowerFrequency);
    Serial.print(" - ");
    Serial.print(upperFrequency);
    Serial.print(" Output ");
    Serial.print(counter);
    Serial.print(":");
    Serial.println(frequency,3);
}

void(* resetDevice) (void) = 0; //declare reset function @ address 0


void checkCommand() {
  if (Serial.available()) {
    char chr = Serial.read();
    switch ( chr ) {
      case 'h': showHelp(); break;
      case 'U':
        if ( range < NRANGES-1) {
            range++;
        }
        setFrequency();
        break;
      case 'D':
        if ( range > 0) {
            range--;
        }
        setFrequency();
        break;
      case 'u':
        if ( counter < 255) {
            counter++;
        }
        setFrequency();
        break;
      case 'd':
        if ( counter > 2) {
            counter--;
        }
        setFrequency();
        break;
      case 'm':
        {
            double measuredFrequency = frequencySensor.readHz();
            double error = frequency - measuredFrequency;
            double percent = 100.0*(error/frequency);
            Serial.print("Frequency input:"); 
            Serial.print(frequency,5); 
            Serial.print("Hz measured: "); 
            Serial.print(measuredFrequency,5);
            Serial.print("Hz error: "); 
            Serial.print(error); 
            Serial.print("Hz error: "); 
            Serial.print(percent); 
            Serial.println("%"); 
        }
        break;
      case 'R':
        Serial.println(F("Restart"));
        delay(100);
        resetDevice();
        break;
    }
  }
}

void setup() {
  Serial.begin(115200);
  frequencySensor.begin();



  pinMode(11, OUTPUT);  //5 Hz with 50% DC signal will appear on PB3/MOSI/D11
  TCCR2A = 0x00;   //reset
  TCCR2B = 0x00;   //TC1 reset and OFF
  //fOC1A (DPin-9) = clckSys/(N*(1+ICR1)); Mode-14 FPWM; OCR1A controls duty cycle 
  // 5 = 16000000/(64*(1+ICR1)) ==> ICR1 = 49999
  TCCR2A = (1<<COM2A0)|(1<<WGM21)|(1<<WGM20); // Fasy PGM mode 7, toggle OC2A on compare match, so ICR2 == 50% cycle
  setFrequency();
  showHelp();

}


void loop() {
    checkCommand();
}