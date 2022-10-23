//
// Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfilterfactory.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "gfilterfactory.h"
#include "gfilterchain.h"
#include "gfilestore.h"
#include "gnullfilter.h"
#include "gnetworkfilter.h"
#include "gexecutablefilter.h"
#include "gspamfilter.h"
#include "gexception.h"
#include "gfactoryparser.h"

GSmtp::FilterFactoryFileStore::FilterFactoryFileStore( FileStore & file_store ) :
	m_file_store(file_store)
{
}

std::unique_ptr<GSmtp::Filter> GSmtp::FilterFactoryFileStore::newFilter( GNet::ExceptionSink es ,
	bool server_side , const FactoryParser::Result & spec , unsigned int timeout ,
	const std::string & log_prefix )
{
	if( spec.first == "chain" )
	{
		// (one level of recursion -- FilterChain::ctor calls newFilter())
		return std::make_unique<FilterChain>( es , *this , server_side , spec , timeout , log_prefix ) ;
	}
	else if( spec.first == "spam" )
	{
		// "spam:" is read-only, not-always-pass
		// "spam-edit:" is read-write, always-pass
		bool edit = spec.third == 1 ;
		bool read_only = !edit ;
		bool always_pass = edit ;
		return std::make_unique<SpamFilter>( es , m_file_store , spec.second , read_only , always_pass , timeout , timeout ) ;
	}
	else if( spec.first == "net" )
	{
		return std::make_unique<NetworkFilter>( es , m_file_store , spec.second , timeout , timeout ) ;
	}
	else if( spec.first == "exit" )
	{
		return std::make_unique<NullFilter>( es , server_side , G::Str::toUInt(spec.second) ) ;
	}
	else if( spec.first == "file" )
	{
		return std::make_unique<ExecutableFilter>( es , m_file_store , server_side , spec.second , timeout , log_prefix ) ;
	}
	else
	{
		throw G::Exception( "invalid filter" , spec.second ) ;
	}
}

