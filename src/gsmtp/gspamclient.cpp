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
// gspamclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gstr.h"
#include "gfile.h"
#include "gspamclient.h"
#include "gassert.h"

GSmtp::SpamClient::SpamClient( const GNet::ResolverInfo & resolver_info ,
	unsigned int connect_timeout , unsigned int response_timeout ) :
		GNet::Client(resolver_info,connect_timeout,response_timeout,0U,"\n") ,
		m_in_size(0UL) ,
		m_in_lines(0UL) ,
		m_out_size(0UL) ,
		m_out_lines(0UL) ,
		m_header_out_index(0U) ,
		m_timer(*this,&SpamClient::onTimeout,*this)
{
	G_DEBUG( "GSmtp::SpamClient::ctor: " << resolver_info.displayString() << ": " 
		<< connect_timeout << " " << response_timeout ) ;
}

GSmtp::SpamClient::~SpamClient()
{
}

bool GSmtp::SpamClient::busy() const
{
	return m_in.get() != NULL || m_out.get() != NULL ;
}

void GSmtp::SpamClient::onDelete( const std::string & , bool )
{
}

void GSmtp::SpamClient::onDeleteImp( const std::string & reason , bool b )
{
	// we have to override onDeleteImp() rather than onDelete() so that we 
	// can get in early enough to guarantee that every request gets a response

	if( !reason.empty() )
		G_WARNING( "GSmtp::SpamClient::onDeleteImp: error: " << reason ) ;

	if( busy() )
	{
		m_in <<= 0 ;
		m_out <<= 0 ;
		eventSignal().emit( "spam" , m_out_size >= headerBodyLength() ? headerResult() : reason ) ;
	}
	Base::onDeleteImp( reason , b ) ; // use typedef because of ms compiler bug
}

void GSmtp::SpamClient::request( const std::string & path )
{
	G_DEBUG( "GSmtp::SpamClient::request: \"" << path << "\"" ) ;
	if( busy() ) 
		throw ProtocolError() ;

	m_path = path ;
	m_in <<= new std::ifstream( path.c_str() , std::ios_base::binary | std::ios_base::in ) ;
	m_in_lines = 0UL ;
	m_in_size = 0UL ;

	std::string username = "spam" ; // TODO -- configurable username in SPAMC protocol? environment variable?
	m_header_out.push_back( std::string() + "PROCESS SPAMC/1.4" ) ;
	m_header_out.push_back( std::string() + "User: " + username ) ;
	m_header_out.push_back( std::string() + "Content-length: " + G::File::sizeString(m_path) ) ;
	m_header_out.push_back( std::string() ) ;
	m_header_out_index = 0U ;

	m_timer.startTimer( 0U ) ;
	m_header_in.clear() ;
}

void GSmtp::SpamClient::onConnect()
{
	G_DEBUG( "GSmtp::SpamClient::onConnect" ) ;
	if( busy() )
		sendContent() ;
}

void GSmtp::SpamClient::onTimeout()
{
	if( connected() )
		sendContent() ;
}

void GSmtp::SpamClient::onSendComplete()
{
	sendContent() ;
}

void GSmtp::SpamClient::sendContent()
{
	std::string line ;
	while( nextContentLine(line) )
	{
		if( !send( line + "\r\n" ) )
			break ;
	}
}

bool GSmtp::SpamClient::nextContentLine( std::string & line )
{
	bool ok = false ;
	if( m_in.get() != NULL )
	{
		if( m_header_out_index < m_header_out.size() )
		{
			line = m_header_out[m_header_out_index++] ;
			G_LOG( "GSmtp::SpamClient::sendContent: spam>>: \"" << G::Str::printable(line) << "\"" ) ;
			ok = true ;
		}
		else
		{
			std::istream & stream = *(m_in.get()) ;
			if( stream.good() )
			{
				G::Str::readLineFrom( stream , "\r\n" , line ) ;
				ok = !! stream ;
			}
			if( ok )
			{
				m_in_lines++ ;
				m_in_size += ( line.length() + 2U ) ;
			}
			else
			{
				G_LOG( "GSmtp::SpamClient::addBody: spam>>: [" << m_in_lines 
						<< " lines of body text, " << m_in_size << " bytes]" ) ;

				// stop sending, start receiving
				turnRound() ; 
			}
		}
	}
	return ok ;
}

void GSmtp::SpamClient::turnRound()
{
	// send eof
	socket().shutdown() ; 

	// close content file for reading, reopen for writing (keeping busy() true)
	m_in <<= 0 ;
	m_out <<= new std::ofstream( m_path.c_str() , std::ios_base::binary | std::ios_base::out | std::ios_base::trunc ) ;
	m_out_size = 0UL ;
	m_out_lines = 0UL ;
}

void GSmtp::SpamClient::onSecure( const std::string & )
{
}

bool GSmtp::SpamClient::onReceive( const std::string & line )
{
	if( m_in.get() != NULL )
		throw ProtocolError( G::Str::printable(line) ) ; // the spamd has sent a response too early

	if( ! haveCompleteHeader() )
		addHeader( line ) ;
	else
		addBody( line ) ;

	return true ;
}

bool GSmtp::SpamClient::haveCompleteHeader() const
{
	return m_header_in.size() != 0U && m_header_in.back().empty() ;
}

void GSmtp::SpamClient::addHeader( const std::string & line )
{
	G_LOG( "GSmtp::SpamClient::onReceive: spam<<: \"" << G::Str::printable(G::Str::trimmed(line,"\r\n")) << "\"" ) ;
	m_header_in.push_back( G::Str::trimmed(line,"\r\n") ) ;
	m_n = 0UL ;
}

void GSmtp::SpamClient::addBody( const std::string & line )
{
	if( m_out.get() != NULL && m_out_size < headerBodyLength() )
	{
		std::ostream & out = *m_out.get() ;
		out << line << "\n" ;
		m_out_size += line.length() + 1U ;
		m_out_lines++ ;

		if( m_out_size >= headerBodyLength() )
			G_LOG( "GSmtp::SpamClient::addBody: spam<<: [" << m_out_lines 
				<< " lines of body text, " << m_out_size << " bytes]" ) ;
	}
}

std::string GSmtp::SpamClient::headerResult() const
{
	return G::Str::lower( part(headerLine("Spam:"),1U,"true") ) == "true" ? "marked as spam" : std::string() ;
}

std::string GSmtp::SpamClient::part( const std::string & line , unsigned int i , const std::string & default_ ) const
{
	StringArray part ;
	G::Str::splitIntoTokens( line , part , "\t :" ) ;
	return part.size() > i ? part[i] : default_ ;
}

unsigned long GSmtp::SpamClient::headerBodyLength() const
{
	if( m_n == 0UL )
		(const_cast<SpamClient*>(this))->m_n = G::Str::toULong( part(headerLine("Content-length:"),1U,"0") ) ;
	return m_n ;
}

std::string GSmtp::SpamClient::headerLine( const std::string & key , const std::string & default_ ) const
{
	for( StringArray::const_iterator p = m_header_in.begin() ; p != m_header_in.end() ; ++p )
	{
		if( (*p).find(key) == 0U )
			return *p ;
	}
	return default_ ;
}

/// \file gspamclient.cpp
