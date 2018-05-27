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
//
// gbufferedserverpeer.cpp
//

#include "gdef.h"
#include "gbufferedserverpeer.h"
#include "gstr.h"
#include "glog.h"

GNet::BufferedServerPeer::BufferedServerPeer( Server::PeerInfo peer_info , LineBufferConfig line_buffer_config ) :
	ServerPeer(peer_info) ,
	m_line_buffer(line_buffer_config)
{
}

GNet::BufferedServerPeer::~BufferedServerPeer()
{
}

void GNet::BufferedServerPeer::onData( const char * p , ServerPeer::size_type n )
{
	m_line_buffer.add( p , n ) ;
	while( m_line_buffer.more() )
	{
		if( !onReceive( m_line_buffer.lineData() , m_line_buffer.lineSize() , m_line_buffer.eolSize() ) )
			break ;
	}
}

void GNet::BufferedServerPeer::expect( size_t n )
{
	m_line_buffer.expect( n ) ;
}

std::string GNet::BufferedServerPeer::lineBufferEndOfLine() const
{
	return m_line_buffer.eol() ;
}

/// \file gbufferedserverpeer.cpp
