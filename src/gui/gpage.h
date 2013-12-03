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
///
/// \file gpage.h
///

#ifndef G_PAGE_H
#define G_PAGE_H

#include "gdef.h"
#include "qt.h"
#include "gstrings.h"
#include "gpath.h"
#include <string>

class GDialog ; 
class QAbstractButton ; 
class QLineEdit ; 
class QComboBox ; 

/// \class GPage
/// A page widget that can be installed in a GDialog.
///
class GPage : public QWidget 
{Q_OBJECT
protected:
	GPage( GDialog & , const std::string & name , const std::string & next_1 , 
		const std::string & next_2 , bool finish_button , bool close_button ) ;
			///< Constructor.

public:
	GDialog & dialog() ;
		///< Returns the dialog passed in to the ctor.

	const GDialog & dialog() const ;
		///< Returns the dialog passed in to the ctor.

	std::string name() const ;
		///< Returns the page name.

	bool useFinishButton() const ;
		///< Returns the ctor's finish_button parameter.

	virtual std::string helpName() const ;
		///< Returns this page's help-page name. Returns the 
		///< empty string if the help button should be disabled. 
		///< This default implementation returns the empty string.

	virtual bool closeButton() const ;
		///< Returns true if the page should have _only_ a close
		///< (and maybe help) button, typically if the last page.
		///<
		///< This default implementation returns the constructor
		///< parameter.

	virtual void onShow( bool back ) ;
		///< Called as this page becomes visible as a result
		///< of the previous page's 'next' button being clicked.

	virtual std::string nextPage() = 0 ;
		///< Returns the name of the next page.
		///< Returns the empty string if last.
		///< Overrides should select next1() or
		///< next2().

	virtual bool isComplete() ;
		///< Returns true if the page is complete
		///< and the 'next' button can be enabled.

	virtual void dump( std::ostream & , bool for_install ) const ;
		///< Dumps the page's state to the given
		///< stream. Overrides should start by
		///< calling this base-class implementation.

	static void setTestMode( int ) ;
		///< Sets a test-mode. Typically this causes widgets
		///< to be initialised in a way that helps with testing,
		///< such as avoiding unnecessary clicks and visiting
		///< every page.

signals:
	void pageUpdateSignal() ;
		///< Emitted when the page's state changes.
		///< This allows the dialog box to update its 
		///< buttons according to the page's new state.

private slots:
	void mechanismUpdateSlot( const QString & ) ;
		///< Emitted when an encryption mechanism combo box
		///< selection is changed.

protected:
	struct NameTip {} ;
	struct PasswordTip {} ;
	static QLabel * newTitle( QString ) ;
	static void tip( QWidget * , const char * ) ;
	static void tip( QWidget * , NameTip ) ;
	static void tip( QWidget * , PasswordTip ) ;
	static QString tip( const char * ) ;
	static QString tip() ;
	std::string next1() const ;
	std::string next2() const ;
	static std::string value( bool ) ;
	static std::string value( const QAbstractButton * ) ;
	static std::string value( const QLineEdit * ) ;
	static std::string value( const QComboBox * ) ;
	bool testMode() const ;
	int testModeValue() const ;
	void dumpItem( std::ostream & , bool , const std::string & , const std::string & ) const ;
	void dumpItem( std::ostream & , bool , const std::string & , const G::Path & value ) const ;
	static QString qstr( const std::string & ansi ) ;

private:
	static std::string stdstr( const QString & ) ;

private:
	GDialog & m_dialog ;
	std::string m_name ;
	std::string m_next_1 ;
	std::string m_next_2 ;
	bool m_finish_button ;
	bool m_close_button ;
	static int m_test_mode ;
} ;

#endif
