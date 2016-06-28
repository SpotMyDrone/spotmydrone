/* NMEA.h - a simple set of functions to recreate NMEA sentences from GPS data */
/*
  Copyright (c) 2015 Roberto Cazzaro.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

*/

String NMEAchk(String sMsg) 
{
  int check = 0;
  // iterate over the string, XOR each byte with the previous data
  for (int c = 0; c < sMsg.length(); c++) 
  {
    check = char(check ^ sMsg.charAt(c));
  } 
  String sTmp = "*";
  if (check < 0x10) sTmp = "*0"; //leading 0 for hex conversion
  sTmp = sTmp + String(check,HEX);
  sTmp.toUpperCase();
  return sTmp;
}

String NMEARMC()
{
/*RMC - NMEA has its own version of essential gps pvt (position, velocity, time) data. It is called RMC, The Recommended Minimum
  
  NMEA: $xxRMC,time,status,lat,NS,long,EW,spd,cog,date,mv,mvEW,posMode*cs<CR><LF>
  PUBX: $PUBX,00,time,lat,NS,long,EW,altRef,navStat,hAcc,vAcc,SOG,COG,vVel,diffAge,HDOP,VDOP,TDOP,numSvs,reserved,DR,*cs<CR><LF>

  Mapping: time, lat, long, NS EW, spd=SOG, COG, 

  Status must be built from DR. NF/DR/TT=V; G2, G3, D2, D3, RK=A 
  Date: must poll Time Of day (below), nmea format ddmmyy, same as UBX
  Mv and mvEW blank, not supported
  PosMode:  NF/TT=N; DR=E; G2, G3, D3, D3, RK=D
*/

  String sTmp = "GPRMC,";
  sTmp = sTmp + uBlox.sTime + ",";

  if (uBlox.Nav == "NF" || uBlox.Nav == "DR" || uBlox.Nav == "TT")   // NMEA uses V or A (Void or Active)
  {
    sTmp = sTmp + "V,";
  }
  else sTmp = sTmp + "A,";
  sTmp = sTmp + uBlox.Lat + "," + uBlox.NS + "," + uBlox.Long + "," + uBlox.EW + ",";
  sTmp = sTmp + uBlox.Speed + ",0," + uBlox.sDate + ",0,W";  
  
/*  if (uBlox.Nav == "NF" || uBlox.Nav == "TT")
  {
    sTmp = sTmp + "N";
  }
  else if (uBlox.Nav == "DR")
  {
    sTmp = sTmp + "E";
  }
  else sTmp = sTmp + "D";
*/  
  sTmp = "$" + sTmp + NMEAchk(sTmp)+" ";

  return sTmp;
}

String NMEAGGA()
{
/*GGA - essential fix data which provide 3D location and accuracy data.
 $xxGGA,time,lat,NS,long,EW,quality,numSV,HDOP,alt,M,sep,M,diffAge,diffStation*cs<CR><LF>
 GGA: quality 0=No Fix, 1=Autonomous GNSS Fix, 2=Differential GNSS Fix, 6=Estimated/Dead Reckoning Fix
 Quality mapping:  NF/TT=0; DR=6; G2, G3, D3, D3, RK=1
 numSV + GPS+Glonass
 M is literal M, for Meters
 se is geoid height, put at 0
 siffAge and diffStation = blank
*/

  String sTmp = "GPGGA,";
  sTmp = sTmp + uBlox.sTime + ",";
  sTmp = sTmp + uBlox.Lat + "," + uBlox.NS + "," + uBlox.Long + "," + uBlox.EW + ",";

  if (uBlox.Nav == "NF" || uBlox.Nav == "TT")
  {
    sTmp = sTmp + "0,";
  }
  else if (uBlox.Nav == "DR")
  {
    sTmp = sTmp + "6,";
  }
  else sTmp = sTmp + "1,";
  int NumSat = uBlox.GpsSat.toInt() + uBlox.Glonass.toInt();
  sTmp = sTmp + NumSat + "," + uBlox.HDOP + "," + uBlox.Alt + ",M,0,M,,";

  sTmp = "$"+ sTmp + NMEAchk(sTmp);

  return sTmp;
}

