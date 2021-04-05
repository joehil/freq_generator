#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <CSV_Parser.h>
#include <SPI.h>
#include <SD.h>

const int chipSelect = 10;

#define KEY A0;

Adafruit_MCP4725 dac;
USBSerial ser;
unsigned int count = 0;
float freq = 10;
float d = 0;
unsigned long myTime = 0;

float calcD(float freq){
    return (7874/freq)-114.17;
}

void setup(void) {
  ser.begin(9600);
  delay(5000);

  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  dac.begin(0x60);
    
  d = calcD(freq);

  ser.print("Initializing SD card...");
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    ser.println("Card failed, or not present");
    
    // don't do anything more:
    while (1);
  }
  ser.println("card initialized.");


  CSV_Parser cp(/*format*/ "dd", /*has_header*/ true, /*delimiter*/ ';');
  cp.readSDfile("datei.csv"); // this wouldn't work if SD.begin wasn't called before

  int16_t *column_1 = (int16_t*)cp["Frequenz"];
  int16_t *column_2 = (int16_t*)cp["Dauer"];

  if (column_1 && column_2) {
    for(int row = 0; row < cp.getRowsCount(); row++) {
      ser.print("row = ");
      ser.print(row, DEC);
      ser.print(", Frequenz = ");
      ser.print(column_1[row], DEC);
      ser.print(", Dauer = ");
      ser.println(column_2[row], DEC);
    }
  } else {
    ser.println("At least 1 of the columns was not found, something went wrong.");
  }
}

void loop(void) {
    uint32_t counter;
//    dac.setVoltage(100,false);
    // Run through the full 12-bit scale for a triangle wave
    // for (counter = 0; counter < 4095; counter++)
    // {
    //   dac.setVoltage(counter, false);
    // }
    //myTime = millis();
    for (counter = 4095; counter > 33; counter-=32)
    {
        dac.setVoltage(counter, false);
        delayMicroseconds(int(d));
    }
    //myTime = millis() - myTime;
//    count++;
//    ser.println(myTime);
}
