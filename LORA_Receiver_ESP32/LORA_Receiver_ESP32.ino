
//#include "SSD1306Wire.h"
#include <LoRa.h> //https://github.com/sandeepmistry/arduino-LoRa
#include "mbedtls/md.h"
extern "C" {
  #include "crypto/base64.h"
}
#include <base64.h>

#define SCK 5 // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18 // GPIO18 - SX1278's CS
#define RST 14 // GPIO14 - SX1278's RESET
#define DI0 26 // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 868E6 // 915E6

//SSD1306Wire display (0x3c, 4, 15);
String rssi = "RSSI ";
String packSize = "-";
String packet;

void setup () {
  pinMode(2, OUTPUT);
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay (50);
  digitalWrite(16, HIGH); // while OLED is running, GPIO16 must go high,
  
  Serial.begin(115200);
  Serial.println("LoRa Receiver Callback");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (! LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.receive();
  Serial.println("init ok");
  delay (1500);
}

void loop () {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {    // received a packet
    Serial.print("Received packet '");
    while (LoRa.available()) {    // read packet
      packet = LoRa.readString();
      //Serial.print(packet);
    }
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());   // print RSSI of packet

 
    Serial.println(packet);
    try{
      String packetPayload = splitStringPT2(packet,".",1);
      char charPayload[packetPayload.length() + 1]; 
      strcpy(charPayload, packetPayload.c_str()); 
  
      String packetSignature = splitStringPT1(packet,".",1);
      char charSignature[packetSignature.length() + 1]; 
      strcpy(charSignature, packetSignature.c_str()); 
      
      char *key = "privatekey";
      String calculatedSignature = hmacSHA(key, charPayload);
      Serial.println(calculatedSignature);
      if(calculatedSignature == packetSignature){
        Serial.println("verified Request");
      }else{
        Serial.println("Bad Request");
      }
      
      //String calculatedSignature = hmacSHA(key, payload);
      //Serial.println(calculatedSignature);
      //if(calculatedSignature == packetSignature){
      //  Serial.println("valid Signature");
      //}
    }catch(...){
      Serial.println("error");
    }
    packet = "";

    digitalWrite(2, HIGH); // turn the LED on (HIGH is the voltage level)
    delay (500); // wait for a second
    digitalWrite(2, LOW); // turn the LED off by making the voltage LOW
    delay (500); // wait for a second
  }
}



//RETURN LAST PART OF SEPARATION
String splitStringPT1(String input,String separator,int separatorSize){

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
  return value1;
}



// ----------------------------------
// PART ABOUT HASING AND STAMPING
// ----------------------------------
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
  String encodedHashContent = base64::encode(hashContent);
  return encodedHashContent;
}
