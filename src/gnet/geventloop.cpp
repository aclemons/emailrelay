//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// geventloop.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "geventloop.h"
#include "gdebug.h"
#include "gassert.h"

GNet::EventLoop * GNet::EventLoop::m_this = NULL ;

GNet::EventLoop::EventLoop()
{
	if( m_this == NULL )
		m_this = this ;
	else
		G_WARNING( "GNet::EventLoop::ctor: multiple instances" ) ;
}

GNet::EventLoop::~EventLoop()
{
	if( m_this == this )
		m_this = NULL ;
}

//static
GNet::EventLoop & GNet::EventLoop::instance()
{
	if( m_this == NULL ) 
		throw NoInstance() ;
	G_ASSERT( m_this != NULL ) ;
	return *m_this ;
}

//static
bool GNet::EventLoop::exists()
{
	return m_this != NULL ;
}

