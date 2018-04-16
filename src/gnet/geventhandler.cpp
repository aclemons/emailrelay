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
// geventhandler.cpp
//

#include "gdef.h"
#include "geventhandler.h"
#include "gtimerlist.h"
#include "geventloop.h"
#include "gexception.h"
#include "gdebug.h"
#include "gassert.h"
#include "gdescriptor.h"
#include "glog.h"
#include <algorithm>

GNet::EventHandler::~EventHandler()
{
}

void GNet::EventHandler::readEvent()
{
	G_DEBUG( "GNet::EventHandler::readEvent: no override" ) ;
}

void GNet::EventHandler::writeEvent()
{
	G_DEBUG( "GNet::EventHandler::writeEvent: no override" ) ;
}

void GNet::EventHandler::oobEvent()
{
	throw G::Exception( "socket disconnect event" ) ; // was "exception event" but too confusing
}

/// \file geventhandler.cpp
