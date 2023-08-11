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
/// \file gdeliveryfilter.h
///

#ifndef G_DELIVERY_FILTER_H
#define G_DELIVERY_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gsimplefilterbase.h"
#include "gfilestore.h"
#include "gexceptionsink.h"

namespace GFilters
{
	class DeliveryFilter ;
}

//| \class GFilters::DeliveryFilter
/// A concrete GSmtp::Filter class that copies the message to multiple
/// spool sub-directories according to the envelope recipient list. A
/// recipient has to match an entry in the password database, otherwise
/// it is copied into to the catch-all "postmaster" sub-directory.
/// Sub-directories are created on-the-fly and content files are
/// hard linked where possible.
///
class GFilters::DeliveryFilter : public SimpleFilterBase
{
public:
	DeliveryFilter( GNet::ExceptionSink es , GStore::FileStore & ,
		Filter::Type , const Filter::Config & , const std::string & spec ) ;
			///< Constructor.

private: // overrides
	Result run( const GStore::MessageId & , bool & , GStore::FileStore::State ) override ;

private:
	GStore::FileStore & m_store ;
	Filter::Type m_filter_type ;
	Filter::Config m_filter_config ;
	std::string m_spec ;
} ;

#endif
