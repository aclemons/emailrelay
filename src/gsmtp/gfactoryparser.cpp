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
#include "glocation.h"
#include "gstringtoken.h"
#include "gstr.h"
#include "gfile.h"
#include "glog.h"

GSmtp::FactoryParser::Result GSmtp::FactoryParser::parse( const std::string & spec , bool is_filter ,
	const G::Path & base_dir , const G::Path & app_dir , G::StringArray * warnings_p )
{
	bool allow_spam = is_filter ;
	bool allow_chain = is_filter ;
	return parseImp( spec , is_filter , base_dir , app_dir , warnings_p , allow_spam , allow_chain ) ;
}

GSmtp::FactoryParser::Result GSmtp::FactoryParser::parseImp( const std::string & spec , bool is_filter ,
	const G::Path & base_dir , const G::Path & app_dir , G::StringArray * warnings_p ,
	bool allow_spam , bool allow_chain )
{
	G_DEBUG( "GSmtp::FactoryParser::parse: [" << spec << "]" ) ;
	Result result ;
	if( spec.empty() )
	{
		result = Result( "exit" , "0" ) ;
	}
	else if( allow_chain && spec.find(',') != std::string::npos )
	{
		std::string new_spec ;
		std::string error ;
		for( G::StringToken t( spec , ","_sv ) ; t ; ++t )
		{
			// (one level of recursion)
			Result sub_result = parseImp( t() , is_filter , base_dir , app_dir , warnings_p , allow_spam , false ) ;
			if( sub_result.first.empty() )
				error = sub_result.second ;
			new_spec.append(",",new_spec.empty()?0U:1U).append(sub_result.first).append(1U,':').append(sub_result.second) ;
		}
		result = error.empty() ? Result( "chain" , new_spec ) : Result( "" , error ) ;
	}
	else if( spec.find("net:") == 0U )
	{
		result = Result( "net" , G::Str::tail(spec,":") ) ;
	}
	else if( allow_spam && spec.find("spam:") == 0U )
	{
		result = Result( "spam" , G::Str::tail(spec,":") , 0 ) ;
	}
	else if( allow_spam && spec.find("spam-edit:") == 0U )
	{
		result = Result( "spam" , G::Str::tail(spec,":") , 1 ) ;
	}
	else if( spec.find("exit:") == 0U )
	{
		result = Result( "exit" , G::Str::tail(spec,":") ) ;
	}
	else if( spec.find("file:") == 0U )
	{
		result = Result( "file" , G::Str::tail(spec,":") ) ;
	}
	else
	{
		result = Result( "file" , spec ) ;
	}
	normalise( result , base_dir , app_dir ) ;
	check( result , is_filter , warnings_p ) ;
	G_DEBUG( "GSmtp::FactoryParser::parse: [" << spec << "] -> [" << result.first << "],[" << result.second << "]" ) ;
	return result ;
}

void GSmtp::FactoryParser::normalise( Result & result , const G::Path & base_dir , const G::Path & app_dir )
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

void GSmtp::FactoryParser::check( Result & result , bool is_filter , G::StringArray * warnings_p )
{
	if( result.first == "chain" )
	{
		// no-op -- sub-parts already checked
	}
	else if( result.first == "file" )
	{
		if( result.second.empty() )
		{
			result.first.clear() ;
			result.second = "empty file path" ;
		}
		else if( warnings_p && !G::File::exists(result.second) )
		{
			warnings_p->push_back( std::string(is_filter?"filter":"verifier")
				.append(" program does not exist: ").append(result.second) ) ;
		}
		else if( warnings_p && G::File::isDirectory(result.second,std::nothrow) )
		{
			warnings_p->push_back( std::string("invalid program: ").append(result.second) ) ;
		}
	}
	else if( result.first == "exit" )
	{
		if( !G::Str::isUInt(result.second) )
		{
			result.first.clear() ;
			result.second = "not a numeric exit code: " + G::Str::printable(result.second) ;
		}
	}
	else if( result.first == "net" || result.first == "spam" )
	{
		try
		{
			GNet::Location::nosocks( result.second ) ;
		}
		catch( std::exception & e )
		{
			result.first.clear() ;
			result.second = e.what() ;
		}
	}
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

