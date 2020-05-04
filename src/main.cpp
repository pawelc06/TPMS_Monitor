#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>



#include "globals.h"
#include "CC1101.h"
#include "display.h"
#include "ToyotaRead.h"



void SendDebug(String Mess)
{
  Serial.println(Mess);
}



int DecodeBitArray()
{
  //convert 1s and 0s array to byte array
  int i;
  int n = 0;
  byte b = 0;

  RXByteCount = 0;
  for (i = 0; i < BitCount; i++)
  {
    b = b << 1;
    b = b + IncomingBits[i];
    n++;
    if (n == 8)
    {
      RXBytes[RXByteCount] = b;
      RXByteCount++;
      n = 0;
      b = 0;
    }

  }
  return (RXByteCount);


}

void setup() {

  byte resp;
  byte ver;
  char verStr[4];
  unsigned int t;
  int LEDState = LOW;
  int i;
  int mcount;
  char idStr[11];
  unsigned long sensorId;

  //SPI CC1101 chip select set up
  pinMode(CC1101_CS, OUTPUT);
  digitalWrite(CC1101_CS, HIGH);


  Serial.begin(115200);



  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_RX, LED_ON);
  //while(1);
  pinMode(RXPin, INPUT);


  SPI.begin();
  //initialise the CC1101
  CC1101_reset();

  delay(2000);

  Serial.println("Starting...");

  resp = readStatusReg(CC1101_PARTNUM);
  Serial.print(F("Part no: "));
  Serial.println(resp, HEX);

  ver = readStatusReg(CC1101_VERSION);
  Serial.print(F("Version: "));
  Serial.println(ver, HEX);

  resp = readStatusReg(CC1101_RSSI);
  Serial.print(F("RSSI: "));
  Serial.println(resp, HEX);

  setIdleState();
  digitalWrite(LED_RX, LED_OFF);

 
  


#if USE_ADAFRUIT
  if (!display.begin(SSD1306_EXTERNALVCC, I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
#else
  Wire.begin();
  Wire.setClock(400000L);
  display.begin(&Adafruit128x64, I2C_ADDRESS);
  display.setFont(Adafruit5x7);
  
#endif



  Serial.println(F("SSD1306 initialised OK"));

   
  digitalWrite(LED_RX, LED_ON);
  LEDState = HIGH;

  pinMode(DEBUGPIN, OUTPUT);

#ifndef USE_PROGMEMCRC
  CalulateTable_CRC8();
#endif

  // Clear the buffer
#if USE_ADAFRUIT
  display.clearDisplay();
  display.display();
#else
  display.clear();
#endif

  InitTPMS();

  itoa(ver,verStr,16);
  
  ShowTitle(verStr);
  
  /*
  for(int i=0;i<4;i++){
    display.setCursor(0,3+i);
    //sprintf(idStr,"%08lX",(unsigned long)IDLookup[i]);
    //ltoa(IDLookup[i], idStr, 16);
    sensorId = pgm_read_dword(&IDLookup[i]);
    oneULong(idStr, sensorId);
    Serial.println(idStr);
    display.print(idStr);
    
  }
  */


  digitalWrite(LED_RX, LED_OFF);

  setRxState();
}

void loop() {
  // put your main code here, to run repeatedly:
  int i;
  static long lastts = millis();
  float diff;
  int RXBitCount = 0;
  int ByteCount = 0;
  byte crcResult;
  boolean TPMS_Changed;


  TPMS_Changed = Check_TPMS_Timeouts();

  InitDataBuffer();

  //wait for carrier status to go low
  while (GetCarrierStatus == true)
  {
  }

  //wait for carrier status to go high  looking for rising edge
  while (GetCarrierStatus == false)
  {
  }

  if (GetCarrierStatus() == true)
  { //looks like some data coming in...
    RXBitCount = ReceiveMessage();
    if (RXBitCount == 72 )
    {
      ByteCount = DecodeBitArray();

      crcResult = Compute_CRC8(ByteCount);

      if (crcResult != 0)
      {
        //Serial.print(F("CRC: "));
        //Serial.println(crcResult, HEX);
        //Serial.println(F("CRC Check failed"));
        //PrintData(BitCount);
      }
      else
      {
        //decode the message...
        DecodeTPMS();
        TPMS_Changed = true;  //indicates the display needs to be updated.
      }
    } 

    if (TPMS_Changed)
    {
      UpdateDisplay();
      TPMS_Changed = false;
    }
  }




}