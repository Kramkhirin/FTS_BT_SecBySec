
//*****************************************************************************************************************************
//*******************************************************19/11/2019************************************************************
//Write Code about Scan Mac BT and Pair BT

//*****************************************************************************************************************************
//*****************************************************************************************************************************
//#include <Arduino.h>
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLE2902.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define LED1 33
#define LED2 25

//************************
#define DATA_OK 0
#define ERR_EOT 1
#define ERR_CHK 2
#define ERR_HDR 3
#define ERR_FRM 4

#define STX 0x02
#define EOT 0x04
//-------------------------------
unsigned long period = 2000; //ระยะเวลาที่ต้องการรอ
unsigned long last_time = 0; //ประกาศตัวแปรเป็น global เพื่อเก็บค่าไว้
bool Flag_A = 1;
//-------------------------------
unsigned long period_Pair = 2000; //ระยะเวลาที่ต้องการรอ
unsigned long last_time_Pair = 0; //ประกาศตัวแปรเป็น global เพื่อเก็บค่าไว้


//-------------------------------
unsigned long currentMillisLED1;
unsigned long LED1Millis;  // when button was released
unsigned long LED1OnDelay; // wait to turn on LED
bool ledState1 = false;
bool onLED1 = false;

unsigned long currentMillisLED2;
unsigned long LED2Millis;  // when button was released
unsigned long LED2OnDelay; // wait to turn on LED
bool ledState2 = false;
bool onLED2 = false;

unsigned long currentMillisLoop;
unsigned long LoopMillis;    // when button was released
unsigned long LoopOnDelay;   // wait to turn on LED
unsigned long LoopOffDelay;  // wait to turn on LED
unsigned long ledTurnedOnAt; // when led was turned on
bool ledReady = false;
bool onLoop = false;
bool statL = false;
bool ledstate = false;
//-----------------------------
char DataIn_Buf[512];
unsigned int frameLen = 0 , dataFreme = 0   , indexbuf ;
unsigned char chksum ;
//char txString[20];
int A = 0;
int pCharacter;
String str , RxBlutooth  = "";
char sbuf[512] ;
bool sendBT_flag = false , isNullRxBT = false , readSD_flag = false ;
String buf_Str  = "";
String Substr_Datain [10] = "" ;
//********************** LED Blink delay millis function ***********************
void LED1_blink()
{
  currentMillisLED1 = millis();
  if (onLED1 == true)
  {
    LED1Millis = currentMillisLED2;
    ledState1 = true;
    onLED1 = false;
  }
  if (ledState1 == true)
  {
    digitalWrite(LED1, HIGH);
    if ((unsigned long)(currentMillisLED1 - LED1Millis) >= LED1OnDelay)
    {
      digitalWrite(LED1, LOW);
      ledState1 = false;
    }
  }
}
void LED2_blink()
{
  currentMillisLED2 = millis();
  if (onLED2 == true)
  {
    LED2Millis = currentMillisLED2;
    ledState2 = true;
    onLED2 = false;
  }
  if (ledState2 == true)
  {
    digitalWrite(LED2, HIGH);
    if ((unsigned long)(currentMillisLED2 - LED2Millis) >= LED2OnDelay)
    {
      digitalWrite(LED2, LOW);
      ledState2 = false;
    }
  }
}

void LED2_Blinlkloop()
{
  currentMillisLoop = millis();
  if (onLoop == true)
  {
    if (statL == false)
    {
      LoopMillis = currentMillisLoop;
      statL = true;
      ledReady = true;
    }
  }
  else
  {
    ledReady = false;
    statL = false;
  }

  if (ledReady == true)
  {
    if ((unsigned long)(currentMillisLoop - LoopMillis) >= LoopOnDelay)
    {
      digitalWrite(LED2, HIGH);
      //Serial.println("*** LED2 ON *****");
      ledstate = true;
      ledTurnedOnAt = currentMillisLoop;
      ledReady = false;
    }
  }
  if (ledstate == true)
  {
    if ((unsigned long)(currentMillisLoop - ledTurnedOnAt) >= LoopOffDelay)
    {
      digitalWrite(LED2, LOW);
      //Serial.println("*** LED2 oFF *****");
      ledstate = false;
      statL = false;
    }
  }
}

void LED1_onBlinlk(unsigned long timeBlink)
{
  onLED1 = true;
  LED1OnDelay = timeBlink;
}

void LED2_onBlinlk(unsigned long timeBlink)
{
  onLED2 = true;
  LED2OnDelay = timeBlink;
}

void LED2_on_Blinlkloop(unsigned long timeOn, unsigned long timeOff)
{
  onLoop = true;
  LoopOnDelay =  timeOff;
  LoopOffDelay = timeOn;
}

void LED2_off_Blinlkloop()
{
  onLoop = false;
  digitalWrite(LED2, HIGH);
}

//*********************************************************************************
void clearTXdata_Serial(int Len)
{
  for (int i = 0; i <= Len; i++)
  {
    DataIn_Buf[i] = NULL;
  }
  indexbuf = 0;
}

// void clearTXdata_BT(int Len)
// {
//   for (int i = 0; i <= Len; i++)
//   {
//     RxBt[i] = NULL;
//   }

// }
//-------------Scanner--------------
int scanTime = 5; //In seconds
BLEScan *pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      String rec , sub_rec , sub_fac , dataRec ;
      String setting[10] , manufacDATA[10];

      dataRec = advertisedDevice.toString().c_str();
      sub_rec = dataRec;

      setting[0] = sub_rec.substring(0, sub_rec.indexOf(','));
      sub_rec = sub_rec.substring(sub_rec.indexOf(',') + 1); //Advertised Device: Name:,

      setting[1] = sub_rec.substring(0, sub_rec.indexOf(','));
      sub_rec = sub_rec.substring(sub_rec.indexOf(',') + 1); // Address: 28:a6:75:06:fb:10,

      setting[2] = sub_rec.substring(0, sub_rec.indexOf(','));
      sub_rec = sub_rec.substring(sub_rec.indexOf(',') + 1); // manufacturer data: 862f078815003b00c8064b

      setting[3] = sub_rec.substring(0, sub_rec.indexOf(','));
      sub_rec = sub_rec.substring(sub_rec.indexOf(',') + 1); // Mabe  txPower: 12

      setting[4] = sub_rec.substring(0, sub_rec.indexOf(','));
      sub_rec = sub_rec.substring(sub_rec.indexOf(',') + 1);  // Other

      //            Serial.print("Scan And Fround ->"); Serial.print(setting[1]);
      //             Serial.print("  Manufact ->"); Serial.println(setting[2] );
      // if(sub_rec.equals (00:15:88))
      sub_fac = setting[1];
      manufacDATA[0] = sub_fac.substring(0, sub_fac.indexOf(':'));  // Name
      //  Serial.print("Cut Max ->"); Serial.println(manufacDATA[0]);

      sub_fac =  sub_fac.substring(sub_fac.indexOf(':') + 1);
      //   Serial.print("sub_fac ->"); Serial.println(sub_fac);

      manufacDATA[1] = sub_fac.substring(1, 3);      //| x | x | -- > 1, 3
      //  Serial.print("manufacDATA[1]->"); Serial.print(manufacDATA[1]);
      manufacDATA[2] = sub_fac.substring(4, 6);
      // Serial.print("manufacDATA[2]->"); Serial.print(manufacDATA[2]);
      manufacDATA[3] = sub_fac.substring(7, 9);
      //   Serial.print("manufacDATA[3]->"); Serial.print(manufacDATA[3]);
      manufacDATA[4] = sub_fac.substring(10, 12);
      //  Serial.print("manufacDATA[4]->"); Serial.print(manufacDATA[4]);
      manufacDATA[5] = sub_fac.substring(13, 15);
      // Serial.print("manufacDATA[5]->"); Serial.print(manufacDATA[5]);
      manufacDATA[6] = sub_fac.substring(16, 18);
      //   Serial.print("manufacDATA[6]->"); Serial.println(manufacDATA[6]);
      //          if(manufacDATA[1].equals ("00") && manufacDATA[2].equals ("15")&& manufacDATA[4].equals ("07")){
      //      Serial.print("  ID  ->"); Serial.println(setting[1] );
      //       { Serial.print("FOBO OEM ->"); Serial.print(manufacDATA[1]);Serial.print(":");
      //                                     Serial.print(manufacDATA[2]);Serial.print(":");
      //                                     Serial.print(manufacDATA[3]);Serial.print(":");
      //                                     Serial.print(manufacDATA[4]);Serial.print(":");
      //                                     Serial.print(manufacDATA[5]);Serial.print(":");
      //                                     Serial.print(manufacDATA[6]);
      //      Serial.print("  Manufact ->"); Serial.println(setting[2] );
      //      Serial.println("______________________________");
      //      Serial.print("");
      //      }
    }
};
//-----------------------------------------------------------------------
//-----------------------------------------------------------------------
void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}
//****************************
void createDir(fs::FS &fs, const char *path)
{
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path))
  {
    Serial.println("Dir created");
  }
  else
  {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char *path)
{
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path))
  {
    Serial.println("Dir removed");
  }
  else
  {
    Serial.println("rmdir failed");
  }
}


void readFile(fs::FS &fs, const char *path)
{
  // Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Serial.print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}


void writeFile(fs::FS &fs, const char *path, const char *message)
{
  // Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    //  Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    //  Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  // Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    //  Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
  }
  else
  {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

void testFileIO(fs::FS &fs, const char *path)
{
  File file = fs.open(path);
  static uint8_t buf[512];
  size_t len = 0;
  uint32_t start = millis();
  uint32_t end = start;
  if (file)
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    while (len)
    {
      size_t toRead = len;
      if (toRead > 512)
      {
        toRead = 512;
      }
      file.read(buf, toRead);
      len -= toRead;
    }
    end = millis() - start;
    Serial.printf("%u bytes read for %u ms\n", flen, end);
    file.close();
  }
  else
  {
    Serial.println("Failed to open file for reading");
  }

  file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }

  size_t i;
  start = millis();
  for (i = 0; i < 2048; i++)
  {
    file.write(buf, 512);
  }
  end = millis() - start;
  Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
  file.close();
}
//****************************
//-----------------------------------
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      char RxBt[512];
      if (rxValue.length() > 0) {
        isNullRxBT = true;
        for (int i = 0; i < rxValue.length(); i++) {
          RxBt[i] = rxValue[i];
        }
        //  Serial.println(" Serial BT Send. . ");
      }
      else isNullRxBT = false;
      RxBlutooth = RxBt;
      for (int i = 0; i <= rxValue.length(); i++)
      {
        RxBt[i] = NULL;
      }
    }
};

//---------------------------------------------------
void setup()
{
  // BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  xTaskCreate(&BLEScan_Task, "BLEScan_Task", 2048, NULL, 0, NULL);
  xTaskCreate(&BLEPair_Task, "BLEPair_Task", 4095, NULL, 0, NULL);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  Serial.begin(115200);
  //  Serial.setTimeout(30);
  Serial2.begin(9600);
  Serial2.setTimeout(30 );
  Serial.setTimeout(30);
  Serial.println("BT - SMART Waitting to Pair . . . !");
  Serial.print("Initializing SD card...");
  digitalWrite(LED2, HIGH);

  /// Create the BLE Device
  std::string ble_name = "1กข9999"; //บลูทูธ
  char buffer_test[] = {0xE0, 0xB9, 0x91,
                        0xE0, 0xB8, 0x81,
                        0xE0, 0xB8, 0x82,
                        0xE0, 0xB9, 0x99,
                        0xE0, 0xB9, 0x99,
                        0xE0, 0xB9, 0x99,
                        0xE0, 0xB9, 0x99,
                        0x0
                       };      // null terminated string;

  //strcpy(buffer_test, ble_name.c_str());  // copy from str to byte[]
  //  for (int i = 0; i < sizeof(buffer_test)-1; i++)
  //  {
  //    Serial.print("0x");
  //    Serial.print(buffer_test[i],HEX);
  //    Serial.print(",");
  //  }

  std::string Serial_Name(buffer_test);
  BLEDevice::init(Serial_Name);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );

  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE
                                          );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  //  Serial2.println(" Waiting a client connection to notify . . . ");
  //-------------Scanner--------------
  if (!SD.begin())
  {
    Serial.println(" SD Card Failed");
  }
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
    createDir(SD, "/Mydir");
    listDir(SD, "/", 2);
  }
  else
  {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  //-----------------------------------
  //  listDir(SD, "/", 2);

  // writeFile(SD, "/hello.txt", "Hello ");
  // appendFile(SD, "/hello.txt", " Mine Name ");
  // appendFile(SD, "/hello.txt", "Kramkhirin ");
  // appendFile(SD, "/hello.txt", "World!\n");
  // readFile(SD, "/hello.txt");

  // deleteFile(SD, "/foo.txt");
  //-----------------------------------
}
void loop()
{
  delay(10);
}
//---------------------------------------------------
void BLEScan_Task(void *p) {
  //  if ( Flag_A == 1) {
  //    //  Serial.println("Scanning...");
  //    pBLEScan = BLEDevice::getScan(); //create new scan
  //    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  //    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  //    pBLEScan->setInterval(100);
  //    pBLEScan->setWindow(99);  // less or equal setInterval value
  //
  //    Flag_A = 0;
  //  }

  while (true) {
    LED2_blink();
    //LED2_Blinlkloop();
    // Serial.println("Task 1 RUN");
    //    delay(500);

    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    //  Serial.print("Devices found: ");
    //  Serial.println(foundDevices.getCount());
    //  Serial.println("Scan done!");
    pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
    Flag_A = 0;
  }

}
//**************
void BLEPair_Task(void *p) {
  while (true) {
    LED1_blink();
    char Buf_DataIn [512];
    String DataIn_Str , DataIn_Cha;
    String Send_Tx_Data ;
    bool flag_DataIn = false;

    struct format_Data {
      String D_M_Y ;
      char Buf_D_M_Y [50];
      String H_M_S ;
      char Buf_H_M_S [50];
      String Latitude ;
      String Longtitude ;

    };
    struct format_Data Data;
    // Serial.println("Task 2 RUN");
    //    delay(500);

    //----------Pair--------
    if (deviceConnected) {
      pTxCharacteristic->setValue("Device Connected !!");
      //  vTaskDelay(200);

      if (sendBT_flag == true) {
        pTxCharacteristic->setValue((unsigned char*)DataIn_Buf, indexbuf);  //<----------------- Send Data
        pTxCharacteristic->notify();
      }

      //     delay(10); // bluetooth stack will go into congestion, if too many packets are sent
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
      //     delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      //  Serial2.println("start advertising");
      oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
    }

    if (Serial2.available() > 0)
    {
      //   DataIn_Str = Serial2.readString();
      //  Serial.println(DataIn_Str);
      indexbuf = Serial2.readBytes(DataIn_Buf, 512);
      for (int i = 0; i <= indexbuf; i++)
      {
        Buf_DataIn[i] = DataIn_Buf[i];
        DataIn_Str = String(DataIn_Str + Buf_DataIn[i]);   // char to Str
        //    Serial.print(Buf_DataIn[i]);
      }

      flag_DataIn  = true;
      sendBT_flag = true;

      if (flag_DataIn == true ) {
        int Statdata = Data_analysis(Buf_DataIn, indexbuf);
        //    Serial.println("OK");
        if (Statdata == DATA_OK) {
          Serial.println(DataIn_Str);
          char sbuf[50];
          sprintf(sbuf, "Data.D_M_Y : %.2X/%.2X/%.2X", DataIn_Str[25], DataIn_Str[26], DataIn_Str[27]);
          Serial.println(sbuf);
         
          sprintf(sbuf, "Data.H_M_S : %.2X/%.2X", DataIn_Str[9], DataIn_Str[10]);
          Serial.println(sbuf);

          

        }
        else {
          PrintDataERR(Statdata);
        }


      }
      else {
        //  Serial.println("NOT OK");
        flag_DataIn  = false;
      }
      //      sendBT_flag = true;
      Serial.println();
      clearDataIn_Serial(indexbuf , Buf_DataIn);
    }

    //  {
    //   Serial.println("\n Write Data Sec by Sec  ");
    //   delay(50);
    //   writeFile(SD, "/Sec_By_Sec.txt", "Start Write : ");
    //        appendFile(SD, "/Sec_By_Sec.txt", Buf_Serial2 );
    //    Substr_Datain [0] = buf_Str.substring( , );


    else sendBT_flag = false;
    if (isNullRxBT == true) {
      LED2_on_Blinlkloop(50, 1000);
      String strrec = String("Recieve RxBlutooth : " + RxBlutooth);
      // Serial2.println(strrec);
      Serial.println(strrec);
      isNullRxBT = false;
    }
  }
  // }
}

//---------------------------------------------------
void clearDataIn_Serial(int Len , char*buff)
{
  for (int i = 0; i <= Len; i++)
  {
    buff[i] = NULL;
  }
  indexbuf = 0;
}

//-----------------------------------
unsigned int Make16(char x, char y) {
  return ((x << 8) | y);
}
//-------------------
void PrintDataERR(int err) {
  switch (err) {
    case 1: Serial.println("EOT ERROR !"); break;
    case 2: Serial.println("CHECKSUM ERROR !"); break;
    case 3: Serial.println("HEADER ERROR !"); break;
    case 4: Serial.println("DATA FRAME ERROR !"); break;
    default : Serial.println("OTHER ERROR PLEASE CHECK AGAIN !");
  }
}
//-------------------
unsigned int Data_analysis(char*buff , int indexbuff) {
  if ((buff[0] == STX) ) {
    dataFreme = Make16(buff[1], buff[2]);
    frameLen = dataFreme + 5;  //Full data freme add STX ,BH ,BL ,chk ,EOT
    if (frameLen >= indexbuff) {

      if (buff[frameLen - 2] == EOT) {//-2
        chksum = 0x00;
        //  Serial.print("frameLen - 3: "); Serial.println(buff[frameLen - 2]);
        for (int i = 1; i < frameLen - 3; i++) {//-3
          chksum ^= buff[i];
        }
        if (buff[frameLen - 3 ] != chksum) {
          return ERR_CHK;                   // Check sum error"
        }
        else {
          return DATA_OK;                   // Data is "OK"
        }
      }
      return ERR_EOT;                       // EOT Error
    }
    else {
      return ERR_FRM;                       // Error data frame
    }
    // Serial.print("Frame len : "); Serial.println(frameLen);
  }
  else {
    return ERR_HDR;                         // Header of data error
  }
}



//-------------------------------------
