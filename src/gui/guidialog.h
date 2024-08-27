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
/// \file guidialog.h
///

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include "gdef.h"
#include "gqt.h"
#include "guipage.h"
#include "gpath.h"
#include <list>
#include <map>
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;

namespace Gui
{
	class Dialog ;
	class Page ;
}

//| \class Gui::Dialog
/// The main forward-back dialog box.
///
class Gui::Dialog : public QDialog
{Q_OBJECT
public:
	Dialog( const G::Path & virgin_flag_file , bool with_launch ) ;
		///< Constructor. Use a sequence of add()s to initialise
		///< ending with add(void).

	void add( Page * ) ;
		///< Adds a page.

	void add( Page * , const std::string & conditional_page_name ) ;
		///< Adds a page but only if the the page name matches
		///< the given conditional name or if the given
		///< conditional name is empty.

	void add() ;
		///< To be called after other add()s.

	Page & page( const std::string & ) ;
		///< Finds a page by name.

	const Page & page( const std::string & ) const ;
		///< Finds a page by name.

	std::string currentPageName() const ;
		///< Returns the current page name.

	Page & previousPage( unsigned int distance = 1U ) ;
		///< Returns the previous page.

	bool historyContains( const std::string & ) const ;
		///< Returns true if the history contains the given page.

	bool empty() const ;
		///< Returns true if there are no pages add()ed.

	void dumpStateVariables( std::ostream & ) const ;
		///< Dump the widget state from all the pages.

	void dumpInstallVariables( std::ostream & ) const ;
		///< Dump the install variables from all the pages.

	void setFinishing( bool ) ;
		///< Disables all buttons. See also Page::allFinished()
		///< and Page::nearlyAllFinished().

private slots:
	void backButtonClicked() ;
	void nextButtonClicked() ;
	void finishButtonClicked() ;
	void pageUpdated() ;

private:
	void dump( std::ostream & , bool for_install ) const ;
	void setFirstPage( Page & ) ;
	void switchPage( const std::string & new_page_name , const std::string & old_page_name = {} , bool back = false ) ;

private:
	using History = std::list<std::string> ;
	using Map = std::map<std::string,Page*> ;
	Map m_map ;
	History m_history ;
	bool m_first ;
	bool m_with_launch ;
	bool m_next_is_launch ;
	QPushButton * m_help_button ;
	QPushButton * m_cancel_button ;
	QPushButton * m_back_button ;
	QPushButton * m_next_button ;
	QPushButton * m_finish_button ;
	QHBoxLayout * m_button_layout ;
	QVBoxLayout * m_main_layout ;
	bool m_finishing ;
	bool m_back_state ;
	bool m_next_state ;
	bool m_finish_state ;
	G::Path m_virgin_flag_file ;
} ;

#endif
