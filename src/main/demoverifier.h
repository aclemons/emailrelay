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
/// \file demoverifier.h
///

#ifndef MAIN_DEMO_VERIFIER_H
#define MAIN_DEMO_VERIFIER_H

#include "gdef.h"
#include "gverifier.h"
#include "gexceptionsink.h"
#include "gslot.h"
#include "gtimer.h"

namespace Main
{
	class DemoVerifier ;
	class Unit ;
	class Run ;
}

//| \class Main::DemoVerifier
/// A concrete Verifier class that does nothing useful.
///
class Main::DemoVerifier : public GSmtp::Verifier
{
public:
	DemoVerifier( GNet::ExceptionSink es , Main::Run & , Main::Unit & , const std::string & spec ) ;
		///< Constructor.

	~DemoVerifier() override ;
		///< Destructor.

private: // overrides
	void verify( Command command , const std::string & , const std::string & , const G::BasicAddress &  ,
		const std::string & , const std::string & ) override ;
	G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & doneSignal() override ;
	void cancel() override ;

private:
	void onTimeout() ;

private:
	using Signal = G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> ;
	Run & m_run ;
	Unit & m_unit ;
	GNet::Timer<DemoVerifier> m_timer ;
	Command m_command ;
	GSmtp::VerifierStatus m_result ;
	Signal m_done_signal ;
} ;

#endif
