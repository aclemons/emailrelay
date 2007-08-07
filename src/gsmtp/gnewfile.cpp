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
// gnewfile.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gmessagestore.h"
#include "gnewfile.h"
#include "gmemory.h"
#include "gprocess.h"
#include "gstrings.h"
#include "groot.h"
#include "gfile.h"
#include "gxtext.h"
#include "gassert.h"
#include "glog.h"
#include <iostream>
#include <fstream>

GSmtp::NewFile::NewFile( const std::string & from , FileStore & store ) :
	m_store(store) ,
	m_from(from) ,
	m_committed(false) ,
	m_eight_bit(false) ,
	m_saved(false)
{
	// ask the store for a unique id
	//
	m_seq = store.newSeq() ;

	// ask the store for a content stream
	//
	m_content_path = m_store.contentPath( m_seq ) ;
	G_LOG( "GSmtp::NewMessage: content file: " << m_content_path ) ;
	std::auto_ptr<std::ostream> content_stream = m_store.stream( m_content_path ) ;
	m_content = content_stream ;
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

void GSmtp::NewFile::flushContent()
{
	G_ASSERT( m_content.get() != NULL ) ;
	m_content->flush() ;
	if( ! m_content->good() )
		throw GSmtp::MessageStore::WriteError( m_content_path.str() ) ;
	m_content <<= 0 ;
}

void GSmtp::NewFile::discardContent()
{
	m_content <<= 0 ;
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

void GSmtp::NewFile::addTo( const std::string & to , bool local )
{
	if( local )
		m_to_local.push_back( to ) ;
	else
		m_to_remote.push_back( to ) ;
}

void GSmtp::NewFile::addText( const std::string & line )
{
	if( ! m_eight_bit )
		m_eight_bit = isEightBit(line) ;

	*(m_content.get()) << line << crlf() ;
}

bool GSmtp::NewFile::isEightBit( const std::string & line )
{
	std::string::const_iterator end = line.end() ;
	for( std::string::const_iterator p = line.begin() ; p != end ; ++p )
	{
		const unsigned char c = static_cast<unsigned char>(*p) ;
		if( c > 0x7fU )
			return true ;
	}
	return false ;
}

std::string GSmtp::NewFile::prepare( const std::string & auth_id , const std::string & client_ip )
{
	// flush and close the content file
	//
	flushContent() ;

	// write the envelope
	//
	m_envelope_path_0 = m_store.envelopeWorkingPath( m_seq ) ;
	m_envelope_path_1 = m_store.envelopePath( m_seq ) ;
	if( ! saveEnvelope( auth_id , client_ip ) )
		throw GSmtp::MessageStore::StorageError( std::string() + "cannot write " + m_envelope_path_0.str() ) ;

	// deliver to local mailboxes
	//
	if( m_to_local.size() != 0U )
	{
		deliver( m_to_local , m_content_path , m_envelope_path_0 , m_envelope_path_1 ) ;
	}

	return m_content_path.str() ;
}

bool GSmtp::NewFile::saveEnvelope( const std::string & auth_id , const std::string & client_ip ) const
{
	std::auto_ptr<std::ostream> envelope_stream = m_store.stream( m_envelope_path_0 ) ;
	writeEnvelope( *(envelope_stream.get()) , m_envelope_path_0.str() , auth_id , client_ip ) ;
	bool ok = envelope_stream->good() ;
	return ok ;
}

void GSmtp::NewFile::commit()
{
	if( ! commitEnvelope() )
		throw GSmtp::MessageStore::StorageError( std::string() + "cannot rename to " + m_envelope_path_1.str() ) ;

	m_committed = true ;
}

bool GSmtp::NewFile::commitEnvelope()
{
	FileWriter claim_writer ;
	m_saved = G::File::rename( m_envelope_path_0 , m_envelope_path_1 , G::File::NoThrow() ) ;
	return m_saved ;
}

void GSmtp::NewFile::rollback()
{
	discardContent() ;
	deleteContent() ;
}

void GSmtp::NewFile::deliver( const G::Strings & /*to*/ , 
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
	const std::string & auth_id , const std::string & client_ip ) const
{
	G_LOG( "GSmtp::NewMessage: envelope file: " << where ) ;

	const std::string x( m_store.x() ) ;

	stream << x << "Format: " << m_store.format() << crlf() ;
	stream << x << "Content: " << (m_eight_bit?"8":"7") << "bit" << crlf() ;
	stream << x << "From: " << m_from << crlf() ;
	stream << x << "ToCount: " << (m_to_local.size()+m_to_remote.size()) << crlf() ;
	{
		G::Strings::const_iterator to_p = m_to_local.begin() ;
		for( ; to_p != m_to_local.end() ; ++to_p )
			stream << x << "To-Local: " << *to_p << crlf() ;
	}
	{
		G::Strings::const_iterator to_p = m_to_remote.begin() ;
		for( ; to_p != m_to_remote.end() ; ++to_p )
			stream << x << "To-Remote: " << *to_p << crlf() ;
	}
	stream << x << "Authentication: " << G::Xtext::encode(auth_id) << crlf() ;
	stream << x << "Client: " << client_ip << crlf() ;
	stream << x << "End: 1" << crlf() ;
	stream.flush() ;
}

const std::string & GSmtp::NewFile::crlf() const
{
	static std::string s( "\015\012" ) ;
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
