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
//
// news.cpp
//

#include "news.h"
#include "gdatetime.h"

std::string Main::News::text( const std::string & eol )
{

// sell-by 2011/1/1
const G::DateTime::EpochTime _2008 = 1199145600U ;
const G::DateTime::EpochTime year = 60U * 60U * 24U * 365U ;
if( G::DateTime::now() > (_2008+year+year+year) ) return std::string() ;

return std::string() +

"E-MailRelay (http://emailrelay.sf.net) has been developed " +
"with free tools on a free operating system. Free " +
"software is a matter of liberty, not price. To understand " +
"the concept, you should think of \"free\" as in \"free speech,\" " +
"not as in \"free beer\". Linux is a free, secure and easy-to-use " +
"alternative to Microsoft Windows, supporting a wide range of desktop " +
"and server applications. You can try Linux for yourself without " +
"installing it on your hard drive by downloading and burning a " +
"\"Live-CD\". Live-CDs are available from http://www.ubuntu.com " +
"and many other Linux distributors. Please refer to the " +
"Free Software Foundation website http://www.fsf.org for " +
"more information about software freedom." +
eol
;
}

/// \file news.cpp
