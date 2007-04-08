//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gfilestore.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gnoncopyable.h"
#include "gfilestore.h"
#include "gnewfile.h"
#include "gstoredfile.h"
#include "gprocess.h"
#include "gdirectory.h"
#include "gmemory.h"
#include "gpath.h"
#include "gfile.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <iostream>
#include <fstream>

namespace GSmtp
{
	class FileIterator ;
}

/// \class GSmtp::FileIterator
/// A 'body' class for the MessageStore::Iterator
///  'handle'. The handle/body pattern allows us to copy
///  iterators by value, and therefore return them
///  from MessageStore::iterator().
/// 
class GSmtp::FileIterator : public GSmtp::MessageStore::IteratorImp , public G::noncopyable 
{
public:
	FileIterator( FileStore & store , const G::Directory & dir , bool lock ) ;
	virtual std::auto_ptr<GSmtp::StoredMessage> next() ;
private:
	FileStore & m_store ;
	G::DirectoryIterator m_iter ;
	bool m_lock ;
} ;

// ===

GSmtp::FileIterator::FileIterator( FileStore & store , const G::Directory & dir , bool lock ) :
	m_store(store) ,
	m_iter(dir,"*.envelope") ,
	m_lock(lock)
{
}

std::auto_ptr<GSmtp::StoredMessage> GSmtp::FileIterator::next()
{
	while( !m_iter.error() && m_iter.more() )
	{
		std::auto_ptr<StoredFile> m( new StoredFile(m_store,m_iter.filePath()) ) ;
		if( m_lock && !m->lock() )
		{
			G_WARNING( "GSmtp::MessageStore: cannot lock file: \"" << m_iter.filePath() << "\"" ) ;
			continue ;
		}

		std::string reason ;
		const bool check = m_lock ; // check for no-remote-recipients
		bool ok = m->readEnvelope(reason,check) && m->openContent(reason) ;
		if( ok )
			return std::auto_ptr<StoredMessage>( m.release() ) ;

		if( m_lock )
			m->fail( reason ) ;
		else
			G_WARNING( "GSmtp::MessageStore: ignoring \"" << m_iter.filePath() << "\": " << reason ) ;
	}
	return std::auto_ptr<StoredMessage>(NULL) ;
}

// ===

GSmtp::FileStore::FileStore( const G::Path & dir , bool optimise ) : 
	m_seq(1UL) ,
	m_dir(dir) ,
	m_optimise(optimise) ,
	m_empty(false) ,
	m_repoll(false)
{
	m_pid_modifier = static_cast<unsigned long>(G::DateTime::now()) % 1000000UL ;
	checkPath( dir ) ;
}

//static
std::string GSmtp::FileStore::x()
{
	return "X-MailRelay-" ;
}

//static
std::string GSmtp::FileStore::format( int n )
{
	if( n == 0 )
		return "#2821.3" ; // current -- includes message authentication and client ip
	else
		return "#2821.2" ; // old
}

//static
void GSmtp::FileStore::checkPath( const G::Path & directory_path )
{
	G::Directory dir_test( directory_path ) ;
	bool ok = false ;

	// fail if not readable (after switching effective userid)
	{
		FileWriter claim_writer ;
		ok = dir_test.valid() ;
	}
	if( !ok )
	{
		throw InvalidDirectory( directory_path.str() ) ;
	}

	// warn if not writeable (after switching effective userid)
	{
		std::string tmp = G::Directory::tmp() ;
		FileWriter claim_writer ;
		ok = dir_test.writeable(tmp) ;
	}
	if( !ok )
	{
		G_WARNING( "GSmtp::MessageStore: directory not writable: \"" << directory_path << "\"" ) ;
	}
}

std::auto_ptr<std::ostream> GSmtp::FileStore::stream( const G::Path & path )
{
	FileWriter claim_writer ;
	std::auto_ptr<std::ostream> ptr( 
		new std::ofstream( path.pathCstr() , 
			std::ios_base::binary | std::ios_base::out | std::ios_base::trunc ) ) ;
	return ptr ;
}

G::Path GSmtp::FileStore::contentPath( unsigned long seq ) const
{
	return fullPath( filePrefix(seq) + ".content" ) ;
}

G::Path GSmtp::FileStore::envelopePath( unsigned long seq ) const
{
	return fullPath( filePrefix(seq) + ".envelope" ) ;
}

G::Path GSmtp::FileStore::envelopeWorkingPath( unsigned long seq ) const
{
	return fullPath( filePrefix(seq) + ".envelope.new" ) ;
}

std::string GSmtp::FileStore::filePrefix( unsigned long seq ) const
{
	std::ostringstream ss ;
	G::Process::Id pid ;
	ss << "emailrelay." << pid.str() << "." << m_pid_modifier << "." << seq ;
	return ss.str() ;
}

G::Path GSmtp::FileStore::fullPath( const std::string & filename ) const
{
	G::Path p( m_dir ) ;
	p.pathAppend( filename ) ;
	return p ;
}

unsigned long GSmtp::FileStore::newSeq()
{
	m_seq++ ;
	if( m_seq == 0UL )
		m_seq++ ;
	return m_seq ;
}

bool GSmtp::FileStore::empty() const
{
	if( m_optimise )
	{
		if( !m_empty )
			const_cast<FileStore*>(this)->m_empty = emptyCore() ;
		return m_empty ;
	}
	else
	{
		return emptyCore() ;
	}
}

bool GSmtp::FileStore::emptyCore() const
{
	FileReader claim_reader ;
	G::Directory dir( m_dir ) ;
	G::DirectoryIterator iter( dir , "*.envelope" ) ;
	const bool no_more = iter.error() || !iter.more() ;
	return no_more ;
}

GSmtp::MessageStore::Iterator GSmtp::FileStore::iterator( bool lock )
{
	FileReader claim_reader ;
	return MessageStore::Iterator( new FileIterator(*this,G::Directory(m_dir),lock) ) ;
}

std::auto_ptr<GSmtp::StoredMessage> GSmtp::FileStore::get( unsigned long id )
{
	G::Path path = envelopePath( id ) ;

	std::auto_ptr<StoredFile> message( new StoredFile(*this,path) ) ;

	if( ! message->lock() )
		throw GetError( path.str() + ": cannot lock the file" ) ;

	std::string reason ;
	const bool check = false ; // don't check for no-remote-recipients
	if( ! message->readEnvelope(reason,check) )
		throw GetError( path.str() + ": cannot read the envelope: " + reason ) ;

	if( ! message->openContent(reason) )
		throw GetError( path.str() + ": cannot read the content: " + reason ) ;

	std::auto_ptr<StoredMessage> message_base_ptr( message.release() ) ;
	return message_base_ptr ;
}

std::auto_ptr<GSmtp::NewMessage> GSmtp::FileStore::newMessage( const std::string & from )
{
	m_empty = false ;
	return std::auto_ptr<NewMessage>( new NewFile(from,*this) ) ;
}

void GSmtp::FileStore::updated()
{
	G_DEBUG( "GSmtp::FileStore::updated" ) ;
	bool repoll = m_repoll ;
	m_repoll = false ;
	m_signal.emit( repoll ) ;
}

G::Signal1<bool> & GSmtp::FileStore::signal()
{
	return m_signal ;
}

void GSmtp::FileStore::repoll()
{
	m_repoll = true ;
}

// ===

GSmtp::FileReader::FileReader()
{
}

GSmtp::FileReader::~FileReader()
{
}

// ===

GSmtp::FileWriter::FileWriter() :
	m_root(false) ,
	m_umask(G::Process::Umask::Tighter)
{
}

GSmtp::FileWriter::~FileWriter()
{
}


