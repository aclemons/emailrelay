//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// ginterfaces_unix.cpp
//

#include "gdef.h"
#include "ginterfaces.h"
#include "gassert.h"
#include "gexception.h"
#include "geventloop.h"
#include "gprocess.h"
#include "gstr.h"
#include "groot.h"
#include <ifaddrs.h>
#include <functional>
#include <sstream>

namespace GNet
{
	class InterfacesNotifierImp ;
}

/// \class GNet::InterfacesNotifierImp
/// Handles read events on a routing netlink socket.
///
class GNet::InterfacesNotifierImp : public InterfacesNotifier
{
public:
	static bool active() ;
	InterfacesNotifierImp( Interfaces * , ExceptionSink es ) ;
	std::string readEvent() override ; // unix
	std::string onFutureEvent() override ; // windows
	void readSocket() ;

public:
	std::vector<char> m_buffer ;
	std::unique_ptr<RawSocket> m_socket ;
} ;

// ==

bool GNet::Interfaces::active()
{
	return InterfacesNotifierImp::active() ;
}

void GNet::Interfaces::loadImp( ExceptionSink es , std::vector<Item> & list )
{
	if( !m_notifier )
		m_notifier = std::make_unique<InterfacesNotifierImp>( this , es ) ;

	ifaddrs * info_p = nullptr ;
	int rc = getifaddrs( &info_p ) ;
	if( rc < 0 )
	{
		int e = G::Process::errno_() ;
		throw G::Exception( "getifaddrs error" , G::Process::strerror(e) ) ;
	}

	using deleter_fn_t = std::pointer_to_unary_function<ifaddrs*,void> ;
	using deleter_t = std::unique_ptr<ifaddrs,deleter_fn_t> ;
	deleter_t deleter( info_p , std::ptr_fun(freeifaddrs) ) ;

	std::size_t nmax = AddressStorage().n() ;
	for( ; info_p != nullptr ; info_p = info_p->ifa_next )
	{
		G_ASSERT( info_p->ifa_name && info_p->ifa_name[0] ) ; if( info_p->ifa_name == nullptr ) continue ;
		G_ASSERT( info_p->ifa_addr ) ; if( info_p->ifa_addr == nullptr ) continue ;

		Item item ;
		item.name = std::string( info_p->ifa_name ) ;
		item.address_family = info_p->ifa_addr->sa_family ;

		if( !Address::supports(info_p->ifa_addr->sa_family,0) )
			continue ;

		item.address = Address( info_p->ifa_addr , nmax ) ;
		item.valid_address = !item.address.isAny() ; // just in case
		if( item.address.family() == Address::Family::ipv6 )
			item.address.setZone( item.name ) ;

		item.has_netmask = info_p->ifa_netmask != nullptr ;
		if( item.has_netmask )
		{
			Address netmask( info_p->ifa_netmask , nmax ) ;
			item.netmask_bits = netmask.bits() ;
		}

		item.up = !!( info_p->ifa_flags & IFF_UP ) ;
		item.loopback = !!( info_p->ifa_flags & IFF_LOOPBACK ) ;

		list.push_back( item ) ;
	}
}

// ==

void GNet::InterfacesNotifierImp::readSocket()
{
	m_buffer.resize( 4096U ) ;
	ssize_t rc = m_socket->read( &m_buffer[0] , m_buffer.size() ) ;
	int e = G::Process::errno_() ; G__IGNORE_VARIABLE(int,e) ;
	if( rc < 0 )
	{
		G_DEBUG( "GNet::InterfacesNotifierImp: read error: " << G::Process::strerror(e) ) ;
		m_buffer.clear() ;
	}
	else
	{
		m_buffer.resize( static_cast<std::size_t>(rc) ) ;
	}
}

std::string GNet::InterfacesNotifierImp::onFutureEvent()
{
	return std::string() ;
}

#if GCONFIG_HAVE_RTNETLINK

#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

bool GNet::InterfacesNotifierImp::active()
{
	return true ;
}

GNet::InterfacesNotifierImp::InterfacesNotifierImp( Interfaces * outer , ExceptionSink es )
{
	if( EventLoop::exists() )
	{
		union netlink_address_union
		{
			struct sockaddr_nl specific ;
			struct sockaddr generic ;
		} ;
		netlink_address_union address {} ;
		address.specific.nl_family = AF_NETLINK ;
		#if GCONFIG_HAVE_IPV6
			address.specific.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR ;
		#else
			address.specific.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR ;
		#endif
		{
			G::Root claim_root ;
			m_socket = std::make_unique<RawSocket>( AF_NETLINK , NETLINK_ROUTE ) ;
			int rc = ::bind( m_socket->fd() , &address.generic , sizeof(address.specific) ) ;
			int e = G::Process::errno_() ;
			if( rc < 0 )
				throw G::Exception( "netlink socket bind error" , G::Process::strerror(e) ) ;
		}
		m_socket->addReadHandler( *outer , es ) ;
	}
}

std::string GNet::InterfacesNotifierImp::readEvent()
{
	readSocket() ;

	std::ostringstream ss ;
	const nlmsghdr * hdr = reinterpret_cast<const nlmsghdr*>( &m_buffer[0] ) ;
	std::size_t size = m_buffer.size() ;
	const char * sep = "" ;
	for( ; NLMSG_OK(hdr,size) ; hdr = NLMSG_NEXT(hdr,size) , sep = ", " )
	{
		if( hdr->nlmsg_type == NLMSG_DONE || hdr->nlmsg_type == NLMSG_ERROR )
			break ;

		if( hdr->nlmsg_type == RTM_NEWLINK ||
			hdr->nlmsg_type == RTM_DELLINK ||
			hdr->nlmsg_type == RTM_GETLINK )
		{
			ifinfomsg * p = static_cast<ifinfomsg*>( NLMSG_DATA(hdr) ) ; G__IGNORE_VARIABLE(ifinfomsg*,p) ;
			int n = NLMSG_PAYLOAD( hdr , size ) ; G__IGNORE_VARIABLE(int,n) ;
			ss << sep << "link" ;
			if( hdr->nlmsg_type == RTM_NEWLINK ) ss << " new" ;
			if( hdr->nlmsg_type == RTM_DELLINK ) ss << " deleted" ;
		}
		else if( hdr->nlmsg_type == RTM_NEWADDR ||
			hdr->nlmsg_type == RTM_DELADDR ||
			hdr->nlmsg_type == RTM_GETADDR )
		{
			ifaddrmsg * p = static_cast<ifaddrmsg*>( NLMSG_DATA(hdr) ) ; G__IGNORE_VARIABLE(ifaddrmsg*,p) ;
			int n = NLMSG_PAYLOAD( hdr , size ) ; G__IGNORE_VARIABLE(int,n) ;
			ss << sep << "address" ;
			if( hdr->nlmsg_type == RTM_NEWADDR ) ss << " new" ;
			if( hdr->nlmsg_type == RTM_DELADDR ) ss << " deleted" ;
		}
	}
	return ss.str() ;
}

#else

#if GCONFIG_HAVE_NETROUTE

// see route(4)
#include <net/route.h>

bool GNet::InterfacesNotifierImp::active()
{
	return true ;
}

GNet::InterfacesNotifierImp::InterfacesNotifierImp( Interfaces * outer , ExceptionSink es )
{
	if( EventLoop::exists() )
	{
		{
			G::Root claim_root ;
			m_socket = std::make_unique<RawSocket>( PF_ROUTE , AF_UNSPEC ) ;
		}
		m_socket->addReadHandler( *outer , es ) ;
	}
}

std::string GNet::InterfacesNotifierImp::readEvent()
{
	readSocket() ;

	std::string result ;
	if( m_buffer.size() >= 4U )
	{
		using Header = struct rt_msghdr ;
		Header * p = reinterpret_cast<Header*>(&m_buffer[0]) ;
		if( p->rtm_msglen != m_buffer.size() )
			G_DEBUG( "GNet::InterfacesNotifierImp::readEvent: invalid message length" ) ;
		if( p->rtm_type == RTM_NEWADDR )
			result = "address new" ;
		else if( p->rtm_type == RTM_DELADDR )
			result = "address deleted" ;
		else if( p->rtm_type == RTM_IFINFO )
			result = "interface change" ;
	}
	return result ;
}

#else

bool GNet::InterfacesNotifierImp::active()
{
	return false ;
}

GNet::InterfacesNotifierImp::InterfacesNotifierImp( Interfaces * , ExceptionSink )
{
}

std::string GNet::InterfacesNotifierImp::readEvent()
{
	return std::string() ;
}

#endif
#endif
