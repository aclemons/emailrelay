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
/// \file gpopstore.cpp
///

#include "gdef.h"
#include "gpopstore.h"
#include "gstr.h"
#include "gfile.h"
#include "gdirectory.h"
#include "gtest.h"
#include "groot.h"
#include "gassert.h"
#include <sstream>
#include <fstream>

namespace GPop
{
	struct FileReader ;
	struct DirectoryReader ;
	struct FileDeleter ;
}

//| \class GPop::FileReader
/// A trivial class which is used like G::Root by GPop::Store for reading files.
/// The implementation does nothing because files in the pop store are group-readable.
///
struct GPop::FileReader
{
	FileReader() ;
} ;

//| \class GPop::DirectoryReader
/// A trivial class which is used like G::Root by GPop::Store for reading
/// directory listings.
///
struct GPop::DirectoryReader : private G::Root
{
	DirectoryReader() = default;
} ;

//| \class GPop::FileDeleter
/// A trivial specialisation of G::Root used by GPop::Store for deleting files.
/// The specialisation is not really necessary because the pop store directory
/// is group-writeable.
///
struct GPop::FileDeleter : private G::Root
{
} ;

// ==

GPop::Store::Store( const G::Path & path , bool by_name , bool allow_delete ) :
	m_path(path) ,
	m_by_name(by_name) ,
	m_allow_delete(allow_delete)
{
	checkPath( path , by_name , allow_delete ) ;
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

void GPop::Store::checkPath( const G::Path & dir_path , bool by_name , bool allow_delete )
{
	if( by_name )
	{
		if( !valid(dir_path,false) )
			throw InvalidDirectory() ;

		G::DirectoryList iter ;
		{
			DirectoryReader claim_reader ;
			iter.readAll( dir_path ) ;
		}

		int n = 0 ;
		while( iter.more() )
		{
			if( iter.isDir() )
			{
				n++ ;
				if( !valid(iter.filePath(),allow_delete) )
				{
					; // no-op -- warning only
				}
			}
		}
		if( n == 0 )
		{
			G_WARNING( "GPop::Store: no sub-directories for pop-by-name found in \"" << dir_path << "\": "
				<< "create one sub-directory for each authorised pop account" ) ;
		}
	}
	else if( !valid(dir_path,allow_delete) )
	{
		throw InvalidDirectory() ;
	}
}

bool GPop::Store::valid( const G::Path & dir_path , bool allow_delete )
{
	G::Directory dir_test( dir_path ) ;
	bool ok = false ;
	if( allow_delete )
	{
		std::string tmp_filename = G::Directory::tmp() ;
		FileDeleter claim_deleter ;
		ok = dir_test.valid() && dir_test.writeable( tmp_filename ) ;
	}
	else
	{
		FileReader claim_reader ;
		ok = dir_test.valid() ;
	}
	if( !ok )
	{
		const char * op = allow_delete ? "writing" : "reading" ;
		G_WARNING( "GPop::Store: directory not valid for " << op << ": \"" << dir_path << "\"" ) ;
	}
	return ok ;
}

// ===

GPop::StoreLock::File::File( const G::Path & content_path ) :
	name(content_path.basename()) ,
	size(toSize(G::File::sizeString(content_path.str())))
{
}

GPop::StoreLock::File::File( const std::string & content_name , const std::string & size_string ) :
	name(content_name) ,
	size(toSize(size_string))
{
}

bool GPop::StoreLock::File::operator<( const File & rhs ) const
{
	return name < rhs.name ;
}

GPop::StoreLock::Size GPop::StoreLock::File::toSize( const std::string & s )
{
	return G::Str::toULong( s , G::Str::Limited() ) ;
}

// ===

GPop::StoreLock::StoreLock( Store & store ) :
	m_store(&store)
{
}

void GPop::StoreLock::lock( const std::string & user )
{
	G_ASSERT( !locked() ) ;
	G_ASSERT( !user.empty() ) ;
	G_ASSERT( m_store != nullptr ) ;

	m_user = user ;
	m_dir = m_store->dir() ;
	if( m_store->byName() )
		m_dir.pathAppend( user ) ;

	// build a read-only list of files (inc. file sizes)
	{
		DirectoryReader claim_reader ;
		G::DirectoryList iter ;
		iter.readType( m_dir , ".envelope" ) ;
		while( iter.more() )
		{
			File file( contentPath(iter.fileName()) ) ;
			m_initial.insert( file ) ;
		}
	}

	if( G::Test::enabled("large-pop-list") )
	{
		// create a larger list
		std::size_t limit = m_initial.size() * 1000U ;
		for( std::size_t i = 0U ; i < limit ; i++ )
		{
			std::ostringstream ss ;
			ss << "dummy." << i << ".content" ;
			m_initial.insert( File(ss.str()) ) ;
		}
	}

	// take a mutable copy
	m_current = m_initial ;

	G_ASSERT( locked() ) ;
}

bool GPop::StoreLock::locked() const
{
	return m_store != nullptr && ! m_user.empty() ;
}

GPop::StoreLock::~StoreLock()
= default;

GPop::StoreLock::Size GPop::StoreLock::messageCount() const
{
	G_ASSERT( locked() ) ;
	return static_cast<Size>(m_current.size()) ;
}

GPop::StoreLock::Size GPop::StoreLock::totalByteCount() const
{
	G_ASSERT( locked() ) ;
	Size total = 0 ;
	for( const auto & item : m_current )
		total += item.size ;
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
	auto initial_p = m_initial.begin() ;
	for( int i = 1 ; i < id && initial_p != m_initial.end() ; i++ , ++initial_p ) ;
	return initial_p ;
}

GPop::StoreLock::Set::const_iterator GPop::StoreLock::find( int id ) const
{
	G_ASSERT( valid(id) ) ;
	auto initial_p = m_initial.begin() ;
	for( int i = 1 ; i < id && initial_p != m_initial.end() ; i++ , ++initial_p ) ;
	return initial_p ;
}

GPop::StoreLock::Set::iterator GPop::StoreLock::find( const std::string & name )
{
	auto current_p = m_current.begin() ;
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
	for( auto p = m_current.begin() ; p != m_current.end() ; ++p , i++ )
	{
		if( id == -1 || id == i )
			list.push_back( Entry(i,(*p).size,(*p).name) ) ;
	}
	return list ;
}

std::unique_ptr<std::istream> GPop::StoreLock::get( int id ) const
{
	G_ASSERT( locked() ) ;
	G_ASSERT( valid(id) ) ;

	G_DEBUG( "GPop::StoreLock::get: " << id << ": " << path(id) ) ;

	auto file = std::make_unique<std::ifstream>() ;
	G::Path file_path = path( id ) ;
	{
		FileReader claim_reader ;
		G::File::open( *file , file_path ) ;
	}

	if( !file->good() )
		throw CannotRead( file_path.str() ) ;

	return std::unique_ptr<std::istream>( file.release() ) ;
}

void GPop::StoreLock::remove( int id )
{
	G_ASSERT( locked() ) ;
	G_ASSERT( valid(id) ) ;

	auto initial_p = find( id ) ;
	auto current_p = find( (*initial_p).name ) ;
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
		m_store = nullptr ;
		doCommit( *store ) ;
	}
	m_store = nullptr ;
}

void GPop::StoreLock::doCommit( Store & store ) const
{
	bool all_ok = true ;
	for( const auto & item : m_deleted )
	{
		if( store.allowDelete() )
		{
			deleteFile( envelopePath(item) , all_ok ) ;
			if( unlinked(store,item) ) // race condition could leave content files undeleted
				deleteFile( contentPath(item) , all_ok ) ;
		}
		else
		{
			G_DEBUG( "StoreLock::doCommit: not deleting \"" << item.name << "\"" ) ;
		}
	}
	if( ! all_ok )
		throw CannotDelete() ;
}

void GPop::StoreLock::deleteFile( const G::Path & path , bool & all_ok ) const
{
	bool ok = false ;
	{
		FileDeleter claim_deleter ;
		ok = G::File::remove( path , std::nothrow ) ;
	}
	all_ok = ok && all_ok ;
	if( ! ok )
		G_ERROR( "StoreLock::remove: failed to delete " << path ) ;
}

std::string GPop::StoreLock::uidl( int id ) const
{
	G_ASSERT( valid(id) ) ;
	auto p = find( id ) ;
	return (*p).name ;
}

G::Path GPop::StoreLock::path( int id ) const
{
	G_ASSERT( valid(id) ) ;
	auto p = find( id ) ;
	const File & file = (*p) ;
	return contentPath( file ) ;
}

G::Path GPop::StoreLock::path( const std::string & filename , bool fallback ) const
{
	// expected path
	G::Path path_1 = m_dir ;
	path_1.pathAppend( filename ) ;

	// or fallback to the parent directory
	G::Path path_2 = m_dir ; path_2.pathAppend("..") ;
	path_2.pathAppend( filename ) ;

	return ( fallback && !G::File::exists(path_1,std::nothrow) ) ? path_2 : path_1 ;
}

std::string GPop::StoreLock::envelopeName( const std::string & content_name ) const
{
	std::string filename = content_name ;
	G::Str::replace( filename , "content" , "envelope" ) ;
	return filename ;
}

std::string GPop::StoreLock::contentName( const std::string & envelope_name ) const
{
	std::string filename = envelope_name ;
	G::Str::replace( filename , "envelope" , "content" ) ;
	return filename ;
}

G::Path GPop::StoreLock::contentPath( const std::string & envelope_name ) const
{
	const bool try_parent_directory = true ;
	return path( contentName(envelope_name) , try_parent_directory ) ;
}

G::Path GPop::StoreLock::contentPath( const File & file ) const
{
	const bool try_parent_directory = true ;
	return path( file.name , try_parent_directory ) ;
}

G::Path GPop::StoreLock::envelopePath( const File & file ) const
{
	const bool try_parent_directory = false ;
	return path( envelopeName(file.name) , try_parent_directory ) ;
}

void GPop::StoreLock::rollback()
{
	G_ASSERT( locked() ) ;
	m_deleted.clear() ;
	m_current = m_initial ;
}

bool GPop::StoreLock::unlinked( Store & store , const File & file ) const
{
	if( !store.byName() )
	{
		G_DEBUG( "StoreLock::unlinked: unlinked since not pop-by-name: " << file.name ) ;
		return true ;
	}

	G::Path normal_content_path = m_dir ; normal_content_path.pathAppend( file.name ) ;
	if( G::File::exists(normal_content_path,std::nothrow) )
	{
		G_DEBUG( "StoreLock::unlinked: unlinked since in its own directory: " << normal_content_path ) ;
		return true ;
	}

	// look for corresponding envelopes in all child directories
	bool found = false ;
	{
		G::DirectoryList iter ;
		{
			DirectoryReader claim_reader ;
			iter.readAll( store.dir() ) ;
		}
		while( iter.more() )
		{
			if( ! iter.isDir() ) continue ;
			G_DEBUG( "Store::unlinked: checking sub-directory: " << iter.fileName() ) ;
			G::Path envelope_path = iter.filePath() ; envelope_path.pathAppend(envelopeName(file.name)) ;
			if( G::File::exists(envelope_path,std::nothrow) )
			{
				G_DEBUG( "StoreLock::unlinked: still in use: envelope exists: " << envelope_path ) ;
				found = true ;
				break ;
			}
		}
	}

	if( ! found )
	{
		G_DEBUG( "StoreLock::unlinked: unlinked since no envelope found in any sub-directory" ) ;
		return true ;
	}

	return false ;
}

GPop::FileReader::FileReader() // NOLINT modernize-use-equals-default because of -Wunused
{
}

