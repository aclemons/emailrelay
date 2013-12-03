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
// gconnectionlookup_win32.cc
//

#include "gdef.h"
#include "gnet.h"
#include "gconnectionlookup.h"
#include "gsocket.h"
#include "glog.h"

#ifdef G_MINGW
	typedef enum {
		TcpConnectionOffloadStateInHost = 0,
		TcpConnectionOffloadStateOffloading ,
		TcpConnectionOffloadStateOffloaded ,
		TcpConnectionOffloadStateUploading ,
		TcpConnectionOffloadStateMax
	} TCP_CONNECTION_OFFLOAD_STATE;
	typedef struct _MIB_TCPROW2 {
		DWORD dwState ;
		DWORD dwLocalAddr ;
		DWORD dwLocalPort ;
		DWORD dwRemoteAddr ;
		DWORD dwRemotePort ;
		DWORD dwOwningPid ;
		TCP_CONNECTION_OFFLOAD_STATE dwOffloadState ;
	} MIB_TCPROW2 ;
	typedef struct _MIB_TCPTABLE2 {
		DWORD dwNumEntries ;
		MIB_TCPROW2 table[1] ;
	} MIB_TCPTABLE2 , *PMIB_TCPTABLE2 ;
	enum MIB_TCP_STATE { MIB_TCP_STATE_ESTAB = 5 } ;
	typedef DWORD WINAPI (*GetTcpTable2Fn)( PMIB_TCPTABLE2 pTcpTable , PDWORD pwdSize , BOOL bOrder ) ;
	typedef BOOL WINAPI (*ConvertSidToStringSidAFn)( PSID , LPSTR * ) ;
#else
	#include <iphlpapi.h>
	typedef DWORD (WINAPI *GetTcpTable2Fn)( PMIB_TCPTABLE2 pTcpTable , PDWORD pwdSize , BOOL bOrder ) ;
	typedef BOOL (WINAPI *ConvertSidToStringSidAFn)( PSID , LPSTR * ) ;
#endif

class GNet::ConnectionLookupImp 
{
public:
	typedef GNet::ConnectionLookup::Connection Connection ;
	ConnectionLookupImp() ;
	Connection find( GNet::Address local , GNet::Address peer ) ;
	std::pair<std::string,std::string> lookup( PSID ) ;

private:
	GetTcpTable2Fn m_GetTcpTable2 ;
	ConvertSidToStringSidAFn m_ConvertSidToStringSidA ;
} ;

// ==

bool GNet::ConnectionLookup::Connection::valid() const 
{
	return m_valid ;
}

std::string GNet::ConnectionLookup::Connection::peerName() const
{
	return m_peer_name ;
}

// ==

GNet::ConnectionLookup::ConnectionLookup() :
	m_imp(new ConnectionLookupImp)
{
}

GNet::ConnectionLookup::~ConnectionLookup()
{
	delete m_imp ;
}

GNet::ConnectionLookup::Connection GNet::ConnectionLookup::find( GNet::Address local , GNet::Address peer )
{
	return m_imp->find( local , peer ) ;
}

// ==

GNet::ConnectionLookupImp::ConnectionLookupImp() :
	m_GetTcpTable2(0) ,
	m_ConvertSidToStringSidA(0)
{
	{
		HMODULE h = LoadLibraryA( "iphlpapi.dll" ) ;
		if( h == NULL )
		{
			G_WARNING( "GNet::ConnectionLookup::find: iphelper load library failed" ) ;
		}
		else
		{
			m_GetTcpTable2 = reinterpret_cast<GetTcpTable2Fn>( GetProcAddress( h , "GetTcpTable2" ) ) ;
			if( m_GetTcpTable2 == 0 )
				G_WARNING( "GNet::ConnectionLookup::find: no GetTcpTable2()" ) ;
			FreeLibrary( h ) ;
		}
	}

	{
		HMODULE h = LoadLibraryA( "advapi32.dll" ) ;
		if( h == NULL )
		{
			G_WARNING( "GNet::ConnectionLookup::find: advapi load library failed" ) ;
		}
		else
		{
			m_ConvertSidToStringSidA = reinterpret_cast<ConvertSidToStringSidAFn>( GetProcAddress( h , "ConvertSidToStringSidA" ) ) ;
			if( m_ConvertSidToStringSidA == 0 )
				G_WARNING( "GNet::ConnectionLookup::find: no ConvertSidToStringSidA()" ) ;
			FreeLibrary( h ) ;
		}
	}
}

GNet::ConnectionLookup::Connection GNet::ConnectionLookupImp::find( GNet::Address local , GNet::Address peer )
{
	Connection invalid_connection ;
	invalid_connection.m_valid = false ;

	DWORD localAddr = 0 ;
	DWORD localPort = 0 ;
	DWORD remoteAddr = 0 ;
	DWORD remotePort = 0 ;
	{
		union
		{ 
			sockaddr_in specific ; 
			sockaddr general ; 
		} u_local , u_remote ;
		u_local.general = *(local.address()) ;
		u_remote.general = *(peer.address()) ;
		if( u_local.general.sa_family == AF_INET &&
			u_remote.general.sa_family == AF_INET )
		{
			localAddr = u_local.specific.sin_addr.s_addr ;
			localPort = u_local.specific.sin_port ;
			remoteAddr = u_remote.specific.sin_addr.s_addr ;
			remotePort = u_remote.specific.sin_port ;
		}
		G_DEBUG( "GNet::ConnectionLookup::find: this connection: "
			<< localAddr << ":" << ntohs(static_cast<g_uint16_t>(localPort)) << " "
			<< remoteAddr << ":" << ntohs(static_cast<g_uint16_t>(remotePort)) ) ;
	}
	if( localAddr == 0 && remoteAddr == 0 )
		return invalid_connection ;

	DWORD pid = 0 ;
	{
		MIB_TCPTABLE2 empty_table ;
		empty_table.dwNumEntries = 0 ;
		ULONG n = sizeof(empty_table) ;
		DWORD rc = (*m_GetTcpTable2)( &empty_table , &n , FALSE ) ;
		if( rc == ERROR_INSUFFICIENT_BUFFER )
		{
			char * buffer = new char[n] ;
			MIB_TCPTABLE2 * table = reinterpret_cast<MIB_TCPTABLE2*>( buffer ) ;
			table->dwNumEntries = 0 ;
			DWORD rc = (*m_GetTcpTable2)( table , &n , FALSE ) ;
			if( rc == NO_ERROR )
			{
				G_DEBUG( "GNet::ConnectionLookup::find: " << table->dwNumEntries ) ;
				for( DWORD i = 0 ; i < table->dwNumEntries ; i++ )
				{
					MIB_TCPROW2 * row = table->table + i ;
					bool match = row->dwState == MIB_TCP_STATE_ESTAB &&
						row->dwRemoteAddr == row->dwLocalAddr &&
						row->dwLocalAddr == remoteAddr &&
						row->dwLocalPort == remotePort ;
					if( match )
						pid = row->dwOwningPid ;
					G_DEBUG( "GNet::ConnectionLookup::find: " << row->dwState << " " 
						<< row->dwLocalAddr << ":" << ntohs(static_cast<g_uint16_t>(row->dwLocalPort)) << " "
						<< row->dwRemoteAddr << ":" << ntohs(static_cast<g_uint16_t>(row->dwRemotePort)) << " " 
						<< row->dwOwningPid << (match?" <<==":"") ) ;
				}
			}
			delete [] buffer ;
		}
		else
		{
			G_DEBUG( "GNet::ConnectionLookup::find: " << rc << " " << n ) ;
		}
	}
	if( pid == 0 )
	{
		G_DEBUG( "GNet::ConnectionLookup::find: no matching connection" ) ;
		return invalid_connection ;
	}

	std::string sid ;
	std::pair<std::string,std::string> domain_and_name ;
	{
		HANDLE access_token = 0 ;
		HANDLE hprocess = 0 ;
		{
			hprocess = OpenProcess( READ_CONTROL | PROCESS_QUERY_INFORMATION , FALSE , pid ) ;
			if( hprocess == 0 )
			{
				G_DEBUG( "GNet::ConnectionLookup::find: cannot get process handle for pid " << pid ) ;
			}
			else if( 0 == OpenProcessToken( hprocess , TOKEN_QUERY , &access_token ) )
			{
				G_DEBUG( "GNet::ConnectionLookup::find: cannot get access token for pid " << pid ) ;
			}
		}

		if( access_token != 0 )
		{
			DWORD n = 0 ;
			GetTokenInformation( access_token , TokenUser , NULL , 0 , &n ) ;
			if( n == 0 )
			{
				DWORD e = GetLastError() ;
				G_DEBUG( "GNet::ConnectionLookup::find: cannot get token information for pid " << pid << " (" << e << ")" ) ;
			}
			else
			{
				char * info_buffer = new char[n] ;
				TOKEN_USER * info = reinterpret_cast<TOKEN_USER*>( info_buffer ) ;
				if( 0 == GetTokenInformation( access_token , TokenUser , info , n , &n ) )
				{
					DWORD e = GetLastError() ;
					G_DEBUG( "GNet::ConnectionLookup::find: cannot get token information for pid " << pid << " (" << e << ")" ) ;
				}
				else
				{
					PSID psid = info->User.Sid ;
					DWORD attributes = info->User.Attributes ;
					char * sid_buffer = NULL ;
					(*m_ConvertSidToStringSidA)( psid , &sid_buffer ) ;
					if( sid_buffer != NULL )
					{
						sid = std::string( sid_buffer ) ;
						LocalFree( sid_buffer ) ;
					}
					domain_and_name = lookup( psid ) ;
				}
				delete [] info_buffer ;
			}
			CloseHandle( access_token ) ;
		}
		if( hprocess != 0 )
			CloseHandle( hprocess ) ;
	}

	std::string peer_name = sid ;
	if( !domain_and_name.first.empty() && !domain_and_name.second.empty() )
	{
		peer_name.append( "=" ) ;
		peer_name.append( domain_and_name.first ) ;
		peer_name.append( "\\" ) ;
		peer_name.append( domain_and_name.second ) ;
	}
	else if( !domain_and_name.second.empty() )
	{
		peer_name.append( "=" ) ;
		peer_name.append( domain_and_name.second ) ;
	}

	Connection connection ;
	connection.m_valid = true ;
	connection.m_peer_name = peer_name ;
	G_LOG( "GNet::ConnectionLookup::find: peer on port " << ntohs(static_cast<g_uint16_t>(remotePort)) << " is local: "
		"pid " << pid << ": user " << peer_name ) ;
	return connection ;
}

std::pair<std::string,std::string> GNet::ConnectionLookupImp::lookup( PSID psid )
{
	std::string domain ;
	std::string name ;
	{
		DWORD m = 0 ;
		DWORD n = 0 ;
		SID_NAME_USE name_use = SidTypeUnknown ;
		LookupAccountSidA( NULL , psid , NULL , &n , NULL , &m , &name_use ) ;
		if( n != 0 && m != 0 )
		{
			char * domain_buffer = new char[m] ;
			char * name_buffer = new char[n] ;
			if( 0 != LookupAccountSidA( NULL , psid , name_buffer , &n , domain_buffer , &m , &name_use ) )
			{
				domain = std::string( domain_buffer ) ;
				name = std::string( name_buffer ) ;
			}
			delete [] name_buffer ;
			delete [] domain_buffer ;
		}
	}
	return std::make_pair( domain , name ) ;
}

/// \file gconnectionlookup_win32.cpp
