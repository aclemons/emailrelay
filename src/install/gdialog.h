//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gdialog.h
//

#ifndef G_DIALOG_H
#define G_DIALOG_H

#include "gpage.h"
#include <QDialog>
#include <list>
#include <map>
class QHBoxLayout; 
class QPushButton; 
class QVBoxLayout; 
class GPage ; 

// Class: GDialog
// Description: The main forward-back dialog box.
//
class GDialog : public QDialog 
{Q_OBJECT
public:
	
	explicit GDialog( QWidget * parent = NULL ) ;
		// Constructor.

	void add( GPage * ) ;
		// Adds a page.

	GPage & page( const std::string & ) ;
		// Finds a page by name.

	std::string currentPageName() const ;
		// Returns the current page name.

	GPage & previousPage( unsigned int distance = 1U ) ;
		// Returns the previous page.

	bool historyContains( const std::string & ) const ;
		// Returns true if the history contains the given page.

private slots:
	void backButtonClicked();
	void nextButtonClicked();
	void pageUpdated();

private:
	void setFirstPage( GPage & page ) ;
	void switchPage( std::string new_page_name , std::string old_page_name = std::string() ) ;

private:
	typedef std::list<std::string> History ;
	typedef std::map<std::string,GPage*> Map ;
	Map m_map ;
	History m_history;
	bool m_first ;
	QPushButton * m_cancel_button;
	QPushButton * m_back_button;
	QPushButton * m_next_button;
	QPushButton * m_finish_button;
	QHBoxLayout * m_button_layout;
	QVBoxLayout * m_main_layout;
};

#endif
