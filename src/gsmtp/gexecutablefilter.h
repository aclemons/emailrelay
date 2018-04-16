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
/// \file gexecutablefilter.h
///

#ifndef G_SMTP_EXECUTABLE_FILTER__H
#define G_SMTP_EXECUTABLE_FILTER__H

#include "gdef.h"
#include "gsmtp.h"
#include "gpath.h"
#include "gfilter.h"
#include "geventhandler.h"
#include "gfutureevent.h"
#include "gtask.h"

namespace GSmtp
{
	class ExecutableFilter ;
}

/// \class GSmtp::ExecutableFilter
/// A Filter class that runs an external helper program.
///
class GSmtp::ExecutableFilter : public Filter, private GNet::TaskCallback
{
public:
	ExecutableFilter( GNet::ExceptionHandler & , bool server_side , const std::string & ) ;
		///< Constructor.

	virtual ~ExecutableFilter() ;
		///< Destructor.

	virtual std::string id() const override ;
		///< Override from from GSmtp::Filter.

	virtual bool simple() const override ;
		///< Override from from GSmtp::Filter.

	virtual G::Slot::Signal1<bool> & doneSignal() override ;
		///< Override from from GSmtp::Filter.

	virtual void start( const std::string & path ) override ;
		///< Override from from GSmtp::Filter.

	virtual void cancel() override ;
		///< Override from from GSmtp::Filter.

	virtual std::string text() const override ;
		///< Override from from GSmtp::Filter.
		///<
		///< Returns any "<<text>>" or "[[text]]" output by the executable.

	virtual bool specialCancelled() const override ;
		///< Override from from GSmtp::Filter.

	virtual bool specialOther() const override ;
		///< Override from from GSmtp::Filter.

private:
	ExecutableFilter( const ExecutableFilter & ) ; // not implemented
	void operator=( const ExecutableFilter & ) ; // not implemented
	std::string parseOutput( std::string ) const ;
	virtual void onTaskDone( int , const std::string & ) override ; // override from GNet::TaskCallback

private:
	G::Slot::Signal1<bool> m_done_signal ;
	bool m_server_side ;
	G::Path m_path ;
	std::string m_text ;
	bool m_ok ;
	bool m_special_cancelled ;
	bool m_special_other ;
	GNet::Task m_task ;
} ;

#endif
