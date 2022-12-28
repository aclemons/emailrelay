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
/// \file gcopyfilter.cpp
///

#include "gdef.h"
#include "gcopyfilter.h"
#include "genvelope.h"
#include "groot.h"
#include "gfile.h"
#include "gscope.h"
#include "gdirectory.h"

GFilters::CopyFilter::CopyFilter( GNet::ExceptionSink es , GStore::FileStore & store ,
	Filter::Type filter_type , const std::string & spec ) :
		m_store(store) ,
		m_filter_type(filter_type) ,
		m_spec(spec) ,
		m_timer(*this,&CopyFilter::onTimeout,es) ,
		m_result(Result::fail)
{
}

GFilters::CopyFilter::~CopyFilter()
= default ;

std::string GFilters::CopyFilter::id() const
{
	return "copy" ;
}

bool GFilters::CopyFilter::simple() const
{
	return false ;
}

void GFilters::CopyFilter::start( const GStore::MessageId & message_id )
{
	m_copies = 0U ;
	m_failures.clear() ;

	if( m_filter_type == Filter::Type::server )
	{
		G::Path envelope_path = m_store.envelopePath( message_id , GStore::FileStore::State::New ) ;
		G::Path content_path = m_store.contentPath( message_id ) ;
		G::Path spool_dir = content_path.dirname() ;

		// search the spool directory for sub-directories
		G::DirectoryList iter ;
		{
			G::Root claim_root ;
			iter.readDirectories( spool_dir ) ;
		}

		// copy the envelope into the sub-directories
		while( iter.more() )
		{
			m_copies++ ;
			G::Path subdir = iter.filePath() ;
			G::Path target = G::Path( subdir , envelope_path.withoutExtension().basename() ) ;
			bool ok = false ;
			{
				G::Root claim_root ;
				ok = G::File::copy( envelope_path , target , std::nothrow ) ;
				G_DEBUG( "GFilters::CopyFilter::start: [" << envelope_path << "] -> [" << target << "]: " << (ok?1:0) ) ;
			}
			if( !ok )
				m_failures.push_back( iter.fileName() ) ;
		}

		// success if copied to all (and at least one) sub-directories
		bool all_copied = m_copies > 0U && m_failures.empty() ;
		m_result = all_copied ? Result::abandon : Result::ok ; // Result::ok is moot
		if( !m_failures.empty() )
			G_ERROR( "GFilters::CopyFilter::start: copy filter failed to copy into "
				<< m_failures.size() << "/" << m_copies << " sub-directories" ) ;

		// remove the original envelope if a successful copy
		if( all_copied )
		{
			G_DEBUG( "GFilters::CopyFilter::start: success: deleting [" << envelope_path << "]" ) ;
			G::Root claim_root ;
			G::File::remove( envelope_path , std::nothrow ) ;
		}
	}
	else
	{
		G_WARNING( "GFilters::CopyFilter::start: invalid use of the copy filter" ) ;
	}
	m_timer.startTimer( 0U ) ;
}

void GFilters::CopyFilter::onTimeout()
{
	m_done_signal.emit( static_cast<int>(m_result) ) ;
}

G::Slot::Signal<int> & GFilters::CopyFilter::doneSignal()
{
	return m_done_signal ;
}

void GFilters::CopyFilter::cancel()
{
	m_timer.cancelTimer() ;
}

GSmtp::Filter::Result GFilters::CopyFilter::result() const
{
	return m_result ;
}

std::string GFilters::CopyFilter::response() const
{
	return {} ;
}

std::string GFilters::CopyFilter::reason() const
{
	return {} ;
}

bool GFilters::CopyFilter::special() const
{
	return false ; // (no special spool-directory scanning behaviour required)
}

