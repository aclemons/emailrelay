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
// gnewfile.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gnewfile.h"
#include "gprocess.h"
#include "gstrings.h"
#include "groot.h"
#include "gfile.h"
#include "gstr.h"
#include "gxtext.h"
#include "gassert.h"
#include "glog.h"
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>

GSmtp::NewFile::NewFile( FileStore & store , const std::string & from ,
	const std::string & from_auth_in , const std::string & from_auth_out ,
	size_t max_size ) :
		m_store(store) ,
		m_from(from) ,
		m_from_auth_in(from_auth_in) ,
		m_from_auth_out(from_auth_out) ,
		m_committed(false) ,
		m_eight_bit(false) ,
		m_saved(false) ,
		m_size(0UL) ,
		m_max_size(max_size)
{
	// ask the store for a unique id
	//
	m_seq = store.newSeq() ;

	// ask the store for a content stream
	//
	m_content_path = m_store.contentPath( m_seq ) ;
	G_LOG( "GSmtp::NewMessage: content file: " << m_content_path ) ;
	unique_ptr<std::ostream> content_stream = m_store.stream( m_content_path ) ;
	m_content.reset( content_stream.release() ) ;
}

GSmtp::NewFile::~NewFile()
{
	try
	{
		G_DEBUG( "GSmtp::NewFile::dtor: " << m_content_path ) ;
		cleanup() ;
		m_store.updated() ;
	}
	catch(...) // dtor
	{
	}
}

void GSmtp::NewFile::cleanup()
{
	discardContent() ;
	if( ! m_committed )
	{
		deleteEnvelope() ;
		deleteContent() ;
	}
}

std::string GSmtp::NewFile::prepare( const std::string & session_auth_id , const std::string & peer_socket_address ,
	const std::string & peer_certificate )
{
	// flush and close the content file
	//
	flushContent() ;

	// write the envelope
	//
	m_envelope_path_0 = m_store.envelopeWorkingPath( m_seq ) ;
	m_envelope_path_1 = m_store.envelopePath( m_seq ) ;
	if( ! saveEnvelope( session_auth_id , peer_socket_address , peer_certificate ) )
		throw FileError( "cannot write envelope file " + m_envelope_path_0.str() ) ;

	// deliver to local mailboxes
	//
	if( m_to_local.size() != 0U )
	{
		deliver( m_to_local , m_content_path , m_envelope_path_0 , m_envelope_path_1 ) ;
	}

	return m_content_path.str() ;
}

void GSmtp::NewFile::commit( bool strict )
{
	m_committed = true ;
	bool ok = commitEnvelope() ;
	if( !ok && strict )
		throw FileError( "cannot rename envelope file to " + m_envelope_path_1.str() ) ;
}

void GSmtp::NewFile::addTo( const std::string & to , bool local )
{
	if( local )
		m_to_local.push_back( to ) ;
	else
		m_to_remote.push_back( to ) ;
}

bool GSmtp::NewFile::addText( const std::string & line )
{
	m_size += static_cast<unsigned long>( line.size() + 2U ) ;
	if( ! m_eight_bit )
		m_eight_bit = isEightBit( line ) ;

	*(m_content.get()) << line << crlf() ;
	return m_max_size == 0UL || m_size < m_max_size ;
}

void GSmtp::NewFile::flushContent()
{
	G_ASSERT( m_content.get() != nullptr ) ;
	m_content->flush() ;
	if( ! m_content->good() )
		throw FileError( "cannot write content file " + m_content_path.str() ) ;
	m_content.reset() ;
}

void GSmtp::NewFile::discardContent()
{
	m_content.reset() ;
}

void GSmtp::NewFile::deleteContent()
{
	FileWriter claim_writer ;
	G::File::remove( m_content_path , G::File::NoThrow() ) ;
}

void GSmtp::NewFile::deleteEnvelope()
{
	if( ! m_envelope_path_0.str().empty() )
	{
		FileWriter claim_writer ;
		G::File::remove( m_envelope_path_0 , G::File::NoThrow() ) ;
	}
}

namespace
{
	struct EightBit : std::unary_function<char,bool>
	{
		bool operator()( char c ) { return !! ( static_cast<unsigned char>(c) & 0x80U ) ; }
	} ;
}

bool GSmtp::NewFile::isEightBit( const std::string & line )
{
	return std::find_if( line.begin() , line.end() , EightBit() ) != line.end() ;
}

bool GSmtp::NewFile::saveEnvelope( const std::string & session_auth_id , const std::string & peer_socket_address ,
	const std::string & peer_certificate ) const
{
	unique_ptr<std::ostream> envelope_stream = m_store.stream( m_envelope_path_0 ) ;
	writeEnvelope( *(envelope_stream.get()) , m_envelope_path_0.str() ,
		session_auth_id , peer_socket_address , peer_certificate ) ;
	bool ok = envelope_stream->good() ;
	return ok ;
}

bool GSmtp::NewFile::commitEnvelope()
{
	FileWriter claim_writer ;
	m_saved = G::File::rename( m_envelope_path_0 , m_envelope_path_1 , G::File::NoThrow() ) ;
	return m_saved ;
}

void GSmtp::NewFile::deliver( const G::StringArray & /*to*/ ,
	const G::Path & content_path , const G::Path & envelope_path_now ,
	const G::Path & envelope_path_later )
{
	// could shell out to "procmail" or "deliver" here, but keep it
	// simple and within the scope -- just copy into ".local" files

	G_LOG_S( "GSmtp::NewMessage: copying message for local recipient(s): "
		<< content_path.basename() << ".local" ) ;

	FileWriter claim_writer ;
	G::File::copy( content_path.str() , content_path.str()+".local" ) ;
	G::File::copy( envelope_path_now.str() , envelope_path_later.str()+".local" ) ;
}

void GSmtp::NewFile::writeEnvelope( std::ostream & stream , const std::string & where ,
	const std::string & session_auth_id , const std::string & peer_socket_address ,
	const std::string & peer_certificate_in ) const
{
	G_LOG( "GSmtp::NewMessage: envelope file: " << where ) ;

	std::string peer_certificate = peer_certificate_in ;
	G::Str::trim( peer_certificate , G::Str::ws() ) ;
	G::Str::replaceAll( peer_certificate , "\r" , "" ) ;
	G::Str::replaceAll( peer_certificate , "\n" , crlf() + " " ) ; // RFC-2822 folding

	const std::string x( m_store.x() ) ;

	stream << x << "Format: " << m_store.format() << crlf() ;
	stream << x << "Content: " << (m_eight_bit?"8":"7") << "bit" << crlf() ;
	stream << x << "From: " << m_from << crlf() ;
	stream << x << "ToCount: " << (m_to_local.size()+m_to_remote.size()) << crlf() ;
	{
		G::StringArray::const_iterator to_p = m_to_local.begin() ;
		for( ; to_p != m_to_local.end() ; ++to_p )
			stream << x << "To-Local: " << *to_p << crlf() ;
	}
	{
		G::StringArray::const_iterator to_p = m_to_remote.begin() ;
		for( ; to_p != m_to_remote.end() ; ++to_p )
			stream << x << "To-Remote: " << *to_p << crlf() ;
	}
	stream << x << "Authentication: " << G::Xtext::encode(session_auth_id) << crlf() ;
	stream << x << "Client: " << peer_socket_address << crlf() ;
	stream << x << "ClientCertificate: " << peer_certificate << crlf() ;
	stream << x << "MailFromAuthIn: " << xnormalise(m_from_auth_in) << crlf() ;
	stream << x << "MailFromAuthOut: " << xnormalise(m_from_auth_out) << crlf() ;
	stream << x << "End: 1" << crlf() ;
	stream.flush() ;
}

std::string GSmtp::NewFile::xnormalise( const std::string & s )
{
	return G::Xtext::encode(G::Xtext::decode(s)) ;
}

const std::string & GSmtp::NewFile::crlf() const
{
	static const std::string s( "\015\012" ) ;
	return s ;
}

unsigned long GSmtp::NewFile::id() const
{
	return m_seq ;
}

G::Path GSmtp::NewFile::contentPath() const
{
	return m_content_path ;
}

/// \file gnewfile.cpp
