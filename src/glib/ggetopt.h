//
// Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// ggetopt.h
//	

#ifndef G_GETOPT_H
#define G_GETOPT_H

#include "gdef.h"
#include "garg.h"
#include "gstrings.h"
#include "gexception.h"
#include <string>
#include <list>
#include <map>

namespace G
{
	class GetOpt ;
}

// Class: G::GetOpt
// Description: A command line switch parser.
// See also: G::Arg
//
class G::GetOpt 
{
public:
	struct Level // Used by G::GetOpt for extra type safety.
		{ unsigned int level ; explicit Level(unsigned int l) : level(l) {} } ;
	G_EXCEPTION( InvalidSpecification , "invalid options specification string" ) ;

	GetOpt( const Arg & arg , const std::string & spec , 
		char sep_major = '|' , char sep_minor = '/' , char escape = '\\' ) ;
			// Constructor taking a Arg reference and a 
			// specification string. Uses specifications like 
			// "p/port/defines the port number/1/port|v/verbose/shows more logging/0/".
			// made up of the following parts:
			//    <single-character-switch-letter>
			//    <multi-character-switch-name>
			//    <switch-description>
			//    <value-type> -- 0 is none, and 1 is a string
			//    <value-description>
			//    <level>
			//
			// If the switch-description field is empty or
			// if the level is zero then the switch is hidden.
			// By convention main-stream switches should have 
			// a level of 1, and obscure ones level 2 and above.

	Arg args() const ;
		// Returns all the non-switch command-line arguments.

	Strings errorList() const ;
		// Returns the list of errors.

	static size_t wrapDefault() ;
		// Returns a default word-wrapping width.

	static size_t tabDefault() ;
		// Returns a default tab-stop.

	static Level levelDefault() ;
		// Returns the default level.

	static std::string introducerDefault() ;
		// Returns "usage: ".

	std::string usageSummary( const std::string & exe , const std::string & args , 
		const std::string & introducer = introducerDefault() ,
		Level level = levelDefault() , size_t wrap_width = wrapDefault() ) const ;
			// Returns a one-line usage summary, as
			// "usage: <exe> <usageSummarySwitches()> <args>"

	std::string usageSummarySwitches( Level level = levelDefault() ) const ;
		// Returns the one-line summary of switches. Does _not_ 
		// include the usual "usage: <exe>" prefix
		// or non-switch arguments.

	std::string usageHelp( Level level = levelDefault() ,
		size_t tab_stop = tabDefault() , size_t wrap_width = wrapDefault() ,
		bool level_exact = false ) const ;
			// Returns a multi-line string giving help on each switch.

	void showUsage( std::ostream & stream , const std::string & exe , 
		const std::string & args , const std::string & introducer = introducerDefault() ,
		Level level = levelDefault() ,
		size_t tab_stop = tabDefault() , 
		size_t wrap_width = wrapDefault() ) const ;
			// Streams out multi-line usage text using
			// usageSummary() and usageHelp().

	void showUsage( std::ostream & stream , const std::string & args , bool verbose ) const ;
		// Streams out multi-line usage text using
		// usageSummary() and usageHelp(). Shows
		// only level one switches if 'verbose' 
		// is false.

	bool hasErrors() const ;
		// Returns true if there are errors.

	void showErrors( std::ostream & stream , std::string prefix_1 , 
		std::string prefix_2 = std::string(": ") ) const ;
			// A convenience function which streams out each errorList()
			// item to the given stream, prefixed with the given
			// prefix(es). The two prefixes are simply concatenated.

	void showErrors( std::ostream & stream ) const ;
		// An overload which uses Arg::prefix() as <prefix_1>.

	void show( std::ostream & stream , std::string prefix ) const ;
		// For debugging.

	bool contains( char switch_letter ) const ;
		// Returns true if the command line contains
		// the given switch.

	bool contains( const std::string & switch_name ) const ;
		// Returns true if the command line contains
		// the given switch.

	std::string value( const std::string & switch_name ) const ;
		// Returns the value related to the given
		// value-based switch.
		//
		// Precondition: contains(switch_name)

	std::string value( char switch_letter ) const ;
		// Returns the value related to the given
		// value-based switch.
		//
		// Precondition: contains(switch_letter)

private:
	struct SwitchSpec // A private implementation structure used by G::GetOpt.
	{ 
		char c ; 
		std::string name ; 
		std::string description ; 
		bool valued ; 
		bool hidden ;
		std::string value_description ;
		unsigned int level ;
		SwitchSpec(char c_,const std::string &name_,const std::string &description_,
			bool v_,const std::string &vd_,unsigned int level_) :
				c(c_) , name(name_) , description(description_) , 
				valued(v_) , hidden(description_.empty()||level_==0U) ,
				value_description(vd_) , level(level_) {}
	} ;
	typedef std::map<std::string,SwitchSpec> SwitchSpecMap ;
	typedef std::pair<bool,std::string> Value ;
	typedef std::map<char,Value> SwitchMap ;

	void operator=( const GetOpt & ) ;
	GetOpt( const GetOpt & ) ;
	void parseSpec( const std::string & spec , char , char , char ) ;
	void addSpec( const std::string & , char c , const std::string & , 
		const std::string & , bool , const std::string & , unsigned int ) ;
	size_t parseArgs( const Arg & args_in ) ;
	bool isOldSwitch( const std::string & arg ) const ;
	bool isNewSwitch( const std::string & arg ) const ;
	bool isSwitchSet( const std::string & arg ) const ;
	void processSwitch( char c ) ;
	void processSwitch( char c , const std::string & value ) ;
	void processSwitch( const std::string & s ) ;
	void processSwitch( const std::string & s , const std::string & value ) ;
	bool valued( char c ) const ;
	bool valued( const std::string & ) const ;
	void errorNoValue( char c ) ;
	void errorNoValue( const std::string & ) ;
	void errorUnknownSwitch( char c ) ;
	void errorUnknownSwitch( const std::string & ) ;
	char key( const std::string & s ) const ;
	void remove( size_t n ) ;
	bool valid( const std::string & ) const ;
	bool valid( char c ) const ;
	std::string usageSummaryPartOne( Level ) const ;
	std::string usageSummaryPartTwo( Level ) const ;
	std::string usageHelpCore( const std::string & , Level , size_t , size_t , bool ) const ;
	static size_t widthLimit( size_t ) ;
	static bool visible( SwitchSpecMap::const_iterator , Level , bool ) ;

private:
	SwitchSpecMap m_spec_map ;
	SwitchMap m_map ;
	Strings m_errors ;
	Arg m_args ;
} ;

#endif
