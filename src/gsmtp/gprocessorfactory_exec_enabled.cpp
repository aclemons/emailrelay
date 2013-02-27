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
// gprocessorfactory_exec_enabled.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gstr.h"
#include "gprocessorfactory.h"
#include "gexception.h"
#include "gfactoryparser.h"
#include "gnullprocessor.h"
#include "gnetworkprocessor.h"
#include "gexecutableprocessor.h"
#include "gspamprocessor.h"

std::string GSmtp::ProcessorFactory::check( const std::string & address )
{
	return FactoryParser::check( address , "spam" ) ;
}

GSmtp::Processor * GSmtp::ProcessorFactory::newProcessor( const std::string & address , unsigned int timeout )
{
	Pair p = FactoryParser::parse( address , "spam" ) ;
	if( p.first.empty() )
	{
		return new NullProcessor ;
	}
	else if( p.first == "spam" )
	{
		return new SpamProcessor( p.second , timeout , timeout ) ;
	}
	else if( p.first == "net" )
	{
		return new NetworkProcessor( p.second , timeout , timeout ) ;
	}
	else if( p.first == "exit" )
	{
		return new NullProcessor( G::Str::toUInt(p.second) ) ;
	}
	else 
	{
		return new ExecutableProcessor( G::Executable(p.second) ) ;
	}
}

/// \file gprocessorfactory_exec_enabled.cpp
