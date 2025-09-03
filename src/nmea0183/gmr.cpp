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

GMR::GMR()
{
   Mnemonic = _T("GMR");
   Empty();
}

GMR::~GMR()
{
   Mnemonic.Empty();
   Empty();
}

void GMR::Empty( void )
{
   CmdUID = 0;
   SourceID = 0;
   TargetID = 0;
   MarkID = 0;
   CmdType = 0;
   ResCode = 0;
   Param1 = 0;
   Param2 = 0;
}

bool GMR::Parse( const SENTENCE& sentence )
{
   /*
   ** GMR - Gear Mark Request/Response
   **
   **        1      2        3        4      5       6       7      8      9
   **        |      |        |        |      |       |       |      |      |
   ** $--GMR,CmdUID,SourceID,TargetID,MarkID,CmdType,ResCode,Param1,Param2*hh<CR><LF>
   **
   ** Field Number: 
   **  1) CmdUID (16-bit)
   **  2) SourceID (16-bit)
   **  3) TargetID (16-bit)
   **  4) MarkID (16-bit)
   **  5) CmdType (enum)
   **  6) ResCode (16-bit)
   **  7) Param1 (16-bit)
   **  8) Param2 (16-bit)
   **  9) Checksum
   */

   /*
   ** First we check the checksum...
   */

   if ( sentence.IsChecksumBad( 9 ) == TRUE )
   {
      SetErrorMessage( _T("Invalid Checksum") );
      return( FALSE );
   } 

   CmdUID    = sentence.Integer( 1 );
   SourceID  = sentence.Integer( 2 );
   TargetID  = sentence.Integer( 3 );
   MarkID    = sentence.Integer( 4 );
   CmdType   = sentence.Integer( 5 );
   ResCode   = sentence.Integer( 6 );
   Param1    = sentence.Integer( 7 );
   Param2    = sentence.Integer( 8 );

   return( TRUE );
}

bool GMR::Write( SENTENCE& sentence )
{
   /*
   ** Let the parent do its thing
   */
   
   RESPONSE::Write( sentence );

   sentence += CmdUID;
   sentence += SourceID;
   sentence += TargetID;
   sentence += MarkID;
   sentence += CmdType;
   sentence += ResCode;
   sentence += Param1;
   sentence += Param2;

   sentence.Finish();

   return( TRUE );
}

const GMR& GMR::operator = ( const GMR& source )
{
   CmdUID    = source.CmdUID;
   SourceID  = source.SourceID;
   TargetID  = source.TargetID;
   MarkID    = source.MarkID;
   CmdType   = source.CmdType;
   ResCode   = source.ResCode;
   Param1    = source.Param1;
   Param2    = source.Param2;

   return( *this );
}