//
// Copyright (C) 2001-2004 Graeme Walker <graeme_walker@users.sourceforge.net>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later
// version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// ===
//
// gprocessor.cpp
//

#include "gdef.h"
#include "gsmtp.h"
#include "gprocess.h"
#include "gprocessor.h"
#include "gstr.h"
#include "groot.h"
#include "glog.h"

GSmtp::Processor::Processor( const G::Executable & exe ) :
	m_exe(exe) ,
	m_cancelled(false) ,
	m_repoll(false)
{
}

bool GSmtp::Processor::cancelled() const
{
	return m_cancelled ;
}

bool GSmtp::Processor::repoll() const
{
	return m_repoll ;
}

std::string GSmtp::Processor::text( const std::string & default_ ) const
{
	return m_text.empty() ? default_ : m_text ;
}

bool GSmtp::Processor::process( const G::Path & path )
{
	if( m_exe.exe() == G::Path() )
		return true ;

	int exit_code = preprocessCore( path ) ;

	bool is_ok = exit_code == 0 ;
	bool is_special = exit_code >= 100 && exit_code <= 107 ;
	bool is_failure = !is_ok && !is_special ;

	if( is_special && ((exit_code-100)&2) != 0 )
	{
		m_repoll = true ;
	}

	// ok, fail or cancel
	//
	if( is_special && ((exit_code-100)&1) == 0 )
	{
		m_cancelled = true ;
		G_LOG( "GSmtp::Processor: message processing cancelled by preprocessor" ) ;
		return false ;
	}
	else if( is_failure )
	{
		G_WARNING( "GSmtp::Processor::preprocess: pre-processing failed: exit code " << exit_code ) ;
		return false ;
	}
	else
	{
		return true ;
	}
}

int GSmtp::Processor::preprocessCore( const G::Path & path )
{
	G_LOG( "GSmtp::Processor::preprocess: executable \"" << m_exe.exe() << "\": content \"" << path << "\"" ) ;
	G::Strings args( m_exe.args() ) ;
	args.push_back( path.str() ) ;
	std::string raw_output ;
	int exit_code = G::Process::spawn( G::Root::nobody() , m_exe.exe() , args , &raw_output ) ;
	m_text = parseOutput( raw_output ) ;
	G_LOG( "GSmtp::Processor::preprocess: exit status " << exit_code << " (\"" << m_text << "\")" ) ;
	return exit_code ;
}

std::string GSmtp::Processor::parseOutput( std::string s ) const
{
	G_DEBUG( "GSmtp::Processor::parseOutput: in: \"" << G::Str::toPrintableAscii(s) << "\"" ) ;
	const std::string start("<<") ;
	const std::string end(">>") ;
	std::string result ;
	G::Str::replaceAll( s , "\r\n" , "\n" ) ;
	G::Str::replaceAll( s , "\r" , "\n" ) ;
	G::Strings lines ;
	G::Str::splitIntoFields( s , lines , "\n" ) ;
	for( G::Strings::iterator p = lines.begin() ; p != lines.end() ; ++p )
	{
		std::string line = *p ;
		size_t pos_start = line.find(start) ;
		size_t pos_end = line.find(end) ;
		if( pos_start == 0U && pos_end != std::string::npos )
		{
			result = G::Str::toPrintableAscii(line.substr(start.length(),pos_end-start.length())) ;
		}
	}
	G_DEBUG( "GSmtp::Processor::parseOutput: in: \"" << G::Str::toPrintableAscii(result) << "\"" ) ;
	return result ;
}

