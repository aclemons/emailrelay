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
/// \file gspamclient.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "gstringtoken.h"
#include "gfile.h"
#include "gtest.h"
#include "gspamclient.h"
#include <sstream>

std::string GSmtp::SpamClient::m_username ;

GSmtp::SpamClient::SpamClient( GNet::ExceptionSink es , const GNet::Location & location , bool read_only ,
	unsigned int connection_timeout , unsigned int response_timeout ) :
		GNet::Client(es,location,
			GNet::Client::Config()
				.set_line_buffer_config(GNet::LineBuffer::Config::newline())
				.set_connection_timeout(connection_timeout)
				.set_response_timeout(response_timeout)) ,
		m_busy(false) ,
		m_timer(*this,&SpamClient::onTimeout,es) ,
		m_request(*this) ,
		m_response(read_only)
{
	G_LOG( "GSmtp::SpamClient::ctor: spam connection to [" << location << "]" ) ;
	G_DEBUG( "GSmtp::SpamClient::ctor: spam read/only=" << read_only ) ;
	G_DEBUG( "GSmtp::SpamClient::ctor: spam connection timeout " << connection_timeout ) ;
	G_DEBUG( "GSmtp::SpamClient::ctor: spam response timeout " << response_timeout ) ;
}

#ifndef G_LIB_SMALL
void GSmtp::SpamClient::username( const std::string & username )
{
	m_username = username ;
}
#endif

bool GSmtp::SpamClient::busy() const
{
	return m_busy ;
}

void GSmtp::SpamClient::request( const std::string & path )
{
	G_DEBUG( "GSmtp::SpamClient::request: path=" << path ) ;
	if( m_busy )
		throw Error( "protocol error" ) ;
	m_busy = true ;
	m_path = path ;
	m_timer.startTimer( 0U ) ;
}

void GSmtp::SpamClient::onTimeout()
{
	G_DEBUG( "GSmtp::SpamClient::onTimeout: connected=" << connected() ) ;
	if( connected() )
		start() ;
}

void GSmtp::SpamClient::onDelete( const std::string & )
{
}

void GSmtp::SpamClient::onSecure( const std::string & , const std::string & , const std::string & )
{
}

void GSmtp::SpamClient::onConnect()
{
	if( m_busy )
		start() ;
}

void GSmtp::SpamClient::start()
{
	m_request.send( m_path , m_username ) ;
}

void GSmtp::SpamClient::onSendComplete()
{
	while( m_request.sendMore() )
	{
		;
	}
}

bool GSmtp::SpamClient::onReceive( const char * line_data , std::size_t line_size , std::size_t , std::size_t , char )
{
	m_response.add( m_path , std::string(line_data,line_size) ) ;
	if( m_response.complete() )
		eventSignal().emit( "spam" , m_response.result() , std::string() ) ;
	return true ;
}

// ==

GSmtp::SpamClient::Request::Request( Client & client ) :
	m_client(&client) ,
	m_buffer(10240U)
{
}

void GSmtp::SpamClient::Request::send( const std::string & path , const std::string & username )
{
	G_LOG( "GSmtp::SpamClient::Request::send: spam request for [" << path << "]" ) ;
	G::File::open( m_stream , path ) ;
	if( !m_stream.good() )
		throw SpamClient::Error( "cannot read content file" , path ) ;

	std::string file_size = G::File::sizeString(path) ;
	G_DEBUG( "GSmtp::SpamClient::Request::send: spam request file size: " << file_size ) ;

	std::ostringstream ss ;
	std::string eol = "\r\n" ;
	ss << "PROCESS SPAMC/1.4" << eol ;
	if( !username.empty() )
		ss << "User: " << username << eol ;
	ss << "Content-length: " << file_size << eol ;
	ss << eol ;

	bool sent = m_client->send( ss.str() ) ;
	while( sent )
	{
		sent = sendMore() ;
	}
	G_DEBUG( "GSmtp::SpamClient::Request::send: spam sent" ) ;
}

bool GSmtp::SpamClient::Request::sendMore()
{
	m_stream.read( &m_buffer[0] , m_buffer.size() ) ; // NOLINT narrowing
	std::streamsize n = m_stream.gcount() ;
	if( n <= 0 )
	{
		G_LOG( "GSmtp::SpamClient::Request::sendMore: spam request done" ) ;
		return false ;
	}
	else
	{
		G_DEBUG( "GSmtp::SpamClient::Request::sendMore: spam request sending " << n << " bytes" ) ;
		return m_client->send( G::string_view(&m_buffer[0],static_cast<std::size_t>(n)) ) ;
	}
}

// ==

GSmtp::SpamClient::Response::Response( bool read_only ) :
	m_read_only(read_only) ,
	m_state(0) ,
	m_content_length(0U) ,
	m_size(0U)
{
}

GSmtp::SpamClient::Response::~Response()
{
	if( m_stream.is_open() )
	{
		m_stream.close() ;
		G::File::remove( m_path_tmp.c_str() , std::nothrow ) ;
	}
}

void GSmtp::SpamClient::Response::add( const std::string & path , const std::string & line )
{
	if( m_state == 0 && !ok(line) )
	{
		throw SpamClient::Error( "invalid response" , G::Str::printable(G::Str::trimmed(line,G::Str::ws())) ) ;
	}
	else if( m_state == 0 )
	{
		G_DEBUG( "GSmtp::SpamClient::Request::sendMore: spam response" ) ;
		m_path_final = path ;
		m_path_tmp = path + ".spamd" ;
		if( !m_read_only && !m_stream.is_open() )
		{
			G::File::open( m_stream , m_path_tmp ) ;
			if( !m_stream.good() )
				throw SpamClient::Error( "cannot write temporary content file" , m_path_tmp ) ;
		}
		m_content_length = m_size = 0U ;
		m_state = 1 ;
	}
	if( m_state == 1 ) // spamc/spamd headers
	{
		G_LOG( "GSmtp::SpamClient::Response::add: spam response line: ["
			<< G::Str::printable(G::Str::trimmed(line,G::Str::ws())) << "]" ) ;
		if( line.find("Spam:") == 0U )
			m_result = G::Str::trimmed( line.substr(5U) , G::Str::ws() ) ;
		else if( G::Str::imatch(line.substr(0U,15U),"Content-length:") )
			m_content_length = G::Str::toUInt( G::Str::trimmed(line.substr(15U),G::Str::ws()) ) ;
		else if( ( line.empty() || line == "\r" ) && m_content_length == 0U )
			throw SpamClient::Error( "invalid response headers" ) ;
		else if( line.empty() || line == "\r" )
			m_state = 2 ;
	}
	else if( m_state == 2 ) // email content
	{
		m_size += ( line.size() + 1U ) ;

		if( m_stream.is_open() )
			m_stream << line << "\n" ;

		if( m_size >= m_content_length )
		{
			if( m_size != m_content_length )
				G_WARNING( "GSmtp::SpamClient::Response::add: incorrect content length in spam response" ) ;
			G_LOG( "GSmtp::SpamClient::add: spam response size: " << m_content_length ) ;

			if( m_stream.is_open() )
			{
				m_stream.close() ;
				if( m_stream.fail() )
					throw SpamClient::Error( "cannot write temporary content file" , m_path_tmp ) ;

				G::File::remove( m_path_final ) ;
				G::File::rename( m_path_tmp , m_path_final ) ;
			}

			m_state = 3 ;
		}
	}
}

bool GSmtp::SpamClient::Response::complete() const
{
	return m_state == 3 ;
}

bool GSmtp::SpamClient::Response::ok( const std::string & line ) const
{
	// eg. "SPAMD/1.0 99 Timeout", "SPAMD/1.1 0 OK"
	if( line.empty() ) return false ;
	if( line.find("SPAMD/") != 0U ) return false ;
	G::string_view line_sv( line ) ;
	G::StringTokenView t( line_sv , G::Str::ws() ) ;
	++t ;
	return t.valid() ? ( t() == "0"_sv ) : false ;
}

std::string GSmtp::SpamClient::Response::result() const
{
	if( G::Str::imatch(m_result.substr(0U,5U),"False") )
		return std::string() ;
	else
		return m_result ; // eg. "True ; 4.5 / 5.0"
}

