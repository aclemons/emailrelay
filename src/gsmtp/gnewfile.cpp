//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
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
#include "groot.h"
#include "gfile.h"
#include "gxtext.h"
#include "gassert.h"
#include "glog.h"
#include <iostream>
#include <fstream>

bool GSmtp::NewFile::m_preprocess = false ;
G::Path GSmtp::NewFile::m_preprocessor ;

GSmtp::NewFile::NewFile( const std::string & from , FileStore & store ) :
	m_store(store),
	m_from(from) ,
	m_eight_bit(false),
	m_saved(false) ,
	m_repoll(false)
{
	m_seq = store.newSeq() ;

	m_content_path = m_store.contentPath( m_seq ) ;
	G_LOG( "GSmtp::NewMessage: content file: " << m_content_path ) ;

	std::auto_ptr<std::ostream> content_stream = m_store.stream( m_content_path ) ;
	m_content = content_stream ;
}

GSmtp::NewFile::~NewFile()
{
	try
	{
		cleanup() ;
		m_store.updated( m_repoll ) ;
	}
	catch(...)
	{
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

bool GSmtp::NewFile::store( const std::string & auth_id , const std::string & client_ip )
{
	// flush the content file
	//
	m_content->flush() ;
	if( ! m_content->good() )
		throw GSmtp::MessageStore::WriteError( m_content_path.str() ) ;
	m_content <<= 0 ;

	// write the envelope
	//
	G::Path p0 = m_store.envelopeWorkingPath( m_seq ) ;
	G::Path p1 = m_store.envelopePath( m_seq ) ;
	bool ok = false ;
	std::string reason = p0.str() ;
	{
		std::auto_ptr<std::ostream> envelope_stream = m_store.stream( p0 ) ;
		ok = saveEnvelope( *(envelope_stream.get()) , p0.str() , auth_id , client_ip ) ;
	}

	// shell out to a message pre-processor
	//
	bool cancelled = false ;
	if( ok )
	{
		ok = preprocess( m_content_path , cancelled ) ;
		if( !ok )
			reason = "pre-processing failed" ;
	}
	G_ASSERT( !(ok&&cancelled) ) ;

	// deliver to local mailboxes
	//
	if( ok && m_to_local.size() != 0U )
	{
		deliver( m_to_local , m_content_path , p0 , p1 ) ;
	}

	// commit the envelope, or rollback the content
	//
	FileWriter claim_writer ;
	if( !ok || !commit(p0,p1) )
	{
		rollback() ;
		if( !cancelled )
			throw GSmtp::MessageStore::StorageError( reason ) ;
	}

	return cancelled ;
}

bool GSmtp::NewFile::commit( const G::Path & p0 , const G::Path & p1 )
{
	m_saved = G::File::rename( p0 , p1 , G::File::NoThrow() ) ;
	return m_saved ;
}

void GSmtp::NewFile::rollback()
{
	G::File::remove( m_content_path , G::File::NoThrow() ) ;
}

void GSmtp::NewFile::cleanup()
{
	if( !m_saved )
	{
		FileWriter claim_writer ;
		G::File::remove( m_content_path , G::File::NoThrow() ) ;
	}
}

bool GSmtp::NewFile::preprocess( const G::Path & path , bool & cancelled )
{
	if( ! m_preprocess ) return true ;

	int exit_code = preprocessCore( path ) ;

	bool is_ok = exit_code == 0 ;
	bool is_special = exit_code >= 100 && exit_code <= 107 ;
	bool is_failure = !is_ok && !is_special ;

	if( is_special && ((exit_code-100)&2) != 0 )
	{
		m_repoll = true ;
	}

	// ok, fail or cancel
	//
	if( is_special && ((exit_code-100)&1) == 0 )
	{
		cancelled = true ;
		G_LOG( "GSmtp::NewFile: message processing cancelled by preprocessor" ) ;
		return false ;
	}
	else if( is_failure )
	{
		G_WARNING( "GSmtp::NewFile::preprocess: pre-processing failed: exit code " << exit_code ) ;
		return false ;
	}
	else
	{
		return true ;
	}
}

int GSmtp::NewFile::preprocessCore( const G::Path & path )
{
	G_LOG( "GSmtp::NewFile::preprocess: " << m_preprocessor << " " << path ) ;
	G::Strings args ;
	args.push_back( path.str() ) ;
	int exit_code = G::Process::spawn( G::Root::nobody() , m_preprocessor , args ) ;
	G_LOG( "GSmtp::NewFile::preprocess: exit status " << exit_code ) ;
	return exit_code ;
}

void GSmtp::NewFile::deliver( const G::Strings & /*to*/ , 
	const G::Path & content_path , const G::Path & envelope_path_now ,
	const G::Path & envelope_path_later )
{
	// could shell out to "procmail" or "deliver" here, but keep it 
	// simple and within the scope of a "message-store" class

	G_LOG_S( "GSmtp::NewMessage: copying message for local recipient(s): " 
		<< content_path.basename() << ".local" ) ;

	FileWriter claim_writer ;
	G::File::copy( content_path.str() , content_path.str()+".local" ) ;
	G::File::copy( envelope_path_now.str() , envelope_path_later.str()+".local" ) ;
}

bool GSmtp::NewFile::saveEnvelope( std::ostream & stream , const std::string & where , 
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
	stream << x << "Authentication: " << Xtext::encode(auth_id) << crlf() ;
	stream << x << "Client: " << client_ip << crlf() ;
	stream << x << "End: 1" << crlf() ;
	stream.flush() ;
	return stream.good() ;
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

void GSmtp::NewFile::setPreprocessor( const G::Path & exe )
{
	if( exe.isRelative() )
		throw InvalidPath( exe.str() ) ;

	m_preprocess = true ;
	m_preprocessor = exe ;
}

