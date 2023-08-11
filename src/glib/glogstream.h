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
/// \file glogstream.h
///

#ifndef G_LOG_STREAM_H
#define G_LOG_STREAM_H

#include "gdef.h"
#include <ios>

namespace G
{
	//| \class G::LogStream
	/// A non-throwing wrapper for std::ostream, used by G::Log.
	/// This allows streaming to a G::Log instance to be inherently
	/// non-throwing without needing a try/catch block at every
	/// call site. The most common streaming operators are
	/// implemented out-of-line as a modest code size optimisation.
	///
	struct LogStream
	{
		explicit LogStream( std::ostream * s ) noexcept :
			m_ostream(s)
		{
		}
		std::ostream * m_ostream ;
	} ;
}

namespace G
{
	LogStream & operator<<( LogStream & s , const std::string & ) noexcept ;
	LogStream & operator<<( LogStream & s , const char * ) noexcept ;
	LogStream & operator<<( LogStream & s , char ) noexcept ;
	LogStream & operator<<( LogStream & s , unsigned char ) noexcept ;
	LogStream & operator<<( LogStream & s , int ) noexcept ;
	LogStream & operator<<( LogStream & s , unsigned int ) noexcept ;
	LogStream & operator<<( LogStream & s , long ) noexcept ;
	LogStream & operator<<( LogStream & s , unsigned long ) noexcept ;

	template <typename T> LogStream & operator<<( LogStream & s , const T & t ) noexcept
	{
		if( s.m_ostream )
		{
			try
			{
					*(s.m_ostream) << t ;
			}
			catch(...)
			{
			}
		}
		return s ;
	}
}

#endif
