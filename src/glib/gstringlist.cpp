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
/// \file gstringlist.cpp
///

#include "gdef.h"
#include "gstringlist.h"
#include "gstr.h"
#include "gassert.h"
#include <algorithm>
#include <functional>
#include <string>

namespace G
{
	namespace StringListImp
	{
		bool inList( StringArray::const_iterator begin , StringArray::const_iterator end ,
			const std::string & s , bool i ) ;
		bool notInList( StringArray::const_iterator begin , StringArray::const_iterator end ,
			const std::string & s , bool i ) ;
		bool match( const std::string & a , const std::string & b , bool ignore_case ) ;
	}
}

bool G::StringListImp::inList( StringArray::const_iterator begin , StringArray::const_iterator end ,
	const std::string & s , bool ignore )
{
	using namespace std::placeholders ;
	return std::any_of( begin , end , std::bind(match,_1,std::cref(s),ignore) ) ;
}

bool G::StringListImp::notInList( StringArray::const_iterator begin , StringArray::const_iterator end ,
	const std::string & s , bool ignore_case )
{
	return !inList( begin , end , s , ignore_case ) ;
}

bool G::StringListImp::match( const std::string & a , const std::string & b , bool ignore_case )
{
	return ignore_case ? sv_imatch(std::string_view(a),std::string_view(b)) : (a==b) ;
}

void G::StringList::keepMatch( StringArray & list , const StringArray & match_list , Ignore ignore )
{
	using namespace std::placeholders ;
	if( !match_list.empty() )
		list.erase(
			std::remove_if( list.begin() , list.end() ,
				std::bind(StringListImp::notInList,match_list.begin(),match_list.end(),_1,ignore==Ignore::Case) ) ,
			list.end() ) ;
}

void G::StringList::removeMatch( StringArray & list , const StringArray & deny_list , Ignore ignore )
{
	using namespace std::placeholders ;
	list.erase(
		std::remove_if( list.begin() , list.end() ,
			std::bind(StringListImp::inList,deny_list.begin(),deny_list.end(),_1,ignore==Ignore::Case) ) ,
		list.end() ) ;
}

bool G::StringList::headMatch( const StringArray & in , std::string_view head )
{
	return std::any_of( in.begin() , in.end() ,
		[&head](const std::string &x){return Str::headMatch(x,head);} ) ;
}

#ifndef G_LIB_SMALL
bool G::StringList::tailMatch( const StringArray & in , std::string_view tail )
{
	return std::any_of( in.begin() , in.end() ,
		[&tail](const std::string &x){return Str::tailMatch(x,tail);} ) ;
}
#endif

std::string G::StringList::headMatchResidue( const StringArray & in , std::string_view head )
{
	const auto end = in.end() ;
	for( auto p = in.begin() ; p != end ; ++p )
	{
		if( Str::headMatch( *p , head ) )
			return (*p).substr( head.size() ) ;
	}
	return {} ;
}

bool G::StringList::match( const StringArray & a , const std::string & b )
{
	return std::find( a.begin() , a.end() , b ) != a.end() ;
}

bool G::StringList::imatch( const StringArray & a , const std::string & b )
{
	using namespace std::placeholders ;
	std::string_view b_sv( b ) ;
	return std::any_of( a.begin() , a.end() ,
		[b_sv](const std::string & a_str){ return Str::imatch(a_str,b_sv) ; } ) ;
}

