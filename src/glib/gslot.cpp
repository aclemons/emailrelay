//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gslot.cpp
//

#include "gdef.h"
#include "gslot.h"

#ifdef G_SLOT_NEW

void G::Slot::SignalImp::check( const SlotImpBase * p )
{
	if( p != nullptr )
		throw AlreadyConnected() ;
}

#else

G::Slot::SlotImpBase::~SlotImpBase()
{
}

G::Slot::SlotImpBase::SlotImpBase() : m_ref_count(1UL)
{
}

void G::Slot::SlotImpBase::up()
{
	m_ref_count++ ;
}

void G::Slot::SlotImpBase::down()
{
	m_ref_count-- ;
	if( m_ref_count == 0UL )
		delete this ;
}

// ===

void G::Slot::SignalImp::check( const SlotImpBase * p )
{
	if( p != nullptr )
		throw AlreadyConnected() ;
}

#endif
/// \file gslot.cpp
