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
/// \file ginternalverifier.cpp
///

#include "gdef.h"
#include "ginternalverifier.h"
#include "glog.h"

GVerifiers::InternalVerifier::InternalVerifier()
= default;

void GVerifiers::InternalVerifier::verify( const GSmtp::Verifier::Request & request )
{
	// accept all addresses as if remote
	auto status = GSmtp::VerifierStatus::remote( request.address ) ;
	doneSignal().emit( request.command , status ) ;
}

G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & GVerifiers::InternalVerifier::doneSignal()
{
	return m_done_signal ;
}

void GVerifiers::InternalVerifier::cancel()
{
}

