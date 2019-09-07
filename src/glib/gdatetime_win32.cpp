//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdatetime_win32.cpp
//

#include "gdef.h"
#include "gdatetime.h"
#include "gassert.h"
#include <stdexcept>
#include <time.h> // gmtime_s()

G::EpochTime G::DateTime::now()
{
	FILETIME ft ; // 100ns ticks since 1601
	GetSystemTimeAsFileTime( &ft ) ;
	ULARGE_INTEGER n ;
	n.u.HighPart = ft.dwHighDateTime ;
	n.u.LowPart = ft.dwLowDateTime ;
	const ULONGLONG f = 10000000ULL ;
	const ULONGLONG x = 11644473600ULL ; // ( 369 * 365 + 89 ) * 24 * 3600 ;
	ULONGLONG s = ( n.QuadPart / f ) - x ;
	ULONGLONG us = ( n.QuadPart % f ) / 10U ;
	G_ASSERT( us < 1000000ULL ) ;
	return EpochTime( static_cast<std::time_t>(s) , static_cast<unsigned int>(us) ) ;
}

struct std::tm * G::DateTime::gmtime_imp( const time_t * t , struct std::tm * p )
{
	errno_t rc = gmtime_s( p , t ) ;
	if( rc )
		throw std::runtime_error( "gmtime_s error" ) ;
	return p ;
}

struct std::tm * G::DateTime::localtime_imp( const time_t * t , struct std::tm * p )
{
	errno_t rc = localtime_s( p , t ) ;
	if( rc )
		throw std::runtime_error( "localtime_s error" ) ;
	return p ;
}

/// \file gdatetime_win32.cpp
