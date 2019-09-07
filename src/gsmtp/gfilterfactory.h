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
///
/// \file gfilterfactory.h
///

#ifndef G_SMTP_FILTER_FACTORY__H
#define G_SMTP_FILTER_FACTORY__H

#include "gdef.h"
#include "gfilter.h"
#include "gexceptionsink.h"
#include <string>
#include <utility>

namespace GSmtp
{
	class FilterFactory ;
}

/// \class GSmtp::FilterFactory
/// A factory for message processors.
///
class GSmtp::FilterFactory
{
public:
	static unique_ptr<Filter> newFilter( GNet::ExceptionSink ,
		bool server_side , const std::string & identifier , unsigned int timeout ) ;
			///< Returns a Filter on the heap. The identifier
			///< is normally prefixed with a processor type, or it
			///< is the file system path of an exectuable.

	static std::string check( const std::string & identifier ) ;
		///< Checks an identifier. Returns an empty string if okay,
		///< or a diagnostic reason string.

private:
	FilterFactory() g__eq_delete ;
} ;

#endif
