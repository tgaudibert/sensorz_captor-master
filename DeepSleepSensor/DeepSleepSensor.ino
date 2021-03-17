/* 
 
*/
#include "mbedtls/md.h"
#include "Arduino.h"
#include <Wire.h>
#include <base64.h>
#include "heltec.h"
#define BAND    868E6  //you can set band here directly,e.g. 868E6,915E6


//Part for the battery
#define Fbattery    3700  //The default battery is 3700mv when the battery is fully charged.
float XS = 0.00225;      //The returned reading is multiplied by this XS to get the battery voltage.
uint16_t MUL = 1000;
uint16_t MMUL = 100;


//Part for the sleeping data
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  540        /* Time ESP32 will go to sleep (in seconds) */
RTC_DATA_ATTR int bootCount = 0;


//ChipNAME and ID
int counter = 0;
uint64_t chipid = ESP.getEfuseMac(); 
String idChip1 = String((uint16_t)(chipid>>32),HEX);
String idChip2 = String((uint32_t)chipid,HEX);
String devicename = idChip1 + idChip2;

//------------------
//PART FOR THE SD CARD
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <time.h>
SPIClass spi1;




void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case 1  :
    {
      Serial.println("Wakeup caused by external signal using RTC_IO");
      delay(2);
    } break;
    case 2  :
    {
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      delay(2);
    } break;
    case 3  :
    {
      Serial.println("Wakeup caused by timer");
      delay(2);
    } break;
    case 4  :
    {
      Serial.println("Wakeup caused by touchpad");
      delay(2);
    } break;
    case 5  :
    {
      Serial.println("Wakeup caused by ULP program");
      delay(2);
    } break;
    default :
    {
      Serial.println("Wakeup was not caused by deep sleep");
      delay(2);
    } break;
  }
}



void setup() {
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  //Increment boot number and print it every reboot
  ++bootCount;


  //part for battery
  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->clear();

  adcAttachPin(13);
  analogSetClockDiv(255); // 1338mS

  uint16_t c  =  analogRead(13)*XS*MUL;
  uint16_t d  =  (analogRead(13)*XS*MUL*MMUL)/Fbattery;
   Serial.println(analogRead(13));
   //Serial.println((String)d);
  // Serial.printf("%x",analogRead(13));
  
  //display voltage of battery
   Heltec.display->drawString(0, 0, "Remaining battery still has:");
   //Heltec.display->drawString(0, 10, "VBAT:");
   //Heltec.display->drawString(35, 10, (String)c);
   //Heltec.display->drawString(60, 10, "(mV)");

   //display porcent battery
   Heltec.display->drawString(0, 10, (String)d);
   Heltec.display->drawString(20, 10, ".");
   Heltec.display->drawString(40, 10, "%");
   Heltec.display->display();
   delay(2000);
   Heltec.display->clear();

 

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");
  delay(10);

  //SET SENSOR OUTPUT ON
  pinMode(2, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  
  digitalWrite(14, HIGH);
  digitalWrite(16, HIGH);    
  
  sendMessage((String)d);
  //hmacSHA();
  digitalWrite(16, LOW);  
  digitalWrite(14, LOW);
  
  
  digitalWrite(25, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                       // wait for a second
  digitalWrite(25, LOW);    // turn the LED off by making the voltage LOW 

  Serial.println("Going to sleep now");
  delay(2);
  esp_deep_sleep_start();

}


void loop() {

}


void sendMessage(String batteryPorcent)
{
  uint8_t sensorStateLow = digitalRead(2);
  Serial.println(sensorStateLow);

  uint8_t sensorStateHigh = digitalRead(4);
  Serial.println(sensorStateHigh);

  
  
  LoRa.beginPacket();                   // start packet
  LoRa.setTxPower(14,RF_PACONFIG_PASELECT_PABOOST);

  //String datatoSend = getSensorValue();
  //Serial.println(datatoSend);
  //LoRa.print(datatoSend);
  uint8_t filling = calculateFilling(sensorStateLow,sensorStateHigh);
  Serial.println(filling);


  LoRa.print("sn:");
  LoRa.print(devicename);
  LoRa.print(":");
  LoRa.print((String)sensorStateLow);// add payload 
  LoRa.print(":");
  LoRa.print((String)sensorStateHigh);// add payload 
  LoRa.print(":");
  LoRa.print((String)bootCount);// add payload
  LoRa.print(":");
  LoRa.print(batteryPorcent);
  LoRa.print(":");
  LoRa.print((String)filling);  
  
  LoRa.endPacket(); // increment message ID

  LoRa.end();
  LoRa.sleep();
}



//hash function
void hmacSHA(){
 
  char *key = "secretKey";
  char *payload = "Hello HMAC SHA 256!";
  byte hmacResult[32];
 
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
 
  const size_t payloadLength = strlen(payload);
  const size_t keyLength = strlen(key);            
 
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);
 
  Serial.print("Hash: ");
 
  for(int i= 0; i< sizeof(hmacResult); i++){
      char str[3];
 
      sprintf(str, "%02x", (int)hmacResult[i]);
      Serial.print(str);
  }
  Serial.println("");
}


 float calculateFilling(uint8_t sensor1,uint8_t sensor2){
   float filling = (((sensor1+sensor2+1)*100)/3);
   return filling;
}


String getSensorValue(){
  uint8_t sensorState = digitalRead(39);
  Serial.println(sensorState);
  Heltec.display->drawString(0, 0, "sensorState: " + (String)sensorState);
  Heltec.display->drawString(0, 9, (String)counter);
  Heltec.display->display();
  delay(1000);
  Heltec.display->clear();
  
  String jsonData = "{\"deviceName\":";
    jsonData += "\""+devicename+"\"";
    jsonData += ",\"isEmpty\":";
    jsonData += "\""+(String)sensorState+"\"";
    jsonData += ",\"counter\":";
    jsonData += "\""+(String)counter+"\"";
    jsonData += ",\"runningTime\":";
    jsonData += "\""+(String)millis()+"\"";
    jsonData += "}"; 

   String encoded = base64::encode(jsonData);
   return encoded;
}




 
