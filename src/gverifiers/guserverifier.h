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
/// A concrete Verifier class that verifies against the password database.
/// Users are valid if they appear in the password database with a uid
/// within the configured range. A sub-range for 'local' users can be
/// configured.
///
class GVerifiers::UserVerifier : public GSmtp::Verifier
{
public:
	UserVerifier( GNet::ExceptionSink es , bool local ,
		const GSmtp::Verifier::Config & config , const std::string & spec ,
		bool allow_postmaster = true ) ;
			///< Constructor. If 'local' (typically for outgoing messages) then
			///< addresses are verified as local if they match the 'spec' range
			///< with the correct domain and remote otherwise. If not 'local'
			///< (typically for incoming messages) then addresses are verified
			///< as remote if they match the 'spec' range and are rejected
			///< otherwise.

	~UserVerifier() override ;
		///< Destructor.

private: // overrides
	void verify( Command command , const std::string & , const GSmtp::Verifier::Info & ) override ;
	G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & doneSignal() override ;
	void cancel() override ;

private:
	void onTimeout() ;
	std::string explain( int uid , const std::string & , const std::string & ) const ;
	static int lookup( const std::string & ) ;
	static std::string normalise( const std::string & ) ;

private:
	using Signal = G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> ;
	Command m_command ;
	bool m_local ;
	GSmtp::Verifier::Config m_config ;
	GNet::Timer<UserVerifier> m_timer ;
	GSmtp::VerifierStatus m_result ;
	Signal m_done_signal ;
	std::pair<int,int> m_range ;
	bool m_allow_postmaster ;
} ;

#endif
