//
// Copyright (C) 2001-2011 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gnocheck_win32.cpp
//

#include "gdef.h"
#include "gnocheck.h"
#include <cstdlib>
#include <crtdbg.h>

class G::NoCheckImp 
{
public:
	_invalid_parameter_handler m_h ;
	int m_mode ;
} ;

namespace
{
	void local_handler( const wchar_t * , const wchar_t * , const wchar_t * , unsigned int , uintptr_t )
	{
	}
}

G::NoCheck::NoCheck() :
	m_imp(new NoCheckImp)
{
	m_imp->m_h = _set_invalid_parameter_handler( local_handler ) ;
	m_imp->m_mode = _CrtSetReportMode( _CRT_ASSERT , 0 ) ;
}

G::NoCheck::~NoCheck()
{
	_set_invalid_parameter_handler( m_imp->m_h ) ;
	_CrtSetReportMode( _CRT_ASSERT , m_imp->m_mode ) ;
	delete m_imp ;
}

/// \file gnocheck_win32.cpp
