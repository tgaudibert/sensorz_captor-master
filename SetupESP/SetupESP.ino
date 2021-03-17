/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Setup PASSWORD: Point your web browser to http://192.168.4.1/xxxx/password
  3. Setup SSID: Point your web browser to http://192.168.4.1/xxxx/ssid
  4. Setup API BASE URL: Point your web browser to http://192.168.4.1/xxxx/base-url 
  4. Setup API KEY: Point your web browser to http://192.168.4.1/xxxx/access-token

*/

/*

  Micro TF card pins to ESP32 GPIOs via this connecthin:
  SD_CS   -- GPIO22
  SD_MOSI -- GPIO23
  SD_SCK  -- GPIO17
  SD_MISO -- GPIO13

*/

//------------------
//PART FOR THE SD CARD
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <time.h>
SPIClass spi1;


//------------------
//Part for LORA
#include <base64.h>
extern "C" {
#include "crypto/base64.h"
}
#include "heltec.h"
#define BAND    868E6  //you can set band here directly,e.g. 868E6,915E6

        
int counter = 2000;         
bool isAppairing = false;
bool isBlocked = false;


//part for the wifi prog
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <HTTPClient.h>


#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED

// Set these to your desired credentials.
const char *ssid = "yourAP";
const char *password = "yourPassword";
String WIFIpassword = "";
String WIFIssid = "";
String API_KEY = "";
String BASE_URL = "";
String CERTIFICATEB64 = "";
char CERTIFICATE[2000]; 


WiFiServer server(80);


void setup() {
    Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.Heltec.Heltec.LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
    Serial.println("Heltec.LoRa init succeeded.");
    
    pinMode(2, INPUT);
    Serial.begin(115200);

    SPIClass(1);
    spi1.begin(17, 13, 23, 22);

    if(!SD.begin(22, spi1)){
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    Serial.print("SD Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.printf("SD Card Size: %lluMB\n", cardSize);


    readFile(SD, "/base-url.txt");
    readFile(SD, "/access-token.txt");
    readFile(SD, "/wifi-id.txt");
    readFile(SD, "/wifi-pass.txt");
    readFile(SD, "/domain.txt");
    readFile(SD, "/certificateb64.txt");

  
}






void loop() {
  
  readPinState();

  if (isAppairing) {
    Serial.println("stop touching btn");
    delay(2000);// if you get a client,
    digitalWrite(16, HIGH);
    appairingProcess();
  }else if(!isAppairing){
    Serial.println("Working mode");
    delay(2000);
    digitalWrite(16, LOW);
    sendingProcess();
  }
}


void readPinState(){
  uint8_t pinState = digitalRead(2);
  if(pinState == 1){
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

  }else{
    if(isBlocked){
      Serial.println("stopping process");
      isBlocked = false;
      return;
    }
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

  Heltec.display->drawString(0, 0, "ready");
  Heltec.display->drawString(0, 9, "for incoming connections");
  Heltec.display->display();
  delay(2000);
  Heltec.display->clear();

  
  while(isAppairing){    
    WiFiClient client = server.available();
    readPinState();
    
    if (client) {             // if you get a client,
      Serial.println("New Client.");           // print a message out the serial port
      String currentLine = "";                // make a String to hold incoming data from the client
      while (client.connected()) {            // loop while the client's connected
        if (client.available()) {// if there's bytes to read from the client,
          digitalWrite(16, LOW);
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

            deleteFile(SD, "/access-token.txt");
            writeFile(SD, "/access-token.txt", charBuf);
            String accessToken = readFile(SD, "/access-token.txt");
            Serial.println("access-token: " + API_KEY);
            
          }

          if (currentLine.endsWith("/password")) {
            String split1 = splitStringPT1(currentLine,"/",1);
            WIFIpassword = splitStringPT2(split1,"/",1);
            char charBuf[500];
            WIFIpassword.toCharArray(charBuf, 500);

            deleteFile(SD, "/wifi-pass.txt");
            writeFile(SD, "/wifi-pass.txt", charBuf);
            String pass = readFile(SD, "/wifi-pass.txt");
            Serial.println("password: " + WIFIpassword);
            
          }
          
          if (currentLine.endsWith("/ssid")) {
            String split1 = splitStringPT1(currentLine,"/",1);
            WIFIssid = splitStringPT2(split1,"/",1);
            char charBuf[500];
            WIFIssid.toCharArray(charBuf, 500);

            deleteFile(SD, "/wifi-id.txt");
            writeFile(SD, "/wifi-id.txt", charBuf);
            String wifiSsid = readFile(SD, "/wifi-id.txt");
            Serial.println("ssid: " + WIFIssid);
             
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
            deleteFile(SD, "/base-url.txt");
            writeFile(SD, "/base-url.txt", charBuf2);
            BASE_URL = readFile(SD, "/base-url.txt");
            Serial.println("Base url: " + BASE_URL);
             
          }

          if (currentLine.endsWith("/certificateb64")) {
            String split1 = splitStringPT1(currentLine,"/",1);
            CERTIFICATEB64 = splitStringPT2(split1,"/",1);
            char charBuf[500];
            BASE_URL.toCharArray(charBuf, 500);

            deleteFile(SD, "/certificateb64.txt");
            writeFile(SD, "/certificateb64.txt", charBuf);
            CERTIFICATEB64 = readFile(SD, "/certificateb64.txt");
            Serial.println("certificateb64: " + CERTIFICATEB64);
             
          }


        }
      }
      // close the connection:
      client.stop();
      
      digitalWrite(16, HIGH);
      Serial.println("Client Disconnected.");
    }
  }
  
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
  WIFIpassword.toCharArray(charWIFIPASS, 500);
  Serial.println(charWIFIPASS);

  char charWIFISSID[500];
  WIFIssid.toCharArray(charWIFISSID, 500);
  Serial.println(charWIFISSID);
  
  
  //WiFi.begin("simple-basique", "thomas35");//fill in "Your WiFi SSID","Your Password"
  WiFi.begin(charWIFISSID, charWIFIPASS);
  delay(100);
  Heltec.display->clear();

  byte count = 0;
  while(WiFi.status() != WL_CONNECTED && !isAppairing)
  {
    count ++;
    delay(500);
    Heltec.display->drawString(0, 0, "Connecting...");
    Heltec.display->display();
    readPinState();
  }
  //Heltec.display->clear();
  //Heltec.display->drawString(0, 9, "connected");
  if(WiFi.status() == WL_CONNECTED)
  {
    Heltec.display->drawString(35, 38, "WIFI SETUP");
    Heltec.display->drawString(0, 9, "OK");
    Heltec.display->display();
    delay(1000);
    Heltec.display->clear();
  }
  else
  {
    //Heltec.display->clear();
    Heltec.display->drawString(0, 9, "Failed");
    Heltec.display->display();
    delay(1000);
    Heltec.display->clear();
  }
}



void sendingProcess()
{
  //WIFI Kit series V1 not support Vext control
  WIFIssid = readFile(SD, "/wifi-id.txt");
  Serial.println("wifiSSID :"+ WIFIssid);
  delay(1000);

  
  WIFIpassword = readFile(SD, "/wifi-pass.txt");
  Serial.println("WifiPWD:" + WIFIpassword);
  delay(1000);


  BASE_URL = readFile(SD, "/base-url.txt");
  Serial.println("BASE_URL:" + BASE_URL);
  delay(1000);


  API_KEY = readFile(SD, "/access-token.txt");
  Serial.println("API_KEY:" + API_KEY);
  delay(1000);

  
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Heltec.LoRa Enable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, BAND /*long BAND*/);
  WIFISetUp();
  digitalWrite(16, LOW);
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
  String incomingFirstElement = splitStringPT2(incoming,":",1);
  Serial.println(incomingFirstElement);
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
    
  if(incomingFirstElement == "sn"){
    String toSend = "{\"data\":";
    toSend += "\""+incoming+"\"";
    toSend += ",\"snr\":";
    toSend += "\""+String(LoRa.packetSnr())+"\"";
    toSend += ",\"rssi\":";
    toSend += "\""+String(LoRa.packetRssi())+"\"";
    toSend += "}"; 

    String url = BASE_URL + "sensor/data";
    postRequest(url, toSend);
    return;
  }

}



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
    Heltec.display->drawString(0, 9, (String)httpResponseCode);
    Heltec.display->display();
    delay(1000);
    Heltec.display->clear();
 
   }else{
    Serial.println("Error on sending POST: ");
    Heltec.display->drawString(0, 9, "error sending POST");
    Heltec.display->display();
    delay(1000);
    Heltec.display->clear();
 
   }
   Serial.println("end request");
   http.end();  //Free resources
 
 }else{
 
    Serial.println("Error in WiFi connection"); 
    Heltec.display->drawString(0, 9, "Error in wifi connection");
    Heltec.display->display();
    delay(1000);
    Heltec.display->clear();
    WIFISetUp();  
 
 }
}



void getRequest(String url)
{
  if(WiFi.status()== WL_CONNECTED){  
 
   HTTPClient http;
   http.begin(url);  //Specify destination for HTTP request
   //Specify content-type header

   http.addHeader("Authorization", "Bearer " + API_KEY);

   int httpResponseCode = http.GET();
   if(httpResponseCode>0){
 
    String response = http.getString();  //Get the response to the request

    Serial.println(httpResponseCode);   //Print return code
    Serial.println(response);           //Print request answer
    Heltec.display->drawString(0, 9, (String)httpResponseCode);
    Heltec.display->display();
    delay(1000);
    Heltec.display->clear();
 
   }else{
 
    Serial.println("Error on sending INITIAL REQUEST: ");
    Heltec.display->drawString(0, 9, "error sending INITIAL REQUEST");
    Heltec.display->display();
    delay(1000);
    Heltec.display->clear();
    //return;
 
   }
   Serial.println("end request");
   http.end();  //Free resources
 
 }else{
 
    Serial.println("Error in WiFi connection"); 
 
 }
}




// ----------------------------------
//PART ABOUT FILESYSTEM - SD CARD
// ----------------------------------

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}


String readFile(fs::FS &fs, const char * path){
    //Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        
        //if file doesn't exist, let's create it bitch
        writeFile(SD, path, "");
        return "";
    }

    //Serial.print("Read from file: ");
    String fileContent = "";

    while (file.available())
    {
      fileContent += (char)file.read();
    }
    Serial.println(path);
    Serial.println(fileContent);
    Heltec.display->drawString(0, 0, (String)path);
    Heltec.display->drawString(0, 9, (String)fileContent);
    Heltec.display->display();
    delay(500);
    Heltec.display->clear();
    file.close();
    return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}






// ----------------------------------
// PART ABOUT HASING AND STAMPING
// ----------------------------------
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


//DECODE BASE64


char* decodeB64(String B64String){
  
    int charLength = B64String.length() + 1;
    char B64Buf[charLength];
    strcpy(B64Buf, B64String.c_str()); 
               
    size_t outputLength;
    unsigned char * decoded = base64_decode((const unsigned char *)B64Buf, strlen(B64Buf), &outputLength);
    free(decoded);
        
    char B64Decoded[outputLength+1]; 
    std::copy(decoded, decoded+outputLength+1, B64Decoded);
    Serial.println(charLength);
    Serial.println(B64Decoded);
    delay(1000);
    return B64Decoded;
}
 
