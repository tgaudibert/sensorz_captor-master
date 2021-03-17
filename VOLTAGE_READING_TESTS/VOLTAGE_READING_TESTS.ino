#include "Arduino.h"
#include <Wire.h>
#include "heltec.h"

//#define Vext 21
//
//#define SDA      4  //OLED SDA pin
//#define SCL     15 //OLED SCL pin
//#define RST     16 //OLED nRST pin
#define Fbattery    3700  //The default battery is 3700mv when the battery is fully charged.

float XS = 0.00225;      //The returned reading is multiplied by this XS to get the battery voltage.
uint16_t MUL = 1000;
uint16_t MMUL = 100;
//SSD1306  display(0x3c, SDA, SCL, RST);

void setup()
{
//  pinMode(Vext, OUTPUT);
//  digitalWrite(Vext, LOW);
//  delay(1000);
Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);


  delay(1000);


  adcAttachPin(13);
  analogSetClockDiv(255); // 1338mS

  Serial.begin(115200);
}

void loop()
{
   uint16_t c  =  analogRead(13)*XS*MUL;
   //uint16_t d  =  (analogRead(13)*XS*MUL*MMUL)/Fbattery;
   Serial.println(analogRead(13));
   //Serial.println((String)d);
  // Serial.printf("%x",analogRead(13));

   delay(2000);
}
