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
/// \file gstringlist.h
///

#ifndef G_STRING_LIST_H
#define G_STRING_LIST_H

#include "gdef.h"
#include "gstr.h"
#include "gstringarray.h"
#include "gstringview.h"
#include "goptional.h"
#include <string>

namespace G
{
	namespace StringList /// Utility functions for lists of strings.
	{
		enum class Ignore
		{
			Case ,
			Nothing
		} ;

		void keepMatch( StringArray & list , const StringArray & allow_list ,
			Ignore = Ignore::Nothing ) ;
				///< Removes items in the list that do not match any entry
				///< in the allow list. Optionally uses a case-insensitive
				///< match.

		void applyMatch( StringArray & list , const StringArray & allow_list ,
			Ignore = Ignore::Nothing ) ;
				///< Removes items in the list that do not match any entry
				///< in the allow list and reorders the result to be the same
				///< as the allow list. Optionally uses a case-insensitive
				///< match.

		void removeMatch( StringArray & list , const StringArray & deny_list ,
			Ignore = Ignore::Nothing ) ;
				///< Removes items in the list that match an entry
				///< in the deny list. Optionally uses a case-insensitive
				///< match.

		bool headMatch( const StringArray & list , string_view head ) ;
			///< Returns true if any string in the array has the given start
			///< (or 'head' is empty).

		bool tailMatch( const StringArray & list , string_view ending ) ;
			///< Returns true if any string in the array has the given ending
			///< (or the given ending is empty).

		std::string headMatchResidue( const StringArray & list , string_view head ) ;
			///< Returns the unmatched part of the first string in the array that has
			///< the given start. Returns the empty string if nothing matches or if
			///< the first match is an exact match for the whole string.

		bool match( const StringArray & , const std::string & ) ;
			///< Returns true if any string in the array matches the given string.

		bool imatch( const StringArray & , const std::string & ) ;
			///< Returns true if any string in the array matches the given string,
			///< ignoring case.

		struct Filter /// Filters a list of strings with allow and deny lists.
		{
			Filter( StringArray & list , Ignore ignore = Ignore::Case ) :
				m_list(list)  ,
				m_ignore(ignore)
			{
			}
			Filter & allow( const optional<std::string> & a )
			{
				auto a_list = Str::splitIntoTokens( a.value_or({}) , "," ) ;
				if( a.has_value() && a_list.empty() )
					m_list.clear() ;
				else
					StringList::keepMatch( m_list , a_list , m_ignore ) ;
				return *this ;
			}
			Filter & deny( const std::string & d )
			{
				auto d_list = Str::splitIntoTokens( d , "," ) ;
				StringList::removeMatch( m_list , d_list , m_ignore ) ;
				return *this ;
			}
			StringArray & m_list ;
			Ignore m_ignore ;
		} ;
	}
}

#endif
