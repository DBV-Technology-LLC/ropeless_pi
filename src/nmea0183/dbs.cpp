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

DBS::DBS()
{
   Mnemonic = _T("DBS");
   Empty();
}

DBS::~DBS()
{
   Mnemonic.Empty();
   Empty();
}

void DBS::Empty( void )
{
   DeckboxID.Empty();
   DeckboxManuf.Empty();
   AcousticStatus.Empty();
   CloudStatus.Empty();
   NumDevices = 0;
}

bool DBS::Parse( const SENTENCE& sentence )
{
   /*
   ** DBS - Ropeless Deckbox Status
   **
   **        1         2            3              4           5       6
   **        |         |            |              |           |       |
   ** $--DBS,DeckboxID,DeckboxManuf,AcousticStatus,CloudStatus,NumDevices*hh<CR><LF>
   **
   ** Field Number: 
   **  1) Deckbox ID
   **  2) Deckbox Manufacturer
   **  3) Acoustic Status
   **  4) Cloud Status
   **  5) Number of Devices
   **  6) Checksum
   */

   /*
   ** First we check the checksum...
   */

   if ( sentence.IsChecksumBad( 6 ) == TRUE )
   {
      SetErrorMessage( _T("Invalid Checksum") );
      return( FALSE );
   } 

   DeckboxID      = sentence.Field( 1 );
   DeckboxManuf   = sentence.Field( 2 );
   AcousticStatus = sentence.Field( 3 );
   CloudStatus    = sentence.Field( 4 );
   NumDevices     = sentence.Integer( 5 );

   return( TRUE );
}

bool DBS::Write( SENTENCE& sentence )
{
   /*
   ** Let the parent do its thing
   */
   
   RESPONSE::Write( sentence );

   sentence += DeckboxID;
   sentence += DeckboxManuf;
   sentence += AcousticStatus;
   sentence += CloudStatus;
   sentence += NumDevices;

   sentence.Finish();

   return( TRUE );
}

const DBS& DBS::operator = ( const DBS& source )
{
   DeckboxID      = source.DeckboxID;
   DeckboxManuf   = source.DeckboxManuf;
   AcousticStatus = source.AcousticStatus;
   CloudStatus    = source.CloudStatus;
   NumDevices     = source.NumDevices;

   return( *this );
}