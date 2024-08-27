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
/// \file glisteners.cpp
///

#include "gdef.h"
#include "glisteners.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>

GNet::Listeners::Listeners( const Interfaces & if_ , const G::StringArray & listener_list , unsigned int port )
{
	// listeners are file-descriptors, addresses or interface names (possibly decorated)
	for( const auto & listener : listener_list )
	{
		int fd = G::is_windows() ? -1 : parseFd( listener ) ;
		if( fd >= 0 )
		{
			m_fds.push_back( fd ) ;
		}
		else if( isAddress(listener,port) )
		{
			m_fixed.push_back( address(listener,port) ) ;
		}
		else
		{
			std::size_t n = if_.addresses( m_dynamic , basename(listener) , port , af(listener) ) ;
			if( n == 0U && isBad(listener) )
				m_bad = listener ;
			(n?m_used:m_empties).push_back( listener ) ;
		}
	}
	if( empty() )
		addWildcards( port ) ;
}

int GNet::Listeners::af( const std::string & s )
{
	if( G::Str::tailMatch(s,"-ipv6") )
		return AF_INET6 ;
	else if( G::Str::tailMatch(s,"-ipv4") )
		return AF_INET ;
	else
		return AF_UNSPEC ;
}

std::string GNet::Listeners::basename( const std::string & s )
{
	return
		G::Str::tailMatch(s,"-ipv6") || G::Str::tailMatch(s,"-ipv4") ?
			s.substr( 0U , s.length()-5U ) :
			s ;
}

int GNet::Listeners::parseFd( const std::string & listener )
{
    if( listener.size() > 3U && listener.find("fd#") == 0U && G::Str::isUInt(listener.substr(3U)) )
    {
        int fd = G::Str::toInt( listener.substr(3U) ) ;
        if( fd < 0 ) throw InvalidFd( listener ) ;
        return fd ;
    }
    return -1 ;
}

void GNet::Listeners::addWildcards( unsigned int port )
{
	if( StreamSocket::supports(Address::Family::ipv4) )
		m_fixed.emplace_back( Address::Family::ipv4 , port ) ;

	if( StreamSocket::supports(Address::Family::ipv6) )
		m_fixed.emplace_back( Address::Family::ipv6 , port ) ;
}

bool GNet::Listeners::isAddress( const std::string & s , unsigned int port )
{
	return Address::validStrings( s , G::Str::fromUInt(port) ) ;
}

GNet::Address GNet::Listeners::address( const std::string & s , unsigned int port )
{
	return Address::parse( s , port ) ;
}

bool GNet::Listeners::empty() const
{
	return m_fds.empty() && m_fixed.empty() && m_dynamic.empty() ;
}

bool GNet::Listeners::defunct() const
{
	return empty() && !Interfaces::active() ;
}

bool GNet::Listeners::idle() const
{
	return empty() && hasEmpties() && Interfaces::active() ;
}

bool GNet::Listeners::noUpdates() const
{
	return !m_used.empty() && !Interfaces::active() ;
}

bool GNet::Listeners::isBad( const std::string & s )
{
	// the input is not an address and not an interface-with-addresses so
	// report it as bad if clearly not an interface-with-no-addresses --
	// a slash is not normally allowed in an interface name, but allow "/dev/..."
	// because of bsd
	return s.empty() || ( s.find('/') != std::string::npos && s.find("/dev/") != 0U ) ;
}

bool GNet::Listeners::hasBad() const
{
	return !m_bad.empty() ;
}

std::string GNet::Listeners::badName() const
{
	return m_bad ;
}

bool GNet::Listeners::hasEmpties() const
{
	return !m_empties.empty() ;
}

std::string GNet::Listeners::logEmpties() const
{
	return std::string(m_empties.size()==1U?" \"":"s \"").append(G::Str::join("\", \"",m_empties)).append(1U,'"') ;
}

const std::vector<int> & GNet::Listeners::fds() const
{
	return m_fds ;
}

const std::vector<GNet::Address> & GNet::Listeners::fixed() const
{
	return m_fixed ;
}

const std::vector<GNet::Address> & GNet::Listeners::dynamic() const
{
	return m_dynamic ;
}

