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
/// \file gmessagedelivery.h
///

#ifndef G_MESSAGE_DELIVERY_H
#define G_MESSAGE_DELIVERY_H

#include "gdef.h"
#include "gmessagestore.h"
#include <string>

namespace GStore
{
	class MessageDelivery ;
}

//| \class GStore::MessageDelivery
/// An interface to deliver a message to its local recipients' mailboxes.
///
class GStore::MessageDelivery
{
public:
	virtual void deliver( const MessageId & ) = 0 ;
		///< Delivers the given message to its local recipients' mailboxes.
		///< The original message is removed from the store.

	virtual ~MessageDelivery() ;
		///< Destructor.
} ;

#endif
