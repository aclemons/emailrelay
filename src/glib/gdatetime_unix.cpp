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
// gdatetime_unix.cpp
//

#include "gdef.h"
#include "gdatetime.h"

std::tm * G::DateTime::gmtime_r( const std::time_t * t , std::tm * p )
{
	return ::gmtime_r(t,p) ; // see gdef.h
}

std::tm * G::DateTime::localtime_r( const std::time_t * t , std::tm * p )
{
	return ::localtime_r(t,p) ; // see gdef.h
}

/// \file gdatetime_unix.cpp
