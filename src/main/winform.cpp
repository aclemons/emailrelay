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
// winform.cpp
//

#include "gdef.h"
#include "gssl.h"
#include "gmonitor.h"
#include "run.h"
#include "winform.h"
#include "news.h"
#include "winapp.h"
#include "legal.h"
#include "resource.h"
#include <sstream>

Main::WinForm::WinForm( WinApp & app , const Main::Configuration & cfg , bool confirm ) :
	GGui::Dialog(app) ,
	m_app(app) ,
	m_edit_box(*this,IDC_EDIT1) ,
	m_cfg(cfg) ,
	m_confirm(confirm)
{
}

bool Main::WinForm::onInit()
{
	m_edit_box.set( text() ) ;
	return true ;
}

std::string Main::WinForm::text() const
{
	const std::string crlf( "\r\n" ) ;

	std::ostringstream ss ;
	ss
		<< "E-MailRelay V" << Main::Run::versionNumber() << crlf << crlf
		<< Main::News::text(crlf+crlf)
		<< Main::Legal::copyright() << crlf << crlf 
		<< GSsl::Library::credit("",crlf,crlf)
		<< Main::Legal::warranty("",crlf) << crlf 
		<< "Configuration..." << crlf 
		<< str(m_cfg,"* ",crlf) 
		;

	if( GSsl::Library::instance() != NULL )
	{
		bool ssl = GSsl::Library::instance()->enabled() ;
		ss << "* tls/ssl smtp client? " << (ssl?"yes":"no") << crlf ;
	}

	if( GNet::Monitor::instance() )
	{
		ss << crlf << "Network connections..." << crlf ;
		GNet::Monitor::instance()->report( ss , "* " , crlf ) ;
	}

	return ss.str() ;
}

void Main::WinForm::close()
{
	end() ;
}

void Main::WinForm::onNcDestroy()
{
	m_app.formDone() ;
}

void Main::WinForm::onCommand( unsigned int id )
{
	if( id == IDOK && ( !m_confirm || m_app.confirm() ) )
	{
		m_app.formOk() ;
	}
}

std::string Main::WinForm::yn( bool b )
{
	return b ? std::string("yes") : std::string("no") ;
}

std::string Main::WinForm::na()
{
	return "<none>" ;
}

std::string Main::WinForm::na( const std::string & s )
{
	return s.empty() ? na() : s ;
}

std::string Main::WinForm::any( const G::Strings & ss )
{
	std::string s = ss.empty() ? std::string() : *ss.begin() ;
	return s.empty() ? std::string("<any>") : s ;
}

std::string Main::WinForm::str( const Configuration & c , const std::string & p , const std::string & eol )
{
	std::ostringstream ss ;
	ss
		<< p << "allow remote clients? " << yn(c.allowRemoteClients()) << eol
		<< p << "smtp listening port: " << (c.doServing()&&c.doSmtp()?G::Str::fromUInt(c.port()):na()) << eol
		<< p << "smtp listening interface: " << (c.doServing()&&c.doSmtp()?any(c.listeningInterfaces("smtp")):na()) << eol
		<< p << "pop listening port: " << (c.doServing()&&c.doPop()?G::Str::fromUInt(c.popPort()):na()) << eol
		<< p << "pop listening interface: " << (c.doServing()&&c.doPop()?any(c.listeningInterfaces("pop")):na()) << eol
		<< p << "admin listening port: " << (c.doAdmin()?G::Str::fromUInt(c.adminPort()):na()) << eol
		<< p << "admin listening interface: " << (c.doAdmin()?any(c.listeningInterfaces("admin")):na()) << eol
		<< p << "forward-to address: " << (c.serverAddress().length()?c.serverAddress():na()) << eol
		<< p << "spool directory: " << c.spoolDir() << eol
		<< p << "smtp client secrets file: " << na(c.clientSecretsFile()) << eol
		<< p << "smtp server secrets file: " << na(c.serverSecretsFile()) << eol
		<< p << "pop server secrets file: " << na(c.popSecretsFile()) << eol
		<< p << "pid file: " << (c.usePidFile()?c.pidFile():na()) << eol
		<< p << "immediate forwarding? " << yn(c.immediate()) << eol
		<< p << "mail processor: " << (c.useFilter()?c.filter():na()) << eol
		<< p << "address verifier: " << na(c.verifier()) << eol
		<< p << "run as daemon? " << yn(c.daemon()) << eol
		<< p << "verbose logging? " << yn(c.verbose()) << eol
		<< p << "debug logging? " << yn(c.debug()) << eol
		<< p << "log to stderr/syslog? " << yn(c.log()) << eol
		<< p << "use syslog? " << yn(c.useSyslog()) << eol
		<< p << "close stderr? " << yn(c.closeStderr()) << eol
		<< p << "connect timeout: " << c.connectionTimeout() << "s" << eol
		<< p << "response timeout: " << c.responseTimeout() << "s" << eol
		<< p << "prompt timeout: " << c.promptTimeout() << "s" << eol
		<< p << "domain override: " << na(c.fqdn()) << eol
		<< p << "polling period: " << (c.pollingTimeout()?(G::Str::fromUInt(c.pollingTimeout())+"s"):na()) << eol
		;
	return ss.str() ;
}

/// \file winform.cpp
