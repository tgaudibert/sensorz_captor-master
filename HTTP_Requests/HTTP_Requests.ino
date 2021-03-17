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


#include <WiFi.h>
#include <HTTPClient.h>
 
const char* ssid = "freebox_VIYIZQ";
const char* password =  "1DFE857440FCCE2D0D12AF8235";
char CERTIFICATE[2000]; 
String CERTIFICATEB64="";


void setup() {
 
  Serial.begin(115200);
  delay(1000);
 
  WiFi.begin(ssid, password); 

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



  CERTIFICATEB64 = readFile(SD, "/certificateb64.txt");
  delay(1000);
  int charLength = CERTIFICATEB64.length() + 1;
  char B64Buf[charLength];
  strcpy(B64Buf, CERTIFICATEB64.c_str()); 
  
  size_t outputLength;
  unsigned char * decoded = base64_decode((const unsigned char *)B64Buf, strlen(B64Buf), &outputLength);
  free(decoded);
        
  std::copy(decoded, decoded+outputLength+1, CERTIFICATE);
  delay(1000);
  Serial.println(CERTIFICATE);

  
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
}
 

void loop() {
 
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
 
    HTTPClient http;
 
    http.begin("https://apisensorz.ydeo-monitor.com/api/v1/masternode/activate"); //Specify the URL and certificate
    int httpCode = http.GET();                                                  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        String payload = http.getString();
        Serial.println(httpCode);
        Serial.println(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
 
  delay(10000);
}






String readFile(fs::FS &fs, const char * path){
    //Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        
        //if file doesn't exist, let's create it bitch
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
 
    delay(500);
    file.close();
    return fileContent;
}
