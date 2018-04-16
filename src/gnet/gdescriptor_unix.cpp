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
// gdescriptor_unix.cpp
//

#include "gdef.h"
#include "gdescriptor.h"

GNet::Descriptor::Descriptor() :
	m_fd(-1) ,
	m_handle(0)
{
}

bool GNet::Descriptor::valid() const
{
	return m_fd >= 0 ;
}

HANDLE GNet::Descriptor::h() const
{
	return 0 ;
}

void GNet::Descriptor::streamOut( std::ostream & stream ) const
{
	stream << m_fd ;
}

/// \file gdescriptor_unix.cpp
