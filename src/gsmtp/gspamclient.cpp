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
// gspamclient.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gstr.h"
#include "gspamclient.h"
#include "gassert.h"

GSmtp::SpamClient::SpamClient( const GNet::ResolverInfo & resolver_info ,
	unsigned int connect_timeout , unsigned int response_timeout ) :
		GNet::Client(resolver_info,connect_timeout,response_timeout,"\n") ,
		m_timer(*this,&SpamClient::onTimeout,*this)
{
	G_DEBUG( "GSmtp::SpamClient::ctor: " << resolver_info.displayString() << ": " 
		<< connect_timeout << " " << response_timeout ) ;
}

GSmtp::SpamClient::~SpamClient()
{
}

void GSmtp::SpamClient::onConnect()
{
	G_DEBUG( "GSmtp::SpamClient::onConnect" ) ;
	if( busy() )
		sendContent() ;
}

void GSmtp::SpamClient::request( const std::string & path )
{
	G_DEBUG( "GSmtp::SpamClient::request: \"" << path << "\"" ) ;
	if( busy() ) 
		throw ProtocolError() ;

	m_path = path ;
	m_in <<= new std::ifstream( path.c_str() ) ;
	m_timer.startTimer( 0U ) ;
}

void GSmtp::SpamClient::onTimeout()
{
	if( connected() )
		sendContent() ;
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
		eventSignal().emit( "spam" , reason.empty() ? result() : reason ) ;
	}
	Base::onDeleteImp( reason , b ) ; // use typedef because of ms compiler bug
}

bool GSmtp::SpamClient::onReceive( const std::string & line )
{
	G_DEBUG( "GSmtp::SpamClient::onReceive: [" << G::Str::printable(line) << "]" ) ;

	if( m_in.get() != NULL )
		throw ProtocolError( G::Str::printable(line) ) ; // the spamd has sent a response too early

	if( m_out.get() == NULL )
		m_out <<= new std::ofstream( m_path.c_str() ) ;

	std::ostream & out = *m_out.get() ;
	out << line << std::endl ;
	return true ;
}

void GSmtp::SpamClient::sendContent()
{
	// TODO -- send whole content file from m_in and then shutdown() the socket 
	// for writing and reset m_in -- (no need for a response timer here)
	G_ERROR( "GSmtp::SpamClient::sendContent: feature not implemented" ) ;
	throw ProtocolError( "not implemented" ) ;
}

void GSmtp::SpamClient::onSendComplete()
{
	// TODO
}

std::string GSmtp::SpamClient::result() const
{
	// TODO -- read the file (m_path) and parse out the X-Spam line -- or 
	// parse as you go in onReceive()
	return std::string() ;
}

/// \file gspamclient.cpp
