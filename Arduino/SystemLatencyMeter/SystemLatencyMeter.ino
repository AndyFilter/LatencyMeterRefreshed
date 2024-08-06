#define EMULATE_MOUSE // Works only on supported devices (with UART), for example Pro Micro

#ifdef EMULATE_MOUSE
#include "HID-Project.h"
#endif

const int LDR = A0;
const int button = 2;

#define THRESHOLD_RELATIVE 1.02f  // the sensor value has to be 2% higher than the base value to count as change
#define THRESHOLD_ABSOLUTE 3

int LDRValue = 0;
int LastLDR = 0;
int AudioValue = 0;
unsigned long Started = 0;
bool isWaiting = 0;
int buttonState = 0; 
int LDRprobeTime = 0;

/* --------- Dynamic Parameters --------- */
bool useMillis = false; // Milli or Micro seconds
bool reverseInput = false;
float thresholdMultip = 1.f;

void setup() {
  Serial.begin(19200);
  pinMode(button, INPUT_PULLUP);
  pinMode(LDR, INPUT);

  // Setup the registers (set the maximum precision)
  // TCCR1A = 0;
  // TCCR1B = 1 << CS10;
  // TCNT1 = 0;

#ifdef EMULATE_MOUSE
  Mouse.begin();
#endif

  delay(100);
  while(Serial.available() > 0)
  {
    Serial.read();
  }
}

const auto timingFunc = micros;

void loop() {
    LDRValue = reverseInput ? (1024 - analogRead(LDR)) : analogRead(LDR); // You might need to change it to LDRValue = 1024 - analogRead(LDR), if using some odd sensor
  #ifdef ARDUINO_AVR_UNO
    buttonState = (PIND >> button & B00000100 >> button); // B00000100, the button is connected at pin 2, so we want to mask only the bit 2 (third one because we start from 0) (This is just a fancy way of writing digitalRead(button))
  #else
    buttonState = digitalRead(button);
  #endif
    
    // 375203 is used, because it's a prime. It's just 3.7 seconds
    if(buttonState == LOW && (timingFunc() - Started > 375203)) 
    {
      Started = timingFunc();
      Serial.print('l');
      isWaiting = 1;
#ifdef EMULATE_MOUSE
      Mouse.click();
#endif
    }

    // Light Detection
    if(isWaiting && (LDRValue > LastLDR*THRESHOLD_RELATIVE) && LDRValue > LastLDR+THRESHOLD_ABSOLUTE) 
    {
      Serial.print((timingFunc() - Started) / (useMillis ? 1000 : 1));
      Serial.print(useMillis ? 'm' : 'u');
      isWaiting = 0;
      LDRprobeTime = millis();
    }
    
    // Input latency check (Ping)
    if (!isWaiting && Serial.available() > 0)
    {
      const char incoming = Serial.read();
      if(incoming == 'p')
        Serial.print('b');
      //Serial.print(incoming);
      LastLDR = LDRValue;
    }

    // Set new base sensor Value every 100ms
    if(!isWaiting && millis() - LDRprobeTime > 100 && !isWaiting)
    {
      LastLDR = LDRValue;
      LDRprobeTime = millis();
    }
}
