//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
///
/// \file gprotocolmessage.cpp
///

#include "gdef.h"
#include "gprotocolmessage.h"
#include "gsmtpserverparser.h"

#ifndef G_LIB_SMALL
void GSmtp::ProtocolMessage::addContentLine( const std::string & line )
{
	addContent( line.data() , line.size() ) ;
	addContent( "\r\n" , 2U ) ;
}
#endif

GSmtp::ProtocolMessage::ToInfo::ToInfo( const VerifierStatus & status_in ) :
	status(status_in) ,
	utf8address(status_in.utf8address())
{
}

