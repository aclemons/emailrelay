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
/// \file ginterfaces_unix.cpp
///
// Linux:
//   ip address add 127.0.0.2/8 dev lo
//   ip address del 127.0.0.2/8 dev lo
//   ip address add fe80::dead:beef/64 dev eth0
//   ip address del fe80::dead:beef/64 dev eth0
//
// BSD:
//   ifconfig lo0 inet 127.0.0.2 alias netmask 255.0.0.0
//   ifconfig lo0 inet 127.0.0.2 -alias
//   ifconfig em0 inet6 fe80::dead:beef/64 alias
//   ifconfig em0 inet6 fe80::dead:beef/64 -alias
//

#include "gdef.h"
#include "ginterfaces.h"
#include "gassert.h"
#include "gexception.h"
#include "geventloop.h"
#include "gprocess.h"
#include "gbuffer.h"
#include "gstr.h"
#include "groot.h"
#include "glog.h"
#include <ifaddrs.h>
#include <functional>
#include <memory>
#include <utility>
#include <sstream>

namespace GNet
{
	class InterfacesNotifierImp ;
}

//| \class GNet::InterfacesNotifierImp
/// Handles read events on a routing netlink socket.
///
class GNet::InterfacesNotifierImp : public InterfacesNotifier
{
public:
	static bool active() ;
	InterfacesNotifierImp( Interfaces * , EventState es ) ;
	template <typename T> std::pair<T*,std::size_t> readSocket() ;

public: // overrides
	std::string readEvent() override ; // unix
	std::string onFutureEvent() override ; // windows

public:
	G::Buffer<char> m_buffer ;
	std::unique_ptr<RawSocket> m_socket ;
} ;

// ==

bool GNet::Interfaces::active()
{
	return InterfacesNotifierImp::active() ;
}

void GNet::Interfaces::loadImp( EventState es , std::vector<Item> & list )
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

	using deleter_fn_t = std::function<void(ifaddrs*)> ;
	using deleter_t = std::unique_ptr<ifaddrs,deleter_fn_t> ;
	deleter_t deleter( info_p , deleter_fn_t(freeifaddrs) ) ;

	const std::size_t nmax = AddressStorage().n() ;
	const bool scope_id_fixup = G::is_bsd() ;
	for( ; info_p != nullptr ; info_p = info_p->ifa_next )
	{
		G_ASSERT( info_p->ifa_name && info_p->ifa_name[0] ) ;
		if( info_p->ifa_name == nullptr )
			continue ;

		if( info_p->ifa_addr == nullptr )
			continue ;

		if( !Address::supports(info_p->ifa_addr->sa_family,0) )
			continue ;

		Item item ;
		item.name = std::string( info_p->ifa_name ) ;
		item.ifindex = index( item.name ) ;
		item.address_family = info_p->ifa_addr->sa_family ;
		item.address = Address( info_p->ifa_addr , nmax , scope_id_fixup ) ;
		item.valid_address = !item.address.isAny() ; // just in case
		item.up = !!( info_p->ifa_flags & IFF_UP ) ;
		item.loopback = !!( info_p->ifa_flags & IFF_LOOPBACK ) ;
		item.has_netmask = info_p->ifa_netmask != nullptr ;

		if( item.has_netmask )
		{
			if( info_p->ifa_netmask->sa_family == AF_UNSPEC ) // openbsd
				info_p->ifa_netmask->sa_family = info_p->ifa_addr->sa_family ;

			Address netmask( info_p->ifa_netmask , nmax ) ;
			item.netmask_bits = netmask.bits() ;
		}

		list.push_back( item ) ;
	}
}

#if GCONFIG_HAVE_IFINDEX
#include <sys/ioctl.h>
#include <net/if.h>
int GNet::Interfaces::index( const std::string & name )
{
	struct ifreq req {} ;
	G::Str::strncpy_s( req.ifr_name , sizeof(req.ifr_name) , name.c_str() , G::Str::truncate ) ;
	int fd = socket( AF_INET , SOCK_DGRAM , 0 ) ; // man netdevice(7): "any socket.. regardless of.. family or type"
	if( fd < 0 ) return 0 ;
	int rc = ioctl( fd , SIOCGIFINDEX , &req , sizeof(req) ) ;
	close( fd ) ;
	return rc ? 0 : req.ifr_ifindex ;
}
#else
int GNet::Interfaces::index( const std::string & )
{
	return 0 ;
}
#endif

// ==

template <typename T>
std::pair<T*,std::size_t> GNet::InterfacesNotifierImp::readSocket()
{
	static_assert( sizeof(T) <= 4096U , "" ) ;
	m_buffer.resize( 4096U ) ;

	ssize_t rc = m_socket->read( m_buffer.data() , m_buffer.size() ) ;
	if( rc < 0 )
	{
		GDEF_UNUSED int e = G::Process::errno_() ;
		G_DEBUG( "GNet::InterfacesNotifierImp: read error: " << G::Process::strerror(e) ) ;
	}

	T * p = G::buffer_cast<T*>( m_buffer , std::nothrow ) ;
	std::size_t n = static_cast<std::size_t>( rc >= 0 && p ? rc : 0 ) ;
	return { p , n } ;
}

std::string GNet::InterfacesNotifierImp::onFutureEvent()
{
	return {} ;
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

GNet::InterfacesNotifierImp::InterfacesNotifierImp( Interfaces * outer , EventState es )
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
			m_socket = std::make_unique<RawSocket>( AF_NETLINK , SOCK_RAW , NETLINK_ROUTE ) ;
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
	auto buffer_pair = readSocket<nlmsghdr>() ;

	const nlmsghdr * hdr = buffer_pair.first ;
	std::size_t size = buffer_pair.second ;
	if( hdr == nullptr || size == 0U )
		return {} ;

	const char * sep = "" ;
	std::ostringstream ss ;
	for( ; NLMSG_OK(hdr,size) ; hdr = NLMSG_NEXT(hdr,size) , sep = ", " ) // NOLINT
	{
		if( hdr->nlmsg_type == NLMSG_DONE || hdr->nlmsg_type == NLMSG_ERROR )
			break ;

		if( hdr->nlmsg_type == RTM_NEWLINK ||
			hdr->nlmsg_type == RTM_DELLINK ||
			hdr->nlmsg_type == RTM_GETLINK )
		{
			GDEF_UNUSED ifinfomsg * p = static_cast<ifinfomsg*>( NLMSG_DATA(hdr) ) ; // NOLINT
			GDEF_UNUSED int n = NLMSG_PAYLOAD( hdr , size ) ;
			ss << sep << "link" ;
			if( hdr->nlmsg_type == RTM_NEWLINK ) ss << " new" ;
			if( hdr->nlmsg_type == RTM_DELLINK ) ss << " deleted" ;
		}
		else if( hdr->nlmsg_type == RTM_NEWADDR ||
			hdr->nlmsg_type == RTM_DELADDR ||
			hdr->nlmsg_type == RTM_GETADDR )
		{
			GDEF_UNUSED ifaddrmsg * p = static_cast<ifaddrmsg*>( NLMSG_DATA(hdr) ) ; // NOLINT
			GDEF_UNUSED int n = NLMSG_PAYLOAD( hdr , size ) ;
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

GNet::InterfacesNotifierImp::InterfacesNotifierImp( Interfaces * outer , EventState es )
{
	if( EventLoop::exists() )
	{
		{
			G::Root claim_root ;
			m_socket = std::make_unique<RawSocket>( PF_ROUTE , SOCK_RAW , AF_UNSPEC ) ;
		}
		m_socket->addReadHandler( *outer , es ) ;
	}
}

std::string GNet::InterfacesNotifierImp::readEvent()
{
	using Header = struct rt_msghdr ;
	std::string result ;
	auto buffer_pair = readSocket<Header>() ;
	if( buffer_pair.second >= 4U )
	{
		Header * p = buffer_pair.first ;
		if( p->rtm_msglen != buffer_pair.second )
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

GNet::InterfacesNotifierImp::InterfacesNotifierImp( Interfaces * , EventState )
{
}

std::string GNet::InterfacesNotifierImp::readEvent()
{
	return {} ;
}

#endif
#endif
