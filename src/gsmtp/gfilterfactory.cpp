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
/// \file gfilterfactory.cpp
///

#include "gdef.h"
#include "gstr.h"
#include "gfilterfactory.h"
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
	bool server_side , const std::string & identifier , unsigned int timeout )
{
	FactoryParser::Result p = FactoryParser::parse( identifier , true ) ;
	if( p.first.empty() )
	{
		return std::make_unique<NullFilter>( es , server_side ) ; // up-cast
	}
	else if( p.first == "spam" )
	{
		// "spam:" is read-only, not-always-pass
		// "spam-edit:" is read-write, always-pass
		bool edit = p.third == 1 ;
		bool read_only = !edit ;
		bool always_pass = edit ;
		return std::make_unique<SpamFilter>( es , m_file_store , p.second , read_only , always_pass , timeout , timeout ) ; // up-cast
	}
	else if( p.first == "net" )
	{
		return std::make_unique<NetworkFilter>( es , m_file_store , p.second , timeout , timeout ) ; // up-cast
	}
	else if( p.first == "exit" )
	{
		return std::make_unique<NullFilter>( es , server_side , G::Str::toUInt(p.second) ) ; // up-cast
	}
	else
	{
		return std::make_unique<ExecutableFilter>( es , m_file_store , server_side , p.second , timeout ) ; // up-cast
	}
}

