//
// Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gexecutableverifier.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gstrings.h"
#include "gexecutableverifier.h"
#include "gprocess.h"
#include "gnewprocess.h"
#include "gfile.h"
#include "groot.h"
#include "gstr.h"
#include "glocal.h"
#include "gassert.h"
#include "glog.h"

GSmtp::ExecutableVerifier::ExecutableVerifier( const G::Executable & exe ) :
	m_exe(exe)
{
}

void GSmtp::ExecutableVerifier::verify( const std::string & to , 
	const std::string & from , const GNet::Address & ip ,
	const std::string & mechanism , const std::string & extra )
{
	G_DEBUG( "GSmtp::ExecutableVerifier::verify: to \"" << to << "\": from \"" << from << "\": "
		<< "ip \"" << ip.displayString(false) << "\": auth-mechanism \"" << mechanism << "\": "
		<< "auth-extra \"" << extra << "\"" ) ;

	std::string user = G::Str::head( to , to.find('@') , to ) ;
	std::string host = G::Str::tail( to , to.find('@') , std::string() ) ;

	VerifierStatus status = 
			verifyExternal( to , G::Str::upper(user) , G::Str::upper(host) , 
				G::Str::upper(GNet::Local::fqdn()) , from , ip , mechanism , extra ) ;

	doneSignal().emit( to , status ) ;
}

GSmtp::VerifierStatus GSmtp::ExecutableVerifier::verifyExternal( const std::string & address , 
	const std::string & user , const std::string & host , const std::string & fqdn , const std::string & from ,
	const GNet::Address & ip , const std::string & mechanism , const std::string & extra ) const
{
	G::Strings args( m_exe.args() ) ;
	args.push_back( address ) ;
	args.push_back( user ) ;
	args.push_back( host ) ;
	args.push_back( fqdn ) ;
	args.push_back( from ) ;
	args.push_back( ip.displayString(false) ) ;
	args.push_back( mechanism ) ;
	args.push_back( extra ) ;
	G_LOG( "GSmtp::ExecutableVerifier: executing " << m_exe.exe() << " " << address << " " << user << " "
		<< host << " " << fqdn << " " << from << " " << ip.displayString(false) << " "
		<< "\"" << mechanism << "\" \"" << extra << "\"" ) ;

	std::string response ;
	int rc = G::NewProcess::spawn( G::Root::nobody() , m_exe.exe() , args , &response ) ;

	G_LOG( "GSmtp::ExecutableVerifier: " << rc << ": \"" << G::Str::printable(response) << "\"" ) ;
	G::Str::trimRight( response , " \n\t" ) ;
	G::Str::replaceAll( response , "\r\n" , "\n" ) ;
	G::Str::replaceAll( response , "\r" , "" ) ;
	G::Strings response_parts ;
	G::Str::splitIntoFields( response , response_parts , "\n" ) ;

	VerifierStatus status ;
	if( ( rc == 0 || rc == 1 ) && response_parts.size() >= 2 )
	{
		status.is_valid = true ;
		status.is_local = rc == 0 ;
		status.full_name = response_parts.front() ;
		response_parts.pop_front() ;
		status.address = response_parts.front() ;
	}
	else if( rc == 100 )
	{
		throw Verifier::AbortRequest() ;
	}
	else
	{
		status.is_valid = false ;
		status.temporary = rc == 3 ;
		status.reason = response.empty() ? G::Str::fromInt(rc) : response ;
		G::Str::replaceAll( status.reason , "\n" , " " ) ;
		status.reason = G::Str::printable( status.reason ) ;
		status.help = "rejected by external verifier program" ;
	}
	return status ;
}

G::Signal2<std::string,GSmtp::VerifierStatus> & GSmtp::ExecutableVerifier::doneSignal()
{
	return m_done_signal ;
}

void GSmtp::ExecutableVerifier::reset()
{
}

/// \file gexecutableverifier.cpp
