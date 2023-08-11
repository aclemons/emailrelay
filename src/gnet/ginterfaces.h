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
/// \file ginterfaces.h
///

#ifndef G_NET_INTERFACES_H
#define G_NET_INTERFACES_H

#include "gdef.h"
#include "gaddress.h"
#include "gstringarray.h"
#include "gexceptionsink.h"
#include "geventhandler.h"
#include "gfutureevent.h"
#include "gsocket.h"
#include <string>
#include <memory>
#include <vector>

namespace GNet
{
	class Interfaces ;
	class InterfacesHandler ;
	class InterfacesNotifier ;
}

//| \class GNet::Interfaces
/// A class for getting a list of network interfaces and their addresses.
/// An InterfacesHandler interface can be supplied to the constructor
/// in order to get dynamic updates.
///
class GNet::Interfaces : public EventHandler , public FutureEventHandler
{
public:
	struct Item /// Used by GNet::Interfaces to describe an interface address binding.
	{
		std::string name ; // interface name
		std::string altname ; // windows friendly name, utf8
		int ifindex {0} ; // interface 1-based index, 0 on error, family-specific on windows
		unsigned int address_family {0} ;
		bool valid_address {false} ;
		Address address ;
		bool has_netmask {false} ;
		unsigned int netmask_bits {0U} ;
		bool up {false} ;
		bool loopback {false} ;
		Item() ;
	} ;
	using const_iterator = std::vector<Item>::const_iterator ;

	Interfaces() ;
		///< Default constructor resulting in an empty list.
		///< Use load() to initialise.

	Interfaces( ExceptionSink , InterfacesHandler & ) ;
		///< Constructor resulting in an empty list with an
		///< attached event handler. Use load() or addresses() to
		///< initialise the list and activate the event
		///< listener.

	~Interfaces() override ;
		///< Destructor.

	static bool supported() ;
		///< Returns false if a stubbed-out implementation.

	static bool active() ;
		///< Returns true if the implementation can raise
		///< InterfacesHandler events.

	void load() ;
		///< Loads or reloads the list.

	bool loaded() const ;
		///< Returns true if load()ed.

	G::StringArray names( bool all = false ) const ;
		///< Returns the interface names, optionally including
		///< interfaces that are not up.

	const_iterator begin() const ;
		///< Returns a begin iterator.

	const_iterator end() const ;
		///< Returns a one-off-the-end iterator.

	std::vector<Address> addresses( const std::string & name , unsigned int port , int af = AF_UNSPEC ) const ;
		///< Returns addresses bound to the given interface.
		///< Does a lazy load().

	std::size_t addresses( std::vector<Address> & out , const std::string & name , unsigned int port , int af = AF_UNSPEC ) const ;
		///< An overload that appends to the given list and returns
		///< the number added.

private: // overrides
	void readEvent() override ; // GNet::EventHandler
	void onFutureEvent() override ; // GNet::FutureEventHandler

public:
	Interfaces( const Interfaces & ) = delete ;
	Interfaces( Interfaces && ) = delete ;
	Interfaces operator=( const Interfaces & ) = delete ;
	Interfaces operator=( Interfaces && ) = delete ;

private:
	using AddressList = std::vector<Address> ;
	void loadImp( ExceptionSink , std::vector<Item> & list ) ;
	static int index( const std::string & ) ;

private:
	ExceptionSink m_es ;
	InterfacesHandler * m_handler{nullptr} ;
	mutable bool m_loaded{false} ;
	mutable std::vector<Item> m_list ;
	std::unique_ptr<InterfacesNotifier> m_notifier ;
} ;

//| \class GNet::InterfacesHandler
/// An interface for receiving notification of network changes.
///
class GNet::InterfacesHandler
{
public:
	virtual void onInterfaceEvent( const std::string & ) = 0 ;
		///< Indicates some network event that might have invalidated
		///< the GNet::Interfaces state, requiring a re-load().

	virtual ~InterfacesHandler() = default ;
		///< Destructor.
} ;

//| \class GNet::InterfacesNotifier
/// A pimple base-class used by GNet::Interfaces.
///
class GNet::InterfacesNotifier
{
public:
	virtual std::string readEvent() = 0 ;
		///< Called by GNet::Interfaces to handle a read event.
		///< Returns a diagnostic representation of the event
		///< or the empty string.

	virtual std::string onFutureEvent() = 0 ;
		///< Called by GNet::Interfaces to handle a future event.
		///< Returns a diagnostic representation of the event
		///< or the empty string.

	virtual ~InterfacesNotifier() = default ;
		///< Destructor.
} ;

#endif
