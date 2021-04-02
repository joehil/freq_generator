#include <Wire.h>
#include <Adafruit_MCP4725.h>
#define KEY A0;

Adafruit_MCP4725 dac;
USBSerial ser;
unsigned int count = 0;
float freq = 10;
float d = 0;
unsigned long myTime = 0;

void setup(void) {
  ser.begin(9600);
  ser.println("Hello!");

  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  dac.begin(0x60);
    
  d = (6493/freq)-113.6;
}

void loop(void) {
    uint32_t counter;
//    dac.setVoltage(100,false);
    // Run through the full 12-bit scale for a triangle wave
    // for (counter = 0; counter < 4095; counter++)
    // {
    //   dac.setVoltage(counter, false);
    // }
    myTime = millis();
    for (counter = 4095; counter > 1023; counter-=20)
    {
        dac.setVoltage(counter, false);
        delayMicroseconds(int(d));
    }
    myTime = millis() - myTime;
    count++;
    ser.println(myTime);
}
