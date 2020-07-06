//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gmessagestore.h"
#include "gnewfile.h"
#include "gprocess.h"
#include "groot.h"
#include "gfile.h"
#include "gstr.h"
#include "gxtext.h"
#include "geightbit.h"
#include "gassert.h"
#include "glog.h"
#include <functional>
#include <algorithm>
#include <iostream>
#include <fstream>

GSmtp::NewFile::NewFile( FileStore & store , const std::string & from ,
	const std::string & from_auth_in , const std::string & from_auth_out ,
	std::size_t max_size , bool test_for_eight_bit ) :
		m_store(store) ,
		m_committed(false) ,
		m_test_for_eight_bit(test_for_eight_bit) ,
		m_saved(false) ,
		m_size(0UL) ,
		m_max_size(max_size)
{
	m_env.m_from = from ;
	m_env.m_from_auth_in = from_auth_in ;
	m_env.m_from_auth_out = from_auth_out ;

	// ask the store for a unique id
	m_seq = store.newSeq() ;

	// ask the store for a content stream
	m_content_path = m_store.contentPath( m_seq ) ;
	G_LOG( "GSmtp::NewMessage: content file: " << m_content_path ) ;
	m_content = m_store.stream( m_content_path ) ;
}

GSmtp::NewFile::~NewFile()
{
	try
	{
		G_DEBUG( "GSmtp::NewFile::dtor: " << m_content_path ) ;
		cleanup() ;
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
	m_env.m_authentication = session_auth_id ;
	m_env.m_client_socket_address = peer_socket_address ;
	m_env.m_client_certificate = peer_certificate ;
	m_envelope_path_0 = m_store.envelopeWorkingPath( m_seq ) ;
	m_envelope_path_1 = m_store.envelopePath( m_seq ) ;
	if( !saveEnvelope() )
		throw FileError( "cannot write envelope file " + m_envelope_path_0.str() ) ;

	// copy or move aside for local mailboxes
	//
	if( !m_env.m_to_local.empty() && m_env.m_to_remote.empty() )
	{
		moveToLocal( m_content_path , m_envelope_path_0 , m_envelope_path_1 ) ;
		return std::string() ;
	}
	else if( !m_env.m_to_local.empty() )
	{
		copyToLocal( m_content_path , m_envelope_path_0 , m_envelope_path_1 ) ;
		return m_content_path.str() ;
	}
	else
	{
		return m_content_path.str() ;
	}
}

void GSmtp::NewFile::commit( bool strict )
{
	m_committed = true ;
	bool ok = commitEnvelope() ;
	if( !ok && strict )
		throw FileError( "cannot rename envelope file to " + m_envelope_path_1.str() ) ;
	if( ok )
		m_store.updated() ;
}

void GSmtp::NewFile::addTo( const std::string & to , bool local )
{
	if( local )
		m_env.m_to_local.push_back( to ) ;
	else
		m_env.m_to_remote.push_back( to ) ;
}

bool GSmtp::NewFile::addText( const char * line_data , std::size_t line_size )
{
	m_size += static_cast<unsigned long>( line_size ) ;

	// testing for eight-bit content can be relatively slow -- not testing
	// the content is implied by RFC-2821 ("a relay should ... relay
	// [messages] without inspecting [the] content"), but RFC-6152
	// disallows sending eight-bit content to a seven-bit server --
	// here we optionally do the test on each line of content -- the
	// final result is written into the envelope file, allowing an
	// external program to change it before forwarding
	//
	if( m_test_for_eight_bit && m_env.m_eight_bit != 1 )
		m_env.m_eight_bit = isEightBit(line_data,line_size) ? 1 : 0 ;

	std::ostream & stream = *m_content ;
	stream.write( line_data , line_size ) ;

	return m_max_size == 0UL || m_size < m_max_size ;
}

void GSmtp::NewFile::flushContent()
{
	G_ASSERT( m_content != nullptr ) ;
	m_content->close() ;
	if( m_content->fail() ) // trap failbit/badbit
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

bool GSmtp::NewFile::isEightBit( const char * line_data , std::size_t line_size )
{
	return G::eightbit( line_data , line_size ) ;
}

bool GSmtp::NewFile::saveEnvelope()
{
	G_LOG( "GSmtp::NewMessage: envelope file: " << m_envelope_path_0.str() ) ;
	std::unique_ptr<std::ofstream> envelope_stream = m_store.stream( m_envelope_path_0 ) ;
	m_env.m_endpos = GSmtp::Envelope::write( *envelope_stream , m_env ) ;
	m_env.m_crlf = true ;
	envelope_stream->close() ;
	return !envelope_stream->fail() ;
}

bool GSmtp::NewFile::commitEnvelope()
{
	FileWriter claim_writer ;
	m_saved = G::File::rename( m_envelope_path_0 , m_envelope_path_1 , G::File::NoThrow() ) ;
	return m_saved ;
}

void GSmtp::NewFile::moveToLocal( const G::Path & content_path , const G::Path & envelope_path_now ,
	const G::Path & envelope_path_later )
{
	G_LOG_S( "GSmtp::NewMessage: message for local-mailbox recipient(s): " << content_path.basename() << ".local" ) ;
	FileWriter claim_writer ;
	G::File::rename( content_path.str() , content_path.str()+".local" ) ;
	G::File::rename( envelope_path_now.str() , envelope_path_later.str()+".local" ) ;
}

void GSmtp::NewFile::copyToLocal( const G::Path & content_path , const G::Path & envelope_path_now ,
	const G::Path & envelope_path_later )
{
	G_LOG_S( "GSmtp::NewMessage: message for local-mailbox recipient(s): " << content_path.basename() << ".local" ) ;
	FileWriter claim_writer ;
	G::File::copy( content_path.str() , content_path.str()+".local" ) ;
	G::File::copy( envelope_path_now.str() , envelope_path_later.str()+".local" ) ;
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
