//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdatetime.cpp
//

#include "gdef.h"
#include "gdatetime.h"
#include "gstr.h"
#include "gassert.h"
#include <sstream>

namespace
{
	const std::time_t minute = 60U ;
	const std::time_t hour = 60U * minute ;
	const std::time_t day = 24U * hour ;
}

G::DateTime::EpochTime G::DateTime::now()
{
	return std::time(NULL) ;
}

G::DateTime::EpochTime G::DateTime::epochTime( const BrokenDownTime & bdt_in )
{
	// get a rough starting point
	BrokenDownTime bdt( bdt_in ) ;
	EpochTime start = std::mktime( &bdt ) ; // localtime

	// iterate over all timezones -- brute force for now
	const std::time_t delta = minute * 30U ;
	for( EpochTime t = (start-day-delta) ; t <= (start+day+delta) ; t += delta )
	{
		if( equivalent( t , bdt_in ) )
			return t ;
	}
	throw Error() ;
}

G::DateTime::BrokenDownTime G::DateTime::utc( EpochTime epoch_time )
{
	static BrokenDownTime zero ;
	BrokenDownTime result = zero ;
	G::DateTime::gmtime_r( &epoch_time , &result ) ;
	return result ;
}

G::DateTime::BrokenDownTime G::DateTime::local( EpochTime epoch_time )
{
	static BrokenDownTime zero ;
	BrokenDownTime bdt_local = zero ;
	G::DateTime::localtime_r( &epoch_time , &bdt_local ) ;
	return bdt_local ;
}

G::DateTime::Offset G::DateTime::offset( EpochTime utc )
{
	BrokenDownTime bdt_local = local(utc) ;

	EpochTime local = epochTime(bdt_local) ;
	bool ahead = local >= utc ; // ie. east-of
	EpochTime n = ahead ? (local-utc) : (utc-local) ;
	return Offset( ahead , static_cast<unsigned int>(n) ) ;
}

std::string G::DateTime::offsetString( Offset offset )
{
	unsigned int hh = offset.second / 3600U ;
	unsigned int mm = (offset.second / 60U) % 60 ;

	std::ostringstream ss ;
	char sign = (offset.first || (hh==0&&mm==0)) ? '+' : '-' ;
	ss << sign << (hh/10U) << (hh%10U) << (mm/10) << (mm%10) ;
	return ss.str() ;
}

bool G::DateTime::equivalent( EpochTime t , const BrokenDownTime & bdt_in )
{
	BrokenDownTime bdt_test = utc(t) ;
	return same( bdt_test , bdt_in ) ;
}

bool G::DateTime::same( const BrokenDownTime & bdt1 , const BrokenDownTime & bdt2 )
{
	return 
		bdt1.tm_mday == bdt2.tm_mday &&
		bdt1.tm_hour == bdt2.tm_hour &&
		bdt1.tm_min == bdt2.tm_min ;
}

/// \file gdatetime.cpp
