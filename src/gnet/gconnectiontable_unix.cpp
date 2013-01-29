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
// gconnectiontable_unix.cc
//

#include "gdef.h"
#include "gnet.h"
#include "gconnectiontable.h"

bool GNet::ConnectionTable::Connection::valid() const 
{
	return false ;
}

std::string GNet::ConnectionTable::Connection::peerName() const
{
	return std::string() ;
}

// ==

GNet::ConnectionTable::ConnectionTable() :
	m_imp(NULL)
{
}

GNet::ConnectionTable::~ConnectionTable()
{
}

GNet::ConnectionTable::Connection GNet::ConnectionTable::find( GNet::Address , GNet::Address )
{
	// not implemented
	Connection invalid_connection ;
	invalid_connection.m_valid = false ;
	return invalid_connection ;
}
/// \file gconnectiontable_unix.cpp
