//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gverifierstatus.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gverifier.h"
#include "gverifierstatus.h"
#include "gstr.h"

GSmtp::VerifierStatus::VerifierStatus() :
	is_valid(false) ,
	is_local(false) ,
	temporary(false)
{
}

GSmtp::VerifierStatus::VerifierStatus( const std::string & mbox ) :
	is_valid(true) ,
	is_local(false) ,
	temporary(false) ,
	address(mbox)
{
}

GSmtp::VerifierStatus GSmtp::VerifierStatus::parse( const std::string & line , std::string & mbox )
{
	try
	{
		std::string sep( 1U , '|' ) ;
		VerifierStatus s ;
		G::StringArray part ;
		G::Str::splitIntoFields( line , part , sep ) ;
		mbox = part[0U] ;
		s.is_valid = part[1U] == "1" ;
		s.is_local = part[2U] == "1" ;
		s.temporary = part[3U] == "1" ;
		s.full_name = part[4U] ;
		s.address = part[5U] ;
		s.reason = part[6U] ;
		s.help = part[7U] ;
		return s ;
	}
	catch( std::exception & )
	{
		G_ERROR( "GSmtp::VerifierStatus::parse: invalid stringised status: [" << line << "]" ) ;
		throw ;
	}
}

std::string GSmtp::VerifierStatus::str( const std::string & mbox ) const
{
	std::string sep( 1U , '|' ) ;
	std::string t( "1" ) ;
	std::string f( "0" ) ;
	return 
		mbox + sep + 
		(is_valid?t:f) + sep + 
		(is_local?t:f) + sep +
		(temporary?t:f) + sep +
		full_name + sep +
		address + sep + 
		reason + sep +
		help ;
}

/// \file gverifierstatus.cpp
