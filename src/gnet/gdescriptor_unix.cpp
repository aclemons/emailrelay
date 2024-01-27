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
/// \file gdescriptor_unix.cpp
///

#include "gdef.h"
#include "gdescriptor.h"

GNet::Descriptor::Descriptor() noexcept :
	m_fd(-1)
{
}

bool GNet::Descriptor::valid() const noexcept
{
	return m_fd >= 0 ;
}

#ifndef G_LIB_SMALL
HANDLE GNet::Descriptor::h() const noexcept
{
	return 0 ;
}
#endif

void GNet::Descriptor::streamOut( std::ostream & stream ) const
{
	stream << m_fd ;
}

