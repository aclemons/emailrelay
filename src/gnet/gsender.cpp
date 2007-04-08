//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gsender.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gsender.h"
#include "gassert.h"
#include "glog.h"

GNet::Sender::Sender( Server::PeerInfo peer_info , bool throw_ ) :
	ServerPeer(peer_info) ,
	m_throw(throw_) ,
	m_n(0UL)
{
}

GNet::Sender::~Sender()
{
}

bool GNet::Sender::send( const std::string & data , size_t offset )
{
	if( data.length() <= offset )
		return true ; // nothing to do

	ssize_t rc = socket().write( data.data()+offset , data.length()-offset ) ;
	G_DEBUG( "GNet::Sender::send: socket write: " << rc ) ;
	if( rc < 0 && ! socket().eWouldBlock() )
	{
		throw SendError() ;
	}
	else if( rc < 0 || static_cast<size_t>(rc) < (data.length()-offset) )
	{
		if( m_throw )
			throw SendError() ;

		size_t sent = rc > 0 ? static_cast<size_t>(rc) : 0U ;
		m_n += sent ;

		m_residue = data ;
		if( (sent+offset) != 0U )
			m_residue.erase( 0U , sent+offset ) ;

		G_DEBUG( "GNet::Sender::send: flow control asserted: "
			<< "after " << m_n << " byte(s): "
			<< "sent " << sent << "/" << (data.length()-offset) << ": "
			<< m_residue.length() << " residue" ) ;

		socket().addWriteHandler(*this) ;
		return false ;
	}
	else
	{
		m_n += data.length() ;
		return true ;
	}
}

void GNet::Sender::writeEvent()
{
	try
	{
		G_DEBUG( "GNet::Sender::writeEvent: flow-control released: residue " << m_residue.length() ) ;
		G_ASSERT( m_residue.length() != 0U ) ;

		ssize_t rc = socket().write( m_residue.data() , m_residue.length() ) ;
		if( rc < 0 && ! socket().eWouldBlock() )
		{
			throw SendError() ; // caught below
		}
		else if( rc < 0 || static_cast<size_t>(rc) < m_residue.length() )
		{
			size_t sent = rc > 0 ? static_cast<size_t>(rc) : 0U ;
			m_n += sent ;

			G_DEBUG( "GNet::Sender::writeEvent: flow-control reasserted: "
				<< "after " << m_n << " byte(s): "
				<< "sent " << sent << "/" << m_residue.length() ) ;

			if( sent != 0U )
				m_residue.erase( 0U , sent ) ;
		}
		else
		{
			m_n += m_residue.length() ;
			m_residue.erase() ; // for luck
			socket().dropWriteHandler() ;
			onResume() ;
		}
	}
	catch( std::exception & e )
	{
		G_WARNING( "GNet::Sender::writeEvent: exception: " << e.what() ) ;
		doDelete() ;
	}
}

/// \file gsender.cpp
