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
/// \file gexecutableverifier.h
///

#ifndef G_EXECUTABLE_VERIFIER_H
#define G_EXECUTABLE_VERIFIER_H

#include "gdef.h"
#include "gverifier.h"
#include "gtask.h"
#include "gtimer.h"
#include <string>

namespace GVerifiers
{
	class ExecutableVerifier ;
}

//| \class GVerifiers::ExecutableVerifier
/// A Verifier that runs an executable.
///
class GVerifiers::ExecutableVerifier : public GSmtp::Verifier, private GNet::TaskCallback
{
public:
	ExecutableVerifier( GNet::ExceptionSink , const G::Path & , unsigned int timeout ) ;
		///< Constructor.

private: // overrides
	G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> & doneSignal() override ; // GSmtp::Verifier
	void cancel() override ; // GSmtp::Verifier
	void onTaskDone( int , const std::string & ) override ; // GNet::TaskCallback
	void verify( GSmtp::Verifier::Command , const std::string & rcpt_to_parameter ,
		const GSmtp::Verifier::Info & ) override ; // GSmtp::Verifier

public:
	~ExecutableVerifier() override = default ;
	ExecutableVerifier( const ExecutableVerifier & ) = delete ;
	ExecutableVerifier( ExecutableVerifier && ) = delete ;
	ExecutableVerifier & operator=( const ExecutableVerifier & ) = delete ;
	ExecutableVerifier & operator=( ExecutableVerifier && ) = delete ;

private:
	void onTimeout() ;

private:
	GNet::Timer<ExecutableVerifier> m_timer ;
	GSmtp::Verifier::Command m_command {GSmtp::Verifier::Command::VRFY} ;
	G::Path m_path ;
	unsigned int m_timeout ;
	G::Slot::Signal<GSmtp::Verifier::Command,const GSmtp::VerifierStatus&> m_done_signal ;
	std::string m_to_address ;
	GNet::Task m_task ;
} ;

#endif
