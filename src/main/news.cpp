//
// Copyright (C) 2001-2005 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// news.cpp
//

#include "news.h"
#include "gdatetime.h"

std::string Main::News::text( const std::string & eol )
{

// sell-by 2008/1/1
if( G::DateTime::now() > 1199145600U ) return std::string() ;

return std::string() +

"E-MailRelay (http://emailrelay.sf.net) has been " +
"developed with free tools (GNU) on a free and " +
"open operating system (Linux). Linux is a secure and easy-to-use " +
"alternative to Microsoft Windows, supporting a wide range of desktop " +
"and server applications. You can try Linux for yourself without " +
"installing it on your hard drive by downloading and burning a " +
"\"Live-CD\". Free Live-CDs and DVDs are available from ubuntu.com, " +
"mandriva.com, suse.com, and many other Linux distributors." +
eol
;
}

