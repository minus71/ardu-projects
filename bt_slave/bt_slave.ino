/*
  Serial Event example
 
 When new serial data arrives, this sketch adds it to a String.
 When a newline is received, the loop prints the string and 
 clears it.
 
 A good test for this is to try it with a GPS receiver 
 that sends out NMEA 0183 sentences. 
 
 Created 9 May 2011
 by Tom Igoe
 
 This example code is in the public domain.
 
 http://www.arduino.cc/en/Tutorial/SerialEvent
 
 */
#define LED_PIN  8
#define MAX_LENGTH 200
#define LED_OFF  0
#define LED_ON  1
#define LED_BLINK 2

String inputString = "";         // a string to hold incoming data
boolean flip = false;
int mode = LED_OFF;

long lastChange = 0;
long blinkOn = 200;
long blinkOff = 1000;
boolean ledOn = false;

void setup() {
  // initialize serial:
  Serial.begin(9600);
  // reserve 200 bytes for the inputString:
  pinMode(LED_PIN,OUTPUT);
  inputString.reserve(MAX_LENGTH);
}

void loop() {
  if(mode==LED_ON){
    digitalWrite(LED_PIN,HIGH);
  }else if(mode==LED_OFF){
    digitalWrite(LED_PIN,LOW);
  }if(mode==LED_BLINK){
    long t = millis();
    if(ledOn){
      if(t-lastChange>blinkOn){
        Serial.println('-');
        digitalWrite(LED_PIN,LOW);
        ledOn=false;
        lastChange = t;
      }
    }else{
      if(t-lastChange>blinkOff){
        Serial.println('+');
        digitalWrite(LED_PIN,HIGH);
        ledOn=true;
        lastChange = t;
      }
    }
  }
}

void checkCommand(){
  if(inputString.equals("ON")){
    mode = LED_ON;
  }else if(inputString.equals("OFF")){
    mode = LED_OFF;
  }else if(inputString.equals("BLK")){
    mode = LED_BLINK;
  }
  Serial.println("["+inputString+"]");
  inputString = "";
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    if (inChar == '\n') {
      Serial.print(inputString+":");
      Serial.println(inputString.length());
      checkCommand();
      Serial.print("Mode:");
      Serial.println(mode);

    }else{
    if(inputString.length()<MAX_LENGTH){
      // add it to the inputString:
      inputString += inChar;
    }
    }
  }
}


