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
//
// news.cpp
//

#include "news.h"
#include "gdatetime.h"

std::string Main::News::text( const std::string & eol )
{
return std::string() +
"Free software is a matter of liberty, not price. To understand " +
"the concept, you should think of \"free\" as in \"free speech,\" " +
"not as in \"free beer\". Windows is not free, even if you did not pay for it, " +
"because it limits your freedoms; check out the Free Software Foundation " +
"website http://www.fsf.org for more information about software freedom. " +
"E-MailRelay was developed on Linux, which is a free, secure and easy-to-use " +
"alternative to Microsoft Windows, supporting a wide range of desktop " +
"and server applications. You can try out Linux, without " +
"affecting your Windows system, by downloading it from " +
"http://www.ubuntu.com. Or try one of the many other Linux distributions " +
"available on the internet." +
eol
;
}

/// \file news.cpp
