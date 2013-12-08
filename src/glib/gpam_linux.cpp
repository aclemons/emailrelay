//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gpam_linux.cpp
//
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
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>
extern "C" 
{ 
// in '/usr/include/security' or '/usr/include/pam'
#include <pam_appl.h> 
}

namespace
{
	char * strdup_( const char * p ) 
	{ 
		p = p ? p : "" ;
		char * copy = static_cast<char*>( std::malloc(std::strlen(p)+1U) ) ;
		if( copy != NULL )
			std::strcpy( copy , p ) ;
		return copy ;
	}
}

// ==

/// \class G::PamImp
/// A pimple-pattern implementation class for Pam.
/// 
class G::PamImp 
{
public:
	typedef pam_handle_t * Handle ;

private:
	typedef struct pam_conv Conversation ;
	typedef G::Pam::Error Error ;
	typedef G::Pam::ItemArray ItemArray ;
	enum { MAGIC = 3456 } ;

public:
	Pam & m_pam ;
	int m_magic ;
	mutable int m_rc ; // required for pam_end()
	Handle m_hpam ;
	Conversation m_conv ;
	bool m_silent ;

public:
	PamImp( G::Pam & pam , const std::string & app , const std::string & user , bool silent ) ;
	~PamImp() ;
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

private:
	static int converse( int n , const struct pam_message ** in , struct pam_response ** out , void * vp ) ;
	static void delay( int , unsigned , void * ) ;
	static std::string decodeStyle( int pam_style ) ;
	static void release( struct pam_response * , size_t ) ;
} ;

// ==

G::PamImp::PamImp( G::Pam & pam , const std::string & application , const std::string & user , bool silent ) :
	m_pam(pam) ,
	m_magic(MAGIC) ,
	m_rc(PAM_SUCCESS) ,
	m_silent(silent)
{
	G_DEBUG( "G::PamImp::ctor: [" << application << "] [" << user << "]" ) ;

	m_conv.conv = converse ;
	m_conv.appdata_ptr = this ;
	m_rc = ::pam_start( application.c_str() , user.c_str() , &m_conv , &m_hpam ) ;
	if( m_rc != PAM_SUCCESS )
	{
		throw Error( "pam_start" , m_rc ) ;
	}

	// (linux-specific)
 #ifdef PAM_FAIL_DELAY
	m_rc = ::pam_set_item( m_hpam , PAM_FAIL_DELAY , reinterpret_cast<const void*>(delay) ) ;
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
	std::string defolt = std::string( "#" ) + G::Str::fromInt( pam_style ) ;
	if( pam_style == PAM_PROMPT_ECHO_OFF ) return "password" ;
	if( pam_style == PAM_PROMPT_ECHO_ON ) return "prompt" ;
	if( pam_style == PAM_ERROR_MSG ) return "error" ;
	if( pam_style == PAM_TEXT_INFO ) return "info" ;
	return defolt ;
}

bool G::PamImp::authenticate( bool require_token )
{
	int flags = 0 ;
	if( silent() ) flags |= PAM_SILENT ;
	if( require_token ) flags |= PAM_DISALLOW_NULL_AUTHTOK ;
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
	const void * vp = NULL ;
	m_rc = ::pam_get_item( hpam() , PAM_USER , &vp ) ;
	check( "pam_get_item" , m_rc ) ;
	const char * cp = reinterpret_cast<const char*>(vp) ;
	return std::string( cp ? cp : "" ) ;
}

void G::PamImp::setCredentials( int flag )
{
	int flags = 0 ;
	flags |= flag ;
	if( silent() ) flags |= PAM_SILENT ;
	m_rc = ::pam_setcred( hpam() , flags ) ;
	check( "pam_setcred" , m_rc ) ;
}

void G::PamImp::checkAccount( bool require_token )
{
	int flags = 0 ;
	if( silent() ) flags |= PAM_SILENT ;
	if( require_token ) flags |= PAM_DISALLOW_NULL_AUTHTOK ;
	m_rc = ::pam_acct_mgmt( hpam() , flags ) ;
	check( "pam_acct_mgmt" , m_rc ) ;
}

void G::PamImp::release( struct pam_response * rsp , size_t n )
{
	if( rsp != NULL )
	{
		for( size_t i = 0U ; i < n ; i++ )
		{
			if( rsp[i].resp != NULL )
				std::free( rsp[i].resp ) ;
		}
	}
	std::free( rsp ) ;
}

int G::PamImp::converse( int n_in , const struct pam_message ** in , struct pam_response ** out , void * vp )
{
	G_ASSERT( n_in > 0 ) ;
	G_ASSERT( out != NULL ) ;
	size_t n = n_in < 0 ? size_t(0U) : static_cast<size_t>(n_in) ;

	// pam_conv(3) on linux points out that the pam interface is under-speficied, and on some 
	// systems, possibly including solaris, the "in" pointer is interpreted differently - this 
	// is only a problem for n greater than one, so warn about it at run-time
	//
	if( n > 1U )
	{
		static bool warned = false ;
		if( !warned )
		{
			G_WARNING( "PamImp::converse: received a complex pam converse() structure: proceed with caution" ) ;
			warned = true ;
		}
	}

	*out = NULL ;
	struct pam_response * rsp = NULL ;
	try
	{
		G_DEBUG( "G::Pam::converse: " << n << " item(s)" ) ;
		PamImp * This = reinterpret_cast<PamImp*>(vp) ;
		G_ASSERT( This->m_magic == MAGIC ) ;

		// convert the c items into a c++ container -- treat
		// "in" as a pointer to a contiguous array of pointers
		// (see linux man pam_conv)
		//
		ItemArray array( n ) ;
		for( size_t i = 0U ; i < n ; i++ )
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
		rsp = reinterpret_cast<struct pam_response*>( std::malloc(n*sizeof(struct pam_response)) ) ;
		if( rsp == NULL )
			throw std::bad_alloc() ;
		for( size_t j = 0U ; j < n ; j++ )
			rsp[j].resp = NULL ;

		// fill in the response from the c++ container
		//
		for( size_t i = 0U ; i < n ; i++ )
		{
			rsp[i].resp_retcode = 0 ;
			if( array[i].out_defined )
			{
				char * response = strdup_( array[i].out.c_str() ) ;
				if( response == NULL )
					throw std::bad_alloc() ;
				rsp[i].resp = response ;
			}
		}

		*out = rsp ;
		G_DEBUG( "G::Pam::converse: complete" ) ;
		return PAM_SUCCESS ;
	}
	catch(...) // c callback
	{
		G_ERROR( "G::Pam::converse: exception" ) ;
		release( rsp , n ) ;
		return PAM_CONV_ERR ;
	}
}

void G::PamImp::delay( int status , unsigned delay_usec , void * pam_vp )
{
	try
	{
		G_DEBUG( "G::Pam::delay: status=" << status << ", delay=" << delay_usec ) ;
		if( status != PAM_SUCCESS )
		{
			PamImp * This = reinterpret_cast<PamImp*>(pam_vp) ;
			if( This != NULL )
			{
				G_ASSERT( This->m_magic == MAGIC ) ;
				This->m_pam.delay( delay_usec ) ;
			}
		}
	}
	catch(...) // c callback
	{
		G_ERROR( "G::Pam::delay: exception" ) ;
	}
}

void G::PamImp::openSession()
{
	int flags = 0 ;
	if( silent() ) flags |= PAM_SILENT ;
	m_rc = ::pam_open_session( hpam() , flags ) ;
	check( "pam_open_session" , m_rc ) ;
}

void G::PamImp::closeSession()
{
	int flags = 0 ;
	if( silent() ) flags |= PAM_SILENT ;
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

// ==

G::Pam::Pam( const std::string & application , const std::string & user , bool silent ) :
	m_imp( new PamImp(*this,application,user,silent) )
{
}

G::Pam::~Pam()
{
	delete m_imp ;
}

bool G::Pam::authenticate( bool require_token )
{
	G_DEBUG( "G::Pam::authenticate" ) ;
	return m_imp->authenticate( require_token ) ;
}

void G::Pam::checkAccount( bool require_token )
{
	G_DEBUG( "G::Pam::checkAccount" ) ;
	return m_imp->checkAccount( require_token ) ;
}

void G::Pam::establishCredentials()
{
	G_DEBUG( "G::Pam::establishCredentials" ) ;
	m_imp->setCredentials( PAM_ESTABLISH_CRED ) ;
}

void G::Pam::openSession()
{
	G_DEBUG( "G::Pam::openSession" ) ;
	m_imp->openSession() ;
}

void G::Pam::closeSession()
{
	G_DEBUG( "G::Pam::closeSession" ) ;
	m_imp->closeSession() ;
}

void G::Pam::deleteCredentials()
{
	m_imp->setCredentials( PAM_DELETE_CRED ) ;
}

void G::Pam::reinitialiseCredentials()
{
	m_imp->setCredentials( PAM_REINITIALIZE_CRED ) ;
}

void G::Pam::refreshCredentials()
{
	m_imp->setCredentials( PAM_REFRESH_CRED ) ;
}

void G::Pam::delay( unsigned int usec )
{
	if( usec != 0U )
	{
		typedef struct timeval Timeval ; // std:: ??
		Timeval timeout ;
		timeout.tv_sec = 0 ;
		timeout.tv_usec = usec ;
		::select( 0 , NULL , NULL , NULL , &timeout ) ;
	}
}

std::string G::Pam::name() const
{
	return m_imp->name() ;
}

