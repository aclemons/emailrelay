//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#ifndef G_LIMITS_H
#define G_LIMITS_H

#include "gdef.h"

namespace G
{
	class limits ;
}

/// \class G::limits
/// A scoping structure for a set of buffer sizes. Intended to be used to
/// reduce memory requirements in embedded environments.
///
class G::limits
{
public:

 #ifndef G_SMALL
	enum { path = 33000 } ; // cf. MAX_PATH, PATH_MAX, MAXPATHLEN
	enum { log = 1000 } ; // log line limit
	enum { file_buffer = 102400 } ; // cf. BUFSIZ
	enum { file_slurp = 100000000 } ;  // read file into contiguous memory
	enum { pipe_buffer = 4096 } ; // one-off read from a pipe
	enum { get_pwnam_r_buffer = 1024 } ; // if no sysconf() value - more than line length in /etc/passwd
	enum { net_buffer = 20000 } ; // best if bigger than the TLS maximum block size of 16k
	enum { net_line_limit = 1000000 } ; // d.o.s. network read line limit
	enum { net_file_limit = 200000000 } ; // d.o.s. network read file limit
	enum { net_hostname = 1024 } ;
	enum { net_listen_queue = 3 } ;
	enum { net_certificate_cache_size = 50 } ;
	enum { win32_subclass_limit = 80 } ;
	enum { win32_classname_buffer = 256 } ;
 #else
	enum { path = 256 } ;
	enum { log = 120 } ;
	enum { file_buffer = 128 } ;
	enum { file_slurp = 10000000 } ;
	enum { pipe_buffer = 128 } ;
	enum { get_pwnam_r_buffer = 200 } ;
	enum { net_buffer = 512 } ;
	enum { net_line_limit = 2000 } ;
	enum { net_file_limit = 10000000 } ;
	enum { net_hostname = 128 } ;
	enum { net_listen_queue = 3 } ;
	enum { net_certificate_cache_size = 2 } ;
	enum { win32_subclass_limit = 2 } ;
	enum { win32_classname_buffer = 128 } ;
 #endif

private:
	limits() ; // not implemented
} ;

#endif
