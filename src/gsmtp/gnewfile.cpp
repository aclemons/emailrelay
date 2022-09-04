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
/// \file gnewfile.cpp
///

#include "gdef.h"
#include "gfilestore.h"
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
		m_id(store.newId()) ,
		m_committed(false) ,
		m_test_for_eight_bit(test_for_eight_bit) ,
		m_saved(false) ,
		m_size(0UL) ,
		m_max_size(max_size)
{
	m_env.m_from = from ;
	m_env.m_from_auth_in = from_auth_in ;
	m_env.m_from_auth_out = from_auth_out ;

	// ask the store for a content stream
	G_LOG( "GSmtp::NewMessage: content file: " << cpath() ) ;
	m_content = m_store.stream( cpath() ) ;
}

GSmtp::NewFile::~NewFile()
{
	try
	{
		G_DEBUG( "GSmtp::NewFile::dtor: " << cpath() ) ;
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

bool GSmtp::NewFile::prepare( const std::string & session_auth_id , const std::string & peer_socket_address ,
	const std::string & peer_certificate )
{
	// flush and close the content file
	G_ASSERT( m_content != nullptr ) ;
	m_content->close() ;
	if( m_content->fail() ) // trap failbit/badbit
		throw FileError( "cannot write content file " + cpath().str() ) ;
	m_content.reset() ;

	// write the envelope
	m_env.m_authentication = session_auth_id ;
	m_env.m_client_socket_address = peer_socket_address ;
	m_env.m_client_certificate = peer_certificate ;
	if( !saveEnvelope() )
		throw FileError( "cannot write envelope file " + epath(State::New).str() ) ;

	// copy or move aside for local mailboxes
	if( !m_env.m_to_local.empty() && m_env.m_to_remote.empty() )
	{
		moveToLocal( cpath() , epath(State::New) , epath(State::Normal) ) ;
		m_store.updated() ; // (new)
		return true ; // no commit() needed
	}
	else if( !m_env.m_to_local.empty() )
	{
		copyToLocal( cpath() , epath(State::New) , epath(State::Normal) ) ;
		return false ;
	}
	else
	{
		return false ;
	}
}

void GSmtp::NewFile::commit( bool throw_on_error )
{
	m_committed = true ;
	bool ok = commitEnvelope() ;
	if( !ok && throw_on_error )
		throw FileError( "cannot rename envelope file to " + epath(State::Normal).str() ) ;
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
	stream.write( line_data , line_size ) ; // NOLINT narrowing

	return m_max_size == 0UL || m_size < m_max_size ;
}

void GSmtp::NewFile::discardContent()
{
	m_content.reset() ;
}

void GSmtp::NewFile::deleteContent()
{
	FileWriter claim_writer ;
	G::File::remove( cpath() , std::nothrow ) ;
}

void GSmtp::NewFile::deleteEnvelope()
{
	FileWriter claim_writer ;
	G::File::remove( epath(State::New) , std::nothrow ) ;
}

bool GSmtp::NewFile::isEightBit( const char * line_data , std::size_t line_size )
{
	return G::eightbit( line_data , line_size ) ;
}

bool GSmtp::NewFile::saveEnvelope()
{
	G_LOG( "GSmtp::NewMessage: envelope file: " << epath(State::New).basename() ) ; // (was full path)
	std::unique_ptr<std::ofstream> envelope_stream = m_store.stream( epath(State::New) ) ;
	m_env.m_endpos = GSmtp::Envelope::write( *envelope_stream , m_env ) ;
	m_env.m_crlf = true ;
	envelope_stream->close() ;
	return !envelope_stream->fail() ;
}

bool GSmtp::NewFile::commitEnvelope()
{
	FileWriter claim_writer ;
	m_saved = G::File::rename( epath(State::New) , epath(State::Normal) , std::nothrow ) ;
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

GSmtp::MessageId GSmtp::NewFile::id() const
{
	return m_id ;
}

std::string GSmtp::NewFile::location() const
{
	return cpath().str() ;
}

G::Path GSmtp::NewFile::cpath() const
{
	return m_store.contentPath( m_id ) ;
}

G::Path GSmtp::NewFile::epath( State state ) const
{
	return state == State::Normal ?
		m_store.envelopePath(m_id) :
		G::Path( m_store.envelopePath(m_id).str() + ".new" ) ;
}

