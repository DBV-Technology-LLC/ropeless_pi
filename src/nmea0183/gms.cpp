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

GMS::GMS()
{
   Mnemonic = _T("GMS");
   Empty();
}

GMS::~GMS()
{
   Mnemonic.Empty();
   Empty();
}

void GMS::Empty( void )
{
   MarkID = 0;
   ReleaseStatus = 0;
   Battery = 0;
   SurfaceRange = 0;
   SlantRange = 0;
   Bearing = 0;
   Tilt = 0;
   SeafloorTemp = 0;
   AirPressure = 0;
   DateNum = 0.0;
}

bool GMS::Parse( const SENTENCE& sentence )
{
   /*
   ** GMS - Gear Mark Status
   **
   **        1      2             3       4            5          6       7    8            9           10      11
   **        |      |             |       |            |          |       |    |            |           |       |
   ** $--GMS,MarkID,ReleaseStatus,Battery,SurfaceRange,SlantRange,Bearing,Tilt,SeafloorTemp,AirPressure,DateNum*hh<CR><LF>
   **
   ** Field Number: 
   **  1) MarkID (16-bit)
   **  2) ReleaseStatus (8-bit)
   **  3) Battery (8-bit)
   **  4) SurfaceRange (16-bit)
   **  5) SlantRange (16-bit)
   **  6) Bearing (8-bit)
   **  7) Tilt (8-bit)
   **  8) SeafloorTemp (8-bit)
   **  9) AirPressure (8-bit)
   ** 10) DateNum (matlab datetime)
   ** 11) Checksum
   */

   /*
   ** First we check the checksum...
   */

   if ( sentence.IsChecksumBad( 11 ) == TRUE )
   {
      SetErrorMessage( _T("Invalid Checksum") );
      return( FALSE );
   } 

   MarkID         = sentence.Integer( 1 );
   ReleaseStatus  = sentence.Integer( 2 );
   Battery        = sentence.Integer( 3 );
   SurfaceRange   = sentence.Integer( 4 );
   SlantRange     = sentence.Integer( 5 );
   Bearing        = sentence.Integer( 6 );
   Tilt           = sentence.Integer( 7 );
   SeafloorTemp   = sentence.Integer( 8 );
   AirPressure    = sentence.Integer( 9 );
   DateNum        = sentence.Double( 10 );

   return( TRUE );
}

bool GMS::Write( SENTENCE& sentence )
{
   /*
   ** Let the parent do its thing
   */
   
   RESPONSE::Write( sentence );

   sentence += MarkID;
   sentence += ReleaseStatus;
   sentence += Battery;
   sentence += SurfaceRange;
   sentence += SlantRange;
   sentence += Bearing;
   sentence += Tilt;
   sentence += SeafloorTemp;
   sentence += AirPressure;
   sentence += DateNum;

   sentence.Finish();

   return( TRUE );
}

const GMS& GMS::operator = ( const GMS& source )
{
   MarkID         = source.MarkID;
   ReleaseStatus  = source.ReleaseStatus;
   Battery        = source.Battery;
   SurfaceRange   = source.SurfaceRange;
   SlantRange     = source.SlantRange;
   Bearing        = source.Bearing;
   Tilt           = source.Tilt;
   SeafloorTemp   = source.SeafloorTemp;
   AirPressure    = source.AirPressure;
   DateNum        = source.DateNum;

   return( *this );
}