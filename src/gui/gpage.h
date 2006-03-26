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
// gpage.h
//

#ifndef G_PAGE_H
#define G_PAGE_H

#include "qt.h"
#include "gstrings.h"
#include <string>

class GDialog ; 
class QAbstractButton ; 
class QLineEdit ; 
class QComboBox ; 

// Class: GPage
// Description: A page widget that can be installed in a GDialog.
//
class GPage : public QWidget 
{Q_OBJECT
protected:
	GPage( GDialog & , const std::string & name , const std::string & next_1 , 
		const std::string & next_2 , bool finish_button , bool close_button ) ;
			// Constructor.

public:
	GDialog & dialog() ;
		// Returns the dialog passed in to the ctor.

	const GDialog & dialog() const ;
		// Returns the dialog passed in to the ctor.

	std::string name() const ;
		// Returns the page name.

	bool useFinishButton() const ;
		// Returns the ctor's finish_button parameter.

	bool closeButton() const ;
		// Returns the ctor's close_button parameter.

	virtual void onShow( bool back ) ;
		// Called as this page becomes visible as a result
		// of the previous page's 'next' button being clicked.

	virtual std::string nextPage() = 0 ;
		// Returns the name of the next page.
		// Returns the empty string if last.
		// Overrides should select next1() or
		// next2().

	virtual bool isComplete() ;
		// Returns true if the page is complete
		// and the 'next' button can be enabled.

	virtual void dump( std::ostream & , const std::string & prefix , const std::string & eol ) const ;
		// Dumps the page's state to the given
		// stream. Overrides should start by
		// calling this base-class implementation.

	static void setTestMode() ;
		// Sets a test-mode.

	static void setTool( const std::string & , const std::string & = std::string() ) ;
		// Squirrels away the install-tool path.

	static std::string tool() ;
		// Returns the setTool() value.

	static G::Strings toolArgs( const std::string & prefix = std::string() ) ;
		// Returns the setTool() argument split into a container.

signals:
	void pageUpdateSignal() ;
		// Emitted when the page's state changes.
		// This allows the dialog box to update its 
		// buttons according to the page's new state.

protected:
	static QLabel * newTitle( QString ) ;
	std::string next1() const ;
	std::string next2() const ;
	static std::string value( const QAbstractButton * ) ;
	static std::string value( const QLineEdit * ) ;
	static std::string value( const QComboBox * ) ;
	bool testMode() const ;

private:
	GDialog & m_dialog ;
	std::string m_name ;
	std::string m_next_1 ;
	std::string m_next_2 ;
	bool m_finish_button ;
	bool m_close_button ;
	static bool m_test_mode ;
	static std::string m_tool ;
	static std::string m_tool_arg ;
} ;

#endif
