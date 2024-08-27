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
/// \file news.h
///

#ifndef G_MAIN_NEWS_H
#define G_MAIN_NEWS_H

#include "gdef.h"
#include <string>

namespace Main
{
	class News ;
}

//| \class Main::News
/// A static class providing some news text.
///
class Main::News
{
public:
	static std::string text( const std::string & eol ) ;
		///< Returns some 'news' text.

public:
	News() = delete ;
} ;

#endif
