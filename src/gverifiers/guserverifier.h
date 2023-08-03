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
/// \file guserverifier.h
///

#ifndef G_USER_VERIFIER_H
#define G_USER_VERIFIER_H

#include "gdef.h"
#include "gverifier.h"
#include "gexceptionsink.h"
#include "gslot.h"
#include "gtimer.h"
#include <utility>

namespace GVerifiers
{
	class UserVerifier ;
}

//| \class GVerifiers::UserVerifier
/// A concrete Verifier class that verifies against the password database
/// (ie. getpwnam() or LookupAccountName()).
///
/// The first part of the recipient address has to match an entry in the
/// password database and the second part has to match the configured domain
/// name. A sub-range for the password database entries can be configured
/// via the 'spec' string. This has a sensible default that excludes
/// system accounts. The domain name match is case insensitive.
///
/// By default the matching addresses are returned as valid local
/// mailboxes and non-matching addresses are rejected. With "remote"
/// the matching addresses are returned as remote. With "check" the
/// non-matching addresses are returned as valid and remote.
///
/// The returned mailbox names are the account names as read from the
/// password database, optionally with seven-bit uppercase letters
/// converted to lowercase.
///
class GVerifiers::UserVerifier : public GSmtp::Verifier
{
public:
	UserVerifier( GNet::ExceptionSink es ,
		const GSmtp::Verifier::Config & config , const std::string & spec ) ;
			///< Constructor. The spec string is semi-colon separated list
			///< of values including a uid range and "lc"/"lowercase"
			///< eg. "1000-1002;pm;lc".

private: // overrides
	void verify( Command command , const std::string & , const GSmtp::Verifier::Info & ) override ;
	G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & doneSignal() override ;
	void cancel() override ;

private:
	void onTimeout() ;
	bool lookup( const std::string & , const std::string & , std::string * = nullptr , std::string * = nullptr ) const ;
	static std::string dequote( const std::string & ) ;

private:
	using Signal = G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> ;
	Command m_command ;
	GSmtp::Verifier::Config m_config ;
	GNet::Timer<UserVerifier> m_timer ;
	GSmtp::VerifierStatus m_result ;
	Signal m_done_signal ;
	std::pair<int,int> m_range ;
	bool m_config_lc {false} ;
	bool m_config_check {false} ;
	bool m_config_remote {false} ;
} ;

#endif
