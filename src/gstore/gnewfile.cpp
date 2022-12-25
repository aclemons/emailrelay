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
#include "gassert.h"
#include "glog.h"
#include <functional>
#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>

GStore::NewFile::NewFile( FileStore & store , const std::string & from ,
	const MessageStore::SmtpInfo & smtp_info , const std::string & from_auth_out ,
	std::size_t max_size ) :
		m_store(store) ,
		m_id(store.newId()) ,
		m_committed(false) ,
		m_saved(false) ,
		m_size(0U) ,
		m_max_size(max_size)
{
	m_env.m_from = from ;
	m_env.m_from_auth_in = smtp_info.auth ;
	m_env.m_from_auth_out = from_auth_out ;
	m_env.m_body_type = Envelope::parseSmtpBodyType( smtp_info.body ) ;
	m_env.m_utf8_mailboxes = smtp_info.utf8address ;

	// ask the store for a content stream
	G_LOG( "GStore::NewMessage: content file: " << cpath() ) ;
	m_content = m_store.stream( cpath() ) ;
}

GStore::NewFile::~NewFile()
{
	try
	{
		G_DEBUG( "GStore::NewFile::dtor: " << cpath() ) ;
		cleanup() ;
	}
	catch(...) // dtor
	{
	}
}

void GStore::NewFile::cleanup()
{
	discardContent() ;
	if( ! m_committed )
	{
		deleteEnvelope() ;
		deleteContent() ;
	}
}

bool GStore::NewFile::prepare( const std::string & session_auth_id ,
	const std::string & peer_socket_address , const std::string & peer_certificate )
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
		static_cast<MessageStore&>(m_store).updated() ;
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

void GStore::NewFile::commit( bool throw_on_error )
{
	m_committed = true ;
	bool ok = commitEnvelope() ;
	if( !ok && throw_on_error )
		throw FileError( "cannot rename envelope file to " + epath(State::Normal).str() ) ;
	if( ok )
		static_cast<MessageStore&>(m_store).updated() ;
}

void GStore::NewFile::addTo( const std::string & to , bool local , bool utf8address )
{
	if( local )
	{
		m_env.m_to_local.push_back( to ) ;
	}
	else
	{
		m_env.m_to_remote.push_back( to ) ;
		if( utf8address )
			m_env.m_utf8_mailboxes = true ;
	}
}

GStore::NewMessage::Status GStore::NewFile::addContent( const char * data , std::size_t data_size )
{
	std::size_t old_size = m_size ;
	std::size_t new_size = m_size + data_size ;
	if( new_size < m_size )
		new_size = std::numeric_limits<std::size_t>::max() ;

	m_size = new_size ;

	// truncate to m_max_size bytes
	if( m_max_size && new_size >= m_max_size )
		data_size = std::max(m_max_size,old_size) - old_size ;

	if( data_size )
	{
		std::ostream & stream = *m_content ;
		stream.write( data , data_size ) ; // NOLINT narrowing
	}

	if( m_content->fail() )
		return NewMessage::Status::Error ;
	else if( m_max_size && m_size >= m_max_size )
		return NewMessage::Status::TooBig ;
	else
		return NewMessage::Status::Ok ;
}

std::size_t GStore::NewFile::contentSize() const
{
	// wrt addContent() -- counts beyond max_size -- not valid if stream.fail()
	return m_size ;
}

void GStore::NewFile::discardContent()
{
	m_content.reset() ;
}

void GStore::NewFile::deleteContent()
{
	FileWriter claim_writer ;
	G::File::remove( cpath() , std::nothrow ) ;
}

void GStore::NewFile::deleteEnvelope()
{
	FileWriter claim_writer ;
	G::File::remove( epath(State::New) , std::nothrow ) ;
}

bool GStore::NewFile::saveEnvelope()
{
	G_LOG( "GStore::NewMessage: envelope file: " << epath(State::New).basename() ) ; // (was full path)
	std::unique_ptr<std::ofstream> envelope_stream = m_store.stream( epath(State::New) ) ;
	m_env.m_endpos = GStore::Envelope::write( *envelope_stream , m_env ) ;
	m_env.m_crlf = true ;
	envelope_stream->close() ;
	return !envelope_stream->fail() ;
}

bool GStore::NewFile::commitEnvelope()
{
	FileWriter claim_writer ;
	m_saved = G::File::rename( epath(State::New) , epath(State::Normal) , std::nothrow ) ;
	return m_saved ;
}

void GStore::NewFile::moveToLocal( const G::Path & content_path , const G::Path & envelope_path_now ,
	const G::Path & envelope_path_later )
{
	G_LOG_S( "GStore::NewMessage: message for local-mailbox recipient(s): " << content_path.basename() << ".local" ) ;
	FileWriter claim_writer ;
	G::File::rename( content_path.str() , content_path.str()+".local" ) ;
	G::File::rename( envelope_path_now.str() , envelope_path_later.str()+".local" ) ;
}

void GStore::NewFile::copyToLocal( const G::Path & content_path , const G::Path & envelope_path_now ,
	const G::Path & envelope_path_later )
{
	G_LOG_S( "GStore::NewMessage: message for local-mailbox recipient(s): " << content_path.basename() << ".local" ) ;
	FileWriter claim_writer ;
	G::File::copy( content_path.str() , content_path.str()+".local" ) ;
	G::File::copy( envelope_path_now.str() , envelope_path_later.str()+".local" ) ;
}

GStore::MessageId GStore::NewFile::id() const
{
	return m_id ;
}

std::string GStore::NewFile::location() const
{
	return cpath().str() ;
}

G::Path GStore::NewFile::cpath() const
{
	return m_store.contentPath( m_id ) ;
}

G::Path GStore::NewFile::epath( State state ) const
{
	return state == State::Normal ?
		m_store.envelopePath(m_id) :
		G::Path( m_store.envelopePath(m_id).str() + ".new" ) ;
}

