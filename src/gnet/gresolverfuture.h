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
/// \file gresolverfuture.h
///

#ifndef G_NET_RESOLVER_FUTURE_H
#define G_NET_RESOLVER_FUTURE_H

#include "gdef.h"
#include "gaddress.h"
#include <utility>
#include <vector>
#include <string>

namespace GNet
{
	class ResolverFuture ;
}

//| \class GNet::ResolverFuture
/// A 'future' shared-state class for asynchronous name resolution that
/// holds parameters and results of a call to getaddrinfo(), as performed
/// by the run() method.
///
/// The run() method can be called from a worker thread and the results
/// collected by the main thread using get() once the worker thread has
/// signalled that it has finished. The signalling mechanism is outside
/// the scope of this class (see GNet::FutureEvent).
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
	using Pair = std::pair<Address,std::string> ;
	using List = std::vector<Address> ;

	ResolverFuture( const std::string & host , const std::string & service ,
		int family , bool dgram , bool for_async_hint = false ) ;
			///< Constructor for resolving the given host and service names.

	~ResolverFuture() ;
		///< Destructor.

	ResolverFuture & run() noexcept ;
		///< Does the synchronous name resolution and stores the result.
		///< Returns *this.

	Pair get() ;
		///< Returns the resolved address/name pair after run() has completed.
		///< Returns default values if an error().

	void get( List & ) ;
		///< Returns the resolved addresses after run() has completed by
		///< appending to the given list. Appends nothing if an error().

	bool error() const ;
		///< Returns true if name resolution failed or no suitable
		///< address was returned. Use after get().

	std::string reason() const ;
		///< Returns the reason for the error().
		///< Precondition: error()

public:
	ResolverFuture( const ResolverFuture & ) = delete ;
	ResolverFuture( ResolverFuture && ) = delete ;
	ResolverFuture & operator=( const ResolverFuture & ) = delete ;
	ResolverFuture & operator=( ResolverFuture && ) = delete ;

private:
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
	struct addrinfo m_ai_hint {} ;
	bool m_test_mode ;
	int m_rc {0} ;
	struct addrinfo * m_ai {nullptr} ;
	std::string m_reason ;
} ;

#endif
