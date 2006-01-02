//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gregistry_win32.cpp
//

#include "gdef.h"
#include "gregistry.h"
#include "gstr.h"
#include "gpath.h"
#include "gdebug.h"
#include "gassert.h"
#include "glog.h"
#include <sstream>

class G::RegistryKeyImp 
{
public:
	HKEY m_key ;
	unsigned long m_ref_count ;
	explicit RegistryKeyImp( HKEY k ) : m_key(k) , m_ref_count(1UL) {}
} ;

// ===

G::RegistryKey::RegistryKey( const Imp & imp , bool is_root ) :
	m_imp( new Imp(imp) ) ,
	m_is_root(is_root)
{
	m_imp->m_ref_count = 1UL ;
}

G::RegistryKey::RegistryKey( const RegistryKey & other ) :
	m_imp( other.m_imp ) ,
	m_is_root( other.m_is_root )
{
	m_imp->m_ref_count++ ;
}

bool G::RegistryKey::valid() const
{
	return m_imp->m_key != 0 ;
}

void G::RegistryKey::operator=( const RegistryKey & other )
{
	down() ;
	m_imp = other.m_imp ;
	m_imp->m_ref_count++ ;
}

const G::RegistryKeyImp & G::RegistryKey::imp() const
{
	return *m_imp ;
}

G::RegistryKey::~RegistryKey()
{
	down() ;
}

void G::RegistryKey::down()
{
	m_imp->m_ref_count-- ;
	if( m_imp->m_ref_count == 0U && !m_is_root && m_imp->m_key != 0 )
	{
		G_DEBUG( "G::RegistryKey::down: closing " << m_imp->m_key ) ;
		::RegCloseKey( m_imp->m_key ) ;
		delete m_imp ;
		m_imp = NULL ;
	}
}

G::RegistryKey G::RegistryKey::create( const std::string & path ) const
{
	bool is_new = false ;
	return create( path , is_new ) ;
}

G::RegistryKey G::RegistryKey::create( std::string path , bool & is_new ) const
{
	if( !valid() ) throw InvalidHandle() ;
	G::Str::replaceAll( path , "/" , "\\" ) ;

	is_new = false ;
	HKEY new_key = 0 ;
	DWORD disposition = 0 ;
	LONG rc = ::RegCreateKeyEx( m_imp->m_key , path.c_str() , 0 ,
		NULL , // ???
		REG_OPTION_NON_VOLATILE ,
		KEY_ALL_ACCESS , NULL ,
		&new_key ,
		&disposition ) ;

	if( rc != ERROR_SUCCESS )
	{
		if( new_key != 0 ) 
			::RegCloseKey(new_key) ;
		G_DEBUG( "G::RegistryKey::create: failed to create \"" << path << "\"" ) ;
		throw Error( path ) ;
	}
	is_new = disposition == REG_CREATED_NEW_KEY ;
	G_DEBUG( "G::RegistryKey::create: " << new_key << ": \"" << path << "\"" << (is_new?" [created]":"") ) ;
	return RegistryKey( Imp(new_key) , false ) ;
}

G::RegistryKey G::RegistryKey::open( const std::string & path , const NoThrow & ) const
{
	return open( path , false ) ;
}

G::RegistryKey G::RegistryKey::open( const std::string & path ) const
{
	return open( path , true ) ;
}

G::RegistryKey G::RegistryKey::open( std::string path , bool do_throw ) const
{
	if( !valid() ) 
	{
		if( do_throw )
			throw InvalidHandle() ;
		G_DEBUG( "G::RegistryKey::open: failed to open \"" << path << "\"" ) ;
		return RegistryKey( Imp(0) , false ) ;
	}
	else
	{
		G::Str::replaceAll( path , "/" , "\\" ) ;

		HKEY new_key = 0 ;
		LONG rc = ::RegOpenKeyEx( m_imp->m_key , path.c_str() , 0 ,
			KEY_ALL_ACCESS , &new_key ) ;

		G_ASSERT( (rc == ERROR_SUCCESS) == (new_key != 0) ) ;
		if( rc == ERROR_SUCCESS )
		{
			G_DEBUG( "G::RegistryKey::open: " << new_key << ": \"" << path << "\"" ) ;
		}
		else
		{
			if( new_key != 0 ) ::RegCloseKey(new_key) ;
			if( do_throw ) throw Error( path ) ;
			new_key = 0 ;
			G_DEBUG( "G::RegistryKey::open: failed to open \"" << path << "\"" ) ;
		}
		return RegistryKey( Imp(new_key) , false ) ;
	}
}

void G::RegistryKey::remove( const std::string & sub_key , const NoThrow & ) const
{
	remove( sub_key , false ) ;
}

void G::RegistryKey::remove( const std::string & sub_key ) const
{
	remove( sub_key , true ) ;
}

void G::RegistryKey::remove( const std::string & sub_key , bool do_throw ) const
{
	if( !valid() )
	{
		if( do_throw ) 
			throw InvalidHandle() ;
		G_DEBUG( "G::RegistryKey::remove: failed to remove \"" << sub_key << "\"" ) ;
	}
	else
	{
		LONG rc = ::RegDeleteKey( m_imp->m_key , sub_key.c_str() ) ;
		if( rc == ERROR_SUCCESS )
		{
			G_DEBUG( "G::RegistryKey::remove: removed \"" << sub_key << "\"" ) ;
		}
		else
		{
			if( do_throw )
				throw RemoveError( sub_key ) ;
			G_DEBUG( "G::RegistryKey::remove: failed to remove \"" << sub_key << "\"" ) ;
		}
	}
}

G::RegistryKey G::RegistryKey::currentUser()
{
	return RegistryKey( Imp(HKEY_CURRENT_USER) , true ) ;
}

G::RegistryKey G::RegistryKey::localMachine()
{
	return RegistryKey( Imp(HKEY_LOCAL_MACHINE) , true ) ;
}

G::RegistryKey G::RegistryKey::classes()
{
	return RegistryKey( Imp(HKEY_CLASSES_ROOT) , true ) ;
}

// ===

G::RegistryValue::RegistryValue( const RegistryKey & hkey , 
	const std::string & key_name ) :
		m_hkey(hkey) ,
		m_key_name(key_name)
{
	if( ! hkey.valid() )
		throw InvalidHandle( key_name ) ;
}

std::string G::RegistryValue::getString( const std::string & defolt ) const
{
	return getString( defolt , false ) ;
}

std::string G::RegistryValue::getString() const
{
	return getString( std::string() , true ) ;
}

std::string G::RegistryValue::getString( const std::string & defolt , 
	bool do_throw ) const
{
	DWORD type = 0 ;
	bool exists = true ;
	std::string result = getData( type , exists ) ;
	if( !exists && do_throw )
	{
		throw MissingValue( m_key_name ) ;
	}
	else if( !exists )
	{
		return defolt ;
	}
	else if( type != REG_SZ && type != REG_EXPAND_SZ )
	{
		throw InvalidType( m_key_name ) ;
	}
	else
	{
		G::Str::trimRight( result , std::string(1U,'\0') ) ;
		return result ;
	}
}

g_uint32_t G::RegistryValue::getDword() const
{
	DWORD type = 0 ;
	bool exists = true ;
	std::string data = getData( type , exists ) ;
	if( !exists ) throw MissingValue( m_key_name ) ;
	if( type != REG_DWORD ) throw InvalidType( m_key_name ) ;
	if( data.length() != 4U ) throw ValueError( m_key_name ) ;
	g_uint32_t result = 0UL ;
	for( int i = 3 ; i >= 0 ; i-- )
	{
		result *= 256UL ;
		result += static_cast<unsigned char>(data[i]) ;
	}
	return result ;
}

bool G::RegistryValue::getBool() const
{
	g_uint32_t n = getDword() ;
	if( n != 0UL && n != 1UL ) throw ValueError( m_key_name ) ;
	return !!n ;
}

std::string G::RegistryValue::getData( g_uint32_t & type , bool & exists ) const
{
	exists = true ;
	std::pair<DWORD,size_t> info = getInfo() ;
	if( info.second == 0U )
	{
		exists = false ;
		return std::string() ;
	}

	type = info.first ;
	size_t buffer_size = info.second + 1U ;
	char * buffer = new char [buffer_size] ;

	try
	{
		buffer_size = get( buffer , buffer_size ) ;
	}
	catch(...)
	{
		delete [] buffer ;
		throw ;
	}

	std::string result( buffer , buffer_size ) ;
	delete [] buffer ;
	return result ;
}

size_t G::RegistryValue::get( char * buffer , size_t buffer_size ) const
{
	DWORD type = 0 ;
	unsigned long size = buffer_size ;
	LONG rc = ::RegQueryValueEx( m_hkey.imp().m_key , m_key_name.c_str() , 0 ,
		&type , reinterpret_cast<unsigned char*>(buffer) , &size ) ;

	if( rc != ERROR_SUCCESS )
	{
		std::ostringstream ss ;
		ss << "get: RegQueryValueEx(" << m_key_name << "): " << rc ;
		throw ValueError( ss.str() ) ;
	}

	return static_cast<size_t>(size) ;
}

std::pair<g_uint32_t,size_t> G::RegistryValue::getInfo() const
{
	std::pair<DWORD,size_t> result( 0 , 0 ) ;

	unsigned long size = 0 ;
	LONG rc = ::RegQueryValueEx( m_hkey.imp().m_key , m_key_name.c_str() , 0 ,
		&result.first , NULL , &size ) ;

	if( rc != ERROR_SUCCESS && rc != ERROR_FILE_NOT_FOUND )
	{
		std::ostringstream ss ;
		ss << "RegQueryValueEx(\"" << m_key_name << "\"): " << rc ;
		throw ValueError( ss.str() ) ;
	}

	if( rc == ERROR_FILE_NOT_FOUND )
		size = 0 ; // just in case

	result.second = static_cast<size_t>(size) ;
	return result ;
}

void G::RegistryValue::set( const char * p )
{
	set( std::string(p?p:"") ) ;
}

void G::RegistryValue::set( const std::string & s )
{
	set( REG_SZ , reinterpret_cast<const void*>(s.c_str()) , s.length()+1U ) ;
}

void G::RegistryValue::set( bool b )
{
	g_uint32_t n = b ? 1UL : 0UL ;
	set( n ) ;
}

void G::RegistryValue::set( g_uint32_t n )
{
	set( REG_DWORD , reinterpret_cast<const void*>(&n) , 4UL ) ;
}

void G::RegistryValue::set( g_uint32_t type , const void * vp , size_t n )
{
	const BYTE * p = reinterpret_cast<const BYTE*>(vp) ;
	LONG rc = ::RegSetValueEx( m_hkey.imp().m_key , m_key_name.c_str() , 0 , type , p , n ) ;
	if( rc != ERROR_SUCCESS )
		throw ValueError( "RegSetValueEx" ) ;
}

