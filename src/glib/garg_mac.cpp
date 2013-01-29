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
// garg_mac.cpp
//

#include "gdef.h"
#include "garg.h"
#include "gfile.h"
#include "genvironment.h"

void G::Arg::setExe()
{
	std::string p = G::Environment::get( "_" , std::string() ) ;
	if( !p.empty() && G::File::exists(p) )
		m_array[0] = p ;
}

/// \file garg_mac.cpp
