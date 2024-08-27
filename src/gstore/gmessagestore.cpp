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
/// \file gmessagestore.cpp
///

#include "gdef.h"
#include "gmessagestore.h"
#include "gstoredmessage.h"
#include "gstr.h"
#include "gassert.h"
#include "gconvert.h"

std::unique_ptr<GStore::StoredMessage> GStore::operator++( std::shared_ptr<MessageStore::Iterator> & iter )
{
	return iter.get() ? iter->next() : std::unique_ptr<StoredMessage>() ;
}

GStore::MessageStore::AddressStyle GStore::MessageStore::addressStyle( std::string_view address )
{
	if( address.empty() || address.at(0U) == '@' || address.at(address.size()-1U) == '@' )
		return AddressStyle::Invalid ; // missing stuff

	if( !G::Str::isPrintable(address) )
		return AddressStyle::Invalid ; // control characters (inc. DEL)

	std::size_t at_pos = address.rfind( '@' ) ;
	std::string_view mailbox = G::Str::headView( address , at_pos , address ) ;
	std::string_view domain = G::Str::tailView( address , at_pos ) ;
	G_ASSERT( !mailbox.empty() ) ;

	bool mailbox_ascii = G::Str::isPrintableAscii( mailbox ) ;
	bool mailbox_utf8 = !mailbox_ascii && G::Convert::valid( mailbox ) ;
	bool domain_ascii = domain.empty() || G::Str::isPrintableAscii( domain ) ;
	bool domain_utf8 = !domain_ascii && G::Convert::valid( domain ) ;

	if( ( !mailbox_ascii && !mailbox_utf8 ) || ( !domain_ascii && !domain_utf8 ) )
		return AddressStyle::Invalid ; // invalid encoding
	else if( mailbox_ascii && domain_ascii )
		return AddressStyle::Ascii ;
	else if( mailbox_ascii && !domain_ascii )
		return AddressStyle::Utf8Domain ;
	else if( !mailbox_ascii && domain_ascii )
		return AddressStyle::Utf8Mailbox ;
	else
		return AddressStyle::Utf8Both ;
}

