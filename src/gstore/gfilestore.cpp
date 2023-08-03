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
/// \file gfilestore.cpp
///

#include "gdef.h"
#include "gfilestore.h"
#include "gnewfile.h"
#include "gstoredfile.h"
#include "gprocess.h"
#include "gdirectory.h"
#include "gformat.h"
#include "ggettext.h"
#include "groot.h"
#include "gpath.h"
#include "gfile.h"
#include "gstr.h"
#include "gtest.h"
#include "glog.h"
#include <iostream>
#include <fstream>

namespace GStore
{
	class FileIterator ;
}

class GStore::FileIterator : public MessageStore::Iterator /// A GStore::MessageStore::Iterator for GStore::FileStore.
{
public:
	FileIterator( FileStore & store , const G::Path & dir , bool lock ) ;
	~FileIterator() override ;

private: // overrides
	std::unique_ptr<StoredMessage> next() override ;

public:
	FileIterator( const FileIterator & ) = delete ;
	FileIterator( FileIterator && ) = delete ;
	FileIterator & operator=( const FileIterator & ) = delete ;
	FileIterator & operator=( FileIterator && ) = delete ;

private:
	FileStore & m_store ;
	G::DirectoryList m_iter ;
	bool m_lock ;
} ;

// ===

GStore::FileIterator::FileIterator( FileStore & store , const G::Path & dir , bool lock ) :
	m_store(store) ,
	m_lock(lock)
{
	DirectoryReader claim_reader ;
	m_iter.readType( dir , ".envelope" ) ;
}

GStore::FileIterator::~FileIterator()
= default;

std::unique_ptr<GStore::StoredMessage> GStore::FileIterator::next()
{
	while( m_iter.more() )
	{
		GStore::MessageId message_id( m_iter.filePath().withoutExtension().basename() ) ;
		if( !message_id.valid() )
			continue ;

		auto message_ptr = std::make_unique<StoredFile>( m_store , message_id ) ;

		if( m_lock && !message_ptr->lock() )
		{
			G_WARNING( "GStore::MessageStore: cannot lock file: \"" << m_iter.filePath().basename() << "\"" ) ;
			continue ;
		}

		bool ok = false ;
		std::string reason ;
		ok = message_ptr->readEnvelope( reason ) && message_ptr->openContent( reason ) ;
		if( !ok )
		{
			G_WARNING( "GStore::MessageStore: ignoring \"" << m_iter.filePath() << "\": " << reason ) ;
			continue ;
		}

		return message_ptr ;
	}
	return {} ;
}

// ===

GStore::FileStore::FileStore( const G::Path & dir , const G::Path & delivery_dir , const Config & config ) :
	m_seq(config.seq) ,
	m_dir(dir) ,
	m_delivery_dir(delivery_dir) ,
	m_config(config)
{
	checkPath( dir ) ;
}

G::Path GStore::FileStore::directory() const
{
	return m_dir ;
}

G::Path GStore::FileStore::deliveryDir() const
{
	return m_delivery_dir.empty() ? m_dir : m_delivery_dir ;
}

std::string GStore::FileStore::x()
{
	return "X-MailRelay-" ;
}

std::string GStore::FileStore::format( int generation )
{
	// use a weird prefix to help with file(1) and magic(5)
	if( generation == -5 )
		return "#2821.3" ; // original
	else if( generation == -4 )
		return "#2821.4" ; // new for 1.9
	else if( generation == -3 )
		return "#2821.5" ; // new for 2.0
	else if( generation == -2 )
		return "#2821.6" ; // new for 2.4
	else if( generation == -1 )
		return "#2821.7" ; // new for 2.5rc
	else
		return "#2821.8" ; // new for 2.5
}

bool GStore::FileStore::knownFormat( const std::string & format_in )
{
	return
		format_in == format(0) ||
		format_in == format(-1) ||
		format_in == format(-2) ||
		format_in == format(-3) ||
		format_in == format(-4) ||
		format_in == format(-5) ;
}

void GStore::FileStore::checkPath( const G::Path & directory_path )
{
	G::Directory dir_test( directory_path ) ;
	bool ok = false ;
	int error = 0 ;
	std::string reason  ;

	// fail if not readable (after switching effective userid)
	{
		FileWriter claim_writer ;
		error = dir_test.usable() ;
		ok = error == 0 ;
	}
	if( !ok )
	{
		throw InvalidDirectory( directory_path.str() , G::Process::strerror(error) ) ;
	}

	// warn if not writeable (after switching effective userid)
	{
		std::string tmp_filename = G::Directory::tmp() ;
		FileWriter claim_writer ;
		ok = dir_test.writeable( tmp_filename ) ;
	}
	if( !ok )
	{
		using G::format ;
		using G::txt ;
		G_WARNING( "GStore::MessageStore: " << format(txt("directory not writable: \"%1%\"")) % directory_path ) ;
	}
}

std::string GStore::FileStore::location( const MessageId & id ) const
{
	return envelopePath(id).str() ;
}

std::unique_ptr<std::ofstream> GStore::FileStore::stream( const G::Path & path )
{
	auto stream_ptr = std::make_unique<std::ofstream>() ;
	FileOp::openOut( *stream_ptr , path ) ;
	return stream_ptr ;
}

G::Path GStore::FileStore::contentPath( const MessageId & id ) const
{
	return envelopePath(id).withExtension( "content" ) ;
}

G::Path GStore::FileStore::envelopePath( const MessageId & id , State state ) const
{
	if( state == State::New )
		return m_dir + id.str().append(".envelope.new") ;
	else if( state == State::Locked )
		return m_dir + id.str().append(".envelope.busy") ;
	else if( state == State::Bad )
		return m_dir + id.str().append(".envelope.bad") ;
	else
		return m_dir + id.str().append(".envelope") ;
}

GStore::MessageId GStore::FileStore::newId()
{
	m_seq++ ;
	if( m_seq == 0UL )
		m_seq++ ;
	return newId( m_seq ) ;
}

GStore::MessageId GStore::FileStore::newId( unsigned long seq )
{
	unsigned long timestamp = static_cast<unsigned long>(G::SystemTime::now().s()) ;
	std::ostringstream ss ;
	ss << "emailrelay." << G::Process::Id().str() << "." << timestamp << "." << seq ;
	return MessageId( ss.str() ) ;
}

bool GStore::FileStore::empty() const
{
	G::DirectoryList list ;
	DirectoryReader claim_reader ;
	list.readType( m_dir , ".envelope" , 1U ) ;
	const bool no_more = !list.more() ;
	return no_more ;
}

std::vector<GStore::MessageId> GStore::FileStore::ids()
{
	G::DirectoryList list ;
	{
		DirectoryReader claim_reader ;
		list.readType( m_dir , ".envelope" ) ;
	}
	std::vector<GStore::MessageId> result ;
	while( list.more() )
		result.emplace_back( list.filePath().withoutExtension().basename() ) ;
	return result ;
}

std::vector<GStore::MessageId> GStore::FileStore::failures()
{
	G::DirectoryList list ;
	{
		DirectoryReader claim_reader ;
		list.readType( m_dir , ".envelope.bad" ) ;
	}
	std::vector<GStore::MessageId> result ;
	while( list.more() )
		result.emplace_back( list.filePath().withoutExtension().withoutExtension().basename() ) ;
	return result ;
}

void GStore::FileStore::unfailAll()
{
	G::DirectoryList list ;
	{
		DirectoryReader claim_reader ;
		list.readType( m_dir , ".envelope.bad" ) ;
	}
	while( list.more() )
	{
		FileWriter claim_writer ;
		FileOp::rename( list.filePath() , list.filePath().withoutExtension() ) ; // ignore errors
	}
}

std::unique_ptr<GStore::MessageStore::Iterator> GStore::FileStore::iterator( bool lock )
{
	return std::make_unique<FileIterator>( *this , m_dir , lock ) ;
}

std::unique_ptr<GStore::StoredMessage> GStore::FileStore::get( const MessageId & id )
{
	auto message = std::make_unique<StoredFile>( *this , id ) ;
	if( !message->lock() )
		throw GetError( id.str().append(": cannot lock the envelope file") ) ;

	std::string reason ;
	if( !message->readEnvelope( reason ) )
		throw GetError( id.str().append(": cannot read the envelope: ").append(reason) ) ;

	if( !message->openContent( reason ) )
		throw GetError( id.str().append(": cannot read the content: ").append(reason) ) ;

	return message ;
}

GStore::Envelope GStore::FileStore::readEnvelope( const G::Path & envelope_path , std::ifstream * stream_p )
{
    std::ifstream strm ;
    std::ifstream & envelope_stream = stream_p ? *stream_p : strm ;
    if( !FileOp::openIn( envelope_stream , envelope_path ) )
        throw EnvelopeReadError( envelope_path.str() , G::Process::strerror(FileOp::errno_()) ) ;

    GStore::Envelope envelope ;
    GStore::Envelope::read( envelope_stream , envelope ) ;
    return envelope ;
}

std::unique_ptr<GStore::NewMessage> GStore::FileStore::newMessage( const std::string & from ,
	const MessageStore::SmtpInfo & smtp_info , const std::string & from_auth_out )
{
	return std::make_unique<NewFile>( *this , from , smtp_info , from_auth_out , m_config.max_size ) ;
}

void GStore::FileStore::updated()
{
	G_DEBUG( "GStore::FileStore::updated" ) ;
	m_update_signal.emit() ;
}

G::Slot::Signal<> & GStore::FileStore::messageStoreUpdateSignal() noexcept
{
	return m_update_signal ;
}

G::Slot::Signal<> & GStore::FileStore::messageStoreRescanSignal() noexcept
{
	return m_rescan_signal ;
}

void GStore::FileStore::rescan()
{
	messageStoreRescanSignal().emit() ;
}

// ===

GStore::FileReader::FileReader()
= default;

GStore::FileReader::~FileReader()
= default;

// ===

GStore::DirectoryReader::DirectoryReader()
= default;

GStore::DirectoryReader::~DirectoryReader()
= default;

// ===

GStore::FileWriter::FileWriter() :
	G::Root(false) ,
	G::Process::Umask(G::Process::Umask::Mode::Tighter)
{
}

GStore::FileWriter::~FileWriter()
= default;

// ===

int & GStore::FileStore::FileOp::errno_() noexcept
{
	static int e {} ;
	return e ;
}

bool GStore::FileStore::FileOp::rename( const G::Path & src , const G::Path & dst )
{
	FileWriter claim_writer ;
	errno_() = 0 ;
	bool ok = G::File::rename( src , dst , std::nothrow ) ;
	errno_() = G::Process::errno_() ;
	return ok ;
}

bool GStore::FileStore::FileOp::renameOnto( const G::Path & src , const G::Path & dst )
{
	FileWriter claim_writer ;
	errno_() = 0 ;
	bool ok = G::File::renameOnto( src , dst , std::nothrow ) ;
	errno_() = G::Process::errno_() ;
	return ok ;
}

bool GStore::FileStore::FileOp::remove( const G::Path & path ) noexcept
{
	try
	{
		FileWriter claim_writer ;
		errno_() = 0 ;
		bool ok = G::File::remove( path , std::nothrow ) ;
		errno_() = G::Process::errno_() ;
		return ok ;
	}
	catch(...)
	{
		return false ;
	}
}

bool GStore::FileStore::FileOp::exists( const G::Path & path )
{
	FileReader claim_reader ; // moot
	errno_() = 0 ;
	bool ok = G::File::exists( path , std::nothrow ) ;
	errno_() = G::Process::errno_() ;
	return ok ;
}

int GStore::FileStore::FileOp::fdopen( const G::Path & path )
{
	FileReader claim_reader ;
	errno_() = 0 ;
	int fd = G::File::open( path.cstr() , G::File::InOutAppend::In ) ;
	errno_() = G::Process::errno_() ;
	return fd ;
}

std::ifstream & GStore::FileStore::FileOp::openIn( std::ifstream & stream , const G::Path & path )
{
	FileReader claim_reader ;
	errno_() = 0 ;
	G::File::open( stream , path ) ;
	errno_() = G::Process::errno_() ;
	return stream ;
}

std::ofstream & GStore::FileStore::FileOp::openOut( std::ofstream & stream , const G::Path & path )
{
	FileWriter claim_writer ;
	errno_() = 0 ;
	G::File::open( stream , path ) ;
	errno_() = G::Process::errno_() ;
	return stream ;
}

std::ofstream & GStore::FileStore::FileOp::openAppend( std::ofstream & stream , const G::Path & path )
{
	FileWriter claim_writer ;
	errno_() = 0 ;
	G::File::open( stream , path , G::File::Append() ) ;
	errno_() = G::Process::errno_() ;
	return stream ;
}

bool GStore::FileStore::FileOp::hardlink( const G::Path & src , const G::Path & dst )
{
	FileWriter claim_writer ;
	errno_() = 0 ;
	bool copied = false ;
	bool linked = G::File::hardlink( src , dst , std::nothrow ) ;
	if( !linked )
		copied = G::File::copy( src , dst , std::nothrow ) ;
	errno_() = G::Process::errno_() ;

	// fix up group ownership if hard-linked into a set-group-id directory
	if( linked )
	{
		auto dir_stat = G::File::stat( dst.simple() ? G::Path(".") : dst.dirname() ) ;
		if( !dir_stat.error && dir_stat.inherit )
			G::File::chgrp( dst , dir_stat.gid , std::nothrow ) ;
	}

	return linked || copied ;
}

bool GStore::FileStore::FileOp::copy( const G::Path & src , const G::Path & dst , bool use_hardlink )
{
	if( use_hardlink )
		return hardlink( src , dst ) ;
	else
		return copy( src , dst ) ;
}

bool GStore::FileStore::FileOp::copy( const G::Path & src , const G::Path & dst )
{
	FileWriter claim_writer ;
	errno_() = 0 ;
	bool ok = G::File::copy( src , dst , std::nothrow ) ;
	errno_() = G::Process::errno_() ;
	return ok ;
}

bool GStore::FileStore::FileOp::mkdir( const G::Path & dir )
{
	FileWriter claim_root ;
	errno_() = 0 ;
	bool ok = G::File::mkdir( dir , std::nothrow ) ;
	errno_() = G::Process::errno_() ;
	return ok ;
}

bool GStore::FileStore::FileOp::isdir( const G::Path & a , const G::Path & b , const G::Path & c )
{
	FileReader claim_reader ;
	return
		G::File::isDirectory(a,std::nothrow) &&
		( b.empty() || G::File::isDirectory(b,std::nothrow) ) &&
		( c.empty() || G::File::isDirectory(c,std::nothrow) ) ;
}

