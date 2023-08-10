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
/// \file glimits.h
///

#ifndef G_LIMITS_H
#define G_LIMITS_H

#include "gdef.h"

namespace G
{
	enum class Scale
	{
		Normal ,
		Small
	} ;
	#ifdef G_SMALL
	template <Scale N = Scale::Small> struct Limits ;
	#else
	template <Scale N = Scale::Normal> struct Limits ;
	#endif
	template <> struct Limits<Scale::Normal> ;
	template <> struct Limits<Scale::Small> ;
}

//| \class G::Limits
/// A set of compile-time buffer sizes. Intended to be used to
/// reduce memory requirements in embedded environments.
///
template <G::Scale N>
struct G::Limits
{
} ;

template <>
struct G::Limits<G::Scale::Normal> /// Normal specialisation of G::Limits.
{
	static constexpr bool small = false ;
	static constexpr int log = 1000 ; // log line limit
	static constexpr int path_buffer = 1024 ; // getcwd() first-attempt buffer size
	static constexpr int file_buffer = 8192 ; // read() buffer size for file copying (BUFSIZ)
	static constexpr int net_buffer = 20000 ; // read() buffer size for network reads (>=16k is best for TLS)
	static constexpr int net_listen_queue = 31 ; // listen(2) backlog parameter (cf. apache 511)
	static constexpr int net_file_limit = 200000000 ; // DoS limit reading a file from the network
	Limits() = delete ;
} ;

template <>
struct G::Limits<G::Scale::Small> /// Small-memory specialisation of G::Limits.
{
	static constexpr bool small = true ;
	static constexpr int log = 120 ;
	static constexpr int path_buffer = 64 ;
	static constexpr int file_buffer = 4096 ;
	static constexpr int net_buffer = 4096 ;
	static constexpr int net_listen_queue = 3 ;
	static constexpr int net_file_limit = 10000000 ;
	Limits() = delete ;
} ;

#endif
