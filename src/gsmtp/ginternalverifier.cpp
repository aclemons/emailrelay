//
// Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
//
// ginternalverifier.cpp
//

#include "gdef.h"
#include "ginternalverifier.h"
#include "glog.h"

GSmtp::InternalVerifier::InternalVerifier()
= default;

void GSmtp::InternalVerifier::verify( const std::string & to , const std::string & , const GNet::Address & ,
	const std::string & , const std::string & )
{
	VerifierStatus status = verifyInternal( to ) ;
	doneSignal().emit( std::string(to) , status ) ;
}

GSmtp::VerifierStatus GSmtp::InternalVerifier::verifyInternal( const std::string & address ) const
{
	// accept all addresses as if remote
	VerifierStatus status ;
	status.is_valid = true ;
	status.is_local = false ;
	status.address = address ;
	return status ;
}

G::Slot::Signal<const std::string&,const GSmtp::VerifierStatus&> & GSmtp::InternalVerifier::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::InternalVerifier::cancel()
{
}

/// \file ginternalverifier.cpp
