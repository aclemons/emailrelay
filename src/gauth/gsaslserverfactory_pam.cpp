//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gsaslserverfactory_pam.cpp
///

#include "gdef.h"
#include "gsaslserverfactory.h"
#include "gexception.h"
#include "gsaslserverbasic.h"
#include "gsaslserverpam.h"

std::unique_ptr<GAuth::SaslServer> GAuth::SaslServerFactory::newSaslServer( const SaslServerSecrets & secrets , const std::string & config , bool allow_apop )
{
	if( secrets.source() == "/pam" )
		return std::make_unique<SaslServerPam>( secrets , config , allow_apop ) ; // up-cast
	else
		return std::make_unique<SaslServerBasic>( secrets , config , allow_apop ) ; // up-cast
}

