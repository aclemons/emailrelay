//
// Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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

#include <string>
namespace GQt
{
	struct Utf8 {} ;
	inline std::string stdstr( QString q )
	{
		QByteArray a = q.toLocal8Bit() ;
		return std::string( a.constData() , a.length() ) ;
	}
	inline std::string stdstr( QString q , Utf8 )
	{
		QByteArray a = q.toUtf8() ;
		return std::string( a.constData() , a.length() ) ;
	}
	inline QString qstr( const std::string & s )
	{
		return QString::fromLocal8Bit( s.data() , static_cast<int>(s.size()) ) ;
	}
	inline QString qstr( const std::string & s , Utf8 )
	{
		return QString::fromUtf8( s.data() , static_cast<int>(s.size()) ) ;
	}
}

#endif
