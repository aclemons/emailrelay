//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gprocessorfactory.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprocessorfactory.h"
#include "gnullprocessor.h"
#include "gnetworkprocessor.h"
#include "gexecutableprocessor.h"
#include "gspamprocessor.h"
#include "gfile.h"

GSmtp::ProcessorFactory::Pair GSmtp::ProcessorFactory::split( const std::string & address )
{
	std::string::size_type colon = address.find(":") ;
	bool has_prefix = colon != std::string::npos && colon >= 1U && (colon+1U) < address.length() ;
	std::string prefix = has_prefix ? address.substr(0U,colon) : std::string() ;
	std::string tail = has_prefix ? address.substr(colon+1U) : address ;
	return
		has_prefix ?
			std::make_pair(prefix,tail) :
			std::make_pair(std::string(),address) ;
}

std::string GSmtp::ProcessorFactory::check( const std::string & address )
{
	Pair p = split( address ) ;
	if( address.empty() )
	{
		return std::string() ;
	}
	else if( p.first == "spam" || p.first == "ip" || p.first == "net" )
	{
		std::string s1 , s2 ;
		return GNet::Resolver::parse(p.second,s1,s2) ? std::string() : "invalid network address" ;
	}
	else
	{
		G::Executable exe( p.second ) ;
		if( ! G::File::exists(exe.exe(),G::File::NoThrow()) )
			return "no such file" ;
		else if( ! G::File::executable(exe.exe()) )
			return "probably not executable" ;
		else if( ! exe.exe().isRelative() )
			return "not an absolute path" ;
		else
			return std::string() ;
	}
}

GSmtp::Processor * GSmtp::ProcessorFactory::newProcessor( const std::string & address , unsigned int timeout )
{
	Pair p = split( address ) ;
	bool ok = check(address).empty() ;

	if( address.empty() )
	{
		return new NullProcessor ;
	}
	else if( ok && p.first == "spam" )
	{
		return new SpamProcessor( p.second , timeout , timeout ) ;
	}
	else if( ok && p.first == "ip" || p.first == "net" )
	{
		return new NetworkProcessor( p.second , timeout , timeout ) ;
	}
	else if( ok && p.first == "file" || p.first == "exe" )
	{
		return new ExecutableProcessor( G::Executable(p.second) ) ;
	}
	else 
	{
		// if not completely valid assume it's a file-system path
		return new ExecutableProcessor( G::Executable(address) ) ;
	}
}

/// \file gprocessorfactory.cpp
