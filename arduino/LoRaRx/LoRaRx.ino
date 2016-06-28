/* LoRaRx.ino - a lost model locator based on LoRa module */
/*
  Copyright (c) 2015 Roberto Cazzaro.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
*/

// Arduino pins for LoRa DRF1278F
// Arduino pin -> DRF1278F pin, Function
// D6 -> 1 RESET
// D7 -> 13 NSS
// D11 -> 12 MOSI
// D12 -> 11 MISO 
// D13 -> 10 SCK 

// Arduino pins for 5110 LCD
// Arduino pin -> 5110 module
// D8 -> 5 D/C 
// D9 -> 4 RESET 
// D10 -> 3 SCE 
// D11 -> 6 DN (MOSI) 
// D13 -> 7 SCLK 
// LED backlight connected to VCC with a resistor

// Arduino pins for HC-06 Bluetooth module
// D2 SoftSerial Rx to HC-06 Tx
// D3 SoftSerial Tx to HC-06 Rx  

#include "uBloxLib.h";

#include "SPI.h";
#include "PCD8544_SPI.h"

// #define USE_FRAME_BUFFER   5110 libraries can work with or without frame buffer (frame buffer need 504 extra bytes in variable space, not used here)

#ifdef USE_FRAME_BUFFER
PCD8544_SPI_FB lcd;
#else
PCD8544_SPI lcd;
#endif

const byte LCD_contrast = 0xB8;    // Values around 0xB0 to 0xBF should work well

// Pin assignments
const byte LoRa_NSS = 7;   //LoRa NSS
const byte LoRa_Reset = 6;   //LoRa RESET
const byte SSer_RX = 2;  // SoftSerial Rx
const byte SSer_TX = 3;  // SoftSerial Tx

//LoRa Tx/Rx constants
const float LoRa_Freq = 434.700;   // change this to change standard frequency
const byte LoRa_TxStation = 42;    // Station ID for the Tx module
const byte LoRa_RxStation = 84;    // Station ID for the Rx module
const byte LoRa_PktSystem = 1;     // system message (main power voltage), 1 byte
const byte LoRa_PktMin = 5;        // minimum gps info (lat, long, fix/HDOP), 9 bytes
const byte LoRa_PktGPS = 10;       // messages containing full location info, 23 bytes

#include "LoRaRX.h";

uBlox_t uBlox;  // uBlox structure
#include "Utils.h";
#include "NMEA.h";
#include <SoftwareSerial.h>;  

SoftwareSerial HC06(SSer_RX, SSer_TX);  // RX, TX

bool toggle = false;  //used to toggle the antenna bitmap
unsigned long lAge = millis();
  
void setup()
{
  Serial.begin(9600);
  Serial.println("LoRa Rx");
//  Serial.println();

  pinMode(LoRa_Reset, OUTPUT);
  digitalWrite(LoRa_Reset, LOW);
  pinMode (LoRa_NSS, OUTPUT);
  digitalWrite(LoRa_NSS, HIGH);
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  LoRa_ResetDev();			//Reset LoRa
  LoRa_Setup();				//Initialize LoRa
  LoRa_SetFreqF(LoRa_Freq); 
  
//  Serial.println("Init Lora");
  LoRa_SetModem(LoRa_BW41_7, LoRa_SF12, LoRa_CR4_5, LoRa_Explicit, LoRa_LowDoptON); //Setup the LoRa modem parameters
//  LoRa_PrintModem();                            //Print the modem parameters  
  LoRa_RXtoReady();

  // initialize 5110 LCD
  lcd.begin(false);
  lcd.contrast(LCD_contrast);    // Added lcd.contrast() to PCD8544 library. Values around 0xB0 to 0xBF should work well 
  lcd.clear();

  // initialize HC-06 Bluetooth
  HC06.begin(9600);

  uBlox.sDate = "151016";     // init uBlox with dummy data to ensure initial NMEA sentences have enough data
  uBlox.sTime = "105535.00";
  uBlox.Nav = "NF";
  uBlox.Alt="1.0";
  uBlox.HAcc="999";
  uBlox.Speed="0.2";
  uBlox.GpsSat = "0";
}


void loop()
{
  char sSerialize[23];
  byte LoRa_Ltemp = LoRa_readRXready();   // is there data waiting in LoRa buffer?

  LoRa_BackGroundRSSI = LoRa_Read(LoRa_RegRssiValue); 

  if (LoRa_Ltemp == 0)    // no data, do nothing
  {
    Serial.print(".");  // debug console heartbeat
    lcd.gotoXY(0,0);
    if (toggle)
    {
      lcd.print("|");   // alternate between custom antenna, 0x7f and |
    }
    else 
    {
      lcd.print((char) 0x7f);   // alternate between custom antenna, 0x7f and |
    }

    toggle = !toggle;
    lcd.print(LoRa_BackGroundRSSI - 164);
    lcd.print("dB ");
    lcd.gotoXY(0,1);
    lcd.print("Age ");
    lcd.print((millis() - lAge) / 1000);
  }

  if (LoRa_Ltemp == 64)   // valid data, process packet
  {
    Serial.println();
    LoRa_ReadPacket();

    lcd.gotoXY(0,0);
    if (toggle)
    {
      lcd.print("|");   // alternate between custom antenna, 0x7f and |
    }
    else 
    {
      lcd.print((char) 0x7f);   // alternate between custom antenna, 0x7f and |
    }    
    lcd.print(LoRa_BackGroundRSSI - 164);
    lcd.print("dB  ");
    lcd.print(LoRa_PacketRSSI - 164);
    lcd.print("dB ");
    toggle = !toggle;

    lcd.gotoXY(0,1);
    lcd.print("Age           ");  // clear Age
    lAge = millis();
    
    switch (LoRa_RXPacketType) 
    {
      case LoRa_PktSystem:    // system message
        //TODO process system message
        break;
      case LoRa_PktMin:   // process short message (roughly 3.4 sec tx time)
        LoRa_RXBuffToArray(sSerialize, 10);
        DeSerializeLLH(sSerialize);
        break;
      case LoRa_PktGPS:   // process full message (roughly 4.5 sec tx time)
        LoRa_RXBuffToArray(sSerialize, 23);
        DeSerialize(sSerialize);
        break;
      default: 
//        Serial.println("Unrecognized packet");
      break;
    }

//LoRa_RXPKTInfo();

    HC06.flush();
    HC06.println(NMEARMC());
    HC06.println();  // workaround for Rocket Locator app
    HC06.println(NMEAGGA());

    Serial.println(NMEARMC());
    Serial.println(NMEAGGA());

    lcd.gotoXY(0,2);
    lcd.println(uBlox.NS + " " + uBlox.Lat + " ");
    lcd.gotoXY(0,3);
    lcd.println(uBlox.EW + " " + uBlox.Long + " ");
    lcd.gotoXY(0,4);  
    lcd.print("H " + uBlox.HDOP + " S " + uBlox.GpsSat + " "); 
    switch (LoRa_RXPacketType) 
    {
      case LoRa_PktMin: 
        lcd.print("S ");   //short message indicator
        break;
      case LoRa_PktGPS:
        lcd.print("L ");   //long message indicator
        break;      
    }
 
    LoRa_RXtoReady();                    // ready for next and clear flags
  }

  if (LoRa_Ltemp == 96)     // CRC error, clear flags, get ready for next packet
  {
    lcd.gotoXY(0,0);
    lcd.print("CRC ERR  ");
//    Serial.println();
//    Serial.println("CRC Error");
    LoRa_RXtoReady();                    // ready for next and clear flags
  }

  delay(1000);
}
