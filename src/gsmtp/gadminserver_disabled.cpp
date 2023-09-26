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
/// \file gadminserver_disabled.cpp
///

#include "gdef.h"
#include "gadminserver.h"

class GSmtp::AdminServerImp
{
} ;

bool GSmtp::AdminServer::enabled()
{
	return false ;
}

GSmtp::AdminServer::AdminServer( GNet::ExceptionSink , GStore::MessageStore & ,
	FilterFactoryBase & , const GAuth::SaslClientSecrets & ,
	const G::StringArray & , const Config & )
{
}

GSmtp::AdminServer::~AdminServer()
{
}

void GSmtp::AdminServer::emitCommand( Command , unsigned int )
{
}

G::Slot::Signal<GSmtp::AdminServer::Command,unsigned int> & GSmtp::AdminServer::commandSignal()
{
	throw NotImplemented() ;
}

void GSmtp::AdminServer::report( const std::string & ) const
{
}

void GSmtp::AdminServer::notify( const std::string & , const std::string & , const std::string & , const std::string & )
{
}

GStore::MessageStore & GSmtp::AdminServer::store()
{
	throw NotImplemented() ;
}

GSmtp::FilterFactoryBase & GSmtp::AdminServer::ff()
{
	throw NotImplemented() ;
}

const GAuth::SaslClientSecrets & GSmtp::AdminServer::clientSecrets() const
{
	throw NotImplemented() ;
}

bool GSmtp::AdminServer::notifying() const
{
	return false ;
}

