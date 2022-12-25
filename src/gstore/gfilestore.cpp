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
	FileIterator( FileStore & store , const G::Path & dir , bool lock , bool failures ) ;
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

GStore::FileIterator::FileIterator( FileStore & store , const G::Path & dir , bool lock , bool failures ) :
	m_store(store) ,
	m_lock(lock)
{
	DirectoryReader claim_reader ;
	m_iter.readType( dir , std::string(failures?".envelope.bad":".envelope") ) ;
}

GStore::FileIterator::~FileIterator()
= default;

std::unique_ptr<GStore::StoredMessage> GStore::FileIterator::next()
{
	while( m_iter.more() )
	{
		auto message_ptr = std::make_unique<StoredFile>( m_store , m_iter.filePath() ) ;

		if( !message_ptr->id().valid() )
			continue ;

		if( m_lock && !message_ptr->lock() )
		{
			G_WARNING( "GStore::MessageStore: cannot lock file: \"" << m_iter.filePath().basename() << "\"" ) ;
			continue ;
		}

		bool ok = false ;
		std::string reason ;
		ok = message_ptr->readEnvelope( reason , m_lock ) && message_ptr->openContent( reason ) ;
		if( !ok )
		{
			if( m_lock )
				static_cast<StoredMessage&>(*message_ptr).fail( reason , 0 ) ; // side-effect
			else
				G_WARNING( "GStore::MessageStore: ignoring \"" << m_iter.filePath() << "\": " << reason ) ;
			continue ;
		}

		return std::unique_ptr<StoredMessage>( message_ptr.release() ) ; // up-cast
	}
	return {} ;
}

// ===

GStore::FileStore::FileStore( const G::Path & dir , const Config & config ) :
	m_seq(config.seq) ,
	m_dir(dir) ,
	m_config(config)
{
	checkPath( dir ) ;
}

std::string GStore::FileStore::x()
{
	return "X-MailRelay-" ;
}

std::string GStore::FileStore::format( int generation )
{
	// use a weird prefix to help with file(1) and magic(5)
	if( generation == -4 )
		return "#2821.3" ; // original
	else if( generation == -3 )
		return "#2821.4" ; // new for 1.9
	else if( generation == -2 )
		return "#2821.5" ; // new for 2.0
	else if( generation == -1 )
		return "#2821.6" ; // new for 2.4
	else
		return "#2821.7" ; // new for 2.5
}

bool GStore::FileStore::knownFormat( const std::string & format_in )
{
	return
		format_in == format(0) ||
		format_in == format(-1) ||
		format_in == format(-2) ||
		format_in == format(-3) ||
		format_in == format(-4) ;
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
	{
		FileWriter claim_writer ; // seteuid(), umask(Tighter)
		G::File::open( *stream_ptr , path ) ;
	}
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
	else
		return m_dir + id.str().append(".envelope") ;
}

GStore::MessageId GStore::FileStore::newId()
{
	unsigned long timestamp = static_cast<unsigned long>(G::SystemTime::now().s()) ;

	m_seq++ ;
	if( m_seq == 0UL )
		m_seq++ ;

	std::ostringstream ss ;
	ss << "emailrelay." << G::Process::Id().str() << "." << timestamp << "." << m_seq ;
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

std::shared_ptr<GStore::MessageStore::Iterator> GStore::FileStore::iterator( bool lock )
{
	return iteratorImp( lock ) ;
}

std::shared_ptr<GStore::MessageStore::Iterator> GStore::FileStore::iteratorImp( bool lock )
{
	return std::make_shared<FileIterator>( *this , m_dir , lock , false ) ; // up-cast
}

std::shared_ptr<GStore::MessageStore::Iterator> GStore::FileStore::failures()
{
	return std::make_shared<FileIterator>( *this , m_dir , false , true ) ; // up-cast
}

std::unique_ptr<GStore::StoredMessage> GStore::FileStore::get( const MessageId & id )
{
	G::Path path = envelopePath( id ) ;

	auto message = std::make_unique<StoredFile>( *this , path ) ;
	if( !message->lock() )
		throw GetError( path.str() + ": cannot lock the file" ) ;

	std::string reason ;
	const bool check_recipients = false ; // don't check for no-remote-recipients
	if( !message->readEnvelope(reason,check_recipients) )
		throw GetError( path.str() + ": cannot read the envelope: " + reason ) ;

	if( !message->openContent(reason) )
		throw GetError( path.str() + ": cannot read the content: " + reason ) ;

	return std::unique_ptr<StoredMessage>( message.release() ) ; // up-cast
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

G::Slot::Signal<> & GStore::FileStore::messageStoreUpdateSignal()
{
	return m_update_signal ;
}

G::Slot::Signal<> & GStore::FileStore::messageStoreRescanSignal()
{
	return m_rescan_signal ;
}

void GStore::FileStore::rescan()
{
	messageStoreRescanSignal().emit() ;
}

void GStore::FileStore::unfailAll()
{
	unfailAllImp() ;
}

void GStore::FileStore::unfailAllImp()
{
	std::shared_ptr<MessageStore::Iterator> iter( failures() ) ;
	for(;;)
	{
		std::unique_ptr<StoredMessage> message = iter->next() ;
		if( message == nullptr )
			break ;
		G_DEBUG( "GStore::FileStore::unfailAllImp: " << message->location() ) ;
		message->unfail() ;
	}
}

#ifndef G_LIB_SMALL
void GStore::FileStore::clearAll()
{
	// for testing...
	std::shared_ptr<MessageStore::Iterator> iter( iteratorImp(true) ) ;
	for(;;)
	{
		std::unique_ptr<StoredMessage> message = iter->next() ;
		if( message )
			message->destroy() ;
		else
			break ;
	}
}
#endif

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

