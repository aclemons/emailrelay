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
/// \file filterfactory.h
///

#ifndef MAIN_FILTER_FACTORY_H
#define MAIN_FILTER_FACTORY_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilterfactory.h"
#include "gfilestore.h"
#include <memory>
#include <cstddef> // std::nullptr_t

namespace Main
{
	class FilterFactory ;
	class Unit ;
	class Run ;
}

//| \class Main::FilterFactory
/// A FilterFactory that knows about classes in the Main namespace.
///
class Main::FilterFactory : public GFilters::FilterFactory
{
public:
	FilterFactory( Run & , Unit & , GStore::FileStore & ) ;
		///< Constructor.

	static Spec parse( const std::string & spec , const G::Path & base_dir = {} ,
		const G::Path & app_dir = {} , G::StringArray * warnings_p = nullptr ) ;
			///< Parses the filter spec calling the base class
			///< as necessary.

private: // overrides
	std::unique_ptr<GSmtp::Filter> newFilter( GNet::ExceptionSink ,
		bool server_side , const Spec & , unsigned int timeout ,
		const std::string & log_prefix ) override ;

private:
	Main::Run & m_run ;
	Main::Unit & m_unit ;
	GStore::FileStore & m_store ;
} ;

#endif
