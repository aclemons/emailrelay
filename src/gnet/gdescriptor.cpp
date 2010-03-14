//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdescriptor.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gdescriptor.h"

GNet::Descriptor::Descriptor( SOCKET fd ) :
	m_fd(fd)
{
}

GNet::Descriptor GNet::Descriptor::invalid()
{
	return Descriptor() ;
}

SOCKET GNet::Descriptor::fd() const
{
	return m_fd ;
}

bool GNet::Descriptor::operator<( const Descriptor & other ) const
{
	return m_fd < other.m_fd ;
}

/// \file gdescriptor.cpp
