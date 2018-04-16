//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gfilter.h
///

#ifndef G_SMTP_FILTER__H
#define G_SMTP_FILTER__H

#include "gdef.h"
#include "gsmtp.h"
#include "gslot.h"

namespace GSmtp
{
	class Filter ;
}

/// \class GSmtp::Filter
/// An interface for processing a message file through a filter.
/// The interface is asynchronous, using a slot/signal completion
/// callback.
///
/// \code
/// Foo::filter()
/// {
///   m_filter->start( message_file ) ;
/// }
/// void Foo::filterDone( bool ok )
/// {
///   if( ok ) { ... }
///   else if( m_filter->specialCancelled() ) { ... }
///   else { handleError(m_filter->text()) ; }
///   if( m_filter->specialOther() ) { ... }
/// }
/// \endcode
///
class GSmtp::Filter
{
public:
	virtual ~Filter() ;
		///< Destructor.

	virtual std::string id() const = 0 ;
		///< Returns the id passed to the derived-class constructor.
		///< Used in logging.

	virtual bool simple() const = 0 ;
		///< Returns true if the concrete filter class is one that can
		///< never change the file (eg. a do-nothing filter class).

	virtual void start( const std::string & path ) = 0 ;
		///< Starts the filter for the given message file. Any previous,
		///< incomplete filtering is cancel()ed. Asynchronous completion
		///< is indicated by a doneSignal().

	virtual G::Slot::Signal1<bool> & doneSignal() = 0 ;
		///< Returns a signal which is raised once start() has completed
		///< or failed. The signal parameter is a simple boolean success
		///< flag but additional information is availble from text()
		///< and specialWhatever().

	virtual void cancel() = 0 ;
		///< Aborts any incomplete filtering.

	virtual std::string text() const = 0 ;
		///< Returns a non-empty reason string if the filter failed.

	virtual bool specialCancelled() const = 0 ;
		///< Returns true if the filter failed and further processesing
		///< of the current message should be cancelled. Further
		///< processing includes failing of the message. This is useful
		///< if the filter has deleted the message, for example.

	virtual bool specialOther() const = 0 ;
		///< Returns true if the filter indicated special handling is
		///< required. What "special handling" means will depend on the
		///< context, esp. client vs. server side. This indicator should
		///< be considered to be orthogonal to success-or-failure.

protected:
	struct Exit /// Interprets an executable filter's exit code.
	{
		Exit( int , bool ) ;
		bool ok ;
		bool cancelled ;
		bool other ;
	} ;

private:
	void operator=( const Filter & ) ; // not implemented
} ;

#endif
