//ADC_MODE(ADC_VCC); //vcc read
#include <driver/adc.h>
#include "Arduino.h"
#include "EEPROM.h"

#include <base64.h>
#include <LoRa.h> //https://github.com/sandeepmistry/arduino-LoRa
#include "mbedtls/md.h"

#define SCK 5 // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18 // GPIO18 - SX1278's CS
#define RST 14 // GPIO14 - SX1278's RESET
#define DI0 26 // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 868E6 // 915E6


bool isAppairing = false;
bool isBlocked = false;

//ChipNAME and ID
uint64_t chipid = ESP.getEfuseMac(); 
String idChip1 = String((uint16_t)(chipid>>32),HEX);
String idChip2 = String((uint32_t)chipid,HEX);
String device_id = idChip1 + idChip2;

const int LED_SWITCH = 17; 
const int ALIM_SWITCH = 12; 
const int SWITCH = 13; 
const byte TRIGGER_PIN = 2; // Broche TRIGGER
const byte ECHO_PIN = 4;    // Broche ECHO
const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s

/* Vitesse du son dans l'air en mm/us */
const float SOUND_SPEED = 340.0 / 1000;

const unsigned long UpdateInterval = 0.25 * (60L * 1000000L); // Update delay in microseconds, currently 15-secs (1/4 of minute)

unsigned long Time_Echo_us = 0; 
unsigned long Len_mm  = 0;  


int shared_secret_location = 1000;
String shared_secret = "";
char *shared_secret_char; 
   


void onReceive(int packetSize){
  if (packetSize == 0) return;          // if there's no packet, return
  String incoming = "";

  while (LoRa.available()){
    incoming += (char)LoRa.read();
  }

  try{
      String packetHeader= splitString(incoming,".",1);
      String lastPacket = splitStringPT2(incoming,".",1);
      String packetPayload = splitString(lastPacket,".",1);
      Serial.println(packetPayload);
      Serial.println(lastPacket);
      Serial.println(packetHeader);
      Serial.println(incoming);
      if(packetHeader == "pvkey"){
        Serial.println("received key");
        shared_secret = packetHeader +"."+ packetPayload;
        Serial.println(shared_secret.length());
        EEPROM.writeString(shared_secret_location, shared_secret);
        EEPROM.commit();
        
        int charLength = shared_secret.length() + 1;
        char charBuf[charLength];
        strcpy(charBuf, shared_secret.c_str());
        free(shared_secret_char);
        shared_secret_char = (char *)charBuf;
        Serial.println(shared_secret_char);
      }
      return;
 
      
    }catch(...){
      Serial.println("error");
      return;
    }

}

void setup() {
  Serial.begin(115200);
  Serial.println("LoRa Sender starting...");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (! LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  if (!EEPROM.begin(4096)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW); // La broche TRIGGER doit être à LOW au repos
  pinMode(ECHO_PIN, INPUT);
  pinMode(SWITCH, INPUT);
  pinMode(ALIM_SWITCH, OUTPUT);
  pinMode(LED_SWITCH, OUTPUT);

  digitalWrite(ALIM_SWITCH, HIGH);
  readPinState();
  if(isAppairing){
    clearEeprom();
    digitalWrite(LED_SWITCH, HIGH);
    while(isAppairing){
      readPinState();
      onReceive(LoRa.parsePacket());
    }
  }else{
    digitalWrite(LED_SWITCH, LOW);
    digitalWrite(ALIM_SWITCH, LOW);
    Serial.println("Sending data packet...");
    Send_and_Display_Sensor_Data();
  }
  //start_sleep();  
}



void start_sleep(){
  esp_sleep_enable_timer_wakeup(UpdateInterval);
 
  Serial.println("Starting deep-sleep period... awake for "+String(millis())+"mS");
  delay(8); // Enough time for the serial port to finish at 115,200 baud
  esp_deep_sleep_start();         // Sleep for the prescribed time
}



void loop() {
  //Send_and_Display_Sensor_Data();
  delay(3000);
  Send_and_Display_Sensor_Data();
}


void readPinState(){
  uint8_t pinState = digitalRead(SWITCH);
  //Serial.println(pinState);
  if(pinState == 1){
    isAppairing = true;
  }else{
    isAppairing = false;
  }
}



void Send_and_Display_Sensor_Data(){
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  long measure = pulseIn(ECHO_PIN, HIGH, MEASURE_TIMEOUT);
  float distance_mm = measure / 2.0 * SOUND_SPEED;
   
  Serial.print(F("Distance: "));
  Serial.println(distance_mm);

  digitalWrite(TRIGGER_PIN, HIGH);                         // Send pulses begin by Trig / Pin
   delayMicroseconds(50);                               // Set the pulse width of 50us (> 10us)
   digitalWrite(TRIGGER_PIN, LOW);                          // The end of the pulse    
   Time_Echo_us = pulseIn(ECHO_PIN, HIGH);               // A pulse width calculating US-100 returned
   if((Time_Echo_us < 60000) && (Time_Echo_us > 1)) {   // Pulse effective range (1, 60000).
    // Len_mm = (Time_Echo_us * 0.34mm/us) / 2 (mm) 
    Len_mm = (Time_Echo_us*34/100)/2;                   // Calculating the distance by a pulse width.   
    Serial.print("Present Distance is: ");              // Output to the serial port monitor 
    Serial.print(Len_mm, DEC);                          // Output to the serial port monitor   
    Serial.println("mm");                               // Output to the serial port monitor
   }  
   delay(1000);                                         // Per second (1000ms) measured

  float voltage = (float)analogRead(39)/4096*4.2;
  Serial.println((String)voltage);
  
  LoRa.beginPacket();// Start LoRa transceiver
  
  String device_name = "fsefazfqzf";
  int battery_level = 32;
  String toSend = "{\"distance\":";
    toSend += "\""+(String)distance_mm+"\"";
    toSend += ",\"device_id\":";
    toSend += "\""+device_id+"\"";
    toSend += ",\"voltage\":";
    toSend += "\""+(String)voltage+"\"";
    toSend += ",\"device_name\":";
    toSend += "\""+device_name+"\"";
    toSend += "}"; 

  String encodedPayload = base64::encode(toSend);
  char charPayload[encodedPayload.length() + 1]; 

  strcpy(charPayload, encodedPayload.c_str()); 

  shared_secret = EEPROM.readString(shared_secret_location);
  Serial.println((String)shared_secret.length());
  Serial.println(shared_secret);
  
  int charLength = shared_secret.length() + 1;
  char charBuf[charLength];
  strcpy(charBuf, shared_secret.c_str());
  shared_secret_char = (char *)charBuf;
  Serial.println(shared_secret_char);

  String signature = hmacSHA(shared_secret_char, charPayload);
  
  String encodedSignature = base64::encode(signature);
  
  LoRa.beginPacket();
  LoRa.print(encodedPayload +"."+ encodedSignature);  // Send BME280 pressure reading, addition of 3.7 is for calibration purposes
  LoRa.endPacket();                                                  // Confirm end of LoRa data packet
  LoRa.sleep();                                                      // Send LoRa transceiver to sleep

}





String hmacSHA(char *key, char *payload){
 
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
  String hashContent = "";
  
  for(int i= 0; i< sizeof(hmacResult); i++){
      char str[3];
    
      sprintf(str, "%02x", (int)hmacResult[i]);
      hashContent += str;
  }
  Serial.println(hashContent);
  return hashContent;
}



// ----------------------------------
//PART SLICING
// ----------------------------------


//RETURN FIRST PART OF SEPARATION
String splitString(String input,String separator,int separatorSize){

  // Declare the variables of the parts of the String
  String value1, value2;
   
  for (int i = 0; i < input.length() - separatorSize; i++) {
    if (input.substring(i, i + separatorSize) == separator) {
      value1 = input.substring(0, i);
      value2= input.substring(i + separatorSize);
      break;
    }
  }
  return value1;
}

//RETURN LAST PART OF SEPARATION
String splitStringPT2(String input,String separator,int separatorSize){

  // Declare the variables of the parts of the String
  String value1, value2;

  for (int i = 0; i < input.length() - separatorSize; i++) {
    if (input.substring(i, i + separatorSize) == separator) {
      value1 = input.substring(0, i);
      value2= input.substring(i + separatorSize);
      break;
    }
  }
  return value2;
}


void clearEeprom(){
  for (int i = 0; i < 1000; ++i) {
    EEPROM.write(i, -1);
  }
  EEPROM.commit();
  return;
}
