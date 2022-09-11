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
/// \file gexecutableverifier.h
///

#ifndef G_SMTP_EXECUTABLE_VERIFIER_H
#define G_SMTP_EXECUTABLE_VERIFIER_H

#include "gdef.h"
#include "gverifier.h"
#include "gtask.h"
#include <string>

namespace GSmtp
{
	class ExecutableVerifier ;
}

//| \class GSmtp::ExecutableVerifier
/// A Verifier that runs an executable.
///
class GSmtp::ExecutableVerifier : public Verifier, private GNet::TaskCallback
{
public:
	ExecutableVerifier( GNet::ExceptionSink , const G::Path & ) ;
		///< Constructor.

private: // overrides
	G::Slot::Signal<const VerifierStatus&> & doneSignal() override ; // Override from GSmtp::Verifier.
	void cancel() override ; // Override from GSmtp::Verifier.
	void onTaskDone( int , const std::string & ) override ; // override from GNet::TaskCallback
	void verify( const std::string & rcpt_to_parameter ,
		const std::string & mail_from_parameter , const GNet::Address & client_ip ,
		const std::string & auth_mechanism , const std::string & auth_extra ) override ; // Override from GSmtp::Verifier.

public:
	~ExecutableVerifier() override = default ;
	ExecutableVerifier( const ExecutableVerifier & ) = delete ;
	ExecutableVerifier( ExecutableVerifier && ) = delete ;
	ExecutableVerifier & operator=( const ExecutableVerifier & ) = delete ;
	ExecutableVerifier & operator=( ExecutableVerifier && ) = delete ;

private:
	G::Path m_path ;
	G::Slot::Signal<const VerifierStatus&> m_done_signal ;
	std::string m_to_address ;
	GNet::Task m_task ;
} ;

#endif
