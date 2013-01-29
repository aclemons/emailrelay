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
/// \file gdialog.h
///

#ifndef G_DIALOG_H
#define G_DIALOG_H

#include "gdef.h"
#include "qt.h"
#include "gpage.h"
#include <list>
#include <map>
class QHBoxLayout; 
class QPushButton; 
class QVBoxLayout; 
class GPage ; 

/// \class GDialog
/// The main forward-back dialog box.
///
class GDialog : public QDialog 
{Q_OBJECT
public:
	
	explicit GDialog( bool with_help ) ;
		///< Constructor. Use a sequence of add()s to initialise
		///< ending with add(void).

	explicit GDialog( QWidget * parent = NULL ) ;
		///< Constructor. Use a sequence of add()s to initialise
		///< ending with add(void).

	void add( GPage * ) ;
		///< Adds a page.

	void add( GPage * , const std::string & conditional_page_name ) ;
		///< Adds a page but only if the the page name
		///< matches the given conditional name or if the given
		///< conditional name is empty.

	void add() ;
		///< To be called after other add()s.

	GPage & page( const std::string & ) ;
		///< Finds a page by name.

	const GPage & page( const std::string & ) const ;
		///< Finds a page by name.

	std::string currentPageName() const ;
		///< Returns the current page name.

	GPage & previousPage( unsigned int distance = 1U ) ;
		///< Returns the previous page.

	bool historyContains( const std::string & ) const ;
		///< Returns true if the history contains the given page.

	bool empty() const ;
		///< Returns true if there are no pages add()ed.

	void dumpStateVariables( std::ostream & ) const ;
		///< Dump the widget state from all the pages.

	void dumpInstallVariables( std::ostream & ) const ;
		///< Dump the install variables from all the pages.

	void wait( bool ) ;
		///< Disables all buttons.

private slots:
	void helpButtonClicked() ;
	void backButtonClicked() ;
	void nextButtonClicked() ;
	void finishButtonClicked() ;
	void pageUpdated() ;

private:
	void init( bool ) ;
	void dump( std::ostream & , bool for_install ) const ;
	void setFirstPage( GPage & page ) ;
	void switchPage( std::string new_page_name , std::string old_page_name = std::string() , bool back = false ) ;

private:
	typedef std::list<std::string> History ;
	typedef std::map<std::string,GPage*> Map ;
	Map m_map ;
	History m_history ;
	bool m_first ;
	QPushButton * m_help_button ;
	QPushButton * m_cancel_button ;
	QPushButton * m_back_button ;
	QPushButton * m_next_button ;
	QPushButton * m_finish_button ;
	QHBoxLayout * m_button_layout ;
	QVBoxLayout * m_main_layout ;
	bool m_waiting ;
	bool m_back_state ;
	bool m_next_state ;
	bool m_finish_state ;
};

#endif
