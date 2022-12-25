//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfilter.cpp
///

#include "gdef.h"
#include "gfilter.h"
#include "gstr.h"

std::string GSmtp::Filter::str( bool server_side ) const
{
	std::string part1 = response().empty() ? "ok=1" : "ok=0" ;
	std::string part2( abandoned() ? "abandon" : "" ) ;
	std::string part3( special() ? (server_side?"rescan":"break") : "" ) ;

	std::ostringstream ss ;
	ss
		<< G::Str::join( " " , part1 , part2 , part3 ) << " "
		<< "response=[" << response() << "]" ;
	if( reason() != response() )
		ss << " reason=[" << reason() << "]" ;

	return ss.str() ;
}

GSmtp::Filter::Exit::Exit( int exit_code , bool server_side ) :
	result(Result::fail) ,
	special(false)
{
	if( exit_code == 0 )
	{
		result = Result::ok ;
	}
	else if( exit_code >= 1 && exit_code < 100 )
	{
		result = Result::fail ;
	}
	else if( exit_code == 100 )
	{
		result = Result::abandon ;
	}
	else if( exit_code == 101 )
	{
		result = Result::ok ;
	}
	if( server_side )
	{
		const bool rescan = true ;
		if( exit_code == 102 )
		{
			result = Result::abandon ; special = rescan ;
		}
		else if( exit_code == 103 )
		{
			result = Result::ok ; special = rescan ;
		}
		else if( exit_code == 104 )
		{
			result = Result::fail ; special = rescan ;
		}
	}
	else // client-side
	{
		const bool stop_scanning = true ;
		if( exit_code == 102 )
		{
			result = Result::ok ; special = stop_scanning ;
		}
		else if( exit_code == 103 )
		{
			result = Result::ok ;
		}
		else if( exit_code == 104 )
		{
			result = Result::abandon ; special = stop_scanning ;
		}
		else if( exit_code == 105 )
		{
			result = Result::fail ; special = stop_scanning ;
		}
	}
}

bool GSmtp::Filter::Exit::ok() const
{
	return result == Result::ok ;
}

bool GSmtp::Filter::Exit::abandon() const
{
	return result == Result::abandon ;
}

bool GSmtp::Filter::Exit::fail() const
{
	return result == Result::fail ;
}

