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
///
/// \file gprocessorfactory.h
///

#ifndef G_SMTP_PROCESSOR_FACTORY_H
#define G_SMTP_PROCESSOR_FACTORY_H

#include "gdef.h"
#include "gsmtp.h"
#include "gprocessor.h"
#include <string>
#include <utility>

/// \namespace GSmtp
namespace GSmtp
{
	class ProcessorFactory ;
}

/// \class GSmtp::ProcessorFactory
/// A factory for message processors.
///
class GSmtp::ProcessorFactory 
{
public:
	static Processor * newProcessor( const std::string & address , unsigned int timeout ) ;
		///< Returns a Processor on the heap.

	static std::string check( const std::string & address ) ;
		///< Checks an address. Returns an empty string if okay,
		///< or a diagnostic reason string.

private:
	typedef std::pair<std::string,std::string> Pair ;
	ProcessorFactory() ; // not implemented
	static Pair split( const std::string & ) ;
} ;

#endif
