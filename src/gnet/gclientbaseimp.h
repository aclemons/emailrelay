//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gclientbaseimp.h
//


#ifndef G_CLIENT_BASE_IMP_H__
#define G_CLIENT_BASE_IMP_H__

#include "gdef.h"
#include "gnet.h"
#include "geventhandler.h"
#include "gaddress.h"
#include <utility>
#include <string>

namespace GNet
{
	class ClientBaseImp ;
}

// Class: GNet::ClientBaseImp
// Description: A base class for GNet::Client pimple-pattern implementation
// classes.
//
class GNet::ClientBaseImp : public GNet::EventHandler 
{
public:
	virtual void run() = 0 ;
	virtual bool connected() const = 0 ;
	virtual void blocked() = 0 ;
	virtual void disconnect() = 0 ;
	virtual std::pair<bool,Address> localAddress() const = 0 ;
	virtual std::pair<bool,Address> peerAddress() const = 0 ;
	virtual std::string peerName() const = 0 ;
	virtual bool connect( std::string host , std::string service , std::string * error , bool sync_dns ) = 0 ;
	virtual void resolveCon( bool ok , const Address & address , std::string reason ) = 0 ;
} ;

#endif
