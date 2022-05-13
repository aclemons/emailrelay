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
/// \file gfilter.h
///

#ifndef G_SMTP_FILTER_H
#define G_SMTP_FILTER_H

#include "gdef.h"
#include "gslot.h"
#include "gmessagestore.h"

namespace GSmtp
{
	class Filter ;
}

//| \class GSmtp::Filter
/// An interface for processing a message file through a filter.
/// The interface is asynchronous, using a slot/signal completion
/// callback.
///
/// Filters return a tri-state value (ok, abandon, fail) and
/// a 'special' flag which is interpreted as 're-scan' for
/// server filters and 'stop-scanning' for client filters.
///
/// The abandon state is treated more like success on the
/// server side but more like failure on the client side.
///
/// The fail state has an associated public response (eg.
/// "rejected") and a more expansive private reason.
///
class GSmtp::Filter
{
public:
	virtual ~Filter() = default ;
		///< Destructor.

	virtual std::string id() const = 0 ;
		///< Returns the id passed to the derived-class constructor.
		///< Used in logging.

	virtual bool simple() const = 0 ;
		///< Returns true if the concrete filter class is one that can
		///< never change the file (eg. a do-nothing filter class).

	virtual void start( const MessageId & ) = 0 ;
		///< Starts the filter for the given message. Any previous,
		///< incomplete filtering is cancel()ed. Asynchronous completion
		///< is indicated by a doneSignal().

	virtual G::Slot::Signal<int> & doneSignal() = 0 ;
		///< Returns a signal which is raised once start() has completed
		///< or failed. The signal parameter is ok=0, abandon=1, fail=2.

	virtual void cancel() = 0 ;
		///< Aborts any incomplete filtering.

	virtual bool abandoned() const = 0 ;
		///< Returns true if the filter result was 'abandoned'.

	virtual std::string response() const = 0 ;
		///< Returns a non-empty response string iff the filter failed,
		///< or an empty response if successful or abandoned.

	virtual std::string reason() const = 0 ;
		///< Returns a non-empty reason string iff the filter failed,
		///< or an empty reason if successful or abandoned.

	virtual bool special() const = 0 ;
		///< Returns true if the filter indicated special handling is
		///< required.

	std::string str( bool server_side ) const ;
		///< Returns a diagnostic string for logging.

public:
	enum class Result // Filter tri-state result value.
	{
		f_ok = 0 ,
		f_abandon = 1 ,
		f_fail = 2
	} ;

protected:
	struct Exit /// Interprets an executable filter's exit code.
	{
		Exit( int exit_code , bool server_side ) ;
		bool ok() const ;
		bool abandon() const ;
		bool fail() const ;
		Result result ;
		bool special ;
	} ;
} ;

#endif
