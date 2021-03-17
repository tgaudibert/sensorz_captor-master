/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Setup PASSWORD: Point your web browser to http://192.168.4.1/xxxx/password
  3. Setup SSID: Point your web browser to http://192.168.4.1/xxxx/ssid
  4. Setup API BASE URL: Point your web browser to http://192.168.4.1/xxxx/base-url 
  4. Setup API KEY: Point your web browser to http://192.168.4.1/xxxx/access-token

*/

#include "EEPROM.h"

//------------------
//Part for LORA
#include <base64.h>
extern "C" {
#include "crypto/base64.h"
}

    
bool isAppairing = false;
bool isBlocked = false;


//part for the wifi prog
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <HTTPClient.h>

#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED;


#include <LoRa.h> //https://github.com/sandeepmistry/arduino-LoRa

#define SCK 5 // GPIO5 - SX1278's SCK
#define MISO 19 // GPIO19 - SX1278's MISO
#define MOSI 27 // GPIO27 - SX1278's MOSI
#define SS 18 // GPIO18 - SX1278's CS
#define RST 14 // GPIO14 - SX1278's RESET
#define DI0 26 // GPIO26 - SX1278's IRQ (interrupt request)
#define BAND 868E6 // 915E6


const int TOUCH = 13; 
const int ERRORLED = 12;  
const int SUCCESSLED = 17;  

const int SENDINGMODELED = 4;  
const int HOTSPOTMODELED = 2;  




String rssi = "RSSI ";
String packSize = "-";
String packet;


// Set these to your desired credentials.
const char *ssid = "ZenFactory";
const char *password = "zenfactory35";

int WIFIpass_location = 0;
int WIFIssid_location = 100;
int API_KEY_location = 200;
int BASE_URL_location = 600;
int shared_secret_location = 1000;
String WIFIpassword = "1DFE857440FCCE2D0D12AF8235";
String WIFIssid = "freebox_VIYIZQ";
String API_KEY = "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJtYXN0ZXJub2RlIjp7ImlkX21hc3Rlcm5vZGUiOjEsIm1hc3Rlcm5vZGVfbmFtZSI6InByaW5jaXBhbCIsImlkdXNlciI6MX0sImlhdCI6MTU4NjM0MzMzNiwiYXVkIjoiaHR0cDovLzEyNy4wLjAuMTozMDAwIiwiaXNzIjoiREVWVEVBTSIsInN1YiI6IkRFVlRFQU0ifQ.mqkeuqK_QriOPiRy6UZI2JJwju5ASmfVhd0aPPuZerzi07IXqpEiPIeUuJ2F0dfjX-_gw-G2P9HCAYxq7SzdYrXwdOCGmo8U4EF1BcT2eweM9c8Gz1wjuiLpKkcm-mzyJc_zFKqaPmdRg5ztWs6izfx29R0o-3h06S2mufDSvXyYQI6a97fUSQ9I5Wg1rp4DLpk6iWUIhP3Yz1VGqOev8HZgnI-czgE96dMvmgtiKgx8DoYUhpycogZ5W9PMmWPJCq9EhNHLFXgoH_igiIgtgmplfXQXh-qTUZ2uiA8gPMO3PUYgLivriVAiPvZUjX7WF75j8SCoMeWZTRbtLqXCew";
String BASE_URL = "https://ydeo-monitor.com/api/v1/";
String shared_secret = "";
char *shared_secret_char; 


WiFiServer server(80);


void setup () {
  pinMode(TOUCH, INPUT);
  pinMode(ERRORLED, OUTPUT);
  pinMode(SUCCESSLED, OUTPUT);
  pinMode(SENDINGMODELED, OUTPUT);
  pinMode(HOTSPOTMODELED, OUTPUT);
  digitalWrite(SENDINGMODELED, LOW); // set GPIO16 low to reset OLED
  delay (50);
  digitalWrite(SENDINGMODELED, HIGH); // while OLED is running, GPIO16 must go high,
  
  Serial.begin(115200);
  Serial.println("LoRa Receiver Callback");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (! LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  //LoRa.onReceive(cbk);
  LoRa.receive();
  Serial.println("init ok");
  delay (1500);
  if (!EEPROM.begin(4096)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
  
  //clearEeprom();
  shared_secret = EEPROM.readString(shared_secret_location);
  Serial.println(shared_secret);

  //EEPROM.writeString(WIFIssid_location, "simple-basique");
  //EEPROM.writeString(WIFIpass_location, "thomas35");
  //EEPROM.writeString(WIFIssid_location, "freebox_VIYIZQ");
  //EEPROM.writeString(WIFIpass_location, "1DFE857440FCCE2D0D12AF8235");
  EEPROM.writeString(API_KEY_location, "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9.eyJtYXN0ZXJub2RlIjp7ImlkX21hc3Rlcm5vZGUiOjEsIm1hc3Rlcm5vZGVfbmFtZSI6InByaW5jaXBhbCIsImlkdXNlciI6MX0sImlhdCI6MTU4NjM0MzMzNiwiYXVkIjoiaHR0cDovLzEyNy4wLjAuMTozMDAwIiwiaXNzIjoiREVWVEVBTSIsInN1YiI6IkRFVlRFQU0ifQ.mqkeuqK_QriOPiRy6UZI2JJwju5ASmfVhd0aPPuZerzi07IXqpEiPIeUuJ2F0dfjX-_gw-G2P9HCAYxq7SzdYrXwdOCGmo8U4EF1BcT2eweM9c8Gz1wjuiLpKkcm-mzyJc_zFKqaPmdRg5ztWs6izfx29R0o-3h06S2mufDSvXyYQI6a97fUSQ9I5Wg1rp4DLpk6iWUIhP3Yz1VGqOev8HZgnI-czgE96dMvmgtiKgx8DoYUhpycogZ5W9PMmWPJCq9EhNHLFXgoH_igiIgtgmplfXQXh-qTUZ2uiA8gPMO3PUYgLivriVAiPvZUjX7WF75j8SCoMeWZTRbtLqXCew");
  EEPROM.writeString(BASE_URL_location, "https://ydeo-monitor.com/api/v1/");
  EEPROM.commit();
  
  if(shared_secret.length() == 0){
    shared_secret = "pvkey."+ gen_random(80);
    EEPROM.writeString(shared_secret_location, shared_secret);
    EEPROM.commit();
    
    int charLength = shared_secret.length() + 1;
    char charBuf[charLength];
    strcpy(charBuf, shared_secret.c_str());
    shared_secret_char = (char *)charBuf;
  }
    
  shared_secret = EEPROM.readString(shared_secret_location);
  Serial.println(shared_secret);
  Serial.println(shared_secret.length());

 
}


void loop() {
  
  readPinState();

  if (isAppairing) {
    Serial.println("stop touching btn");
    delay(2000);// if you get a client,
    digitalWrite(SENDINGMODELED, LOW);
    digitalWrite(HOTSPOTMODELED, HIGH);
    LoRa.print(shared_secret + ".end");  // Send BME280 pressure reading, addition of 3.7 is for calibration purposes
    LoRa.endPacket();    
    appairingProcess();
  }else if(!isAppairing){
    Serial.println("Working mode");
    delay(2000);
    digitalWrite(SENDINGMODELED, HIGH);
    digitalWrite(HOTSPOTMODELED, LOW);
    sendingProcess();
  }
}


void readPinState(){
  uint8_t pinState = digitalRead(TOUCH);
  if(pinState == 1){
    isAppairing = true;
    /*
    if(!isBlocked  && !isAppairing){
      Serial.println("start appiaring process");
      isBlocked = true;
      isAppairing = true;
      return;
    }else if(!isBlocked  && isAppairing){
      Serial.println("stop appairign process");
      isBlocked = true;
      isAppairing = false;
      return;
    }
    */

  }else{
    isAppairing = false;
    /*
    if(isBlocked){
      Serial.println("stopping process");
      isBlocked = false;
      return;
    }
    */
  }
}








// ----------------------------------
//PART TO CREATE WIFI HOTSPOT
// ----------------------------------

void appairingProcess(){
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.begin();
  Serial.println("Server started");


  WiFiClient client = server.available();   // listen for incoming clients
  Serial.println(isAppairing);

  delay(2000);

  
  while(isAppairing){    
    WiFiClient client = server.available();
    readPinState();
    
    if (client) {             // if you get a client,
      Serial.println("New Client.");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {// if there's bytes to read from the client,
          digitalWrite(SUCCESSLED, LOW);
          char c = client.read();             // read a byte, then
          Serial.write(c);                    // print it out the serial monitor
          if (c == '\n') {                    // if the byte is a newline character
            if (currentLine.length() == 0) {

              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();
              client.print("<h1>OK</h1>");
              client.println();  
              break;
              
            } else {    // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
  
          if (currentLine.endsWith("/access-token")) {
            String split1 = splitStringPT1(currentLine,"/",1);
            API_KEY = splitStringPT2(split1,"/",1);
            char charBuf[1000];
            API_KEY.toCharArray(charBuf, 1000);
            EEPROM.writeString(API_KEY_location, charBuf);
            EEPROM.commit();

            
          }

          if (currentLine.endsWith("/password")) {
            String split1 = splitStringPT1(currentLine,"/",1);
            WIFIpassword = splitStringPT2(split1,"/",1);
            char charBuf[500];
            WIFIpassword.toCharArray(charBuf, 500);
            EEPROM.writeString(WIFIpass_location, charBuf);
            EEPROM.commit();

            
          }
          
          if (currentLine.endsWith("/ssid")) {
            String split1 = splitStringPT1(currentLine,"/",1);
            WIFIssid = splitStringPT2(split1,"/",1);
            char charBuf[500];
            WIFIssid.toCharArray(charBuf, 500);
            EEPROM.writeString(WIFIssid_location, charBuf);
            EEPROM.commit();

             
          }

          if (currentLine.endsWith("/base-url")) {
            String split1 = splitStringPT1(currentLine,"/",1);
            String DomainBase64 = splitStringPT2(split1,"/",1);
            int charLength = DomainBase64.length() + 1;
            char charBuf[charLength];
            strcpy(charBuf, DomainBase64.c_str()); 
               
            size_t outputLength;
            unsigned char * decoded = base64_decode((const unsigned char *)charBuf, strlen(charBuf), &outputLength);
            free(decoded);
        
            char charBuf2[outputLength+1]; 
            std::copy(decoded, decoded+outputLength+1, charBuf2);
            Serial.println(charBuf2);
            EEPROM.writeString(BASE_URL_location, charBuf2);
            EEPROM.commit();

          }

        }
      }

      client.stop();      
      digitalWrite(SUCCESSLED, HIGH);
      Serial.println("Client Disconnected.");
    }
  }
  
}




// ----------------------------------
// PART TO CONNECT TO A WIFI NETWORK
// ----------------------------------

void WIFISetUp(void)
{
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(true); 

  char charWIFIPASS[500];
  String pass = EEPROM.readString(WIFIpass_location);
  pass.toCharArray(charWIFIPASS, 500);
  
  char charWIFISSID[500];
  String ssid = EEPROM.readString(WIFIssid_location);
  ssid.toCharArray(charWIFISSID, 500);
 
  
  //WiFi.begin("simple-basique", "thomas35");//fill in "Your WiFi SSID","Your Password"
  WiFi.begin(charWIFISSID, charWIFIPASS);
  delay(100);

  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && !isAppairing)
  {
    count ++;
    delay(500);
    readPinState();
  }
  
  if(WiFi.status() == WL_CONNECTED)
  {
    Serial.println("connected");
    delay(1000);
  }
  else
  {
    Serial.println(";connection failed");
    delay(1000);
  }
}



void sendingProcess()
{
  //WIFI Kit series V1 not support Vext control
  Serial.println(EEPROM.readString(WIFIpass_location));
  WIFIssid = EEPROM.readString(WIFIpass_location);
  Serial.println("wifiSSID :"+ WIFIssid);
  delay(1000);
  
  WIFIpassword = EEPROM.readString(WIFIssid_location);
  Serial.println("WifiPWD:" + WIFIpassword);
  delay(1000);

  BASE_URL = EEPROM.readString(BASE_URL_location);
  Serial.println("BASE_URL:" + BASE_URL);

  API_KEY = EEPROM.readString(API_KEY_location);
  Serial.println("API_KEY:" + API_KEY);
  delay(1000);

  shared_secret = EEPROM.readString(shared_secret_location);
  int charLength = shared_secret.length() + 1;
  char charBuf[charLength];
  strcpy(charBuf, shared_secret.c_str());
  shared_secret_char = (char *)charBuf;
  
  Serial.println("shared_secret:" + shared_secret);
  delay(1000);
 
  WIFISetUp();
  digitalWrite(HOTSPOTMODELED, LOW);
  Serial.println("Heltec.LoRa Duplex");

  String url = BASE_URL + "activate/masternode";
  getRequest(url);
  
  while(!isAppairing){
    readPinState();
    onReceive(LoRa.parsePacket());
  }
}


void onReceive(int packetSize)
{
  if (packetSize == 0) return;          // if there's no packet, return
  String incoming = "";

  while (LoRa.available())
  {
    incoming += (char)LoRa.read();
  }

  try{
      String packetPayload = splitStringPT2(incoming,".",1);
      char charPayload[packetPayload.length() + 1]; 
      strcpy(charPayload, packetPayload.c_str()); 
  
      String packetSignature = splitStringPT1(incoming,".",1);
      
      String calculatedSignature = hmacSHA(shared_secret_char, charPayload);
      Serial.println(shared_secret_char);
      Serial.println(calculatedSignature);
      
      if(calculatedSignature == packetSignature){
        Serial.println("verified Request");
        String toSend = "{\"data\":";
          toSend += "\""+packetPayload+"\"";
          toSend += ",\"snr\":";
          toSend += "\""+String(LoRa.packetSnr())+"\"";
          toSend += ",\"rssi\":";
          toSend += "\""+String(LoRa.packetRssi())+"\"";
          toSend += "}"; 
      
        String url = BASE_URL + "sensor/data";
        postRequest(url, toSend);
        return;
      }else{
        Serial.println("Bad Request");
        return;
      }
      
    }catch(...){
      Serial.println("error");
      return;
    }

}




// ----------------------------------
// HTTP METHODS
// ----------------------------------

void postRequest(String url, String toSend)
{
  if(WiFi.status()== WL_CONNECTED){   //Check WiFi connection status
 
   HTTPClient http;
   http.begin(url);  

   http.addHeader("Content-Type", "application/json" , "Content-Length", toSend.length());
   http.addHeader("Authorization", "Bearer " + API_KEY);
   int httpResponseCode = http.POST(toSend);  

   if(httpResponseCode>0){
    String response = http.getString();  
    Serial.println(httpResponseCode);  
    Serial.println(response);        
    delay(1000);
 
   }else{
    Serial.println("Error on sending POST: ");
    delay(1000);
 
   }
   Serial.println("end request");
   http.end();  //Free resources
 
 }else{
 
    Serial.println("Error in WiFi connection"); 
    delay(1000);
    WIFISetUp();  
 
 }
}



void getRequest(String url)
{
  if(WiFi.status()== WL_CONNECTED){  
 
   HTTPClient http;
   http.begin(url);  //Specify destination for HTTP request
   http.addHeader("Authorization", "Bearer " + API_KEY);

   int httpResponseCode = http.GET();
   if(httpResponseCode>0){
 
    String response = http.getString();  //Get the response to the request
    Serial.println(httpResponseCode);   //Print return code
    Serial.println(response);           //Print request answer
    delay(1000);
 
   }else{
    Serial.println("Error on sending INITIAL REQUEST: ");
    delay(1000);
 
   }
   Serial.println("end request");
   http.end();  //Free resources
 
 }else{
 
    Serial.println("Error in WiFi connection"); 
 
 }
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
 
  String hashContent = "";
  
  for(int i= 0; i< sizeof(hmacResult); i++){
      char str[3];
    
      sprintf(str, "%02x", (int)hmacResult[i]);
      hashContent += str;
  }
  String encodedHashContent = base64::encode(hashContent);
  return encodedHashContent;
}




// ----------------------------------
//PART SLICING
// ----------------------------------


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


//RETURN FIRST PART OF SEPARATION
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



//generate random token at ESP STARTUP
String gen_random(int numBytes) {

    int i = 0;
    int j = 0;
    String letters[40] = {"a", "b", "c", "d", "e", "f","g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    String randString = "";

    for(i = 0; i<numBytes; i++){
     randString = randString + letters[random(0, 40)];
    }
    Serial.println(randString.length());
    return randString;
}


void clearEeprom(){
  for (int i = 0; i < 1000; ++i) {
    EEPROM.write(i, -1);
  }
  EEPROM.commit();
  return;
}
