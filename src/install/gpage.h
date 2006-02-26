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

#include <QWidget>
#include <QLabel>
#include <string>

class GDialog ; 

// Class: GPage
// Description: A page widget that can be installed in a GDialog.
//
class GPage : public QWidget 
{Q_OBJECT
public:
	GPage( GDialog & , const std::string & name , 
		const std::string & next_1 = std::string() , 
		const std::string & next_2 = std::string() ) ;
			// Constructor.

	GDialog & dialog() ;
		// Returns the dialog passed in to the ctor.

	const GDialog & dialog() const ;
		// Returns the dialog passed in to the ctor.

	std::string name() const ;
		// Returns the page name.

	virtual void reset() ;
		// Resets the page contents.

	virtual std::string nextPage() = 0 ;
		// Returns the name of the next page.
		// Returns the empty string if last.
		// Overrides should select next1() or
		// next2().

	virtual bool isComplete();
		// Returns true if the page is complete
		// and the 'next' button can be enabled.

signals:
	void onUpdate() ;
		// Emitted when the page's state changes.
		// This allows the dialog box to update its 
		// buttons according to the page's new state.

protected:
	static QLabel * newTitle( QString ) ;
	std::string next1() const ;
	std::string next2() const ;

private:
	GDialog & m_dialog ;
	std::string m_name ;
	std::string m_next_1 ;
	std::string m_next_2 ;
} ;

#endif
