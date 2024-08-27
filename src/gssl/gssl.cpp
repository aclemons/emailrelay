//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gssl.cpp
///

#include "gdef.h"
#include "gssl.h"
#include "gstr.h"
#include "gexception.h"
#include "glog.h"
#include <algorithm>

GSsl::Library * GSsl::Library::m_this = nullptr ;

GSsl::Library::Library( bool active , const std::string & library_config , LogFn log_fn , bool verbose )
{
	if( m_this == nullptr )
		m_this = this ;

	if( active )
	{
		G::StringArray config = G::Str::splitIntoTokens( library_config , "," ) ;
		m_imp = newLibraryImp( config , log_fn , verbose ) ;
		bool ignore_extra = LibraryImpBase::consume( config , "ignoreextra" ) ;
		if( !config.empty() && !ignore_extra )
			G_WARNING( "GSsl::Library::Library: tls-config: tls configuration items ignored: [" << G::Str::join(",",config) << "]" ) ;
	}
}

GSsl::Library::~Library()
{
	if( m_this == this )
		m_this = nullptr ;
}

#ifndef G_LIB_SMALL
bool GSsl::Library::real()
{
	return true ;
}
#endif

GSsl::Library * GSsl::Library::instance()
{
	return m_this ;
}

bool GSsl::Library::enabled() const
{
	return m_imp != nullptr ;
}

#ifndef G_LIB_SMALL
std::string GSsl::Library::id() const
{
	return m_imp->id() ;
}
#endif

void GSsl::Library::addProfile( const std::string & profile_name , bool is_server_profile ,
	const std::string & key_file , const std::string & cert_file , const std::string & ca_file ,
	const std::string & default_peer_certificate_name , const std::string & default_peer_host_name ,
	const std::string & profile_config )
{
	if( m_imp != nullptr )
		m_imp->addProfile( profile_name , is_server_profile , key_file , cert_file , ca_file ,
			default_peer_certificate_name , default_peer_host_name , profile_config ) ;
}

bool GSsl::Library::hasProfile( const std::string & profile_name ) const
{
	return m_imp->hasProfile( profile_name ) ;
}

const GSsl::Profile & GSsl::Library::profile( const std::string & profile_name ) const
{
	if( !imp().hasProfile(profile_name) )
		throw BadProfileName( profile_name ) ;
	return imp().profile( profile_name ) ;
}

bool GSsl::Library::enabledAs( const std::string & profile_name )
{
	return instance() != nullptr && instance()->enabled() && instance()->hasProfile( profile_name ) ;
}

GSsl::LibraryImpBase & GSsl::Library::impstance()
{
	if( instance() == nullptr )
		throw NoInstance() ;
	return instance()->imp() ;
}

GSsl::LibraryImpBase & GSsl::Library::imp()
{
	if( m_imp == nullptr )
		throw NoInstance() ;
	return *m_imp ;
}

const GSsl::LibraryImpBase & GSsl::Library::imp() const
{
	if( m_imp == nullptr )
		throw NoInstance() ;
	return *m_imp ;
}

void GSsl::Library::log( int level , const std::string & log_line )
{
	if( level == 1 )
		G_DEBUG( "GSsl::Library::log: tls: " << log_line ) ;
	else if( level == 2 )
		G_LOG( "GSsl::Library::log: tls: " << log_line ) ;
	else
		G_WARNING( "GSsl::Library::log: tls: " << log_line ) ;
}

G::StringArray GSsl::Library::digesters( bool require_state )
{
	return instance() == nullptr || instance()->m_imp == nullptr ? G::StringArray() : impstance().digesters(require_state) ;
}

GSsl::Digester GSsl::Library::digester( const std::string & hash_function , const std::string & state , bool need_state ) const
{
	return impstance().digester( hash_function , state , need_state ) ;
}

// ==

GSsl::Protocol::Protocol( const Profile & profile , const std::string & peer_certificate_name , const std::string & peer_host_name ) :
	m_imp( profile.newProtocol(peer_certificate_name,peer_host_name) )
{
}

GSsl::Protocol::~Protocol()
= default;

std::string GSsl::Protocol::peerCertificate() const
{
	return m_imp->peerCertificate() ;
}

#ifndef G_LIB_SMALL
std::string GSsl::Protocol::peerCertificateChain() const
{
	return m_imp->peerCertificateChain() ;
}
#endif

std::string GSsl::Protocol::protocol() const
{
	return m_imp->protocol() ;
}

std::string GSsl::Protocol::cipher() const
{
	return m_imp->cipher() ;
}

#ifndef G_LIB_SMALL
bool GSsl::Protocol::verified() const
{
	return m_imp->verified() ;
}
#endif

std::string GSsl::Protocol::str( Result result )
{
	if( result == Result::ok ) return "Result_ok" ;
	if( result == Result::read ) return "Result_read" ;
	if( result == Result::write ) return "Result_write" ;
	if( result == Result::error ) return "Result_error" ;
	return "Result_undefined" ;
}

GSsl::Protocol::Result GSsl::Protocol::connect( G::ReadWrite & io )
{
	return m_imp->connect( io ) ;
}

GSsl::Protocol::Result GSsl::Protocol::accept( G::ReadWrite & io )
{
	return m_imp->accept( io ) ;
}

GSsl::Protocol::Result GSsl::Protocol::read( char * buffer , std::size_t buffer_size_in , ssize_t & data_size_out )
{
	return m_imp->read( buffer , buffer_size_in , data_size_out ) ;
}

GSsl::Protocol::Result GSsl::Protocol::write( const char * buffer , std::size_t data_size_in , ssize_t & data_size_out)
{
	return m_imp->write( buffer , data_size_in , data_size_out ) ;
}

GSsl::Protocol::Result GSsl::Protocol::shutdown()
{
	return m_imp->shutdown() ;
}

// ==

GSsl::Digester::Digester( std::unique_ptr<DigesterImpBase> p ) :
	m_imp(p.release())
{
}

void GSsl::Digester::add( std::string_view sv )
{
	m_imp->add( sv ) ;
}

std::string GSsl::Digester::value()
{
	return m_imp->value() ;
}

std::string GSsl::Digester::state()
{
	return m_imp->state() ;
}

std::size_t GSsl::Digester::blocksize() const noexcept
{
	return m_imp->blocksize() ;
}

std::size_t GSsl::Digester::valuesize() const noexcept
{
	return m_imp->valuesize() ;
}

std::size_t GSsl::Digester::statesize() const noexcept
{
	return m_imp->statesize() ;
}

// ==

bool GSsl::LibraryImpBase::consume( G::StringArray & list , std::string_view key )
{
	auto p = std::find( list.begin() , list.end() , G::sv_to_string(key) ) ;
	if( p != list.end() )
	{
		list.erase( p ) ;
		return true ;
	}
	else
	{
		return false ;
	}
}

