//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gexe.h"
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

	virtual ~NullProcessor() ;
		///< Destructor.

	virtual G::Signal1<bool> & doneSignal() ;
		///< From Processor.

	virtual void start( const std::string & path ) ;
		///< From Processor.

	virtual void abort() ;
		///< From Processor.

	virtual std::string text() const ;
		///< From Processor. 
		///<
		///< Returns any "<<text>>" output by the executable.

	virtual bool cancelled() const ;
		///< From Processor.

	virtual bool repoll() const ;
		///< From Processor.

private:
	NullProcessor( const NullProcessor & ) ; // not implemented
	void operator=( const NullProcessor & ) ; // not implemented

private:
	G::Signal1<bool> m_done_signal ;
} ;

#endif
