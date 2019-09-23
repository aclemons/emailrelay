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
// gclientptr.cpp
//

#include "gdef.h"
#include "gclientptr.h"

GNet::ClientPtrBase::ClientPtrBase()
{
}

void GNet::ClientPtrBase::connectSignals( Client & client )
{
	client.eventSignal().connect( G::Slot::slot(*this,&ClientPtrBase::eventSlot) ) ;
}

G::Slot::Signal1<std::string> & GNet::ClientPtrBase::deletedSignal()
{
	return m_deleted_signal ;
}

G::Slot::Signal3<std::string,std::string,std::string> & GNet::ClientPtrBase::eventSignal()
{
	return m_event_signal ;
}

G::Slot::Signal1<std::string> & GNet::ClientPtrBase::deleteSignal()
{
	return m_delete_signal ;
}

void GNet::ClientPtrBase::disconnectSignals( Client & client )
{
	client.eventSignal().disconnect() ;
}

void GNet::ClientPtrBase::eventSlot( std::string s1 , std::string s2 , std::string s3 )
{
	m_event_signal.emit( s1 , s2 , s3 ) ;
}

/// \file gclientptr.cpp
