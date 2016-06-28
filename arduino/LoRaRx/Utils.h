/* Utils.h - a simple set of functions to serialise/deserialise GPS data */
/*
  Copyright (c) 2015 Roberto Cazzaro.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

*/

int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}


byte Serialize(char sSerialize[])
{
  /*
  //serialize 23 bytes
  //date/time into unsigned long (4 bytes)
  //Lat positive = N, negative = S; float (4)
  //Longitude positive =E, negative = W  ; float (4)
  //Altidtude: float (4)
  //HAcc, horizontal accuracy multiply by 10, unsigned int (2)
  //Speed multiply by 1,000 (in km/h), unsigned int, 65535=higher than max speed (2)
  //HDOP multiply by 100 unsigned int (2)
  //Nav=(0-7, based on table below) + 8 * (gpsSt+glonass) =  byte (gps+glonass <=31)  (1)
  
  Navigation Status Description
  NF No Fix
  DR Dead reckoning only solution
  G2 Stand alone 2D solution
  G3 Stand alone 3D solution
  D2 Differential 2D solution
  D3 Differential 3D solution
  RK Combined GPS + dead reckoning solution
  TT Time only solution
  */
  
  unsigned long lTemp;
  unsigned int iTemp;
  float fTemp;
  byte index = 0;    // points to where we serialize next into the sSerialize array
  
  // Serialize uses pointers to various types, and reads the binary values into a char array, for transfer as binary.
  // Can send a 68 character string as only 23 bytes
  char *clTemp = ( char* ) &lTemp;  // pointer to an unsigned long type
  char *ciTemp = ( char* ) &iTemp;  // pointer to unsigned int type
  char *cfTemp = ( char* ) &fTemp;  // pointer to a float type
  
  // Start serializing time/date. Converts everything to seconds
  lTemp = uBlox.sTime.substring(4, 6).toInt() + 60 * uBlox.sTime.substring(2, 4).toInt() + 3600 * uBlox.sTime.substring(0, 2).toInt() + 
            86400 * uBlox.sDate.substring(4, 6).toInt() + 2678400 * uBlox.sDate.substring(2, 4).toInt() + 32140800 * uBlox.sDate.substring(0, 2).toInt();
  for( byte i = 0 ; i < sizeof( unsigned long ) ; i++ )   // serialize time and date as unsigned long
  {
    sSerialize[i] = clTemp[i];
  }
  index = index + sizeof( unsigned long );
  
  fTemp = uBlox.Lat.toFloat();  // Latitude
  if (uBlox.NS == "S") fTemp = -fTemp ;   // Southern latitudes are negative
  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // serialize latitude as float
  {
    sSerialize[i + index] = cfTemp[i];
  }
  index = index + sizeof( float );
  
  fTemp = uBlox.Long.toFloat();  // Longitude
  if (uBlox.EW == "W") fTemp = -fTemp ;   // Southern latitudes are negative
  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // serialize longitude as float
  {
    sSerialize[i + index] = cfTemp[i];
  }
  index = index + sizeof( float );
  
  fTemp = uBlox.Alt.toFloat();  // Altitude
  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // serialize Altitude as float
  {
    sSerialize[i + index] = cfTemp[i];
  }
  index = index + sizeof( float );
  
  fTemp = uBlox.HAcc.toFloat();  // Horizontal Accuracy
  fTemp = fTemp * 10;
  if (fTemp > 65535) fTemp = 65535;   // we lose some info, any position where HAcc = 65535 means worse than 6.5Km, basically out of range
  iTemp = fTemp + 0.5;   // need to add 0.5 to avoid rounding errors in the conversion from float to int
  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // serialize HAcc as unsigned int
  {
    sSerialize[i + index] = ciTemp[i];
  }
  index = index + sizeof( unsigned int );

  fTemp = uBlox.Speed.toFloat();  // Speed in Km/h
  fTemp = fTemp * 1000;
  if (fTemp > 65535) fTemp = 65535;   // we lose some info,  any value where speed > 65535 means faster than 65 Km/h, good enough for a tracker of lost models
  iTemp = fTemp + 0.5;   // need to add 0.5 to avoid rounding errors in the conversion from float to int
  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // serialize speed as unsigned int
  {
    sSerialize[i + index] = ciTemp[i];
  }
  index = index + sizeof( unsigned int );

  fTemp = uBlox.HDOP.toFloat();  // HDOP
  fTemp = fTemp * 100;
  if (fTemp > 9999) fTemp = 9999;   // HDOP should never be higher than 99.99, i.e. 9999, but just in case
  iTemp = fTemp + 0.5;   // need to add 0.5 to avoid rounding errors in the conversion from float to int
  
  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // serialize HDOP as unsigned int
  {
    sSerialize[i + index] = ciTemp[i];
  }
  index = index + sizeof( unsigned int );

  // serialize various values into 1 byte: Nav=(0-7, based on table below) + 8 * (gpsSt+glonass) =  byte (gps+glonass is always <=31)
  iTemp = 0;   // if none of the known values is found, return 0, NF
  if (uBlox.Nav == "NF") iTemp = 0;  // NF No Fix
  if (uBlox.Nav == "DR") iTemp = 1;  // DR Dead reckoning only solution
  if (uBlox.Nav == "G2") iTemp = 2;  // G2 Stand alone 2D solution
  if (uBlox.Nav == "G3") iTemp = 3;  // G3 Stand alone 3D solution
  if (uBlox.Nav == "D2") iTemp = 4;  // D2 Differential 2D solution
  if (uBlox.Nav == "D3") iTemp = 5;  // D3 Differential 3D solution
  if (uBlox.Nav == "RK") iTemp = 6;  // RK Combined GPS + dead reckoning solution
  if (uBlox.Nav == "TT") iTemp = 7;  // TT Time only solution

  if ((uBlox.GpsSat.toInt() + uBlox.Glonass.toInt()) < 31)  // should never be more than 31 satellites (uBlox reports up to 19), but just in case
  {
    iTemp = iTemp + 8 * (uBlox.GpsSat.toInt() + uBlox.Glonass.toInt());  // actual number of satellites
  }
  else
  {
    iTemp = iTemp + 248 ;  //31 satellites
  }
  sSerialize[index] = char(iTemp);

  index++;  

  return index;   // returns last used element in the sSerialize array, current version is 23
}

void DeSerialize(char sSerialize[])
{
  /*
  //deserialize 23 bytes
  //date/time from unsigned long (4 bytes)
  //Lat positive = N, negative = S; float (4)
  //Longitude positive =E, negative = W  ; float (4)
  //Altidtude: float (4)
  //HAcc, horizontal accuracy multiply by 10, unsigned int (2)
  //Speed multiply by 1,000 (in km/h), unsigned int, 65535=higher than max speed (2)
  //HDOP multiply by 100 unsigned int (2)
  //Nav=(0-7, based on table below) + 8 * (gpsSt+glonass) =  byte (gps+glonass <=31)  (1)
  
  Navigation Status Description
  NF No Fix
  DR Dead reckoning only solution
  G2 Stand alone 2D solution
  G3 Stand alone 3D solution
  D2 Differential 2D solution
  D3 Differential 3D solution
  RK Combined GPS + dead reckoning solution
  TT Time only solution
  */
  
  unsigned long lTemp;
  unsigned int iTemp;
  float fTemp;
  byte index = 0;    // points to where we serialize next into the sSerialize array
  String sTemp;
  
  // DeSerialize uses pointers to various types, and writes the binary values into a char array, for conversion.
  // Decompresses a set of 23 binary values into the uBlox structure
  char *clTemp = ( char* ) &lTemp;  // pointer to an unsigned long type
  char *ciTemp = ( char* ) &iTemp;  // pointer to unsigned int type
  char *cfTemp = ( char* ) &fTemp;  // pointer to a float type

  for( byte i = 0 ; i < sizeof( unsigned long ) ; i++ )   // Deserialize time and date as unsigned long
  {
    clTemp[i] = sSerialize[i];
  }
  index = index + sizeof( unsigned long );  
  sTemp = "00" + String(lTemp / 32140800);  // use padding to handle 1-9
  uBlox.sDate = sTemp.substring(sTemp.length() - 2); //reduce to 2 digits
  lTemp = lTemp - (lTemp / 32140800) * 32140800;
  sTemp = "00" + String(lTemp / 2678400);  // use padding to handle 1-9
  uBlox.sDate = uBlox.sDate + sTemp.substring(sTemp.length() - 2); //reduce to 2 digits
  lTemp = lTemp - (lTemp / 2678400) * 2678400;
  sTemp = "00" + String(lTemp / 86400);  // use padding to handle 1-9
  uBlox.sDate = uBlox.sDate + sTemp.substring(sTemp.length() - 2); //reduce to 2 digits
  lTemp = lTemp - (lTemp / 86400) * 86400;  

  sTemp = "00" + String(lTemp / 3600);  // use padding to handle 1-9
  uBlox.sTime = sTemp.substring(sTemp.length() - 2); //reduce to 2 digits
  lTemp = lTemp - (lTemp / 3600) * 3600;
  sTemp = "00" + String(lTemp / 60);  // use padding to handle 1-9
  uBlox.sTime = uBlox.sTime + sTemp.substring(sTemp.length() - 2); //reduce to 2 digits
  lTemp = lTemp - (lTemp / 60) * 60;
  sTemp = "00" + String(lTemp);  // use padding to handle 1-9
  uBlox.sTime = uBlox.sTime + sTemp.substring(sTemp.length() - 2) + ".00"; //reduce to 2 digits, add ".00" to the end, since uBlox fix is only precise to the second

  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // Deserialize latitude as float
  {
    cfTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( float );    

  if (fTemp < 0)    // Negative latitude = Southern emisphere
  {
    fTemp = -fTemp;
    uBlox.NS = "S";
  }
  else uBlox.NS = "N";
  uBlox.Lat = "00000" + String(fTemp, 5);  // pad string, use 5 digits after the decimal point
  uBlox.Lat = uBlox.Lat.substring(uBlox.Lat.length() - 10); // use the rightmost 10 characters

  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // Deserialize longitude as float
  {
    cfTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( float );    

  if (fTemp < 0)   // negative longitude = Eastern
  {
    fTemp = -fTemp;
    uBlox.EW = "W";
  }
  else uBlox.EW = "E";
  uBlox.Long = "000000" + String(fTemp, 5);  // pad string, use 5 digits after the decimal point
  uBlox.Long = uBlox.Long.substring(uBlox.Long.length() - 11); // use the rightmost 11 characters
  
  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // Deserialize altitude as float
  {
    cfTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( float );  
  uBlox.Alt = String(fTemp, 3);  // use 3 digits after the decimal point

  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // Deserialize horizontal accuracy as unsigned int
  {
    ciTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( unsigned int );  
  uBlox.HAcc = String(iTemp / 10);   // store integer part
  if (iTemp != (iTemp / 10) * 10)   // check if we need to process decimal point
  {
    uBlox.HAcc = uBlox.HAcc + "." + String(iTemp - (iTemp / 10) * 10);    // add decimal part
  }

  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // Deserialize speed as unsigned int
  {
    ciTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( unsigned int );  
  uBlox.Speed = String(iTemp / 1000);   // store integer part
  if (iTemp != (iTemp / 1000) * 1000)   // check if we need to process decimal point
  {
    uBlox.Speed = uBlox.Speed + "." + String(iTemp - (iTemp / 1000) * 1000);    // add decimal part
  }

  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // Deserialize HDOP as unsigned int
  {
    ciTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( unsigned int );  
  uBlox.HDOP = String(iTemp / 100);   // store integer part
  if (iTemp != (iTemp / 100) * 100)   // check if we need to process decimal point
  {
    uBlox.HDOP = uBlox.HDOP + "." + String(iTemp - (iTemp / 100) * 100);    // add decimal part
  }

  byte(bTemp) = sSerialize[index];  // deserialize Nav and number of satellites, last character in array
  uBlox.GpsSat = String(bTemp / 8);
  uBlox.Glonass = "0";   // Glonass always zero, not sure if used. What matters is total number of satellites, now stored in GpsSat, including Glonass

  bTemp = bTemp - (bTemp / 8) * 8;
  if (bTemp == 0) uBlox.Nav = "NF";  // NF No Fix
  if (bTemp == 1) uBlox.Nav = "DR";  // DR Dead reckoning only solution
  if (bTemp == 2) uBlox.Nav = "G2";  // G2 Stand alone 2D solution
  if (bTemp == 3) uBlox.Nav = "G3";  // G3 Stand alone 3D solution
  if (bTemp == 4) uBlox.Nav = "D2";  // D2 Differential 2D solution
  if (bTemp == 5) uBlox.Nav = "D3";  // D3 Differential 3D solution
  if (bTemp == 6) uBlox.Nav = "RK";  // RK Combined GPS + dead reckoning solution
  if (bTemp == 7) uBlox.Nav = "TT";  // TT Time only solution
}


byte SerializeLLH(char sSerialize[])
{
  // Serialize only latitude, longitude and HDOP

/*
  uBlox.NS="N";
  uBlox.Lat="5130.87919";
  uBlox.EW="W";
  uBlox.Long="00011.62597";
  uBlox.HDOP="8.76";

  serialize 10 bytes
  Lat positive = N, negative = S; float (4)
  Longitude positive =E, negative = W  ; float (4)
  HDOP multiply by 100 unsigned int (2)
  */

  unsigned int iTemp;
  float fTemp;
  byte index = 0;    // points to where we serialize next into the sSerialize array
  
  // Serialize uses pointers to various types, and reads the binary values into a char array, for transfer as binary.
  char *ciTemp = ( char* ) &iTemp;  // pointer to unsigned int type
  char *cfTemp = ( char* ) &fTemp;  // pointer to a float type
  
  fTemp = uBlox.Lat.toFloat();  // Latitude
  if (uBlox.NS == "S") fTemp = -fTemp ;   // Southern latitudes are negative
  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // serialize latitude as float
  {
    sSerialize[i + index] = cfTemp[i];
  }
  index = index + sizeof( float );
  
  fTemp = uBlox.Long.toFloat();  // Longitude
  if (uBlox.EW == "W") fTemp = -fTemp ;   // Southern latitudes are negative
  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // serialize longitude as float
  {
    sSerialize[i + index] = cfTemp[i];
  }
  index = index + sizeof( float );
  
  fTemp = uBlox.HDOP.toFloat();  // HDOP
  fTemp = fTemp * 100;
  if (fTemp > 9999) fTemp = 9999;   // HDOP should never be higher than 99.99, i.e. 9999, but just in case
  iTemp = fTemp + 0.5;   // need to add 0.5 to avoid rounding errors in the conversion from float to int
  
  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // serialize HDOP as unsigned int
  {
    sSerialize[i + index] = ciTemp[i];
  }
  index = index + sizeof( unsigned int );

  return index;   // returns last used element in the sSerialize array, current version is 10
}


void DeSerializeLLH(char sSerialize[])
{
  /*
  //deserialize 10 bytes
  //Lat positive = N, negative = S; float (4)
  //Longitude positive =E, negative = W  ; float (4)
  //HDOP multiply by 100 unsigned int (2)
  */
  
  unsigned int iTemp;
  float fTemp;
  byte index = 0;    // points to where we serialize next into the sSerialize array
  String sTemp;
  
  // DeSerialize uses pointers to various types, and writes the binary values into a char array, for conversion.
  // Decompresses a set of 23 binary values into the uBlox structure
  char *ciTemp = ( char* ) &iTemp;  // pointer to unsigned int type
  char *cfTemp = ( char* ) &fTemp;  // pointer to a float type

  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // Deserialize latitude as float
  {
    cfTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( float );    

  if (fTemp < 0)    // Negative latitude = Southern emisphere
  {
    fTemp = -fTemp;
    uBlox.NS = "S";
  }
  else uBlox.NS = "N";
  uBlox.Lat = "00000" + String(fTemp, 5);  // pad string, use 5 digits after the decimal point
  uBlox.Lat = uBlox.Lat.substring(uBlox.Lat.length() - 10); // use the rightmost 10 characters

  for( byte i = 0 ; i < sizeof( float ) ; i++ )   // Deserialize longitude as float
  {
    cfTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( float );    

  if (fTemp < 0)   // negative longitude = Eastern
  {
    fTemp = -fTemp;
    uBlox.EW = "W";
  }
  else uBlox.EW = "E";
  uBlox.Long = "000000" + String(fTemp, 5);  // pad string, use 5 digits after the decimal point
  uBlox.Long = uBlox.Long.substring(uBlox.Long.length() - 11); // use the rightmost 11 characters

  for( byte i = 0 ; i < sizeof( unsigned int ) ; i++ )   // Deserialize HDOP as unsigned int
  {
    ciTemp[i] = sSerialize[i + index];
  }
  index = index + sizeof( unsigned int );  
  uBlox.HDOP = String(iTemp / 100);   // store integer part
  if (iTemp != (iTemp / 100) * 100)   // check if we need to process decimal point
  {
    uBlox.HDOP = uBlox.HDOP + "." + String(iTemp - (iTemp / 100) * 100);    // add decimal part
  }
}


void printuBlox(String toprint)
{
Serial.print(toprint);
Serial.print(uBlox.sTime);
Serial.print(", ");
Serial.print(uBlox.sDate);
Serial.print(", ");
Serial.print(uBlox.NS);
Serial.print(uBlox.Lat);
Serial.print(", ");
Serial.print(uBlox.EW);
Serial.print(uBlox.Long);
Serial.print(", ");
Serial.print(uBlox.Alt);
Serial.print(", ");
Serial.print(uBlox.Nav);
Serial.print(", ");
Serial.print(uBlox.HAcc);
Serial.print(", ");
Serial.print(uBlox.Speed);
Serial.print(", ");
Serial.print(uBlox.HDOP);
Serial.print(", ");
Serial.print(uBlox.GpsSat);
Serial.print(", ");
Serial.println(uBlox.Glonass);
}

