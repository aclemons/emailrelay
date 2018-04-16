//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gfilter.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gfilter.h"

GSmtp::Filter::~Filter()
{
}

GSmtp::Filter::Exit::Exit( int exit_code , bool server_side ) :
	ok(false) ,
	cancelled(false) ,
	other(false)
{
	if( exit_code == 0 )
	{
		ok = true ;
	}
	else if( exit_code >= 100 && exit_code <= 115 )
	{
		if( exit_code == 100 )
		{
			cancelled = true ;
		}
		else if( exit_code == 101 )
		{
			ok = true ;
			other = true ;
		}
		else
		{
			bool goodbit = !!( (exit_code-100) & 1 ) ;
			bool rescanbit = !!( (exit_code-100) & 2 ) ;
			bool stopscanbit = !!( (exit_code-100) & 4 ) ;
			bool cancelbit = !!( (exit_code-100) & 8 ) ;

			cancelled = cancelbit ;
			ok = cancelled ? false : goodbit ;
			other = server_side ? rescanbit : stopscanbit ;
		}
	}
	else
	{
		ok = false ;
	}
}

/// \file gfilter.cpp
