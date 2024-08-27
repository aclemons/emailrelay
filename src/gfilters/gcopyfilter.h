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
/// \file gcopyfilter.h
///

#ifndef G_COPY_FILTER_H
#define G_COPY_FILTER_H

#include "gdef.h"
#include "gsimplefilterbase.h"
#include "gfilestore.h"
#include "genvelope.h"
#include "gexception.h"

namespace GFilters
{
	class CopyFilter ;
}

//| \class GFilters::CopyFilter
/// A concrete GSmtp::Filter class that copies the message to all
/// pre-existing sub-directories of the spool directory. This is
/// similar to the 'emailrelay-filter-copy' utility.
///
class GFilters::CopyFilter : public SimpleFilterBase
{
public:
	G_EXCEPTION( Error , tx("copy filter failed to copy message files into sub-directory") )

	CopyFilter( GNet::EventState es , GStore::FileStore & ,
		Filter::Type , const Filter::Config & , const std::string & spec ) ;
			///< Constructor.

private: // overrides
	Result run( const GStore::MessageId & , bool & , GStore::FileStore::State ) override ;

private:
	using FileOp = GStore::FileStore::FileOp ;
	GStore::FileStore & m_store ;
	Filter::Config m_filter_config ;
	std::string m_spec ;
	bool m_pop_by_name {false} ;
	bool m_hardlink {false} ;
	bool m_no_delete {false} ;
} ;

#endif
