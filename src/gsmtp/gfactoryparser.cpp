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
/// \file gfactoryparser.cpp
///

#include "gdef.h"
#include "gfactoryparser.h"
#include "gaddress.h"
#include "gresolver.h"
#include "gexecutablecommand.h"
#include "gstr.h"
#include "gfile.h"
#include "glog.h"

GSmtp::FactoryParser::Result GSmtp::FactoryParser::parse( const std::string & spec , bool allow_spam , bool allow_chain )
{
	G_DEBUG( "GSmtp::FactoryParser::parse: [" << spec << "]" ) ;
	if( spec.empty() )
	{
		return Result( "exit" , "0" ) ;
	}
	else if( allow_chain && spec.find(',') != std::string::npos )
	{
		G::StringArray parts = G::Str::splitIntoTokens( spec , {",",1U} ) ;
		for( const auto & sub : parts )
			parse( sub , allow_spam , false ) ;
		return Result( "chain" , spec ) ;
	}
	else if( spec.find("net:") == 0U )
	{
		return Result( "net" , G::Str::tail(spec,":") ) ;
	}
	else if( allow_spam && spec.find("spam:") == 0U )
	{
		return Result( "spam" , G::Str::tail(spec,":") , 0 ) ;
	}
	else if( allow_spam && spec.find("spam-edit:") == 0U )
	{
		return Result( "spam" , G::Str::tail(spec,":") , 1 ) ;
	}
	else if( spec.find("exit:") == 0U )
	{
		return Result( "exit" , checkExit(G::Str::tail(spec,":")) ) ;
	}
	else if( spec.find("file:") == 0U )
	{
		std::string path = G::Str::tail( spec , ":" ) ;
		checkFile( path ) ;
		return Result( "file" , path ) ;
	}
	else
	{
		checkFile( spec ) ;
		return Result( "file" , spec ) ;
	}
}

void GSmtp::FactoryParser::checkFile( const G::Path & exe )
{
	if( !G::File::exists(exe,std::nothrow) )
		throw Error( "no such file" , G::Str::printable(exe.str()) ) ;
	else if( !G::is_windows() && !G::File::isExecutable(exe,std::nothrow) )
		throw Error( "probably not executable" , G::Str::printable(exe.str()) ) ;
	else if( !exe.isAbsolute() )
		throw Error( "not an absolute path" , G::Str::printable(exe.str()) ) ;
}

std::string GSmtp::FactoryParser::checkExit( const std::string & s )
{
	if( !G::Str::isUInt(s) )
		throw Error( "not a numeric exit code" , G::Str::printable(s) ) ;
	return s ;
}

// ==

GSmtp::FactoryParser::Result::Result()
= default ;

GSmtp::FactoryParser::Result::Result( const std::string & first_ , const std::string & second_ ) :
	first(first_) ,
	second(second_)
{
}

GSmtp::FactoryParser::Result::Result( const std::string & first_ , const std::string & second_ , int third_ ) :
	first(first_) ,
	second(second_) ,
	third(third_)
{
}

