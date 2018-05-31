//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// gssl.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gstr.h"
#include "gexception.h"
#include "glog.h"
#include <algorithm>

GSsl::Library * GSsl::Library::m_this = nullptr ;

GSsl::Library::Library( bool active , const std::string & library_config , LogFn log_fn , bool verbose ) :
	m_imp(nullptr)
{
	if( m_this == nullptr )
		m_this = this ;

	if( active )
	{
		G::StringArray config = G::Str::splitIntoTokens( library_config , "," ) ;

		// TODO remove backwards compatibility for numeric tls-config flags
		if( G::Str::isUInt(library_config) )
		{
			G_WARNING( "GSsl::Library::ctor: tls-config: numeric tls configuration flags are deprecated" ) ;
			unsigned int flags = G::Str::toUInt( library_config ) ;
			if( (flags&(~3U)) == 0U )
				config.clear() ;
			if( (flags&3U) == 2U )
				config.push_back( "sslv23" ) ;
			else if( (flags&3U) == 3U )
				config.push_back( "sslv3" ) ;
		}

		m_imp = newLibraryImp( config , log_fn , verbose ) ;

		if( !config.empty() )
			G_WARNING( "GSsl::Library::Library: tls-config: tls configuration items ignored: [" << G::Str::join(",",config) << "]" ) ;
	}
}

GSsl::Library::~Library()
{
	delete m_imp ;
	if( m_this == nullptr )
		m_this = nullptr ;
}

bool GSsl::Library::real()
{
	return true ;
}

GSsl::Library * GSsl::Library::instance()
{
	return m_this ;
}

bool GSsl::Library::enabled() const
{
	return m_imp != nullptr ;
}

std::string GSsl::Library::id() const
{
	return m_imp->id() ;
}

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
		throw G::Exception( "invalid tls profile name [" + profile_name + "]" ) ;
	return imp().profile( profile_name ) ;
}

bool GSsl::Library::enabledAs( const std::string & profile_name )
{
	return instance() != nullptr && instance()->enabled() && instance()->hasProfile( profile_name ) ;
}

GSsl::LibraryImpBase & GSsl::Library::impstance()
{
	if( instance() == nullptr )
		throw G::Exception( "no tls library instance" ) ;
	return instance()->imp() ;
}

GSsl::LibraryImpBase & GSsl::Library::imp()
{
	if( m_imp == nullptr )
		throw G::Exception( "no tls library instance" ) ;
	return *m_imp ;
}

const GSsl::LibraryImpBase & GSsl::Library::imp() const
{
	if( m_imp == nullptr )
		throw G::Exception( "no tls library instance" ) ;
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

GSsl::Digester GSsl::Library::digester( const std::string & hash_function , const std::string & state ) const
{
	return impstance().digester( hash_function , state ) ;
}

// ==

GSsl::Protocol::Protocol( const Profile & profile , const std::string & peer_certificate_name , const std::string & peer_host_name ) :
	m_imp( profile.newProtocol(peer_certificate_name,peer_host_name) )
{
}

GSsl::Protocol::~Protocol()
{
	delete m_imp ;
}

std::string GSsl::Protocol::peerCertificate() const
{
	return m_imp->peerCertificate() ;
}

std::string GSsl::Protocol::peerCertificateChain() const
{
	return m_imp->peerCertificateChain() ;
}

bool GSsl::Protocol::verified() const
{
	return m_imp->verified() ;
}

std::string GSsl::Protocol::str( Result result )
{
	if( result == Result_ok ) return "Result_ok" ;
	if( result == Result_read ) return "Result_read" ;
	if( result == Result_write ) return "Result_write" ;
	if( result == Result_error ) return "Result_error" ;
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

GSsl::Protocol::Result GSsl::Protocol::read( char * buffer , size_t buffer_size_in , ssize_t & data_size_out )
{
	return m_imp->read( buffer , buffer_size_in , data_size_out ) ;
}

GSsl::Protocol::Result GSsl::Protocol::write( const char * buffer , size_t data_size_in , ssize_t & data_size_out)
{
	return m_imp->write( buffer , data_size_in , data_size_out ) ;
}

GSsl::Protocol::Result GSsl::Protocol::stop()
{
	return m_imp->stop() ;
}

// ==

GSsl::Digester::Digester( DigesterImpBase * p ) :
	m_imp(p)
{
}

void GSsl::Digester::add( const std::string & s )
{
	m_imp->add( s ) ;
}

std::string GSsl::Digester::value()
{
	return m_imp->value() ;
}

std::string GSsl::Digester::state()
{
	return m_imp->state() ;
}

size_t GSsl::Digester::blocksize() const
{
	return m_imp->blocksize() ;
}

size_t GSsl::Digester::valuesize() const
{
	return m_imp->valuesize() ;
}

size_t GSsl::Digester::statesize() const
{
	return m_imp->statesize() ;
}

// ==

GSsl::LibraryImpBase::~LibraryImpBase()
{
}

bool GSsl::LibraryImpBase::consume( G::StringArray & list , const std::string & key )
{
	G::StringArray::iterator p = std::find( list.begin() , list.end() , key ) ;
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

// ==

GSsl::Profile::~Profile()
{
}

GSsl::ProtocolImpBase::~ProtocolImpBase()
{
}

GSsl::DigesterImpBase::~DigesterImpBase()
{
}

/// \file gssl.cpp
