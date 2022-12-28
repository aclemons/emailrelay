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
/// \file gcopyfilter.h
///

#ifndef G_COPY_FILTER_H
#define G_COPY_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gfilestore.h"
#include "gexceptionsink.h"
#include "gstringarray.h"
#include "gtimer.h"

namespace GFilters
{
	class CopyFilter ;
}

//| \class GFilters::CopyFilter
/// A concrete GSmtp::Filter class that copies the envelope file into
/// all sub-directories of the spool directory and then deletes the
/// original. (This is useful for distributing to multiple POP clients.)
///
class GFilters::CopyFilter : public GSmtp::Filter
{
public:
	CopyFilter( GNet::ExceptionSink es , GStore::FileStore & ,
		Filter::Type , const std::string & spec ) ;
			///< Constructor.

	~CopyFilter() override ;
		///< Destructor.

private: // overrided
	std::string id() const override ;
	bool simple() const override ;
	void start( const GStore::MessageId & ) override ;
	G::Slot::Signal<int> & doneSignal() override ;
	void cancel() override ;
	Result result() const override ;
	std::string response() const override ;
	std::string reason() const override ;
	bool special() const override ;

private:
	void onTimeout() ;

private:
	GStore::FileStore & m_store ;
	Filter::Type m_filter_type ;
	std::string m_spec ;
	GNet::Timer<CopyFilter> m_timer ;
	G::Slot::Signal<int> m_done_signal ;
	std::size_t m_copies ;
	G::StringArray m_failures ;
	Result m_result ;
} ;

#endif
