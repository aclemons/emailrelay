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
/// \file gfilestore_win32.cpp
///

#include "gdef.h"
#include "gfilestore.h"
#include "gpath.h"
#include "genvironment.h"
#include <cstdio>
#include <crtdbg.h>

namespace GStore
{
	namespace FileStoreImp
	{
		struct NoCheck
		{
			NoCheck() ;
			~NoCheck() ;
			NoCheck( const NoCheck & ) = delete ;
			NoCheck( NoCheck && ) = delete ;
			NoCheck & operator=( const NoCheck & ) = delete ;
			NoCheck & operator=( NoCheck && ) = delete ;
			_invalid_parameter_handler m_handler ;
			int m_mode ;
			static void handler( const wchar_t * , const wchar_t * , const wchar_t * , unsigned int , uintptr_t ) ;
		} ;
	}
}

G::Path GStore::FileStore::defaultDirectory()
{
	return G::Path(G::Environment::get("ProgramData","c:/ProgramData"))/"E-MailRelay"/"spool" ;
}

void GStore::FileStore::osinit()
{
	constexpr int limit = 8192 ;
	if( _getmaxstdio() < limit )
	{
		FileStoreImp::NoCheck no_check ;
		_setmaxstdio( limit ) ;
	}
}

GStore::FileStoreImp::NoCheck::NoCheck() :
	m_handler(_set_invalid_parameter_handler(NoCheck::handler)) ,
	m_mode(_CrtSetReportMode(_CRT_ASSERT,0))
{
}

GStore::FileStoreImp::NoCheck::~NoCheck()
{
	_set_invalid_parameter_handler( m_handler ) ;
	_CrtSetReportMode( _CRT_ASSERT , m_mode ) ;
}

void GStore::FileStoreImp::NoCheck::handler( const wchar_t * , const wchar_t * , const wchar_t * , unsigned int , uintptr_t )
{
	// no-op
}

