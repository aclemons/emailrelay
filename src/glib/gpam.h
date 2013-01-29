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
///
/// \file gpam.h
///

#ifndef G_PAM_H
#define G_PAM_H

#include "gdef.h"
#include "gexception.h"
#include <string>
#include <vector>

/// \namespace G
namespace G
{
	class Pam ;
	class PamImp ;
}

/// \class G::Pam
/// A thin abstract interface to the system PAM library.
///
/// Derived classes should implement converse() to supply passwords
/// etc. and delay() to implement anti-brute-force delays. 
///
/// As per the PAM model the user code should authenticate(), 
/// then checkAccount(), then establishCredentials() and finally 
/// openSession().
///
/// Usage:
/// \code
/// Pam pam("foo","me");
/// bool complete = pam.authenticate() ;
/// if( !complete ) ...
/// pam.checkAccount() ;
/// pam.establishCredentials() ;
/// pam.openSession() ;
/// ...
/// pam.closeSession() ;
/// \endcode
///
class G::Pam 
{
public:
	/// A structure used by G::Pam to hold conversation items.
	struct Item 
	{
		const std::string in_type ; // "password", "prompt", "error", "info"
		const std::string in ; // password prompt, non-password prompt, error text, infomation message, etc
		std::string out ; // password, or whatever was prompted for
		bool out_defined ; // to be set to true if 'out' is assigned
	} ;
	typedef std::vector<Item> ItemArray ;

	/// An exception class used by G::Pam.
	class Error : public G::Exception 
	{ 
		public: int m_pam_error ;
		public: Error( const std::string & op , int pam_error ) ;
		public: Error( const std::string & op , int pam_error , const char * ) ;
	} ;

	Pam( const std::string & app , const std::string & user , bool silent ) ;
		///< Constructor.

	virtual ~Pam() ;
		///< Destructor.

	bool authenticate( bool require_token ) ;
		///< Authenticates the user. Typically issues a challenge, 
		///< such as password request, using the converse() callback. 
		///<
		///< Returns false if it needs to be called again because
		///< converse() did not fill in all the prompted values.
		///< Returns true if authenticated. Throws on error.

	std::string name() const ;
		///< Returns the authenticated user name. In principle this can 
		///< be different from the requesting user name passed in the 
		///< constructor.

	void checkAccount( bool require_token ) ;
		///< Does "account management", checking that the authenticated
		///< user is currently allowed to use the system.

	void establishCredentials() ;
		///< Embues the authenticated user with their credentials, such
		///< as "tickets" in the form of environment variables etc.

	void openSession() ;
		///< Starts a session.

	void closeSession() ;
		///< Closes a session.

	void deleteCredentials() ;
		///< Deletes credentials.

	void reinitialiseCredentials() ;
		///< Reinitialises credentials.

	void refreshCredentials() ;
		///< Refreshes credentials.

	virtual void converse( ItemArray & ) = 0 ;
		///< Called to pass a message to the user, or request 
		///< a password etc.
		///<
		///< Typically the array is a single password prompt. 
		///< The password should then be put into the 'out' 
		///< string and the boolean flag set.
		///<
		///< For each item in the array which is a prompt the 
		///< implementation is required to supply a response 
		///< value. 
		///<
		///< In an event-driven environment the response values 
		///< can be left unassigned, in which case the outer 
		///< authenticate() call will return false. The 
		///< authenticate() can then be called a second time 
		///< once the requested information is available.

	virtual void delay( unsigned int usec ) = 0 ;
		///< Called when the pam library wants the application 
		///< to introduce a delay to prevent brute-force attacks. 
		///< The parameter may be zero.
		///<
		///< Typically called from within authenticate(), ie. 
		///< before authenticate returns.
		///<
		///< A default implementation is provided (sic) that 
		///< does a sleep.
		///<
		///< In an event-driven application the implementation 
		///< of this method should start a timer and avoid 
		///< initiating any new authentication while the timer 
		///< is running.

private:
	Pam( const Pam & ) ; // not implemented
	void operator=( const Pam & ) ; // not implemented

private:
	PamImp * m_imp ;
} ;

#endif

