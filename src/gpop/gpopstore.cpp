//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpopstore.cpp
//

#include "gdef.h"
#include "gpop.h"
#include "gpopstore.h"
#include "gstr.h"
#include "gfile.h"
#include "gdirectory.h"
#include "gmemory.h"
#include "groot.h"
#include "gassert.h"
#include <sstream>
#include <fstream>

struct FileReader // stub -- message files are readable by group "daemon"
{
	FileReader() {}
} ;

// ==

struct FileDeleter : private G::Root // not really necessary -- the spool directory is writeable by group "daemon"
{
} ;

// ==

GPop::Store::Store( G::Path path , bool by_name , bool allow_delete ) :
	m_path(path) ,
	m_by_name(by_name) ,
	m_allow_delete(allow_delete)
{
}

G::Path GPop::Store::dir() const
{
	return m_path ;
}

bool GPop::Store::allowDelete() const
{
	return m_allow_delete ;
}

bool GPop::Store::byName() const
{
	return m_by_name ;
}

// ===

GPop::StoreLock::File::File( const G::Path & path ) :
	name(path.basename()) ,
	size(toSize(G::File::sizeString(path.str())))
{
}

GPop::StoreLock::File::File( const std::string & name_ , const std::string & size_string ) :
	name(name_) ,
	size(toSize(size_string))
{
}

bool GPop::StoreLock::File::operator<( const File & rhs ) const
{
	return name < rhs.name ;
}

GPop::StoreLock::Size GPop::StoreLock::File::toSize( const std::string & s )
{
	// could do better...
	Size size = 0 ;
	std::stringstream ss ;
	ss << s ;
	ss >> size ;
	return size ;
}

// ===

GPop::StoreLock::StoreLock( Store & store ) :
	m_store(&store)
{
}

void GPop::StoreLock::lock( const std::string & user )
{
	G_ASSERT( ! locked() ) ;
	G_ASSERT( ! user.empty() ) ;
	G_ASSERT( m_store != NULL ) ;

	m_user = user ;
	m_dir = m_store->dir() ;
	if( m_store->byName() )
		m_dir.pathAppend( user ) ;

	// build a read-only list of files (inc. file sizes)
	{
		FileReader claim_reader ;
		G::Directory dir( m_dir ) ;
		G::DirectoryIterator iter( dir , "emailrelay.*.envelope" ) ;
		while( iter.more() )
		{
			File file( contentPath(iter.fileName().str()) ) ;
			G_DEBUG( "StoreLock::lock: " << file.name << " (" << file.size << ")" ) ;
			m_initial.insert( file ) ;
		}
	}

	// take a mutable copy
	m_current = m_initial ;

	G_ASSERT( locked() ) ;
}

bool GPop::StoreLock::locked() const
{
	return m_store != NULL && ! m_user.empty() ;
}

GPop::StoreLock::~StoreLock() 
{ 
}

GPop::StoreLock::Size GPop::StoreLock::messageCount() const
{
	G_ASSERT( locked() ) ;
	return m_current.size() ;
}

GPop::StoreLock::Size GPop::StoreLock::totalByteCount() const
{
	G_ASSERT( locked() ) ;
	Size total = 0 ;
	for( Set::const_iterator p = m_current.begin() ; p != m_current.end() ; ++p )
		total += (*p).size ;
	return total ;
}

bool GPop::StoreLock::valid( int id ) const
{
	G_ASSERT( locked() ) ;
	return id >= 1 && id <= static_cast<int>(m_initial.size()) ;
}

GPop::StoreLock::Set::iterator GPop::StoreLock::find( int id )
{
	G_ASSERT( valid(id) ) ;
	Set::const_iterator initial_p = m_initial.begin() ;
	for( int i = 1 ; i < id && initial_p != m_initial.end() ; i++ , ++initial_p ) ;
	return initial_p ;
}

GPop::StoreLock::Set::const_iterator GPop::StoreLock::find( int id ) const
{
	G_ASSERT( valid(id) ) ;
	Set::const_iterator initial_p = m_initial.begin() ;
	for( int i = 1 ; i < id && initial_p != m_initial.end() ; i++ , ++initial_p ) ;
	return initial_p ;
}

GPop::StoreLock::Set::iterator GPop::StoreLock::find( const std::string & name )
{
	Set::const_iterator current_p = m_current.begin() ;
	for( ; current_p != m_current.end() ; ++current_p )
	{
		if( (*current_p).name == name )
			break ;
	}
	return current_p ;
}

GPop::StoreLock::Size GPop::StoreLock::byteCount( int id ) const
{
	G_ASSERT( locked() ) ;
	return (*find(id)).size ;
}

GPop::StoreLock::List GPop::StoreLock::list( int id ) const
{
	G_ASSERT( locked() ) ;
	List list ;
	int i = 1 ;
	for( Set::const_iterator p = m_current.begin() ; p != m_current.end() ; ++p , i++ )
	{
		if( id == -1 || id == i )
			list.push_back( Entry(i,(*p).size,(*p).name) ) ;
	}
	return list ;
}

std::auto_ptr<std::istream> GPop::StoreLock::get( int id ) const
{
	G_ASSERT( locked() ) ;
	G_ASSERT( valid(id) ) ;

	G_DEBUG( "GPop::StoreLock::get: " << id << ": " << path(id) ) ;

	std::auto_ptr<std::ifstream> file ;
	FileReader claim_reader ;
	file <<= new std::ifstream( path(id).str().c_str() ) ;
	return std::auto_ptr<std::istream>( file.release() ) ;
}

void GPop::StoreLock::remove( int id )
{
	G_ASSERT( locked() ) ;
	G_ASSERT( valid(id) ) ;

	Set::iterator initial_p = find( id ) ;
	Set::iterator current_p = find( (*initial_p).name ) ;
	if( current_p != m_current.end() )
	{
		m_deleted.insert( *initial_p ) ;
		m_current.erase( current_p ) ;
	}
}

void GPop::StoreLock::commit()
{
	G_ASSERT( locked() ) ;
	if( m_store )
	{
		Store * store = m_store ;
		m_store = NULL ;
		doCommit( *store ) ;
	}
	m_store = NULL ;
}

void GPop::StoreLock::doCommit( Store & store ) const
{
	bool all_ok = true ;
	for( Set::const_iterator p = m_deleted.begin() ; p != m_deleted.end() ; ++p )
	{
		if( store.allowDelete() )
		{
			deleteFile( envelopePath(*p) , all_ok ) ;
			deleteFile( contentPath(*p) , all_ok ) ;
		}
		else
		{
			G_DEBUG( "StoreLock::doCommit: not deleting \"" << (*p).name << "\"" ) ;
		}
	}
	if( ! all_ok )
		throw CannotDelete() ;
}

void GPop::StoreLock::deleteFile( const G::Path & path , bool & all_ok ) const
{
	FileDeleter claim_deleter ;
	bool ok = G::File::remove( path , G::File::NoThrow() ) ;
	all_ok = ok && all_ok ;
	if( ! ok )
		G_ERROR( "StoreLock::remove: failed to delete " << path ) ;
}

std::string GPop::StoreLock::uidl( int id ) const
{
	G_ASSERT( valid(id) ) ;
	Set::const_iterator p = find(id) ;
	return (*p).name ;
}

G::Path GPop::StoreLock::path( int id ) const
{
	G_ASSERT( valid(id) ) ;
	Set::const_iterator p = find(id) ;
	const File & file = (*p) ;
	return contentPath( file ) ;
}

G::Path GPop::StoreLock::path( const std::string & filename ) const
{
	G::Path result = m_dir ;
	result.pathAppend( filename ) ;
	return result ;
}

std::string GPop::StoreLock::contentName( const std::string & envelope_name ) const
{
	std::string filename = envelope_name ;
	G::Str::replace( filename , "envelope" , "content" ) ;
	return filename ;
}

G::Path GPop::StoreLock::contentPath( const std::string & envelope_name ) const
{
	return path( contentName(envelope_name) ) ;
}

G::Path GPop::StoreLock::contentPath( const File & file ) const
{
	return path( file.name ) ;
}

G::Path GPop::StoreLock::envelopePath( const File & file ) const
{
	std::string filename = file.name ;
	G::Str::replace( filename , "content" , "envelope" ) ;
	return path( filename ) ;
}

void GPop::StoreLock::rollback()
{
	G_ASSERT( locked() ) ;
	m_deleted.clear() ;
	m_current = m_initial ;
}

