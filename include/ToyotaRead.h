

void ClearTPMSData(int i)
{
  if (i > 4)
    return;

  TPMS[i].TPMS_ID = 0;
  TPMS[i].lastupdated = 0;

}

void PulseDebugPin(int width_us)
{
  digitalWrite(DEBUGPIN, HIGH);
  delayMicroseconds(width_us);
  digitalWrite(DEBUGPIN, LOW);
}


int GetPreferredIndex(unsigned long ID)
{
  int i;

  for (i = 0; i  < (sizeof(IDLookup) / sizeof(IDLookup[0])); i++)
  {
    if (IDLookup[i] == ID)
    {
      return (i);
    }

  }
  return (-1);
}



void PrintTimings(byte StartPoint, byte Count)
{
  byte i;
  for (i = 0; i < Count; i++)
  {
    Serial.print(Timings[StartPoint + i]);
    Serial.print(F(","));
  }
  Serial.println(F(""));
  //    for (i = 0;i<Count;i++)
  //    {
  //          Serial.print(BitTimings[StartPoint + i]);
  //          Serial.print(",");
  //    }
  //    Serial.println("");


}

void PrintData(byte Count)
{
  byte i;
  byte hexdata;
  for (i = 0; i < Count; i++)
  {
    Serial.print(IncomingBits[i]);
    hexdata = (hexdata << 1) + IncomingBits[i];
    if ((i + 1) % 8 == 0)
    {
      Serial.print(F(" ["));
      Serial.print(hexdata, HEX);
      Serial.print(F("] "));
      hexdata = 0;
    }
  }
  Serial.println(F(""));
}



void InitTPMS()
{
  int i;

  for (i = 0; i < 4; i++)
  {
    ClearTPMSData(i);
  }

  UpdateDisplay();

}

void UpdateTPMSData(int index, unsigned long ID, unsigned int status, float Temperature, float Pressure)
{

  if (index >= 4)
    return;

  TPMS[index].TPMS_ID = ID;
  TPMS[index].TPMS_Status = status;
  TPMS[index].lastupdated = millis();
  TPMS[index].TPMS_Temperature = Temperature;
  TPMS[index].TPMS_Pressure = Pressure;
}

void DisplayStatusInfo()
{
  Serial.print (F("FreqOffset: "));
  Serial.print (FreqOffset);
  Serial.print (F("  DemodLinkQuality: "));
  Serial.print (DemodLinkQuality);
  Serial.print (F("  RSSI: "));
  Serial.println (RSSIvalue);
}

boolean Check_TPMS_Timeouts()
{
   byte i;
   boolean ret = false;
    
    //clear any data not updated in the last 5 minutes
  for (i = 0; i < 4; i++)
  {
    #ifdef SHOWDEGUGINFO
      Serial.print(TPMS[i].TPMS_ID, HEX);
      Serial.print(F("   "));
    #endif

    if ((TPMS[i].TPMS_ID != 0) && (millis() - TPMS[i].lastupdated > TPMS_TIMEOUT))
    {
      #ifdef SHOWDEGUGINFO
         Serial.print(F("Clearing ID "));
         Serial.println(TPMS[i].TPMS_ID, HEX);
      #endif
      ClearTPMSData(i);
      ret = true;
    }

  }
  return(ret);
}

void DecodeTPMS()
{
  int i;
  unsigned long id = 0;
  unsigned int status, pressure1, pressure2, temp;
  float realpressure;
  float realtemp;
  bool IDFound = false;
  int prefindex;

  for (i = 0; i < 4; i++)
  {
    id = id << 8;
    id = id + RXBytes[i];

  }

  // id = (unsigned)RXBytes[0] << 24 | RXBytes[1] << 16 | RXBytes[2] << 8 | RXBytes[3];

  status = (RXBytes[4] & 0x80) | (RXBytes[6] & 0x7f); // status bit and 0 filler

  pressure1 = (RXBytes[4] & 0x7f) << 1 | RXBytes[5] >> 7;

  temp = (RXBytes[5] & 0x7f) << 1 | RXBytes[6] >> 7;

  pressure2 = RXBytes[7] ^ 0xff;



  if (pressure1 != pressure2)
  {
    Serial.println(F("Pressure check mis-match"));
    return;
  }

  

  realpressure = pressure1 * 0.25 - 7.0; //in PSI
  realpressure = realpressure * 6.89475729;
  
  Serial.print(F("Pressure: "));
  Serial.println(realpressure);
  
  realtemp = temp - 40.0;

#ifdef SHOWDEGUGINFO
  Serial.print(F("ID: "));
  Serial.print(id, HEX);
  Serial.print(F("   Status: "));
  Serial.print(status);
  Serial.print(F("   Temperature: "));
  Serial.print(realtemp);
  Serial.print(F("   Tyre Pressure: "));
  Serial.print(realpressure);
  Serial.println(F(""));
#endif

  //DisplayStatusInfo();

  //update the array of tyres data
  for (i = 0; i < 4; i++)
  { //find a matching ID if it already exists
    if (id == TPMS[i].TPMS_ID)
    {
      UpdateTPMSData(i, id, status, realtemp, realpressure);
      IDFound = true;
      break;
    }

  }

  //no matching IDs in the array, so see if there is an empty slot to add it into, otherwise, ignore it.
  if (IDFound == false)
  {

    prefindex = GetPreferredIndex(id);
    if (prefindex == -1)
    { //not found a specified index, so use the next available one..
      for (i = 0; i < 4; i++)
      {
        if (TPMS[i].TPMS_ID == 0)
        {
          UpdateTPMSData(i, id, status, realtemp, realpressure);
        }
      }
    }
    else
    { //found a match in the known ID list...
      UpdateTPMSData(prefindex, id, status, realtemp, realpressure);
    }

  }


  #ifdef SHOWDEGUGINFO
     Serial.println(F(""));
  #endif


  //UpdateDisplay();
}


#ifndef USE_PROGMEMCRC
  void CalulateTable_CRC8()
  {
    const byte generator = 0x07;
  
    /* iterate over all byte values 0 - 255 */
    for (int divident = 0; divident < 256; divident++)
    {
      byte currByte = (byte)divident;
      /* calculate the CRC-8 value for current byte */
      for (byte bit = 0; bit < 8; bit++)
      {
        if ((currByte & 0x80) != 0)
        {
          currByte <<= 1;
          currByte ^= generator;
        }
        else
        {
          currByte <<= 1;
        }
      }
      /* store CRC value in lookup table */
      crctable[divident] = currByte;
      Serial.print("0x");
      if (currByte < 16)
         Serial.print("0");
      Serial.print(currByte,HEX);
      Serial.print(", ");
    }
  }
#endif

byte Compute_CRC8( int bcount)
{
  byte crc = 0x80;
  int c;
  for (c = 0; c < bcount; c++)
  {
    byte b = RXBytes[c];
    /* XOR-in next input byte */
    byte data = (byte)(b ^ crc);
    /* get current CRC value = remainder */
    #ifdef USE_PROGMEMCRC
        crc = (byte)(pgm_read_byte(&crctable2[data]));
    #else
        crc = (byte)(crctable[data]);
    #endif

  }

  return crc;
}


void ClearRXBuffer()
{
  int i;

  for (i = 0; i < sizeof(RXBytes); i++)
  {
    RXBytes[i] = 0;
  }
}

void EdgeInterrupt()
{
  unsigned long ts = micros();
  unsigned long BitWidth;

  if (TimingsIndex == 255)
  {
    return;
  }


  BitWidth = ts - LastEdgeTime_us;
  if (BitWidth <= 12)  //ignore glitches
  {
    return;
  }
  if (BitWidth > 255)
    BitWidth = 255;

  LastEdgeTime_us = ts;
  //    if ((BitWidth >= 38) && (BitWidth <= 250))
  //    {//ignore out of spec pulses
  Timings[TimingsIndex++] = (byte)BitWidth;
  //    }

  //    digitalWrite(DEBUGPIN,HIGH);
  //    delayMicroseconds(3);
  //    digitalWrite(DEBUGPIN,LOW);




}

bool IsValidSync(byte Width)
{
  if (Width >=  175)
  {
    return (true);
  }
  else
  {
    return (false);
  }
}

bool IsValidShort(byte Width)
{
  if ((Width >= 40) && (Width <= 60))
  {
    return (true);
  }
  else
  {
    return (false);
  }
}


bool IsValidLong(byte Width)
{
  if ((Width >= 80) && (Width <= 120))
  {
    return (true);
  }
  else
  {
    return (false);
  }
}

int ValidateBit()
{
  byte BitWidth = Timings[CheckIndex];
  byte BitWidthNext = Timings[CheckIndex + 1];

  if (IsValidLong(BitWidth))
  {
    return (1);
  }

  if (IsValidShort(BitWidth))
  {
    return (0);
  }

  if (IsValidSync(BitWidth))
  {
    return (2);
  }


  return (-1);

}


void ValidateTimings()
{


  byte BitWidth;
  byte BitWidthNext;
  byte BitWidthNextPlus1;
  byte BitWidthPrevious;
  byte diff = TimingsIndex - CheckIndex;
  //unsigned long tmp;
  bool WaitingTrailingZeroEdge = false;
  int ret;

  StartDataIndex = 0;

  if (diff < 72)
  { //not enough in the buffer to consider a valid message
    Serial.println(F("Insufficient data in buffer"));
    return;
  }

  SyncFound = true;

  while ((diff > 0) && (BitCount < 72))
  { //something in buffer to process...
    diff = TimingsIndex - CheckIndex;

    BitWidth = Timings[CheckIndex];

    if (SyncFound == false)
    {
      if (IsValidSync(BitWidth))
      {
        SyncFound = true;
        BitIndex = 0;
        BitCount = 0;
        WaitingTrailingZeroEdge = false;
        StartDataIndex = CheckIndex + 1;
      }

    }
    else
    {
      ret = ValidateBit();
      switch (ret)
      {
        case -1:
          //invalid bit
          BitIndex = 0;
          BitCount = 0;
          WaitingTrailingZeroEdge = false;
          StartDataIndex = CheckIndex + 1;
          break;

        case 0:
          if (WaitingTrailingZeroEdge)
          {
            //BitTimings[BitIndex] = BitWidth;
            IncomingBits[BitIndex++] = 0;
            BitCount++;
            WaitingTrailingZeroEdge = false;
          }
          else
          {
            WaitingTrailingZeroEdge = true;
          }
          break;

        case 1:
          if (WaitingTrailingZeroEdge)
          { //shouldn't get a long pulse when waiting for the second short pulse (i.e. expecting bit = 0)
            //try to resync from here?
            BitIndex = 0;
            BitCount = 0;
            WaitingTrailingZeroEdge = false;
            CheckIndex--;  //recheck this entry
            StartDataIndex = CheckIndex + 1;
          }
          else
          {
            //BitTimings[BitIndex] = BitWidth;
            IncomingBits[BitIndex++] = 1;
            BitCount++;
          }
          break;

        case 2:
          SyncFound = true;
          BitIndex = 0;
          BitCount = 0;
          WaitingTrailingZeroEdge = false;
          StartDataIndex = CheckIndex + 1;
          break;
      }
    }
    CheckIndex++;
  }


}



void InitDataBuffer()
{
  BitIndex = 0;
  BitCount = 0;
  ValidBlock = false;
  //WaitingTrailingZeroEdge = false;
  WaitingFirstEdge  = true;
  CheckIndex = 0;
  TimingsIndex = 0;
  SyncFound = false;
  //digitalWrite(DEBUGPIN, LOW);

}

void UpdateStatusInfo()
{
  FreqOffset = readStatusReg(CC1101_FREQEST);
  DemodLinkQuality = readStatusReg(CC1101_LQI);
  RSSIvalue = readStatusReg(CC1101_RSSI);
}

int ReceiveMessage()
{

  //Check bytes in FIFO
  int FIFOcount;
  int resp;

  //set up timing of edges using interrupts...
  LastEdgeTime_us = micros();
  CD_Width = micros();

  digitalWrite(DEBUGPIN,HIGH);

  attachInterrupt(digitalPinToInterrupt(RXPin), EdgeInterrupt, CHANGE);
  while (GetCarrierStatus() == true)
  {
  }
  detachInterrupt(digitalPinToInterrupt(RXPin));

  digitalWrite(DEBUGPIN,LOW);

  CD_Width = micros() - CD_Width;
  if(CD_Width > 6800){
    Serial.print(CD_Width, DEC);
    Serial.print(",");
  }
  if ((CD_Width >= 7600) && (CD_Width <= 8200))
  //if ((CD_Width >= 6800) && (CD_Width <= 8200))
  {
    //Serial.println(F("Checking"));
    digitalWrite(LED_RX,LED_ON);
    CheckIndex = 0;
    ValidateTimings();
    //Serial.println(F("Checking complete"));
    digitalWrite(LED_RX,LED_OFF);
    return (BitCount);
  }
  else
  {
    return (0);
  }



}