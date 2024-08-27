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
/// \file gdescriptor_win32.cpp
///

#include "gdef.h"
#include "gdescriptor.h"

GNet::Descriptor::Descriptor() noexcept :
	m_fd(INVALID_SOCKET) ,
	m_handle(HNULL)
{
}

bool GNet::Descriptor::validfd() const noexcept
{
	return m_fd != INVALID_SOCKET ;
}

HANDLE GNet::Descriptor::h() const noexcept
{
	return m_handle ;
}

void GNet::Descriptor::streamOut( std::ostream & stream ) const
{
	if( m_fd == INVALID_SOCKET )
		stream << "-1" ;
	else
		stream << m_fd ;

	stream << "," << m_handle ;
}

