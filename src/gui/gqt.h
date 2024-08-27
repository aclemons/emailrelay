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
/// \file gqt.h
///

#ifndef G_MAIN_GUI_QT_H
#define G_MAIN_GUI_QT_H

#include "gdef.h"
#include "gpath.h"
#include <string>
#include <cstring>

#ifdef MemoryBarrier
#undef MemoryBarrier
#endif

#include <QtCore/QtCore>
#if QT_VERSION < 0x050000
#error Qt is too old
#endif
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtCore/QtPlugin>

namespace GQt
{
	inline std::string u8string_from_qstring( const QString & q )
	{
		QByteArray a = q.toUtf8() ;
		return std::string( a.constData() , a.length() ) ;
	}

	inline QString qstring_from_u8string( const std::string & s )
	{
		return QString::fromUtf8( s.data() , static_cast<int>(s.size()) ) ;
	}

	inline QString qstring_from_path( const G::Path & p )
	{
		#if defined(G_WINDOWS) && defined(G_ANSI)
			// (G_ANSI is deprecated)
			return QString::fromLocal8Bit( p.cstr() ) ;
		#else
			return QString::fromUtf8( p.cstr() ) ;
		#endif
	}

	inline G::Path path_from_qstring( const QString & q )
	{
		#if defined(G_WINDOWS) && defined(G_ANSI)
			// (G_ANSI is deprecated)
			QByteArray a = q.toLocal8Bit() ;
			const char * p = a.constData() ;
			std::size_t n = static_cast<std::size_t>(a.length()) ;
			return {std::string_view{p,n}} ;
		#else
			return {u8string_from_qstring(q)} ;
		#endif
	}
}

#endif
