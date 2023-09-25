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
/// \file ginterfaces_win32.cpp
///
// Test with:
//   netsh interface ipv4 add address name="Local Area Connection" address=10.0.0.1
//   netsh interface ipv6 add address interface="Local Area Connection" dead::beef
//   netsh interface ipv4 show addresses
//   ipconfig /all
//

#include "gdef.h"
#include "ginterfaces.h"
#include "gconvert.h"
#include "gbuffer.h"
#include "gexception.h"
#include <iphlpapi.h>
#include <ipifcons.h>

namespace GNet
{
	class InterfacesNotifierImp ;
}

class GNet::InterfacesNotifierImp : public InterfacesNotifier
{
public:
	InterfacesNotifierImp( Interfaces * , ExceptionSink es ) ;
		// Constructor.

	~InterfacesNotifierImp() override ;
		// Destructor.

private: // overrides
	std::string readEvent() override ; // unix
	std::string onFutureEvent() override ; // windows

private:
	static void CALLBACK interface_callback_fn( void * p , MIB_IPINTERFACE_ROW * , MIB_NOTIFICATION_TYPE ) ;
	static void CALLBACK address_callback_fn( void * p , MIB_UNICASTIPADDRESS_ROW * , MIB_NOTIFICATION_TYPE ) ;

private:
	static constexpr unsigned int MAGIC = 0xdeadbeef ;
	unsigned int m_magic ;
	HANDLE m_notify_1 ;
	HANDLE m_notify_2 ;
	HANDLE m_handle ;
	FutureEvent m_future_event ;
} ;

bool GNet::Interfaces::active()
{
	return true ;
}

void GNet::Interfaces::loadImp( ExceptionSink es , std::vector<Item> & list )
{
	if( !m_notifier.get() )
		m_notifier = std::make_unique<InterfacesNotifierImp>( this ,es ) ;

	ULONG flags = 0 ;
	flags |= GAA_FLAG_SKIP_ANYCAST ;
	flags |= GAA_FLAG_SKIP_MULTICAST ;
	flags |= GAA_FLAG_SKIP_DNS_SERVER ;
	//flags |= GAA_FLAG_INCLUDE_ALL_INTERFACES ;
	//flags |= GAA_FLAG_INCLUDE_PREFIX ;

	G::Buffer<char> buffer ;
	buffer.resize( 15000U ) ; // size as recommended
	ULONG size = static_cast<ULONG>( buffer.size() ) ;
	IP_ADAPTER_ADDRESSES * p = G::buffer_cast<IP_ADAPTER_ADDRESSES*>( buffer ) ;

	ULONG rc = GetAdaptersAddresses( AF_UNSPEC , flags , /*reserved=*/nullptr , p , &size ) ;

	if( rc == ERROR_BUFFER_OVERFLOW )
	{
		buffer.resize( size ) ;
		p = G::buffer_cast<IP_ADAPTER_ADDRESSES*>( buffer ) ;
		rc = GetAdaptersAddresses( AF_UNSPEC , flags , nullptr , p , &size ) ;
	}

	if( rc != ERROR_NO_DATA )
	{
		if( rc != ERROR_SUCCESS )
			throw G::Exception( "GetAdaptersAddresses failed" ) ;

		for( ; p ; p = p->Next )
		{
			Item item ;
			item.name = std::string( p->AdapterName ) ;
			G::Convert::utf8 altname ;
			G::Convert::convert( altname , std::wstring(p->FriendlyName) ) ;
			item.altname = altname.s ;
			item.up = p->OperStatus == IfOperStatusUp ;
			item.loopback = p->IfType == IF_TYPE_SOFTWARE_LOOPBACK ;

			for( IP_ADAPTER_UNICAST_ADDRESS * ap = p->FirstUnicastAddress ; ap ; ap = ap->Next )
			{
				item.address_family = ap->Address.lpSockaddr->sa_family ;
				if( Address::supports(ap->Address.lpSockaddr->sa_family,0) )
				{
					item.address = Address( ap->Address.lpSockaddr , ap->Address.iSockaddrLength ) ;
					item.valid_address = !item.address.isAny() ; // just in case
				}
#ifndef G_MINGW
				UINT8 prefix_length = ap->OnLinkPrefixLength ;
				if( prefix_length <= 128U )
				{
					item.has_netmask = true ;
					item.netmask_bits = prefix_length ;
				}
#endif
				item.ifindex = p->IfIndex ? p->IfIndex : p->Ipv6IfIndex ;
				list.push_back( item ) ;
			}
		}
	}
}

// ==

GNet::InterfacesNotifierImp::InterfacesNotifierImp( Interfaces * outer , ExceptionSink es ) :
	m_magic(MAGIC) ,
	m_notify_1(HNULL) ,
	m_notify_2(HNULL) ,
	m_handle(HNULL) ,
	m_future_event(*outer,es)
{
	m_handle = m_future_event.handle() ;

	NotifyIpInterfaceChange( AF_UNSPEC , &InterfacesNotifierImp::interface_callback_fn ,
		this , FALSE , &m_notify_1 ) ;

	NotifyUnicastIpAddressChange( AF_UNSPEC , &InterfacesNotifierImp::address_callback_fn ,
		this , FALSE , &m_notify_2 ) ;
}

GNet::InterfacesNotifierImp::~InterfacesNotifierImp()
{
	m_magic = 0 ;
	if( m_notify_1 ) CancelMibChangeNotify2( m_notify_1 ) ;
	if( m_notify_2 ) CancelMibChangeNotify2( m_notify_2 ) ;
}

void GNet::InterfacesNotifierImp::interface_callback_fn( void * this_vp , MIB_IPINTERFACE_ROW * ,
	MIB_NOTIFICATION_TYPE )
{
	// worker thread -- keep it simple
	InterfacesNotifierImp * this_ = static_cast<InterfacesNotifierImp*>(this_vp) ;
	if( this_->m_magic == InterfacesNotifierImp::MAGIC && this_->m_handle )
	{
		; // no-op -- rely on address notifications
	}
}

void GNet::InterfacesNotifierImp::address_callback_fn( void * this_vp , MIB_UNICASTIPADDRESS_ROW * ,
	MIB_NOTIFICATION_TYPE )
{
	// worker thread -- keep it simple
	InterfacesNotifierImp * this_ = static_cast<InterfacesNotifierImp*>(this_vp) ;
	if( this_->m_magic == InterfacesNotifierImp::MAGIC && this_->m_handle )
	{
		FutureEvent::send( this_->m_handle , false ) ;
	}
}

std::string GNet::InterfacesNotifierImp::readEvent()
{
	// never gets here
	return std::string() ;
}

std::string GNet::InterfacesNotifierImp::onFutureEvent()
{
	return "network-change" ;
}

