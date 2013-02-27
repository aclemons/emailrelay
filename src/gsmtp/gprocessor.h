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
/// \file gprocessor.h
///

#ifndef G_SMTP_PROCESSOR_H
#define G_SMTP_PROCESSOR_H

#include "gdef.h"
#include "gsmtp.h"
#include "gslot.h"

/// \namespace GSmtp
namespace GSmtp
{
	class Processor ;
}

/// \class GSmtp::Processor
/// An interface for processing message files.
/// \code
///
/// Foo::process()
/// {
///   m_processor->start( message ) ;
/// }
/// void Foo::processorDone( bool ok )
/// {
///   if( ok ) { ... }
///   else if( m_processor->cancelled() ) { ... }
///   else { handleError(m_processor->text()) ; }
///   if( m_processor->repoll() ) { ... }
/// }
/// \endcode
///
class GSmtp::Processor 
{
public:
	virtual ~Processor() ;
		///< Destructor.

	virtual void start( const std::string & path ) = 0 ;
		///< Starts the processor for the given message file.
		///<
		///< Any previous, incomplete processing is abort()ed. 
		///<
		///< Asynchronous completion is indicated by a doneSignal(). 
		///< The signal may be raised before start() returns.

	virtual G::Signal1<bool> & doneSignal() = 0 ;
		///< Returns a signal which is raised once start() has
		///< completed or failed. The signal parameter is a 
		///< success flag.

	virtual void abort() = 0 ;
		///< Aborts any incomplete processing.

	virtual std::string text() const = 0 ;
		///< Returns a non-empty reason string if the 
		///< processor failed.

	virtual bool cancelled() const = 0 ;
		///< Returns true if the processor indicated that
		///< further processesing of the message should
		///< be cancelled. This allows the processor to
		///< delete the message if it wants to.

	virtual bool repoll() const = 0 ;
		///< Returns true if the processor indicated that 
		///< the message store should be repolled immediately.
		///< This this indicator is a side-effect of
		///< message processing, independent of success
		///< or failure.

private:
	void operator=( const Processor & ) ; // not implemented
} ;

#endif
