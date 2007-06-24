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

GNet::Sender::Sender( EventHandler & handler ) :
	m_handler(handler) ,
	m_failed(false) ,
	m_n(0UL)
{
}

GNet::Sender::~Sender()
{
}

bool GNet::Sender::send( Socket & socket , const std::string & data , std::string::size_type offset )
{
	if( data.length() <= offset )
		return true ; // nothing to do

	ssize_t rc = socket.write( data.data()+offset , data.length()-offset ) ;
	if( rc < 0 && ! socket.eWouldBlock() )
	{
		// fatal error, eg. disconnection
		m_failed = true ;
		return false ; 
	}
	else if( rc < 0 || static_cast<std::string::size_type>(rc) < (data.length()-offset) )
	{
		// flow control asserted
		std::string::size_type sent = rc > 0 ? static_cast<size_t>(rc) : 0U ;
		m_n += sent ;

		m_residue = data ;
		if( (sent+offset) != 0U )
			m_residue.erase( 0U , sent+offset ) ;

		G_DEBUG( "GNet::Sender::send: flow control asserted: "
			<< "after " << m_n << " byte(s): "
			<< "sent " << sent << "/" << (data.length()-offset) << ": "
			<< m_residue.length() << " residue" ) ;

		socket.addWriteHandler(m_handler) ;
		return false ;
	}
	else
	{
		// all sent
		m_n += data.length() ;
		return true ;
	}
}

bool GNet::Sender::resumeSending( Socket & socket )
{
	G_DEBUG( "GNet::Sender::resumeSending: flow-control released: residue " << m_residue.length() ) ;
	G_ASSERT( m_residue.length() != 0U ) ;

	ssize_t rc = socket.write( m_residue.data() , m_residue.length() ) ;
	if( rc < 0 && ! socket.eWouldBlock() )
	{
		// fatal error, eg. disconnection
		m_failed = true ;
		return false ;
	}
	else if( rc < 0 || static_cast<std::string::size_type>(rc) < m_residue.length() )
	{
		// flow control re-asserted
		std::string::size_type sent = rc > 0 ? static_cast<std::string::size_type>(rc) : 0U ;
		m_n += sent ;

		G_DEBUG( "GNet::Sender::resumeSending: flow-control reasserted: "
			<< "after " << m_n << " byte(s): "
			<< "sent " << sent << "/" << m_residue.length() ) ;

		if( sent != 0U )
			m_residue.erase( 0U , sent ) ;

		return false ;
	}
	else
	{
		// all sent
		m_n += m_residue.length() ;
		m_residue.erase() ;
		socket.dropWriteHandler() ;
		return true ;
	}
}

bool GNet::Sender::failed() const
{
	return m_failed ;
}

bool GNet::Sender::busy() const
{
	return ! m_residue.empty() ;
}

/// \file gsender.cpp
