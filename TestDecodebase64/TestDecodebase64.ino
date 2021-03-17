#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <time.h>
SPIClass spi1;


//------------------
#include "heltec.h"
#define BAND    868E6  //you can set band here directly,e.g. 868E6,915E6

#include <base64.h>
extern "C" {
  #include "crypto/base64.h"
}



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
    

    readFile(SD, "/domain.txt");


  /*
    char *buf = new char[DomainBase64.length()];
    strcpy(buf,DomainBase64);
    */
    String DomainBase64 = "ZnNlZnNlZnNlZnNlZnNlZmlkaWxhbWEhISE=";
    
    int charLength = DomainBase64.length() + 1;
    char charBuf[charLength];
    strcpy(charBuf, DomainBase64.c_str()); 
       
    char * toDecode = "ZnNlZnNlZnNlZnNlZnNlZmlkaWxhbWEhISE=";
    size_t outputLength;
    unsigned char * decoded = base64_decode((const unsigned char *)charBuf, strlen(charBuf), &outputLength);
    free(decoded);

    char charBuf2[outputLength+1]; 
    std::copy(decoded, decoded+outputLength+1, charBuf2);
    Serial.println();  
    writeFile(SD, "/domain.txt", charBuf2);
    String domain = readFile(SD, "/domain.txt");
    Serial.println("domain: " + domain);
  
}


void loop() {
  // put your main code here, to run repeatedly:

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
    //Serial.printf("Writing file: %s\n", path);

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
