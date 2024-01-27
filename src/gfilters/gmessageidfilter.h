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
/// \file gmessageidfilter.h
///

#ifndef G_MESSAGE_ID_FILTER_H
#define G_MESSAGE_ID_FILTER_H

#include "gdef.h"
#include "gsimplefilterbase.h"
#include "gfilestore.h"
#include "gexception.h"

namespace GFilters
{
	class MessageIdFilter ;
}

//| \class GFilters::MessageIdFilter
/// A filter that adds a RFC-822 Message-ID to the message content if
/// it does not have one already.
///
class GFilters::MessageIdFilter : public SimpleFilterBase
{
public:
	G_EXCEPTION( Error , tx("failed to add message id to content file") ) ;

	MessageIdFilter( GNet::ExceptionSink , GStore::FileStore & ,
		Filter::Type , const Filter::Config & , const std::string & spec ) ;
			///< Constructor.

	static std::string process( const G::Path & , const std::string & domain ) ;
		///< Edits a content file by adding a message-id if necessary.
		///< Returns an error message on error.

private: // overrides
	Result run( const GStore::MessageId & , bool & , GStore::FileStore::State ) override ;

private:
	static bool isId( G::string_view ) noexcept ;
	static std::string newId( const std::string & ) ;

private:
	GStore::FileStore & m_store ;
	std::string m_domain ;
} ;

#endif
