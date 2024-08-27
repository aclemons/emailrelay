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
/// \file gresolver.h
///

#ifndef G_NET_RESOLVER_H
#define G_NET_RESOLVER_H

#include "gdef.h"
#include "glocation.h"
#include "geventhandler.h"
#include "geventstate.h"
#include "gexception.h"
#include "gaddress.h"
#include <vector>
#include <utility>
#include <memory>

namespace GNet
{
	class Resolver ;
	class ResolverImp ;
}

//| \class GNet::Resolver
/// A class for synchronous or asynchronous network name to address resolution.
/// The implementation uses getaddrinfo() at its core, with std::thread used for
/// asynchronous resolve requests, with hooks into the GNet::EventLoop via
/// GNet::FutureEvent.
///
class GNet::Resolver
{
public:
	G_EXCEPTION( Error , tx("asynchronous resolver error") )
	G_EXCEPTION( BusyError , tx("asynchronous resolver still busy") )
	struct Config /// A configuration structure for GNet::Resolver.
	{
		Config() ;
		bool with_canonical_name {false} ;
		bool raw {false} ; // straight to getaddrinfo() with no "xn--<punycode>" conversion
		bool datagram {false} ; // for datagram sockets
		bool idn_flag {false} ; // use glibc's AI_IDN flag if available
		bool test_slow {false} ; // run slow for testing
		Config & set_with_canonical_name( bool = true ) noexcept ;
		Config & set_raw( bool = true ) noexcept ;
		Config & set_convert_to_punycode( bool = true ) noexcept ;
		Config & set_datagram( bool = true ) noexcept ;
		Config & set_idn_flag( bool = true ) noexcept ;
	} ;
	using AddressList = std::vector<Address> ;
	struct Callback /// An interface used for GNet::Resolver callbacks.
	{
		virtual ~Callback() = default ;
			///< Destructor.

		virtual void onResolved( std::string error , Location ) = 0 ;
			///< Called on completion of GNet::Resolver name resolution.
	} ;

	Resolver( Callback & , EventState ) ;
		///< Constructor taking a callback interface reference.
		///< The exception sink is called if an exception is thrown
		///< out of Callback::onResolved().

	~Resolver() ;
		///< Destructor. The results of any pending asynchronous resolve
		///< request are discarded asynchronously, although in extreme
		///< cases this destructor may block doing a thread join.

	void start( const Location & , const Config & = {} ) ;
		///< Starts asynchronous name-to-address resolution.
		///< Precondition: async() && !busy()

	static std::pair<std::string,std::string> resolve( Location & , const Config & ) ;
		///< Does synchronous name resolution. Fills in the address
		///< of the supplied Location structure. Returns
		///< an error string (empty on success) and the canonical
		///< name, if requested (see Config::with_canonical_name).

	static std::string resolve( Location & ) ;
		///< Does synchronous name resolution. Fills in the address
		///< of the supplied Location structure. Returns an
		///< error string, empty on success.

	static AddressList resolve( const std::string & host , const std::string & service , int family = AF_UNSPEC , const Config & = {} ) ;
		///< Does synchronous name resolution returning a list
		///< of addresses. Errors are not reported. The empty
		///< list is returned on error.

	static bool async() ;
		///< Returns true if the resolver supports asynchronous operation.
		///< If it doesnt then start() will always throw.

	bool busy() const ;
		///< Returns true if there is a pending resolve request.

public:
	Resolver( const Resolver & ) = delete ;
	Resolver( Resolver && ) = delete ;
	Resolver & operator=( const Resolver & ) = delete ;
	Resolver & operator=( Resolver && ) = delete ;

private:
	friend class GNet::ResolverImp ;
	void done( const std::string & , const Location & ) ;

private:
	Callback & m_callback ;
	EventState m_es ;
	std::unique_ptr<ResolverImp> m_imp ;
} ;

inline GNet::Resolver::Config & GNet::Resolver::Config::set_with_canonical_name( bool b ) noexcept { with_canonical_name = b ; return *this ; }
inline GNet::Resolver::Config & GNet::Resolver::Config::set_raw( bool b ) noexcept { raw = b ; return *this ; }
inline GNet::Resolver::Config & GNet::Resolver::Config::set_datagram( bool b ) noexcept { datagram = b ; return *this ; }
inline GNet::Resolver::Config & GNet::Resolver::Config::set_idn_flag( bool b ) noexcept { idn_flag = b ; return *this ; }

#endif
