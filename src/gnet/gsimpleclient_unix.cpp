//
// Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gsimpleclient_unix.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gclient.h"
#include "gtest.h"

bool GNet::SimpleClient::synchronousDnsDefault()
{
	if( G::Test::enabled("asynchronous-dns") )
		return false ;

	return true ;
}

/// \file gsimpleclient_unix.cpp
