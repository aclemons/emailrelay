//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gscannerclient.h
//

#ifndef G_SCANNER_CLIENT_H
#define G_SCANNER_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gclient.h"
#include "gpath.h"
#include "gslot.h"
#include "gtimer.h"
#include "glinebuffer.h"
#include "gexception.h"

namespace GSmtp
{
	class ScannerClient ;
}

// Class: GSmtp::ScannerClient
// Description: A class which interacts with a remote 'scanner' process. The
// interface is asynchronous, with separate 'connect' and 'scan' stages.
//
class GSmtp::ScannerClient : private GNet::Client , private GNet::TimeoutHandler 
{
public:
	G_EXCEPTION( FormatError , "scanner server format error" ) ;

	ScannerClient( const std::string & host_and_service , 
		unsigned int connect_timeout , unsigned int response_timeout ) ;
			// Constructor.

	ScannerClient( const std::string & host , const std::string & service , 
		unsigned int connect_timeout , unsigned int response_timeout ) ;
			// Constructor.

	G::Signal2<std::string,bool> & connectedSignal() ;
		// Returns a signal which indicates that connection
		// is complete.
		//
		// The signal parameters are the empty string on success
		// or a failure reason, and a boolean flag which is
		// true if the failure reason implies a temporary
		// error.

	G::Signal2<bool,std::string> & doneSignal() ;
		// Returns a signal which indicates that scanning
		// is complete.
		//
		// The signal parameters are a boolean flag and
		// a string. If the flag is true then the string is
		// the response from the scanner, empty on success. 
		// If the flag is false then there has been a network 
		// error and the string is a reason string.

	void startConnecting() ;
		// Initiates a connection to the scanner.
		//
		// The connectedSignal() will get raised
		// some time later.

	std::string startScanning( const G::Path & path ) ;
		// Starts the scanning process for the given
		// content file.
		//
		// Returns an error string if an immediate error. 
		//
		// The doneSignal() will get raised some time 
		// after startScanning() returns the empty 
		// string.

private:
	virtual void onConnect( GNet::Socket & socket ) ; // GNet::Client
	virtual void onDisconnect() ; // GNet::Client
	virtual void onData( const char * data , size_t size ) ; // GNet::Client
	virtual void onWriteable() ; // GNet::Client
	virtual void onError( const std::string & error ) ; // GNet::Client
	virtual void onTimeout( GNet::Timer & ) ; // GNet::TimeoutHandler
	void raiseSignal( const std::string & ) ;
	std::string request( const G::Path & ) const ;
	bool isDone() const ;
	std::string result() ;
	void setState( const std::string & ) ;
	static std::string hostPart( const std::string & ) ;
	static std::string servicePart( const std::string & ) ;
	ScannerClient( const ScannerClient & ) ; // not implemented
	void operator=( const ScannerClient & ) ; // not implemented

private:
	G::Signal2<bool,std::string> m_done_signal ;
	G::Signal2<std::string,bool> m_connected_signal ;
	GNet::Timer m_timer ;
	unsigned int m_connect_timeout ;
	unsigned int m_response_timeout ;
	std::string m_state ;
	GNet::Socket * m_socket ;
	GNet::LineBuffer m_line_buffer ;
	std::string m_host ;
	std::string m_service ;
} ;

#endif
