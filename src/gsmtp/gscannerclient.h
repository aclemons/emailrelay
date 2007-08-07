//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gscannerclient.h
///

#ifndef G_SCANNER_CLIENT_H
#define G_SCANNER_CLIENT_H

#include "gdef.h"
#include "gnet.h"
#include "gsmtp.h"
#include "gclient.h"
#include "gtimer.h"
#include "gpath.h"
#include "gslot.h"
#include "gexception.h"

/// \namespace GSmtp
namespace GSmtp
{
	class ScannerClient ;
}

/// \class GSmtp::ScannerClient
/// A client class which interacts with a remote 'scanner' 
/// process. The client connects to the scanner process and asks it
/// to scan one or more files.
///
class GSmtp::ScannerClient : public GNet::Client 
{
public:
	G_EXCEPTION( FormatError , "scanner server format error" ) ;
	G_EXCEPTION( ProtocolError , "scanner protocol error" ) ;

	ScannerClient( const GNet::ResolverInfo & host_and_service , 
		unsigned int connect_timeout , unsigned int response_timeout ) ;
			///< Constructor.

	void startScanning( const G::Path & path ) ;
		///< Starts the scanning process for the specified content file. The
		///< base class's "event" signal emitted when scanning is complete 
		///< with a first signal parameter of "scanner" and a second parameter
		///< giving the results of the scan (empty on success). Every
		///< scanning request will get a single response as long as this
		///< method is not called re-entrantly from within the previous
		///< request's response signal.

	bool busy() const ;
		///< Returns true after startScanning() and before the subsequent
		///< event signal.

protected:
	virtual ~ScannerClient() ;
		///< Destructor.

private:
	ScannerClient( const ScannerClient & ) ; // not implemented
	void operator=( const ScannerClient & ) ; // not implemented
	virtual void onConnect() ; // GNet::SimpleClient
	virtual bool onReceive( const std::string & ) ; // GNet::Client
	virtual void onSendComplete() ; // GNet::BufferedClient
	virtual void onDelete( const std::string & , bool ) ; // GNet::HeapClient
	virtual void onDeleteImp( const std::string & , bool ) ; // GNet::Client
	void onTimeout() ;
	std::string request( const G::Path & ) const ;
	std::string result( std::string ) const ;
	static std::string eol() ;

private:
	G::Path m_path ;
	GNet::Timer<ScannerClient> m_timer ;
} ;

#endif
