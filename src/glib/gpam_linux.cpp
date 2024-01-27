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
/// \file gpam_linux.cpp
///
// See: http://www.linux-pam.org/Linux-PAM-html/
//

#include "gdef.h"
#include "gpam.h"
#include "glog.h"
#include "gstr.h"
#include "gexception.h"
#include "gassert.h"
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <new>

#if GCONFIG_HAVE_PAM_IN_INCLUDE
#include <pam_appl.h>
#else
#if GCONFIG_HAVE_PAM_IN_PAM
#include <pam/pam_appl.h>
#else
#include <security/pam_appl.h>
#endif
#endif
#if GCONFIG_PAM_CONST
#define G_PAM_CONST const
#else
#define G_PAM_CONST
#endif

//| \class G::PamImp
/// A pimple-pattern implementation class for G::Pam.
///
class G::PamImp
{
public:
	using Handle = pam_handle_t * ;
	using Conversation = struct pam_conv ;
	PamImp( Pam & pam , const std::string & app , const std::string & user , bool silent ) ;
	Handle hpam() const ;
	bool silent() const ;
	bool authenticate( bool ) ;
	void check( const std::string & , int ) const ;
	static bool success( int ) ;
	void setCredentials( int ) ;
	void checkAccount( bool ) ;
	void openSession() ;
	void closeSession() ;
	std::string name() const ;

public:
	~PamImp() ;
	PamImp( const PamImp & ) = delete ;
	PamImp( PamImp && ) = delete ;
	PamImp & operator=( const PamImp & ) = delete ;
	PamImp & operator=( PamImp && ) = delete ;

public:
	Pam & m_pam ;
	int m_magic {MAGIC} ;
	mutable int m_rc {PAM_SUCCESS} ; // required for pam_end()
	Handle m_hpam {nullptr} ;
	Conversation m_conv ;
	bool m_silent ;

private:
	using Error = Pam::Error ;
	using ItemArray = Pam::ItemArray ;
	static constexpr int MAGIC = 3456 ;

private:
	static int converseCallback( int n , G_PAM_CONST struct pam_message ** in ,
		struct pam_response ** out , void * vp ) ;
	static void delayCallback( int , unsigned , void * ) ;
	static std::string decodeStyle( int pam_style ) ;
	static void release( struct pam_response * , std::size_t ) ;
	static char * strdup_( const char * ) ;
} ;

// ==

G::PamImp::PamImp( G::Pam & pam , const std::string & application , const std::string & user , bool silent ) :
	m_pam(pam) ,
	m_conv{} ,
	m_silent(silent)
{
	G_DEBUG( "G::PamImp::ctor: [" << application << "] [" << user << "]" ) ;

	m_conv.conv = converseCallback ;
	m_conv.appdata_ptr = this ;
	m_rc = ::pam_start( application.c_str() , user.c_str() , &m_conv , &m_hpam ) ;
	if( m_rc != PAM_SUCCESS )
	{
		throw Error( "pam_start" , m_rc ) ;
	}

	// (linux-specific)
 #ifdef PAM_FAIL_DELAY
	m_rc = ::pam_set_item( m_hpam , PAM_FAIL_DELAY , reinterpret_cast<const void*>(delayCallback) ) ;
	if( m_rc != PAM_SUCCESS )
	{
		::pam_end( m_hpam , m_rc ) ;
		throw Error( "pam_set_item" , m_rc , ::pam_strerror(hpam(),m_rc) ) ;
	}
 #endif
}

G::PamImp::~PamImp()
{
	try
	{
		G_DEBUG( "G::PamImp::dtor" ) ;
		::pam_end( m_hpam , m_rc ) ;
		m_magic = 0 ;
	}
	catch(...)
	{
	}
}

G::PamImp::Handle G::PamImp::hpam() const
{
	return m_hpam ;
}

bool G::PamImp::silent() const
{
	return m_silent ;
}

std::string G::PamImp::decodeStyle( int pam_style )
{
	std::string defolt = std::string( "#" ) + Str::fromInt( pam_style ) ;
	if( pam_style == PAM_PROMPT_ECHO_OFF ) return "password" ;
	if( pam_style == PAM_PROMPT_ECHO_ON ) return "prompt" ;
	if( pam_style == PAM_ERROR_MSG ) return "error" ;
	if( pam_style == PAM_TEXT_INFO ) return "info" ;
	return defolt ;
}

bool G::PamImp::authenticate( bool require_token )
{
	int flags = 0 ;
	if( silent() ) flags |= static_cast<int>(PAM_SILENT) ;
	if( require_token ) flags |= static_cast<int>(PAM_DISALLOW_NULL_AUTHTOK) ;
	m_rc = ::pam_authenticate( hpam() , flags ) ;
 #ifdef PAM_INCOMPLETE
	if( m_rc == PAM_INCOMPLETE )
		return false ;
 #endif

	check( "pam_authenticate" , m_rc ) ;
	return true ;
}

std::string G::PamImp::name() const
{
	G_PAM_CONST void * vp = nullptr ;
	m_rc = ::pam_get_item( hpam() , PAM_USER , &vp ) ;
	check( "pam_get_item" , m_rc ) ;
	const char * cp = static_cast<const char*>(vp) ;
	return { cp ? cp : "" } ;
}

void G::PamImp::setCredentials( int flag )
{
	int flags = 0 ;
	flags |= flag ;
	if( silent() ) flags |= static_cast<int>(PAM_SILENT) ;
	m_rc = ::pam_setcred( hpam() , flags ) ;
	check( "pam_setcred" , m_rc ) ;
}

void G::PamImp::checkAccount( bool require_token )
{
	int flags = 0 ;
	if( silent() ) flags |= static_cast<int>(PAM_SILENT) ;
	if( require_token ) flags |= static_cast<int>(PAM_DISALLOW_NULL_AUTHTOK) ;
	m_rc = ::pam_acct_mgmt( hpam() , flags ) ;
	check( "pam_acct_mgmt" , m_rc ) ;
}

void G::PamImp::release( struct pam_response * rsp , std::size_t n )
{
	if( rsp != nullptr )
	{
		for( std::size_t i = 0U ; i < n ; i++ )
		{
			if( rsp[i].resp != nullptr )
				std::free( rsp[i].resp ) ; // NOLINT
		}
	}
	std::free( rsp ) ; // NOLINT
}

int G::PamImp::converseCallback( int n_in , G_PAM_CONST struct pam_message ** in ,
	struct pam_response ** out , void * vp )
{
	G_ASSERT( out != nullptr ) ;
	if( n_in <= 0 )
	{
		G_ERROR( "G::Pam::converseCallback: invalid count" ) ;
		return PAM_CONV_ERR ;
	}
	std::size_t n = static_cast<std::size_t>(n_in) ;

	// pam_conv(3) on linux points out that the pam interface is under-specified, and on some
	// systems, possibly including solaris, the "in" pointer is interpreted differently - this
	// is only a problem for n greater than one, so warn about it at run-time
	//
	if( n > 1U )
	{
		G_WARNING_ONCE( "PamImp::converseCallback: received a complex pam converse() structure: "
			"proceed with caution" ) ;
	}

	*out = nullptr ;
	struct pam_response * rsp = nullptr ;
	try
	{
		G_DEBUG( "G::Pam::converseCallback: called back from pam with " << n << " item(s)" ) ;
		PamImp * This = static_cast<PamImp*>(vp) ;
		G_ASSERT( This->m_magic == MAGIC ) ;

		// convert the c items into a c++ container -- treat
		// "in" as a pointer to a contiguous array of pointers
		// (see linux man pam_conv)
		//
		ItemArray array( n ) ;
		for( std::size_t i = 0U ; i < n ; i++ )
		{
			std::string & s1 = const_cast<std::string&>(array[i].in_type) ;
			s1 = decodeStyle( in[i]->msg_style ) ;

			std::string & s2 = const_cast<std::string&>(array[i].in) ;
			s2 = std::string(in[i]->msg ? in[i]->msg : "") ;

			array[i].out_defined = false ;
		}

		// do the conversation
		//
		This->m_pam.converse( array ) ;
		G_ASSERT( array.size() == n ) ;

		// allocate the response - treat "out" as a pointer to a pointer
		// to a contiguous array of structures (see linux man pam_conv)
		//
		rsp = static_cast<struct pam_response*>( std::malloc(n*sizeof(struct pam_response)) ) ; // NOLINT
		if( rsp == nullptr )
			throw std::bad_alloc() ;
		for( std::size_t j = 0U ; j < n ; j++ )
			rsp[j].resp = nullptr ;

		// fill in the response from the c++ container
		//
		for( std::size_t i = 0U ; i < n ; i++ )
		{
			rsp[i].resp_retcode = 0 ;
			if( array[i].out_defined )
			{
				char * response = strdup_( array[i].out.c_str() ) ;
				if( response == nullptr )
					throw std::bad_alloc() ;
				rsp[i].resp = response ;
			}
		}

		*out = rsp ;
		G_DEBUG( "G::Pam::converseCallback: returning to pam from callback" ) ;
		return PAM_SUCCESS ;
	}
	catch(...) // c callback
	{
		G_ERROR( "G::Pam::converseCallback: exception" ) ;
		release( rsp , n ) ;
		return PAM_CONV_ERR ;
	}
}

void G::PamImp::delayCallback( int status , unsigned delay_usec , void * pam_vp )
{
	try
	{
		G_DEBUG( "G::Pam::delayCallback: status=" << status << ", delay=" << delay_usec ) ;
		if( status != PAM_SUCCESS )
		{
			PamImp * This = static_cast<PamImp*>(pam_vp) ;
			if( This != nullptr )
			{
				G_ASSERT( This->m_magic == MAGIC ) ;
				This->m_pam.delay( delay_usec ) ;
			}
		}
	}
	catch(...) // c callback
	{
		G_ERROR( "G::Pam::delayCallback: exception" ) ;
	}
}

void G::PamImp::openSession()
{
	int flags = 0 ;
	if( silent() ) flags |= static_cast<int>(PAM_SILENT) ;
	m_rc = ::pam_open_session( hpam() , flags ) ;
	check( "pam_open_session" , m_rc ) ;
}

void G::PamImp::closeSession()
{
	int flags = 0 ;
	if( silent() ) flags |= static_cast<int>(PAM_SILENT) ;
	m_rc = ::pam_close_session( hpam() , flags ) ;
	check( "pam_close_session" , m_rc ) ;
}

bool G::PamImp::success( int rc )
{
	return rc == PAM_SUCCESS ;
}

void G::PamImp::check( const std::string & op , int rc ) const
{
	if( !success(rc) )
		throw Error( op , rc , ::pam_strerror(hpam(),rc) ) ;
}

char * G::PamImp::strdup_( const char * p )
{
	p = p ? p : "" ;
	char * copy = static_cast<char*>( std::malloc(std::strlen(p)+1U) ) ; // NOLINT
	if( copy != nullptr )
		std::strcpy( copy , p ) ; // NOLINT
	return copy ;
}

// ==

G::Pam::Pam( const std::string & application , const std::string & user , bool silent ) :
	m_imp(std::make_unique<PamImp>(*this,application,user,silent))
{
}

G::Pam::~Pam()
= default;

bool G::Pam::authenticate( bool require_token )
{
	G_DEBUG( "G::Pam::authenticate" ) ;
	return m_imp->authenticate( require_token ) ;
}

#ifndef G_LIB_SMALL
void G::Pam::checkAccount( bool require_token )
{
	G_DEBUG( "G::Pam::checkAccount" ) ;
	return m_imp->checkAccount( require_token ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::Pam::establishCredentials()
{
	G_DEBUG( "G::Pam::establishCredentials" ) ;
	m_imp->setCredentials( PAM_ESTABLISH_CRED ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::Pam::openSession()
{
	G_DEBUG( "G::Pam::openSession" ) ;
	m_imp->openSession() ;
}
#endif

#ifndef G_LIB_SMALL
void G::Pam::closeSession()
{
	G_DEBUG( "G::Pam::closeSession" ) ;
	m_imp->closeSession() ;
}
#endif

#ifndef G_LIB_SMALL
void G::Pam::deleteCredentials()
{
	m_imp->setCredentials( PAM_DELETE_CRED ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::Pam::reinitialiseCredentials()
{
	m_imp->setCredentials( PAM_REINITIALIZE_CRED ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::Pam::refreshCredentials()
{
	m_imp->setCredentials( PAM_REFRESH_CRED ) ;
}
#endif

#ifndef G_LIB_SMALL
void G::Pam::delay( unsigned int usec )
{
	// this is the default implementation, usually overridden
	if( usec != 0U )
	{
		// (sys/select.h is included from gdef.h)
		using Timeval = struct timeval ;
		Timeval timeout ;
		timeout.tv_sec = usec / 1000000U ;
		timeout.tv_usec = usec % 1000000U ;
		::select( 0 , nullptr , nullptr , nullptr , &timeout ) ;
	}
}
#endif

#ifndef G_LIB_SMALL
std::string G::Pam::name() const
{
	return m_imp->name() ;
}
#endif

