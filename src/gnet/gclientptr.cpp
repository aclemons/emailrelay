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
/// \file gclientptr.cpp
///

#include "gdef.h"
#include "gclientptr.h"

GNet::ClientPtrBase::ClientPtrBase()
= default;

G::Slot::Signal<const std::string&> & GNet::ClientPtrBase::deletedSignal() noexcept
{
	return m_deleted_signal ;
}

G::Slot::Signal<const std::string&,const std::string&,const std::string&> & GNet::ClientPtrBase::eventSignal() noexcept
{
	return m_event_signal ;
}

G::Slot::Signal<const std::string&> & GNet::ClientPtrBase::deleteSignal() noexcept
{
	return m_delete_signal ;
}

void GNet::ClientPtrBase::eventSlot( const std::string & s1 , const std::string & s2 , const std::string & s3 )
{
	m_event_signal.emit( std::string(s1) , std::string(s2) , std::string(s3) ) ;
}

