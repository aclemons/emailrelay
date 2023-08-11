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
/// \file glogstream.cpp
///

#include "gdef.h"
#include "glogstream.h"
#include <ostream>

G::LogStream & G::operator<<( LogStream & s , const std::string & value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}

G::LogStream & G::operator<<( LogStream & s , const char * value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}

G::LogStream & G::operator<<( LogStream & s , char value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}

#ifndef G_LIB_SMALL
G::LogStream & G::operator<<( LogStream & s , unsigned char value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}
#endif

G::LogStream & G::operator<<( LogStream & s , int value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}

G::LogStream & G::operator<<( LogStream & s , unsigned int value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}

G::LogStream & G::operator<<( LogStream & s , long value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}

G::LogStream & G::operator<<( LogStream & s , unsigned long value ) noexcept
{
	try
	{
		if( s.m_ostream ) *(s.m_ostream) << value ;
	}
	catch(...)
	{
	}
	return s ;
}

