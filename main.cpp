#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MCP4725.h>
#include <CSV_Parser.h>
#include <SPI.h>
#include <SD.h>

const int chipSelect = A3;

#define KEY A0;

Adafruit_MCP4725 dac;
USBSerial ser;

Sd2Card card;
SdVolume volume;
SdFile root;
CSV_Parser cp(/*format*/ "ddd", /*has_header*/ true, /*delimiter*/ ';');
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

unsigned int count = 0;
int rows = 0;
unsigned int gesamt = 0;
int16_t freq = 10;
float d = 0;
unsigned long myTime = 0;
unsigned int wrkfl = 0;
String keypressed = "";

const String menue1ist1[] = { "Datei auswaehlen    ", "Magnetfeld messen   ",  "Ende                "};

int16_t *art;
int16_t *frequenz;
int16_t *dauer;

float calcD(int16_t freq){
    return (7874/freq)-114.17;
}

void readKey(void){
  if (digitalRead(PB12)==0){
    keypressed = "up";
  }
  else if (digitalRead(PB13)==0){
    keypressed = "ok";
  }
  else if (digitalRead(PB14)==0){
    keypressed = "down";
  } 
  else if (digitalRead(PB15)==0){
    keypressed = "back";
  } 
}

void menue1(void){
  unsigned int n=0;
  unsigned int j=1;
  String entry;
  lcd.clear();
  lcd.print("Bitte auswaehlen:");
  lcd.blink_on();
  keypressed = "";
  while (keypressed == ""){
    for (int m=0; m<3;m++){
      entry=menue1ist1[m];
      lcd.setCursor(0,m+1);
      lcd.print(entry);
    }
    lcd.setCursor(0,j);
    readKey();
    if (keypressed == "down"){
      j++;
      keypressed = "";
    }
    if (keypressed == "up"){
      j--;
      keypressed = "";
    }
    if (j>3) j=1;
    if (j<1) j=3;
    delay(500);
  }
}

void menue2(void){
  
}

void readFile(char *datei){
//  CSV_Parser cp(/*format*/ "cdd", /*has_header*/ true, /*delimiter*/ ';');
  cp.readSDfile(datei); // this wouldn't work if SD.begin wasn't called before

  art =   (int16_t*)cp["Art"];
  frequenz = (int16_t*)cp["Frequenz"];
  dauer = (int16_t*)cp["Dauer"];

  if (art && frequenz && dauer) {
    for(int row = 0; row < cp.getRowsCount(); row++) {
      ser.print("Zeile = ");
      ser.print(row, DEC);
      ser.print(", Art = ");
      ser.print(art[row], DEC);
      ser.print(", Frequenz = ");
      ser.print(frequenz[row], DEC);
      ser.print(", Dauer = ");
      ser.println(dauer[row], DEC);
      gesamt += dauer[row];
    }
  } else {
    ser.println("Die Tabelle ist nicht in Ordnung.");
  }
  rows = cp.getRowsCount()-1;
  SD.end();
//  digitalWrite(chipSelect, HIGH);
}

void saw(unsigned secs, int16_t freq) {
    uint32_t counter;
    d = calcD(freq);
    myTime = millis();
    while ((millis()-myTime)/1000 < secs && keypressed != "back"){
      for (counter = 4095; counter > 33; counter-=32)
      {
        dac.setVoltage(counter, false);
        delayMicroseconds(int(d));
        readKey();
      }
    }
}

void square(unsigned int secs, int16_t freq) {
    d = 500000/freq;
    myTime = millis();
    while ((millis()-myTime)/1000 < secs && keypressed != "back"){
        dac.setVoltage(4095, false);
        delayMicroseconds(int(d));
        dac.setVoltage(0, false);
        delayMicroseconds(int(d));
        readKey();
    }
}

void setup(void) {
  ser.begin(9600);

  pinMode(PB12, INPUT_PULLUP);
  pinMode(PB13, INPUT_PULLUP);
  pinMode(PB14, INPUT_PULLUP);
  pinMode(PB15, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Starten ...");

  delay(2000);

  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  dac.begin(0x60);
    
  ser.print("Einrichten der SD-Karte...");
  lcd.setCursor(0,1);
  lcd.println("Starten der SD-Karte ...");

  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect,LOW);

  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);

  delay(10000);

  if (!card.init(SPI_FULL_SPEED, chipSelect)) {
    ser.println("initialization failed. Things to check:");
    ser.println("* is a card inserted?");
    ser.println("* is your wiring correct?");
    ser.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } else {
    ser.println("Anschluss der SD-Karte ist in Ordnung.");
  }  

  ser.flush();
  delay(500);

  digitalWrite(chipSelect,LOW);
  pinMode(PC13, OUTPUT);
  digitalWrite(PC13,LOW);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    ser.println("Card failed, or not present");
    ser.flush();
    while (1){
      delay(5000);
    }
  }
  ser.println("card initialized.");
  lcd.clear();
  lcd.print("SD-Karte ok ...");

//  SD.end();
//  digitalWrite(chipSelect, HIGH);
  digitalWrite(PC13,HIGH);
  lcd.setCursor(0,1);
  lcd.print("SD-Karte gelesen");
  wrkfl = 0;
  keypressed = "ok";
  delay(2000);
  lcd.clear();
}

void loop(void) {
  ser.println(keypressed);
  ser.println(wrkfl);
  if ((wrkfl == 0) & (keypressed == "ok")){
    menue1();
    lcd.setCursor(0,0);
    lcd.print("Menue 0");
    wrkfl = 1;
    keypressed = "";
  }
  if ((wrkfl == 1) & (keypressed == "ok")){
    menue2();
    lcd.setCursor(0,0);
    lcd.print("Menue 1");
    wrkfl = 2;
    keypressed = "";
  }
  if ((wrkfl == 2) & (keypressed == "ok")){
    readFile("datei.csv");
    wrkfl = 3;
    keypressed = "";
  }
  if (wrkfl == 3){
    keypressed = "";
    for(int row = 0; row < rows; row++) {
      ser.print("Zeile = ");
      ser.print(row, DEC);
      ser.print(", Art = ");
      ser.print(art[row], DEC);
      ser.print(", Frequenz = ");
      ser.print(frequenz[row], DEC);
      ser.print(", Dauer = ");
      ser.println(dauer[row], DEC);
      lcd.clear();
      if (art[row] == 0){
        lcd.setCursor(0,0);
        lcd.printf("Saegezahn %d/%d",row+1,rows+1);
        lcd.setCursor(0,1);
        lcd.printf("F:%d  D:%d/%d",frequenz[row],dauer[row],gesamt);
        saw(dauer[row],frequenz[row]);
      }
      if (art[row] == 1){
        lcd.setCursor(0,0);
        lcd.printf("Rechteck %d/%d",row+1,rows+1);
        lcd.setCursor(0,1);
        lcd.printf("F:%d  D:%d/%d",frequenz[row],dauer[row],gesamt);
        square(dauer[row],frequenz[row]);
      }
      if (keypressed == "back"){
        wrkfl = 0;
        keypressed = "";
      }
    }
    while (keypressed != "back" && wrkfl == 3) {
      ser.println("Ende");
      lcd.clear();
      lcd.print("Ende ...");
      delay(1000);
      readKey();
      if (keypressed == "back"){
        wrkfl = 0;
        keypressed = "";
      }
    }
  }
  delay(1000);
  keypressed = "";
  readKey();
}
