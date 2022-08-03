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
#include "gstringtoken.h"
#include "gstr.h"
#include "gfile.h"
#include "glog.h"

// TODO -- restructure FactoryParser
// so that parsing and normalising are both done
// up-front and the result passed as an object
// through the various Config classes

GSmtp::FactoryParser::Result GSmtp::FactoryParser::parse( const std::string & spec , bool is_filter )
{
	return parse( spec , is_filter , is_filter , true ) ;
}

GSmtp::FactoryParser::Result GSmtp::FactoryParser::parse( const std::string & spec , bool allow_spam , bool allow_chain , bool do_check_file )
{
	G_DEBUG( "GSmtp::FactoryParser::parse: [" << spec << "]" ) ;
	if( spec.empty() )
	{
		return Result( "exit" , "0" ) ;
	}
	else if( allow_chain && spec.find(',') != std::string::npos )
	{
		for( G::StringToken t( spec , ","_sv ) ; t ; ++t )
			parse( t() , allow_spam , false , do_check_file ) ;
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
		if( do_check_file )
			checkFile( path ) ;
		return Result( "file" , path ) ;
	}
	else
	{
		if( do_check_file )
			checkFile( spec ) ;
		return Result( "file" , spec ) ;
	}
}

std::string GSmtp::FactoryParser::normalise( const std::string & spec , bool is_filter ,
	const G::Path & base_dir , const G::Path & app_dir )
{
	if( is_filter && spec.find(',') != std::string::npos )
	{
		G::StringArray results ;
		for( G::StringToken t( spec , ","_sv ) ; t ; ++t )
		{
			Result result = parse( t() , is_filter , is_filter , false ) ;
			normalise( result , is_filter , base_dir , app_dir ) ;
			results.push_back( result.first.append(1U,':').append(result.second) ) ;
		}
		return G::Str::join( "," , results ) ;
	}
	else
	{
		Result result = parse( spec , is_filter , is_filter , false ) ;
		if( result.first == "file" )
			normalise( result , is_filter , base_dir , app_dir ) ;
		return result.first.append(1U,':').append(result.second) ;
	}
}

void GSmtp::FactoryParser::normalise( Result & result , bool /*is_filter_spec*/ ,
	const G::Path & base_dir , const G::Path & app_dir )
{
	if( result.first == "file" )
	{
		if( !app_dir.empty() && result.second.find("@app") == 0U )
		{
			G::Str::replace( result.second , "@app" , app_dir.str() ) ;
		}
		else if( !base_dir.empty() && G::Path(result.second).isRelative() )
		{
			result.second = (base_dir+result.second).str() ;
		}
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

