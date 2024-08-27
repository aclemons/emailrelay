//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gmessageidfilter.cpp
///

#include "gdef.h"
#include "gmessageidfilter.h"
#include "gstr.h"
#include "gstringview.h"
#include "groot.h"
#include "gfile.h"
#include "gdatetime.h"
#include "gprocess.h"
#include <fstream>
#include <sstream>

GFilters::MessageIdFilter::MessageIdFilter( GNet::EventState es , GStore::FileStore & store ,
	Filter::Type filter_type , const Filter::Config & config , const std::string & ) :
		SimpleFilterBase(es,filter_type,"msgid:") ,
		m_store(store) ,
		m_domain(config.domain)
{
}

GSmtp::Filter::Result GFilters::MessageIdFilter::run( const GStore::MessageId & message_id ,
	bool & , GStore::FileStore::State )
{
	std::string e = process( m_store.contentPath(message_id) , m_domain ) ;
	if( !e.empty() )
		throw Error( e ) ;
	return Result::ok ;
}

std::string GFilters::MessageIdFilter::process( const G::Path & path_in , const std::string & domain )
{
	std::ifstream in ;
	{
		G::Root claim_root ;
		G::File::open( in , path_in ) ;
	}
	if( !in.good() )
		return "open error" ;

	bool have_id = false ;
	{
		static constexpr std::size_t line_limit = 10000U ;
		std::string line ;
		line.reserve( 2000U ) ;
		while( G::Str::readLine( in , line , "\n" , true , line_limit ) && line.size() > 1U && !have_id )
			have_id = isId( line ) ;
		if( !in.good() )
			return "format error" ; // eg. no empty line, line too long
	}

	if( !have_id )
	{
		G::Path path_out = path_in.str().append(".tmp") ;
		std::ofstream out ;
		{
			G::Root claim_root ;
			G::File::open( out , path_out ) ;
		}
		if( !out.good() )
			return "create error" ;

		out << "Message-ID: " << newId(domain) << "\r\n" ;
		in.seekg( 0 ) ;
		G::File::copy( in , out ) ;

		in.close() ;
		out.close() ;
		if( !in.eof() )
			return "read error" ;
		if( out.fail() )
			return "write error" ;

		bool ok = false ;
		{
			G::Root claim_root ;
			ok = G::File::renameOnto( path_out , path_in , std::nothrow ) ;
		}
		if( !ok )
			return "rename error" ;
	}
	return {} ;
}

bool GFilters::MessageIdFilter::isId( std::string_view line ) noexcept
{
	return line.find(':') != std::string::npos && G::sv_imatch( G::sv_substr_noexcept(line,0U,line.find(':')) , "message-id" ) ;
}

std::string GFilters::MessageIdFilter::newId( const std::string & domain )
{
	std::ostringstream ss ;
	G::SystemTime now = G::SystemTime::now() ;
	static int generator = 0 ;
	ss << "<"
		<< now.s() << "." << now.us() << "."
		<< G::Process::Id().str() << "." << generator++
		<< "@" << domain << ">" ;
	return ss.str() ;
}

