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
// gfactoryparser.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gfactoryparser.h"
#include "gresolver.h"
#include "gexecutable.h"
#include "gstr.h"
#include "gfile.h"

std::pair<std::string,std::string> GSmtp::FactoryParser::parse( const std::string & address , 
	const std::string & extra_net_prefix )
{
	G_DEBUG( "GSmtp::FactoryParser::parse: [" << address << "] [" << extra_net_prefix << "]" ) ;
	if( address.find("ip:") == 0U || address.find("net:") == 0U )
	{
		return std::make_pair( std::string("net") , G::Str::tail(address,address.find(":")) ) ;
	}
	else if( address.find(extra_net_prefix+":") == 0U )
	{
		return std::make_pair( extra_net_prefix , G::Str::tail(address,address.find(":")) ) ;
	}
	else if( address.find("file:") == 0U || address.find("exe:") == 0U )
	{
		return std::make_pair( std::string("file") , G::Str::tail(address,address.find(":")) ) ;
	}
	else if( address.find("exit:") == 0U )
	{
		return std::make_pair( std::string("exit") , G::Str::tail(address,address.find(":")) ) ;
	}
	else if( address.empty() )
	{
		return std::make_pair( std::string() , std::string() ) ;
	}
	else
	{
		return std::make_pair( std::string("file") , address ) ;
	}
}

std::string GSmtp::FactoryParser::check( const std::string & address , const std::string & extra_net_prefix )
{
	std::pair<std::string,std::string> p = parse( address , extra_net_prefix ) ;
	if( p.first == "net" || p.first == extra_net_prefix )
	{
		std::string s1 , s2 ;
		if( ! GNet::Resolver::parse(p.second,s1,s2) )
			return std::string() + "invalid network address: " + p.second ;
		else
			return std::string() ;
	}
	else if( p.first == "file" )
	{
		G::Executable exe( p.second ) ;
		if( ! G::File::exists(exe.exe(),G::File::NoThrow()) )
			return "no such file" ;
		else if( ! G::File::executable(exe.exe()) )
			return "probably not executable" ;
		else if( ! exe.exe().isAbsolute() )
			return "not an absolute path" ;
		else
			return std::string() ;
	}
	else if( p.first == "exit" )
	{
		if( !G::Str::isUInt(p.second) )
			return "not a numeric exit code" ;
		else
			return std::string() ;
	}
	else
	{
		return std::string() ;
	}
}

/// \file gfactoryparser.cpp
