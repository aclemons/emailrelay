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
///
/// \file gexecutableprocessor.h
///

#ifndef G_SMTP_EXECUTABLE_PROCESSOR_H
#define G_SMTP_EXECUTABLE_PROCESSOR_H

#include "gdef.h"
#include "gsmtp.h"
#include "gexecutable.h"
#include "gprocessor.h"

/// \namespace GSmtp
namespace GSmtp
{
	class ExecutableProcessor ;
}

/// \class GSmtp::ExecutableProcessor
/// A Processor class that processes message files 
/// using an external preprocessor program.
///
class GSmtp::ExecutableProcessor : public GSmtp::Processor 
{
public:
	explicit ExecutableProcessor( const G::Executable & ) ;
		///< Constructor.

	virtual ~ExecutableProcessor() ;
		///< Destructor.

	virtual G::Signal1<bool> & doneSignal() ;
		///< Final override from from GSmtp::Processor.

	virtual void start( const std::string & path ) ;
		///< Final override from from GSmtp::Processor.

	virtual void abort() ;
		///< Final override from from GSmtp::Processor.

	virtual std::string text() const ;
		///< Final override from from GSmtp::Processor.
		///<
		///< Returns any "<<text>>" or "[[text]]" output by the executable.

	virtual bool cancelled() const ;
		///< Final override from from GSmtp::Processor.

	virtual bool repoll() const ;
		///< Final override from from GSmtp::Processor.

private:
	ExecutableProcessor( const ExecutableProcessor & ) ; // not implemented
	void operator=( const ExecutableProcessor & ) ; // not implemented
	int preprocessCore( const G::Path & ) ;
	std::string parseOutput( std::string ) const ;
	bool process( const std::string & path ) ;
	static std::string execErrorHandler( int error ) ;

private:
	G::Signal1<bool> m_done_signal ;
	G::Executable m_exe ;
	std::string m_text ;
	bool m_ok ;
	bool m_cancelled ;
	bool m_repoll ;
} ;

#endif
