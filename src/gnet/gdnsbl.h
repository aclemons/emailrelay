//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gdnsbl.h
///

#ifndef G_NET_DNSBL_H
#define G_NET_DNSBL_H

#include "gdef.h"
#include "geventstate.h"
#include "gaddress.h"
#include "gstringview.h"
#include <functional>
#include <memory>

namespace GNet
{
	class Dnsbl ;
	class DnsblImp ;
}

//| \class GNet::Dnsbl
/// A minimal bridge to GNet::DnsBlock
///
class GNet::Dnsbl
{
public:
	Dnsbl( std::function<void(bool)> callback , EventState , std::string_view config = {} ) ;
		///< Constructor. See DnsBlock::DnsBlock().

	~Dnsbl() ;
		///< Destructor.

	void start( const Address & ) ;
		///< Starts an asychronous check on the given address. The result
		///< is delivered via the callback function passed to the ctor.

	bool busy() const ;
		///< Returns true after start() and before the completion callback.

	static void checkConfig( const std::string & ) ;
		///< See DnsBlock::checkConfig().

public:
	Dnsbl( const Dnsbl & ) = delete ;
	Dnsbl( Dnsbl && ) = delete ;
	Dnsbl & operator=( const Dnsbl & ) = delete ;
	Dnsbl & operator=( Dnsbl && ) = delete ;

private:
	std::function<void(bool)> m_callback ;
	std::unique_ptr<DnsblImp> m_imp ;
} ;

#endif
