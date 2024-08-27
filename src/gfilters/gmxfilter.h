//
// Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
/// \file gmxfilter.h
///

#ifndef G_MX_FILTER_H
#define G_MX_FILTER_H

#include "gdef.h"
#include "gfilter.h"
#include "gmxlookup.h"
#include "gfilestore.h"
#include "gstoredfile.h"
#include "gstringview.h"
#include "gtimer.h"
#include "geventstate.h"
#include <utility>
#include <memory>

namespace GFilters
{
	class MxFilter ;
}

//| \class GFilters::MxFilter
/// A concrete GSmtp::Filter class for message routing: if the
/// message's 'forward-to' envelope field is set then the
/// 'forward-to-address' field is populated with the result of
/// a MX lookup. Does nothing if run as a client filter because
/// by then it will have already run as a routing filter.
///
class GFilters::MxFilter : public GSmtp::Filter
{
public:
	MxFilter( GNet::EventState es , GStore::FileStore & ,
		Filter::Type , const Filter::Config & , const std::string & spec ) ;
			///< Constructor.

	~MxFilter() override ;
		///< Destructor.

public:
	MxFilter( const MxFilter & ) = delete ;
	MxFilter & operator=( const MxFilter & ) = delete ;
	MxFilter( MxFilter && ) = delete ;
	MxFilter & operator=( MxFilter && ) = delete ;

private: // overrides
	std::string id() const override ; // GSmtp::Filter
	bool quiet() const override ; // GSmtp::Filter
	G::Slot::Signal<int> & doneSignal() noexcept override ; // GSmtp::Filter
	void start( const GStore::MessageId & ) override ; // GSmtp::Filter
	void cancel() override ; // GSmtp::Filter
	Result result() const override ; // GSmtp::Filter
	std::string response() const override ; // GSmtp::Filter
	int responseCode() const override ; // GSmtp::Filter
	std::string reason() const override ; // GSmtp::Filter
	bool special() const override ; // GSmtp::Filter

private:
	void onTimeout() ;
	void lookupDone( GStore::MessageId , std::string , std::string ) ;
	GStore::FileStore::State storestate() const ;
	std::string prefix() const ;
	static MxLookup::Config parseSpec( std::string_view , std::vector<GNet::Address> & ) ;
	struct ParserResult
	{
		std::string domain ;
		unsigned int port ;
		std::string address ;
	} ;
	static ParserResult parseForwardTo( const std::string & ) ;
	static std::string addressLiteral( std::string_view s , unsigned int port = 0U ) ;

private:
	using FileOp = GStore::FileStore::FileOp ;
	GNet::EventState m_es ;
	GStore::FileStore & m_store ;
	Filter::Type m_filter_type ;
	Filter::Config m_filter_config ;
	std::string m_spec ;
	MxLookup::Config m_mxlookup_config ;
	std::vector<GNet::Address> m_mxlookup_nameservers ;
	std::string m_id ;
	Result m_result {Result::fail} ;
	bool m_special {false} ;
	GNet::Timer<MxFilter> m_timer ;
	G::Slot::Signal<int> m_done_signal ;
	std::unique_ptr<MxLookup> m_lookup ;
} ;

#endif
