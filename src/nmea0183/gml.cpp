/***************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  NMEA0183 Support Classes
 * Author:   Samuel R. Blackburn, David S. Register
 *
 ***************************************************************************
 *   Copyright (C) 2010 by Samuel R. Blackburn, David S Register           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************
 *
 *   S Blackburn's original source license:                                *
 *         "You can use it any way you like."                              *
 *   More recent (2010) license statement:                                 *
 *         "It is BSD license, do with it what you will"                   *
 */

#include "nmea0183.h"

/*
** Author: Samuel R. Blackburn
** CI$: 76300,326
** Internet: sammy@sed.csc.com
**
** You can use it any way you like.
*/

GML::GML()
{
   Mnemonic = _T("GML");
   Empty();
}

GML::~GML()
{
   Mnemonic.Empty();
   Empty();
}

void GML::Empty( void )
{
   MarkID = 0;
   MarkType = 0;
   PosStatus = 0;
   TrawlID = 0;
   TrawlNum = 0;
   Latitude = 0.0;
   Longitude = 0.0;
   Depth = 0;
   Ownership = 0;
   Source = 0;
   DateNum = 0.0;
}

bool GML::Parse( const SENTENCE& sentence )
{
   /*
   ** GML - Gear Mark Location
   **
   **        1      2        3         4       5       6         7         8     9     10  11        12     13      14
   **        |      |        |         |       |       |         |         |     |     |   |         |      |       |
   ** $--GML,MarkID,MarkType,PosStatus,TrawlID,TrawlNum,Latitude,Longitude,Depth,Ownership,Source,DateNum*hh<CR><LF>
   **
   ** Field Number: 
   **  1) MarkID (0-8 enum)
   **  2) MarkType (enum)
   **  3) PosStatus (0-5 enum)
   **  4) TrawlID (16-bit ID)
   **  5) TrawlNum (8-bit)
   **  6) Latitude (decimal degrees)
   **  7) Longitude (decimal degrees)
   **  8) Depth (16-bit)
   **  9) Ownership (8-bit)
   ** 10) Source (enum)
   ** 11) DateNum (matlab utc datetime)
   ** 12) Checksum
   **
   ** Note: Manufacturer ID is derived from MarkID internally:
   **       MarkID = [8-bit manufacturer code][24-bit serial number]
   */

   /*
   ** First we check the checksum...
   */

   if ( sentence.IsChecksumBad( 12 ) == TRUE )
   {
      SetErrorMessage( _T("Invalid Checksum") );
      return( FALSE );
   } 

   MarkID     = sentence.Integer( 1 );
   MarkType   = sentence.Integer( 2 );
   PosStatus  = sentence.Integer( 3 );
   TrawlID    = sentence.Integer( 4 );
   TrawlNum   = sentence.Integer( 5 );

    // Check for missing latitude
    if (sentence.Field(6).IsEmpty())
        Latitude = INVALID_LATLON;
    else
        Latitude = sentence.Double(6);

    // Check for missing longitude
    if (sentence.Field(7).IsEmpty())
        Longitude = INVALID_LATLON;
    else
        Longitude = sentence.Double(7);

   Depth      = sentence.Integer( 8 );

   Ownership  = sentence.Integer( 9 );
   Source     = sentence.Integer( 10 );
   DateNum    = sentence.Double( 11 );

   return( TRUE );
}

bool GML::Write( SENTENCE& sentence )
{
   /*
   ** Let the parent do its thing
   */
   
   RESPONSE::Write( sentence );

   sentence += MarkID;
   sentence += MarkType;
   sentence += PosStatus;
   sentence += TrawlID;
   sentence += TrawlNum;
   sentence += Latitude;
   sentence += Longitude;
   sentence += Depth;

   sentence += Ownership;
   sentence += Source;
   sentence += DateNum;

   sentence.Finish();

   return( TRUE );
}

const GML& GML::operator = ( const GML& source )
{
   MarkID     = source.MarkID;
   MarkType   = source.MarkType;
   PosStatus  = source.PosStatus;
   TrawlID    = source.TrawlID;
   TrawlNum   = source.TrawlNum;
   Latitude   = source.Latitude;
   Longitude  = source.Longitude;
   Depth      = source.Depth;
   Ownership  = source.Ownership;
   Source     = source.Source;
   DateNum    = source.DateNum;

   return( *this );
}