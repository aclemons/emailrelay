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
///
/// \file gresolverfuture.h
///

#ifndef G_NET_RESOLVER_FUTURE__H
#define G_NET_RESOLVER_FUTURE__H

#include "gdef.h"
#include "gaddress.h"
#include <utility>
#include <vector>
#include <string>

namespace GNet
{
	class ResolverFuture ;
}

/// \class GNet::ResolverFuture
/// A 'future' object for asynchronous name resolution, in practice holding
/// just enough state for a single call to getaddrinfo().
///
/// The run() method can be called from a worker thread and the results
/// collected by the main thread with get() once the run() has finished.
///
/// Signalling completion from worker thread to main thread is out of scope
/// for this class; see GNet::FutureEvent.
///
/// Eg:
/// \code
///
/// ResolverFuture f( "example.com" , "smtp" , AF_INET , false ) ;
/// std::thread t( &ResolverFuture::run , f ) ;
/// ...
/// t.join() ;
/// Address a = f.get().first ;
/// if( f.error() ) throw std::runtime_error( f.reason() ) ;
/// \endcode
///
class GNet::ResolverFuture
{
public:
	typedef std::pair<Address,std::string> Pair ;
	typedef std::vector<Address> List ;

	ResolverFuture( const std::string & host , const std::string & service ,
		int family , bool dgram , bool for_async_hint = false ) ;
			///< Constructor for resolving the given host and service names.

	~ResolverFuture() ;
		///< Destructor.

	void run() ;
		///< Does the name resolution.

	Pair get() ;
		///< Returns the resolved address/name pair.

	void get( List & ) ;
		///< Returns the list of resolved addresses.

	bool error() const ;
		///< Returns true if name resolution failed or no suitable
		///< address was returned. Use after get().

	std::string reason() const ;
		///< Returns the reason for the error().
		///< Precondition: error()

private:
	ResolverFuture( const ResolverFuture & ) ;
	void operator=( const ResolverFuture & ) ;
	std::string failure() const ;
	bool fetch( List & ) const ;
	bool fetch( Pair & ) const ;
	bool failed() const ;
	std::string none() const ;
	std::string ipvx() const ;

private:
	bool m_numeric_service ;
	int m_socktype ;
	std::string m_host ;
	const char * m_host_p ;
	std::string m_service ;
	const char * m_service_p ;
	int m_family ;
	struct addrinfo m_ai_hint ;
	bool m_test_mode ;
	int m_rc ;
	struct addrinfo * m_ai ;
	std::string m_reason ;
} ;

#endif
