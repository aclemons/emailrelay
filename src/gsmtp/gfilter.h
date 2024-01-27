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
/// \file gfilter.h
///

#ifndef G_SMTP_FILTER_H
#define G_SMTP_FILTER_H

#include "gdef.h"
#include "gslot.h"
#include "gmessagestore.h"
#include "gstringview.h"

namespace GSmtp
{
	class Filter ;
}

//| \class GSmtp::Filter
/// An interface for processing a message file through a filter.
/// The interface is asynchronous, using a slot/signal completion
/// callback.
///
/// Filters return a tri-state value (ok, abandon, fail) and a
/// 'special' flag which is interpreted as 're-scan' for server
/// filters and 'stop-scanning' for client filters.
///
/// The abandon state is treated more like success on the server
/// side but more like failure on the client side.
///
/// The fail state has an associated SMTP response string (eg.
/// "rejected"), an override for the SMTP response code, and a
/// more expansive reason string for logging.
///
class GSmtp::Filter
{
public:
	enum class Result // Filter tri-state result value.
	{
		ok = 0 ,
		abandon = 1 ,
		fail = 2
	} ;
	enum class Type // Filter type enum.
	{
		server ,
		client ,
		routing
	} ;
	struct Config /// Configuration passed to filter constructors.
	{
		unsigned int timeout {60U} ;
		std::string domain ; // postcondition: !domain.empty()
		Config & set_timeout( unsigned int ) noexcept ;
		Config & set_domain( const std::string & ) ;
	} ;

	virtual ~Filter() = default ;
		///< Destructor.

	virtual std::string id() const = 0 ;
		///< Returns the id passed to the derived-class constructor.
		///< Used in logging.

	virtual bool quiet() const = 0 ;
		///< Returns true if there is no need for logging.

	virtual void start( const GStore::MessageId & ) = 0 ;
		///< Starts the filter for the given message. Any previous,
		///< incomplete filtering is cancel()ed. Asynchronous completion
		///< is indicated by a doneSignal().

	virtual G::Slot::Signal<int> & doneSignal() noexcept = 0 ;
		///< Returns a signal which is raised once start() has completed
		///< or failed. The signal parameter is the integer value
		///< of result().

	virtual void cancel() = 0 ;
		///< Aborts any incomplete filtering.

	virtual Result result() const = 0 ;
		///< Returns the filter result, after the doneSignal() has been
		///< emitted.

	virtual std::string response() const = 0 ;
		///< Returns a non-empty SMTP response string iff the filter
		///< failed, or an empty response if successful or abandoned.

	virtual int responseCode() const = 0 ;
		///< An override for the SMTP response code for when the filter
		///< has failed. Many implementations should just return zero.

	virtual std::string reason() const = 0 ;
		///< Returns a non-empty reason string iff the filter failed,
		///< or an empty reason if successful or abandoned.

	virtual bool special() const = 0 ;
		///< Returns true if the filter indicated special handling is
		///< required.

	std::string str( Type type ) const ;
		///< Returns a diagnostic string for logging, including the
		///< filter result.

	static G::string_view strtype( Type type ) noexcept ;
		///< Returns a type string for logging: "filter",
		///< "client-filter" or "routing-filter".

protected:
	struct Exit /// Interprets an executable filter's exit code.
	{
		Exit( int exit_code , Type ) ;
		bool ok() const ;
		bool abandon() const ;
		bool fail() const ;
		Result result {Result::fail} ;
		bool special {false} ;
	} ;
} ;

inline GSmtp::Filter::Config & GSmtp::Filter::Config::set_timeout( unsigned int n ) noexcept { timeout = n ; return *this ; }
inline GSmtp::Filter::Config & GSmtp::Filter::Config::set_domain( const std::string & s ) { domain = s ; return *this ; }

#endif
