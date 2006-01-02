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
// gregistry.h
//

#ifndef G_REGISTRY_H
#define G_REGISTRY_H

#include "gdef.h"
#include "gexception.h"
#include <string>

namespace G
{
	class RegistryKey ;
	class RegistryKeyImp ;
	class RegistryValue ;
}

// Class: G::RegistryKey
// Description: Used to navigate the system registry. Works with
// the G::RegistryValue class to get and set values.
// See also: G::RegistryValue
//
class G::RegistryKey 
{
public:
	G_EXCEPTION( InvalidHandle , "registry handle error" ) ;
	G_EXCEPTION( RemoveError , "registry removal error" ) ;
	G_EXCEPTION( Error , "registry error" ) ;
	struct NoThrow // Overload discriminator for G::RegistryKey.
		{} ;

	static RegistryKey currentUser() ;
		// Returns a key for the current-user "hive".

	static RegistryKey localMachine() ;
		// Returns a key for the local-machine "hive".

	static RegistryKey classes() ;
		// Returns a key for the classes-root "hive".

	~RegistryKey() ;
		// Destructor.

	RegistryKey create( std::string sub_path , bool & created ) const ;
		// Opens or creates a sub-key.

	RegistryKey create( const std::string & sub_path ) const ;
		// Opens or creates a sub-key.

	RegistryKey open( const std::string & sub_path ) const ;
		// Opens an existing sub-key. Throws if non-existant.

	RegistryKey open( const std::string & sub_path , const NoThrow & ) const ;
		// Opens an existing sub-key. Returns
		// an invalid key on error (eg. if non-existant).

	bool valid() const ;
		// Returns true if a valid key.
		// (Invalid keys are only created
		// by the NoThrow overload of
		// open().)

	RegistryKey( const RegistryKey & ) ;
		// Copy ctor.

	void operator=( const RegistryKey & ) ;
		// Assignment operator.

	void remove( const std::string & sub_path ) const ;
		// Removes the named sub-key.
		// Throws on error.

	void remove( const std::string & sub_path , const NoThrow & ) const ;
		// Removes the named sub-key.
		// Ignores errors.

	const RegistryKeyImp & imp() const ;
		// Used by RegistryValue.

private:
	typedef RegistryKeyImp Imp ;
	RegistryKey( const Imp & , bool ) ;
	void down() ;
	RegistryKey open( std::string , bool ) const ;
	void remove( const std::string & , bool ) const ;

private:
	Imp * m_imp ;
	bool m_is_root ;
} ;

// Class: G::RegistryValue
// Description: Works with G::RegistryKey to get and set
// registry values.
// See also: G::RegistryKey
//
class G::RegistryValue 
{
public:
	G_EXCEPTION( InvalidHandle , "registry handle error" ) ;
	G_EXCEPTION( ValueError , "registry value error" ) ;
	G_EXCEPTION( InvalidType , "registry type error" ) ;
	G_EXCEPTION( MissingValue , "missing registry value" ) ;

	explicit RegistryValue( const RegistryKey & hkey , 
		const std::string & name = std::string() ) ;
			// Constructor.

	std::string getString() const ;
		// Returns a string value. Throws if the value
		// does not exist, or if it is not a string type.

	std::string getString( const std::string & defolt ) const ;
		// Returns a string. Returns the supplied default 
		// value if it does not exist.

	bool getBool() const ;
		// Returns a boolean value. Throws if the value
		// does not exist.

	g_uint32_t getDword() const ;
		// Returns an unsigned 32-bit value. Throws if the value
		// does not exist.

	void set( const std::string & ) ;
		// Stores a string value.

	void set( const char * ) ;
		// Stores a string value.

	void set( bool ) ;
		// Stores a boolean value.

	void set( g_uint32_t ) ;
		// Stores an unsigned 32-bit value.

private:
	std::string getString( const std::string & defolt , bool ) const ;
	std::pair<g_uint32_t,size_t> getInfo() const ;
	std::string getData( g_uint32_t & type , bool & ) const ;
	size_t get( char * , size_t ) const ;
	void set( g_uint32_t type , const void * p , size_t n ) ;

private:
	const RegistryKey & m_hkey ;
	std::string m_key_name ;
} ;

#endif
