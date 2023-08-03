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
/// \file gsplitfilter.h
///

#ifndef G_SPLIT_FILTER_H
#define G_SPLIT_FILTER_H

#include "gdef.h"
#include "gsimplefilterbase.h"

namespace GFilters
{
	class SplitFilter ;
}

//| \class GFilters::SplitFilter
/// A concrete GSmtp::Filter class for message routing: if the
/// message has recipients for multiple domains then it is
/// split up into a separate message for each domain, with the
/// forward-to envelope field containing the domain name.
///
class GFilters::SplitFilter : public SimpleFilterBase
{
public:
	SplitFilter( GNet::ExceptionSink es , GStore::FileStore & ,
		Filter::Type , const Filter::Config & , const std::string & spec ) ;
			///< Constructor.

private: // overrides
	Result run( const GStore::MessageId & , bool & , GStore::FileStore::State ) override ; // GFilters::SimpleFilterBase

private:
	G::StringArray matching( const G::StringArray & , const std::string & ) const ;
	std::string forwardTo( const std::string & ) const ;
	static bool match( const std::string & , const std::string & , bool raw ) ;
	static void normalise( std::string & , bool raw ) ;

private:
	using FileOp = GStore::FileStore::FileOp ;
	GStore::FileStore & m_store ;
	Filter::Config m_filter_config ;
	bool m_raw {false} ;
	std::string m_port ;
} ;

#endif
