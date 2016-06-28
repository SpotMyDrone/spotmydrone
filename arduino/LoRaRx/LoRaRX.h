/*
**************************************************************************************************
Original libraries Copyright of Stuart Robinson - 02/07/2015 15:00
Modified by Roberto Cazzaro

These programs may be used free of charge for personal, recreational and educational purposes only.

This program, or parts of it, may not be used for or in connection with any commercial purpose without
the explicit permission of the author Stuart Robinson.

The programs are supplied as is, it is up to individual to decide if the programs are suitable for the
intended purpose and free from errors.
**************************************************************************************************
*/

byte LoRa_RXPacketType;    //type received packet
byte LoRa_BackGroundRSSI;    //measured background noise level
byte LoRa_PacketRSSI;      //RSSI of received packet
byte LoRa_PacketSNR;     //signal to noise ratio of received packet
byte LoRa_RXPacketL;     //length of packet received, includes packet type.
byte LoRa_RXBUFF[128];     //buffer where received packets are stored


//LoRa names for bandwidth settings
const byte LoRa_BW7_8 = 0;      //7.8khz
const byte LoRa_BW10_4 = 16;    //10.4khz
const byte LoRa_BW15_6 = 32;    //15.6khz
const byte LoRa_BW20_8 = 48;    //20.8khz
const byte LoRa_BW31_2 = 64;    //31.2khz
const byte LoRa_BW41_7 = 80;    //41.7khz
const byte LoRa_BW62_5 = 96;    //62.5khz
const byte LoRa_BW125 = 112;    //125khz
const byte LoRa_BW250 = 128;    //250khz
const byte LoRa_BW500 = 144;    //500khz

//Spreading Factors
const byte LoRa_SF6 = 6;
const byte LoRa_SF7 = 7;
const byte LoRa_SF8 = 8;
const byte LoRa_SF9 = 9;
const byte LoRa_SF10 = 10;
const byte LoRa_SF11 = 11;
const byte LoRa_SF12 = 12;

//LORA names for coding rate settings
const byte LoRa_CR4_5 = 2;	//4:5
const byte LoRa_CR4_6 = 4;	//4:6
const byte LoRa_CR4_7 = 6;	//4:7
const byte LoRa_CR4_8 = 8;	//4:8

//LORA Header Settings
const byte LoRa_Explicit    = 0;	//Use to set explicit header
const byte LoRa_Implicit    = 1;	//Use to set implicit header

//Misc definitions
const byte LoRa_Deviation = 0x52;
const byte LoRa_LowDoptON = 0x08;       //value to turn low data rate optimization on
const byte LoRa_LowDoptOFF = 0x00;      //value to turn low data rate optimization off
const byte LoRa_PrintASC = 0;           //value to cause buffer print to appear as ASCII
const byte LoRa_PrintNum = 1;           //value to cause buffer print to appear as decimal numbers
const byte LoRa_PrintHEX = 2;           //value to cause buffer print to appear as hexadecimal numbers


//SX1278 Register names
const byte LoRa_RegFifo = 0x00;
const byte LoRa_WRegFifo = 0x80;
const byte LoRa_RegOpMode = 0x01;
const byte LoRa_RegFdevLsb = 0x05;
const byte LoRa_RegFrMsb = 0x06;
const byte LoRa_RegFrMid = 0x07;
const byte LoRa_RegFrLsb = 0x08;
const byte LoRa_RegPaConfig = 0x09;
const byte LoRa_RegOcp = 0x0B;
const byte LoRa_RegLna = 0x0C;
const byte LoRa_RegFifoAddrPtr = 0x0D;
const byte LoRa_RegFifoTxBaseAddr = 0x0E;
const byte LoRa_RegFifoRxBaseAddr = 0x0F;
const byte LoRa_RegIrqFlagsMask = 0x11;
const byte LoRa_RegIrqFlags = 0x12;
const byte LoRa_RegRxNbBytes = 0x13;
const byte LoRa_RegRxHeaderCntValueMsb = 0x14;
const byte LoRa_RegRxHeaderCntValueLsb = 0x15;
const byte LoRa_RegRxPacketCntValueMsb = 0x16;
const byte LoRa_RegRxPacketCntValueLsb = 0x17;
const byte LoRa_RegPktSnrValue = 0x19;
const byte LoRa_RegPktRssiValue = 0x1A;
const byte LoRa_RegRssiValue = 0x1B;
const byte LoRa_RegFsiMSB = 0x1D;
const byte LoRa_RegFsiLSB = 0x1E;
const byte LoRa_RegModemConfig1 = 0x1D;
const byte LoRa_RegModemConfig2 = 0x1E;
const byte LoRa_RegSymbTimeoutLsb = 0x1F;
const byte LoRa_RegPreambleLsb = 0x21;
const byte LoRa_RegPayloadLength = 0x22;
const byte LoRa_RegFifoRxByteAddr = 0x25;
const byte LoRa_RegModemConfig3 = 0x26;
const byte LoRa_RegPacketConfig2 = 0x31;
const byte LoRa_TXdefaultpower = 10;


void LoRa_ResetDev()
{
  digitalWrite(LoRa_Reset, LOW);		// take reset line low
  delay(5);
  digitalWrite(LoRa_Reset, HIGH);	// take it high
}


void LoRa_Write(byte LoRa_LReg, byte LoRa_LData)
{
  digitalWrite(LoRa_NSS, LOW);		// set NSS low
  SPI.transfer(LoRa_LReg | 0x80);		// mask address for write
  SPI.transfer(LoRa_LData);			// write the byte
  digitalWrite(LoRa_NSS, HIGH);			// set NSS high
}


byte LoRa_Read(byte LoRa_LReg)
{
  byte LoRa_LRegData;
  digitalWrite(LoRa_NSS, LOW);		// set NSS low
  SPI.transfer(LoRa_LReg & 0x7F);		// mask address for read
  LoRa_LRegData = SPI.transfer(0);	// read the byte
  digitalWrite(LoRa_NSS, HIGH);		// set NSS high
  return LoRa_LRegData;
}


void LoRa_SetFreq(byte LoRa_LFMsb, byte LoRa_LFMid, byte LoRa_LFLsb)
{
  LoRa_Write(LoRa_RegFrMsb, LoRa_LFMsb);
  LoRa_Write(LoRa_RegFrMid, LoRa_LFMid);
  LoRa_Write(LoRa_RegFrLsb, LoRa_LFLsb);
//  Serial.println(LoRa_LFMsb);
//  Serial.println(LoRa_LFMid);
//  Serial.println(LoRa_LFLsb);
}


void LoRa_SetFreqF(float LoRa_LFreq)
{
  //set the LoRa frequency
  byte LoRa_LFMsb, LoRa_LFMid, LoRa_LFLsb;
  long LoRa_LLongFreq;
  LoRa_LLongFreq = ((LoRa_LFreq * 1000000) / 61.03515625);
//  Serial.print("LoRa_setFreq() ");
//  Serial.print(LoRa_LFreq);
//  Serial.print(" 0x");
//  Serial.print(LoRa_LLongFreq, HEX);
//  Serial.println();
  LoRa_LFMsb = LoRa_LLongFreq >> 16;
  LoRa_LFMid = (LoRa_LLongFreq & 0x00FF00) >> 8;
  LoRa_LFLsb = (LoRa_LLongFreq & 0x0000FF);
  LoRa_SetFreq(LoRa_LFMsb, LoRa_LFMid, LoRa_LFLsb);
}


float LoRa_GetFreq()
{
  //get the current set LoRa frequency
  byte LoRa_LFMsb, LoRa_LFMid, LoRa_LFLsb;
  unsigned long LoRa_Ltemp;
  LoRa_LFMsb = LoRa_Read(LoRa_RegFrMsb);
  LoRa_LFMid = LoRa_Read(LoRa_RegFrMid);
  LoRa_LFLsb = LoRa_Read(LoRa_RegFrLsb);
//  Serial.println(LoRa_LFMsb);
//  Serial.println(LoRa_LFMid);
//  Serial.println(LoRa_LFLsb);
  LoRa_Ltemp = ((LoRa_LFMsb * 0x10000ul) + (LoRa_LFMid * 0x100ul) + LoRa_LFLsb);
  return ((LoRa_Ltemp * 61.03515625) / 1000000ul);
}


void LoRa_Setup()
{
  //initialize LoRa device registers
  LoRa_ResetDev();								// Clear all registers to default
  LoRa_Write(LoRa_RegOpMode, 0x08);				// RegOpMode, need to set to sleep mode before configure for LoRa mode
  LoRa_Write(LoRa_RegOcp, 0x0B);					// RegOcp
  LoRa_Write(LoRa_RegLna, 0x23);					// RegLna
  LoRa_Write(LoRa_RegSymbTimeoutLsb, 0xFF);		// RegSymbTimeoutLsb
  LoRa_Write(LoRa_RegPreambleLsb, 0x0C);			// RegPreambleLsb, default
  LoRa_Write(LoRa_RegFdevLsb, LoRa_Deviation);	// LSB of deviation, 5kHz
}


void LoRa_DirectSetup()
{
  //setup LoRa device for direct modulation mode
  LoRa_Write(LoRa_RegOpMode, 0x08);
  LoRa_Write(LoRa_RegPacketConfig2, 0x00);			// set continuous mode
}


void LoRa_SetModem(byte LoRa_LBW, byte LoRa_LSF, byte LoRa_LCR, byte LoRa_LHDR, byte LoRa_LLDROPT)
{
  //setup the LoRa modem parameters
  LoRa_Write(LoRa_RegOpMode, 0x08);						// RegOpMode, need to set to sleep mode before configure for LoRa mode
  LoRa_Write(LoRa_RegOpMode, 0x88);						// goto LoRa mode
  LoRa_Write(LoRa_RegModemConfig1, LoRa_LBW + LoRa_LCR + LoRa_LHDR);  // calculate value of RegModemConfig1
  LoRa_Write(LoRa_RegModemConfig2, LoRa_LSF * 16 + 7);   // calculate value of RegModemConfig2, ($07; Header indicates CRC on, RX Time-Out MSB = 11
  LoRa_Write(LoRa_RegModemConfig3, LoRa_LLDROPT);
}


void LoRa_PrintModem()
{
  //Print the LoRa modem parameters
  Serial.print("LoRa_PrintModem() ");
  Serial.print(LoRa_Read(LoRa_RegModemConfig1));
  Serial.print(" ");
  Serial.print(LoRa_Read(LoRa_RegModemConfig2));
  Serial.print(" ");
  Serial.println(LoRa_Read(LoRa_RegModemConfig3));
}


void LoRa_RXtoReady()
{
  //'puts SX1278 into listen mode and receives packet exits with packet in array LoRa_RXBUFF(256)
//  Serial.println("LoRa_RXtoReady()");
  LoRa_RXPacketL = 0;
  LoRa_RXPacketType = 0;

  LoRa_Write(LoRa_RegOpMode, 0x09);
  LoRa_Write(LoRa_RegFifoRxBaseAddr, 0x00);
  LoRa_Write(LoRa_RegFifoAddrPtr, 0x00);
  LoRa_Write(LoRa_RegIrqFlagsMask, 0x9F);                // only allow rxdone and crc error
  LoRa_Write(LoRa_RegIrqFlags, 0xFF);
  LoRa_Write(LoRa_RegOpMode, 0x8D);
  LoRa_BackGroundRSSI = LoRa_Read(LoRa_RegRssiValue);    // get the background noise level
//Serial.print("RSSI ");
//Serial.println(LoRa_BackGroundRSSI);
}

void LoRa_ReadPacket()
{
  LoRa_RXPacketL = LoRa_Read(LoRa_RegRxNbBytes) - 1;
  LoRa_PacketRSSI = LoRa_Read(LoRa_RegPktRssiValue);
  LoRa_PacketSNR = LoRa_Read(LoRa_RegPktSnrValue);
  LoRa_PacketRSSI = LoRa_Read(LoRa_RegPktRssiValue);

  LoRa_Write(LoRa_RegFifoAddrPtr,0);              // set RX FIFO ptr
    
  digitalWrite(LoRa_NSS, LOW);                     // start the burst read
  SPI.transfer(LoRa_RegFifo);               // address for burst read
  LoRa_RXPacketType = SPI.transfer(0);

  for (byte i = 0; i < LoRa_RXPacketL; i++)
  {
    LoRa_RXBUFF[i] = SPI.transfer(0);
  }
  digitalWrite(LoRa_NSS, HIGH);               // finish, turn off LoRa device
}


byte LoRa_readRXready()
{
  return LoRa_Read(LoRa_RegIrqFlags);
}


void LoRa_RXPKTInfo()
{
  //print the information for packet last received
  Serial.print("RXtype,");
  Serial.print(LoRa_RXPacketType);
  Serial.print(",Length,");
  Serial.print(LoRa_RXPacketL);
  Serial.println();

  Serial.print("RSSI -");
  Serial.print(164 - LoRa_PacketRSSI);
  Serial.print("dBm");
  Serial.println();

  Serial.print("Noise -");
  Serial.print(164 - LoRa_BackGroundRSSI);
  Serial.print("dBm");
  Serial.println();

  Serial.print("SNR ");
  if (LoRa_PacketSNR > 127)
  {
    Serial.print("-");
    Serial.print((255 - LoRa_PacketSNR) / 4);
  }
  else
  {
    Serial.print("+");
    Serial.print(LoRa_PacketSNR / 4);
  }

  Serial.print("dB");
  Serial.println();
}


byte LoRa_RXBuffPrint(byte LoRa_LPrint)
{
  //Print contents of RX buffer as ASCII,decimal or HEX
  Serial.print("LoRa_RXBuffPrint(");
  Serial.print(LoRa_LPrint);
  Serial.print(") Start>>");                           // print start marker so we can be sure where packet data starts

  for (byte i = 0; i < LoRa_RXPacketL; i++)
  {
    if (LoRa_LPrint == 0)
    {
      Serial.write(LoRa_RXBUFF[i]);
    }
    if (LoRa_LPrint == 1)
    {
      Serial.print(LoRa_RXBUFF[i]);
      Serial.print(" ");
    }

    if (LoRa_LPrint == 2)
    {
      Serial.print(LoRa_RXBUFF[i], HEX);
      Serial.print(" ");
    }
  }
  Serial.print("<<End   ");                                 // print end marker so we can be sure where packet data ends
  Serial.println();
}


byte LoRa_RXBuffToArray(char sSerialize[], byte buffsize)
{
  // fill sSerialize array with received data
//  Serial.print("LoRa_RXBuffPrint(");
//  Serial.print(") ");

  for (byte i = 0; i < buffsize; i++)
  {
    sSerialize[i] = LoRa_RXBUFF[i];
  }
}

