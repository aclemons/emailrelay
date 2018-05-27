//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gaddress.h"
#include "gresolver.h"
#include "gexecutablecommand.h"
#include "gstr.h"
#include "gfile.h"

GSmtp::FactoryParser::Result GSmtp::FactoryParser::parse( const std::string & identifier , bool allow_spam )
{
	G_DEBUG( "GSmtp::FactoryParser::parse: [" << identifier << "]" ) ;
	if( identifier.find("net:") == 0U )
	{
		return Result( "net" , G::Str::tail(identifier,":") ) ;
	}
	else if( allow_spam && identifier.find("spam:") == 0U )
	{
		return Result( "spam" , G::Str::tail(identifier,":") , 0 ) ;
	}
	else if( allow_spam && identifier.find("spam-edit:") == 0U )
	{
		return Result( "spam" , G::Str::tail(identifier,":") , 1 ) ;
	}
	else if( identifier.find("file:") == 0U )
	{
		return Result( "file" , G::Str::tail(identifier,":") ) ;
	}
	else if( identifier.find("exit:") == 0U )
	{
		return Result( "exit" , G::Str::tail(identifier,":") ) ;
	}
	else if( !identifier.empty() )
	{
		return Result( "file" , identifier ) ;
	}
	else
	{
		return Result() ;
	}
}

std::string GSmtp::FactoryParser::check( const std::string & identifier , bool allow_spam )
{
	Result p = parse( identifier , allow_spam ) ;
	if( p.first == "net" || ( allow_spam && p.first == "spam" ) )
	{
		return std::string() ;
	}
	else if( p.first == "file" )
	{
		G::Path exe = p.second ;
		if( ! G::File::exists(exe,G::File::NoThrow()) )
			return "no such file" ;
		else if( ! G::File::executable(exe) )
			return "probably not executable" ;
		else if( ! exe.isAbsolute() )
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

GSmtp::FactoryParser::Result::Result() :
	third(0)
{
}

GSmtp::FactoryParser::Result::Result( const std::string & first_ , const std::string & second_ ) :
	first(first_) ,
	second(second_) ,
	third(0)
{
}

GSmtp::FactoryParser::Result::Result( const std::string & first_ , const std::string & second_ , int third_ ) :
	first(first_) ,
	second(second_) ,
	third(third_)
{
}

/// \file gfactoryparser.cpp
