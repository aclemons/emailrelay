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
/// \file gnullprocessor.h
///

#ifndef G_SMTP_NULL_PROCESSOR_H
#define G_SMTP_NULL_PROCESSOR_H

#include "gdef.h"
#include "gsmtp.h"
#include "gexecutable.h"
#include "gprocessor.h"

/// \namespace GSmtp
namespace GSmtp
{
	class NullProcessor ;
}

/// \class GSmtp::NullProcessor
/// A Processor class that does nothing.
///
class GSmtp::NullProcessor : public GSmtp::Processor 
{
public:
	NullProcessor() ;
		///< Constructor.

	explicit NullProcessor( unsigned int exit_code ) ;
		///< Constructor for a processor that behaves like an
		///< executable that always exits with the given 
		///< exit code.

	virtual ~NullProcessor() ;
		///< Destructor.

	virtual G::Signal1<bool> & doneSignal() ;
		///< Final override from GSmtp::Processor.

	virtual void start( const std::string & path ) ;
		///< Final override from GSmtp::Processor.

	virtual void abort() ;
		///< Final override from GSmtp::Processor.

	virtual std::string text() const ;
		///< Final override from GSmtp::Processor.

	virtual bool cancelled() const ;
		///< Final override from GSmtp::Processor.

	virtual bool repoll() const ;
		///< Final override from GSmtp::Processor.

private:
	NullProcessor( const NullProcessor & ) ; // not implemented
	void operator=( const NullProcessor & ) ; // not implemented

private:
	G::Signal1<bool> m_done_signal ;
	bool m_cancelled ;
	bool m_repoll ;
	bool m_ok ;
} ;

#endif
