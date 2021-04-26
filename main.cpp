#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_MCP4725.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <CSV_Parser.h>
#include <SPI.h>
#include <SD.h>

const int chipSelect = A3;

#define KEY A0;

Adafruit_MCP4725 dac;
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
USBSerial ser;

File jhroot;

Sd2Card card;
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

const String menue1ist1[] = { "Datei auswaehlen    ", 
                              "Magnetfeld messen   ",  
                              "Ende                "};

const unsigned int wrkfllist1[] = {1,90,99};

String entries[100];
unsigned int entr_len=0;
String datei;

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
    if (keypressed == "ok"){
      wrkfl = wrkfllist1[j-1];
    }
    delay(500);
  }
}

void menue2(void){
  unsigned int n=0;
  unsigned int j=1;
  String entry;
  lcd.clear();
  lcd.print("Datei auswaehlen    ");
  lcd.blink_on();
  keypressed = "";
  while (keypressed == ""){
    for (int m=0; m<3;m++){
      entry=entries[m+n];
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
    if (j>3) {
      ser.print("n: ");
      ser.println(n+3);
      ser.print("len: ");
      ser.println(entr_len);
      if (n+3 < entr_len) {
        n++;
        j=3;
      }
      else j=1;
    }
    if (j<1) j=3;
    if (keypressed == "ok"){
      datei = entries[j-1+n];
      wrkfl = 2;
    }
    delay(500);
  }
}

void displaySensorDetails(void)
{
  sensor_t sensor;
  sensors_event_t event; 
  float magval=0;
  float offset=0;
  float val=0;

  if(!mag.begin())
  {
    /* There was a problem detecting the HMC5883 ... check your connections */
    ser.println("Ooops, no HMC5883 detected ... Check your wiring!");
    while(1);
  }

  mag.getSensor(&sensor);
  ser.println("------------------------------------");
  ser.print  ("Sensor:       "); Serial.println(sensor.name);
  ser.print  ("Driver Ver:   "); Serial.println(sensor.version);
  ser.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  ser.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" uT");
  ser.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" uT");
  ser.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" uT");  
  ser.println("------------------------------------");
  ser.println("");
  delay(500);

  keypressed = "";

  while(keypressed != "ok"){
    mag.getEvent(&event);
    magval = event.magnetic.z;

    val=magval-offset;
    if (abs(val)<1) val=0;
 
    ser.println(val);
    lcd.clear();
    lcd.print(val);

    readKey(); 
    if (keypressed == "up"){
      keypressed = "";
      offset=magval;
    }
    if (keypressed == "down"){
      dac.setVoltage(4095, false);
      keypressed = "";
    }
    delay(500);
  }
}

void magnetfeld(void){
  displaySensorDetails();
  wrkfl = 0;
}

bool isCsv(String filename) {
  int8_t len = filename.length();
  bool result;
  if (filename.endsWith(".csv") 
   || filename.endsWith(".CSV")
  ) {
    result = true;
  } else {
    result = false;
  }
  return result;
}

void printDirectory(File dir, int numTabs) {
  entr_len=0;
  while (true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      ser.print('\t');
    }
    ser.print(entry.name());
    if (entry.isDirectory()) {
      ser.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      ser.print("\t\t");
      ser.println(entry.size(), DEC);
      if (isCsv(entry.name())){
        entries[entr_len]=entry.name();
        entr_len++;
      }
    }
    entry.close();
  }
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
  lcd.print("Starten der SD-Karte");

  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect,LOW);

  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);

  delay(10000);

  if (!card.init(SPI_FULL_SPEED, chipSelect)) {
    lcd.clear();
    lcd.print(card.errorCode());
    lcd.setCursor(0,2);
    lcd.print("Fehler beim Starten");
    lcd.setCursor(0,3);
    lcd.print("der SD-Karte");
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

  jhroot = SD.open("/");
  printDirectory(jhroot, 0);

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
  }
  if ((wrkfl == 1) & (keypressed == "ok")){
    delay(2000);
    menue2();
  }
  if ((wrkfl == 2) & (keypressed == "ok")){
    char *tdatei;
    char datei_ar[datei.length()+1];
    datei.toCharArray(datei_ar, datei.length()+1);
    tdatei = &datei_ar[0];
    ser.println(tdatei);
    readFile(tdatei);
    wrkfl = 3;
    keypressed = "";
  }
  if (wrkfl == 3){
    unsigned int summe=0;
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
        summe += dauer[row];
        lcd.setCursor(0,0);
        lcd.printf("Saegezahn %d/%d",row+1,rows+1);
        lcd.setCursor(0,1);
        lcd.printf("F:%d  D:%d/%d/%d",frequenz[row],dauer[row],summe,gesamt);
        saw(dauer[row],frequenz[row]);
      }
      if (art[row] == 1){
        summe += dauer[row];
        lcd.setCursor(0,0);
        lcd.printf("Rechteck %d/%d",row+1,rows+1);
        lcd.setCursor(0,1);
        lcd.printf("F:%d  D:%d/%d/%d",frequenz[row],dauer[row],summe,gesamt);
        square(dauer[row],frequenz[row]);
      }
      if (keypressed == "back"){
        wrkfl = 0;
        keypressed = "";
      }
      wrkfl = 99;
    }
  }
  if ((wrkfl == 90) & (keypressed == "ok")){
    delay(2000);
    magnetfeld();
  }
  if (wrkfl == 99){
    lcd.clear();
    lcd.print("Ende ...");
    while(1);
  }

  delay(500);
  keypressed = "";
  readKey();
}
