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
/// \file gfilterfactorybase.cpp
///

#include "gdef.h"
#include "gfilterfactorybase.h"

GSmtp::FilterFactoryBase::Spec::Spec()
= default ;

GSmtp::FilterFactoryBase::Spec::Spec( const std::string & first_ , const std::string & second_ ) :
	first(first_) ,
	second(second_)
{
}

GSmtp::FilterFactoryBase::Spec & GSmtp::FilterFactoryBase::Spec::operator+=( const Spec & rhs )
{
	if( first.empty() && !second.empty() )
	{
		; // already in error state
	}
	else if( rhs.first.empty() )
	{
		first.clear() ; // error state
		second = rhs.second ;
	}
	else
	{
		second.append(",",second.empty()?0U:1U).append(rhs.first).append(1U,':').append(rhs.second) ;
	}
	return *this ;
}

