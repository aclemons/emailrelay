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
/// \file guipage.h
///

#ifndef GUI_PAGE_H
#define GUI_PAGE_H

#include "gdef.h"
#include "gqt.h"
#include "gstringarray.h"
#include "gpath.h"
#include "gexecutablecommand.h"
#include <string>

class QAbstractButton ;
class QLineEdit ;
class QComboBox ;

namespace Gui
{
	class Page ;
	class Dialog ;
}

//| \class Gui::Page
/// A page widget that can be installed in a Gui::Dialog.
///
class Gui::Page : public QWidget
{Q_OBJECT
protected:
	Page( Dialog & , const std::string & name ,
		const std::string & next_1 , const std::string & next_2 ) ;
			///< Constructor.

public:
	Dialog & dialog() ;
		///< Returns the dialog passed in to the ctor.

	const Dialog & dialog() const ;
		///< Returns the dialog passed in to the ctor.

	std::string name() const ;
		///< Returns the page name.

	virtual bool isReadyToFinishPage() const ;
		///< Returns true if the dialog is nearly complete so the
		///< 'next' button should be disabled on this page.

	virtual bool isFinishPage() const ;
		///< Returns true if this is the finishing page and no
		///< further page navigation should be allowed.

	virtual void onShow( bool back ) ;
		///< Called as this page becomes visible as a result
		///< of the previous page's 'next' button being clicked.
		///< This default implementation does nothing.

	virtual void onLaunch() ;
		///< Called when the launch button is clicked.

	virtual std::string nextPage() = 0 ;
		///< Returns the name of the next page.
		///< Returns the empty string if last.
		///< Overrides should select next1() or
		///< next2().

	virtual bool isComplete() ;
		///< Returns true if the page is complete and the 'next'
		///< button can be enabled. This default implementation
		///< returns true.

	virtual bool isFinishing() ;
		///< Returns true if isFinishPage() and still busy finishing.

	virtual bool canLaunch() ;
		///< Returns true if isFinishPage() and the launch button
		///< can be enabled.

	virtual void dump( std::ostream & , bool for_install ) const ;
		///< Dumps the page's state to the given stream.
		///< Overrides should start by calling this base-class
		///< implementation.

	virtual std::string helpUrl( const std::string & language ) const ;
		///< Overridable help url.

	static void setTestMode( int = 1 ) ;
		///< Sets a test-mode. Typically this causes widgets
		///< to be initialised in a way that helps with testing,
		///< such as avoiding unnecessary clicks and causing
		///< every page to be visited.

signals:
	void pageUpdateSignal() ;
		///< Emitted when the page's state changes.
		///< This allows the dialog box to update its
		///< buttons according to the page's new state.

private slots:
	void helpKeyTriggered() ;

protected:
	struct NameTip {} ;
	struct PasswordTip {} ;
	static QLabel * newTitle( QString ) ;
	static void tip( QWidget * , const char * ) ;
	static void tip( QWidget * , const QString & ) ;
	static void tip( QWidget * , NameTip ) ;
	static void tip( QWidget * , PasswordTip ) ;
	std::string next1() const ;
	std::string next2() const ;
	static std::string value( bool ) ;
	static std::string value( const QAbstractButton * ) ;
	static std::string value( const QLineEdit * ) ;
	static std::string value_utf8( const QLineEdit * ) ;
	static std::string value( const QComboBox * ) ;
	bool testMode() const ;
	int testModeValue() const ;
	void dumpItem( std::ostream & , bool , const std::string & , const std::string & ) const ;
	void dumpItem( std::ostream & , bool , const std::string & , const G::Path & value ) const ;
	static QString qstr( const std::string & ansi ) ;

private:
	static std::string stdstr( const QString & ) ;
	static std::string stdstr_utf8( const QString & ) ;
	void addHelpAction() ;

private:
	Dialog & m_dialog ;
	std::string m_name ;
	std::string m_next_1 ;
	std::string m_next_2 ;
	static int m_test_mode ;
} ;

#endif
