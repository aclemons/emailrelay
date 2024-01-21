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
/// \file gfilter.cpp
///

#include "gdef.h"
#include "gfilter.h"
#include "gstringview.h"
#include "gstr.h"

std::string GSmtp::Filter::str( Filter::Type type ) const
{
	std::ostringstream ss ;

	auto r = result() ;
	if( r == Result::fail )
		ss << "failed " ;
	else if( r == Result::abandon )
		ss << "ok(abandon) " ;
	else
		ss << "ok " ;

	if( special() )
		ss << ( type == Filter::Type::server ? "+rescan " : "+break " ) ;

	ss << "response=[" << response() << "]" ;

	if( reason() != response() )
		ss << " reason=[" << reason() << "]" ;

	return ss.str() ;
}

G::string_view GSmtp::Filter::strtype( Filter::Type type ) noexcept
{
	return type == Type::server ? "filter"_sv :
		( type == Type::client ? "client-filter"_sv : "routing-filter"_sv ) ;
}

GSmtp::Filter::Exit::Exit( int exit_code , Filter::Type type )
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

	bool server_side = type == Filter::Type::server ;
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

#ifndef G_LIB_SMALL
bool GSmtp::Filter::Exit::fail() const
{
	return result == Result::fail ;
}
#endif

