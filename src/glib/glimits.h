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
/// \file glimits.h
///

#ifndef G_LIMITS_H__
#define G_LIMITS_H__

#include "gdef.h"

/// \namespace G
namespace G
{
	struct limits ;
}

/// \class G::limits
/// A scoping structure for a set of buffer sizes.
/// Intended to be used to reduce memory requirements in 
/// embedded environments.
///
class G::limits 
{
public:
	enum { path = 10000 } ; // cf. MAX_PATH, PATH_MAX, MAXPATHLEN
	enum { log = 1000 } ; // log line limit
	enum { file_buffer = 102400 } ; // cf. BUFSIZ
	enum { pipe_buffer = 4096 } ; // one-off read from a pipe
	enum { net_buffer = 1500 } ;
	enum { net_line_limit = 1000000 } ; // denial of service limit
	enum { net_hostname = 1024 } ;
	enum { win32_subclass_limit = 80 } ;
	enum { win32_classname_buffer = 256 } ;

private:
	limits() ; // not implemented
} ;

#endif
