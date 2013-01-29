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
// gsaslserverfactory_pam.cpp
//

#include "gdef.h"
#include "gnet.h"
#include "gauth.h"
#include "gsaslserverfactory.h"
#include "gexception.h"
#include "gsaslserverbasic.h"
#include "gsaslserverpam.h"

std::auto_ptr<GAuth::SaslServer> GAuth::SaslServerFactory::newSaslServer( const SaslServer::Secrets & secrets , 
	bool b1 , bool b2 )
{
	if( secrets.source() == "/pam" )
		return std::auto_ptr<SaslServer>( new SaslServerPam(secrets,b1,b2) ) ;
	else
		return std::auto_ptr<SaslServer>( new SaslServerBasic(secrets,b1,b2) ) ;
}

/// \file gsaslserverfactory_pam.cpp
