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
/// \file gfilterfactorybase.h
///

#ifndef G_SMTP_FILTER_FACTORY_BASE_H
#define G_SMTP_FILTER_FACTORY_BASE_H

#include "gdef.h"
#include "gfilter.h"
#include "gstringview.h"
#include "gfilestore.h"
#include "geventstate.h"
#include "gexception.h"
#include <string>
#include <utility>
#include <memory>

namespace GSmtp
{
	class FilterFactoryBase ;
}

//| \class GSmtp::FilterFactoryBase
/// A factory interface for making GSmtp::Filter message processors.
///
class GSmtp::FilterFactoryBase
{
public:
	struct Spec /// Filter specification tuple for GSmtp::FilterFactoryBase::newFilter().
	{
		Spec() ;
		Spec( std::string_view , std::string_view ) ;
		Spec & operator+=( const Spec & ) ;
		std::string first ; // "exit", "file", "net", "spam", "chain", empty on error
		std::string second ; // reason on error, or eg. "/bin/a" if "file", eg. "file:/bin/a,file:/bin/b" if "chain"
	} ;

	virtual std::unique_ptr<Filter> newFilter( GNet::EventState ,
		Filter::Type , const Filter::Config & , const Spec & spec ) = 0 ;
			///< Returns a Filter on the heap. Optionally throws if
			///< an invalid or unsupported filter specification.

	virtual ~FilterFactoryBase() = default ;
		///< Destructor.
} ;

#endif
