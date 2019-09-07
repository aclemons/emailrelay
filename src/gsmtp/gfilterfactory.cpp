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
// gfilterfactory.cpp
//

#include "gdef.h"
#include "gstr.h"
#include "gfilterfactory.h"
#include "gnullfilter.h"
#include "gnetworkfilter.h"
#include "gexecutablefilter.h"
#include "gspamfilter.h"
#include "gexception.h"
#include "gfactoryparser.h"

std::string GSmtp::FilterFactory::check( const std::string & identifier )
{
	return FactoryParser::check( identifier , true ) ;
}

unique_ptr<GSmtp::Filter> GSmtp::FilterFactory::newFilter( GNet::ExceptionSink es ,
	bool server_side , const std::string & identifier , unsigned int timeout )
{
	FactoryParser::Result p = FactoryParser::parse( identifier , true ) ;
	if( p.first.empty() )
	{
		return unique_ptr<GSmtp::Filter>( new NullFilter( es , server_side ) ) ;
	}
	else if( p.first == "spam" )
	{
		// "spam:" is read-only, not-always-pass
		// "spam-edit:" is read-write, always-pass
		bool edit = p.third == 1 ;
		bool read_only = !edit ;
		bool always_pass = edit ;
		return unique_ptr<GSmtp::Filter>( new SpamFilter( es , p.second , read_only , always_pass , timeout , timeout ) ) ;
	}
	else if( p.first == "net" )
	{
		return unique_ptr<GSmtp::Filter>( new NetworkFilter( es , p.second , timeout , timeout ) ) ;
	}
	else if( p.first == "exit" )
	{
		return unique_ptr<GSmtp::Filter>( new NullFilter( es , server_side , G::Str::toUInt(p.second) ) ) ;
	}
	else
	{
		return unique_ptr<GSmtp::Filter>( new ExecutableFilter( es , server_side , p.second ) ) ;
	}
}

/// \file gfilterfactory.cpp
