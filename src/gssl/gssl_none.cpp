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
/// \file gssl_none.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "gexception.h"
#include <memory>
#include <utility>

GSsl::Library * GSsl::Library::m_this = nullptr ;

GSsl::Library::Library( const bool , const std::string & , LogFn , bool )
{
	if( m_this == nullptr )
		m_this = this ;
}

GSsl::Library::~Library()
{
	if( this == m_this )
		m_this = nullptr ;
}

void GSsl::Library::log( int , const std::string & )
{
}

bool GSsl::Library::real()
{
	return false ;
}

std::string GSsl::Library::id() const
{
	return ids() ;
}

GSsl::Library * GSsl::Library::instance()
{
	return m_this ;
}

void GSsl::Library::addProfile( const std::string & , bool , const std::string & , const std::string & ,
	const std::string & , const std::string & , const std::string & , const std::string & )
{
}

bool GSsl::Library::hasProfile( const std::string & ) const
{
	return false ;
}

const GSsl::Profile & GSsl::Library::profile( const std::string & ) const
{
	throw G::Exception( "internal error" ) ;
}

bool GSsl::Library::enabled() const
{
	return false ;
}

bool GSsl::Library::enabledAs( const std::string & )
{
	return false ;
}

std::string GSsl::Library::credit( const std::string & , const std::string & , const std::string & )
{
	return {} ;
}

std::string GSsl::Library::ids()
{
	return "none" ;
}

G::StringArray GSsl::Library::digesters( bool )
{
	return {} ;
}

GSsl::Digester GSsl::Library::digester( const std::string & , const std::string & , bool ) const
{
	throw G::Exception( "internal error" ) ;
	//return Digester( nullptr ) ; // never gets here
}

// ==

GSsl::Protocol::Protocol( const Profile & , const std::string & , const std::string & )
{
}

GSsl::Protocol::~Protocol()
= default;

GSsl::Protocol::Result GSsl::Protocol::connect( G::ReadWrite & )
{
	return Result::error ;
}

GSsl::Protocol::Result GSsl::Protocol::accept( G::ReadWrite & )
{
	return Result::error ;
}

GSsl::Protocol::Result GSsl::Protocol::shutdown()
{
	return Result::error ;
}

GSsl::Protocol::Result GSsl::Protocol::read( char * , std::size_t , ssize_t & )
{
	return Result::error ;
}

GSsl::Protocol::Result GSsl::Protocol::write( const char * , std::size_t , ssize_t & )
{
	return Result::error ;
}

std::string GSsl::Protocol::str( Protocol::Result )
{
	return {} ;
}

std::string GSsl::Protocol::peerCertificate() const
{
	return {} ;
}

std::string GSsl::Protocol::peerCertificateChain() const
{
	return {} ;
}

std::string GSsl::Protocol::protocol() const
{
	return {} ;
}

std::string GSsl::Protocol::cipher() const
{
	return {} ;
}

bool GSsl::Protocol::verified() const
{
	return false ;
}

// ==

GSsl::Digester::Digester( std::unique_ptr<DigesterImpBase> p ) :
	m_imp(p.release())
{
}

void GSsl::Digester::add( G::string_view )
{
}

std::string GSsl::Digester::value()
{
	return {} ;
}

std::string GSsl::Digester::state()
{
	return {} ;
}

std::size_t GSsl::Digester::blocksize() const noexcept
{
	return 1U ;
}

std::size_t GSsl::Digester::valuesize() const noexcept
{
	return 1U ;
}

std::size_t GSsl::Digester::statesize() const noexcept
{
	return 0U ;
}

