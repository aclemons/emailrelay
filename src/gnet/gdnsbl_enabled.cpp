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
/// \file gdnsbl_enabled.cpp
///

#include "gdef.h"
#include "gdnsbl.h"
#include "gdnsblock.h"
#include <utility>

class GNet::DnsblImp : private DnsBlockCallback
{
public:
	DnsblImp( std::function<void(bool)> callback , ExceptionSink es , G::string_view config ) :
		m_callback(std::move(callback)) ,
		m_block(*this,es,config)
	{
	}
	void onDnsBlockResult( const DnsBlockResult & result ) override
	{
		result.log() ;
		result.warn() ;
		m_callback( result.allow() ) ;
	}
	std::function<void(bool)> m_callback ;
	DnsBlock m_block ;
} ;

GNet::Dnsbl::Dnsbl( std::function<void(bool)> callback , ExceptionSink es , G::string_view config ) :
	m_imp(std::make_unique<DnsblImp>(callback,es,config))
{
}

GNet::Dnsbl::~Dnsbl()
= default ;

void GNet::Dnsbl::start( const Address & address )
{
	m_imp->m_block.start( address ) ;
}

bool GNet::Dnsbl::busy() const
{
	return m_imp->m_block.busy() ;
}

void GNet::Dnsbl::checkConfig( const std::string & config )
{
	DnsBlock::checkConfig( config ) ;
}

