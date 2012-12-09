#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "RTClib.h"

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// Using Adafruit logging shield so setting 
// chipselect to pin 10
const int chipSelect = 10;    

// include the SD library:
#include <SD.h>

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

#define LOGGING 0
#define CACHING 1
int state = CACHING;

int const button = 0;
boolean cardPresent = false;
boolean conversionLock =  false;
long tempRequestTime=0;
boolean lastButtonState = HIGH;

String baseName="dlog_0.txt";
char fileName[13];
boolean fileselected = false;
boolean retry = true;

float cache[10];
long mcache[10];
int cacheIndex=-1;
float lastTemp=-300.0;
const float noise = 0.1;
byte degsym[8] = {0x0,0x4,0xa,0x4,0x0,0x0,0x0};
int degref = 0;
#include <LiquidCrystal.h>
LiquidCrystal lcd(4,5,6,7,8,9);
RTC_DS1307 RTC;


int displayHour = 0;
int displayMinute = 0;
int displaySecond = 0;


void setup(void)
{
  lcd.begin(16, 2);
  lcd.createChar(degref,degsym);
  pinMode(10, OUTPUT);     // change this to 53 on a mega
  digitalWrite(10,HIGH);
  
  // Button moved to anaolog 0
  pinMode(button, INPUT);


  // start serial port
  Serial.begin(9600);
  lcd.setCursor(0,1);
  lcd.print("MEM             ");
  
  
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }

  // Start up the library
  sensors.begin();
  sensors.setWaitForConversion(false);
  
  
}

void loop(void)
{ 
  // if the conversion lock is off request temperature
  if(!conversionLock){
    //Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    conversionLock = true;
    tempRequestTime = millis();
  }

  long t = millis();
  
  if(conversionLock && t-tempRequestTime>800){
    float temp = sensors.getTempCByIndex(0);
    saveData(temp);
    conversionLock = false;
    //Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(temp);
    lcd.setCursor(0,0);
    lcd.print(temp);
    lcd.write(degref);
  }

  int v = analogRead(button);
  int buttonValue = (v >128)?HIGH:LOW;
  
  if(buttonValue != lastButtonState){
    lastButtonState = buttonValue;
    onButton(buttonValue);
  }
  
  displayTime();
}

void onButton(int value){
  if(value==HIGH){
    Serial.println("Button released");
    onRelease();
  }else{
    Serial.println("Button pressed");
    onPress();
  }
}

void onPress(){
}

void onRelease(){
  Serial.print("State:");
  Serial.print(state);
  
  if(state==LOGGING){
    lcd.setCursor(0,1);
    lcd.print("MEM             ");
    Serial.println("SD released, device is caching");
    state=CACHING;
  }else{
    if(initLogger()){
      Serial.println("Start loggig to SD");
      state=LOGGING;
      lcd.setCursor(0,1);
      lcd.print("SD ");
      lcd.print(fileName);
  }else{
      Serial.println("Failed to access SD card");
    }
  }
}
boolean initLogger(){
  Serial.print("Check for SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card not present");
    cardPresent = false;
    if(retry){
      cardPresent = true;
    }else{
      return false;
    }
  }else{
    Serial.println("Card present.");
    cardPresent = true;
  }
  

  
  if(cardPresent){
    if(!fileselected){
      char tmpName[13] ;
      baseName.toCharArray(tmpName,13);
      for(int i=0;i<10;i++){
        tmpName[5]++;
        if(tmpName[5]>'9'){
          tmpName[5]='0';
        }
        Serial.print("Check ");
        Serial.println(tmpName);
        if(!SD.exists(tmpName)){
          for(int j=0;j<11;j++){
            fileName[j] = tmpName[j];
          }
          break;
        }
      }
      Serial.print("Datalog file name:");
      Serial.println(fileName);
      fileselected = true;
    }
  }
  if(fileselected){
    File logFile=SD.open(fileName,FILE_WRITE);
    if(logFile){
       if(cacheIndex>=0){
         for(int i=0;i<=cacheIndex;i++){
          logFile.print(cache[i]);
          logFile.print(";");
          logFile.println(mcache[i]);
           
          Serial.print("Persisting cache date:");
          Serial.print(i);
          Serial.print(" - ");
          Serial.print(cache[i]);
          Serial.print(",");
          Serial.print(mcache[i]);
          Serial.println(".");

         }
         logFile.close();
         cacheIndex = -1;
      }
    }else{
      return false;
    }
  }
  return true;
}

void saveData(float temp){
  if(temp>lastTemp-noise && temp < lastTemp+noise){
    Serial.println("skip");
    return ;
  }else{
    lastTemp = temp;
  }
  
  
  
  if(state == LOGGING){
    File logFile=SD.open(fileName,FILE_WRITE);
    if(logFile){
       // open the file. note that only one file can be open at a time,
      // so you have to close this one before opening another.
      // if the file is available, write to it:
      
      DateTime now = RTC.now();

      
      logFile.print(temp);
      logFile.print(";");
      logFile.println(now.unixtime());
      logFile.close();
      Serial.print("Saving to ");
      Serial.println(fileName);
    } else {
      Serial.print("error opening ");
      Serial.println(fileName);
    } 
  }else{
    cacheIndex++;
    if(cacheIndex>=10){
      Serial.println("Cache full");
      cacheIndex=9;
    }else{
      DateTime now = RTC.now();
      cache[cacheIndex]=temp;
      mcache[cacheIndex]=now.unixtime();
      lcd.setCursor(4,1);
      lcd.print(cacheIndex+1);
      lcd.print("/");
      lcd.print("10");
      Serial.print("Saving data to cache:");
      Serial.print(cacheIndex);
      Serial.print(" - ");
      Serial.print(cache[cacheIndex]);
      Serial.print(",");
      Serial.print(mcache[cacheIndex]);
      Serial.println(".");
    }
  }
}

void displayTime(){
  DateTime dt  = RTC.now();
  short currentHour = dt.hour();
  if(currentHour != displayHour){
    lcd.setCursor(8,0);
    lcd.print("00");
    lcd.setCursor(8,0);
    lcd.print(currentHour, DEC);
  }

  short currentMinute = dt.minute();
  if(currentMinute != displayMinute){
    lcd.setCursor(10,0);
    lcd.print(":00");
    if(currentMinute>9){
      lcd.setCursor(11,0);
    }else{
      lcd.setCursor(12,0);
    }
    lcd.print(currentMinute, DEC);
  }
  
  
  short currentSecond = dt.second();
  if(currentSecond != displaySecond){
    lcd.setCursor(13,0);
    lcd.print(":00");
    if(currentSecond>9){
      lcd.setCursor(14,0);
    }else{
      lcd.setCursor(15,0);
    }
    lcd.print(currentSecond, DEC);
  }
  
  displayHour = currentHour;
  displayMinute = currentMinute;
  displaySecond = currentSecond;

}  

