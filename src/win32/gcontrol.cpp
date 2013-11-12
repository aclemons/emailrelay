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
//
// gcontrol.cpp
//

#include "gdef.h"
#include "gdialog.h"
#include "gcontrol.h"
#include "glog.h"
#include "gassert.h"
#include "gdebug.h"
#include "gdc.h"

LRESULT CALLBACK gcontrol_wndproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;

GGui::Control::Control( Dialog &dialog , int id ) :
	m_valid(true) ,
	m_dialog(dialog) ,
	m_id(id) ,
	m_hwnd(0) ,
	m_no_redraw_count(0)
{
	G_ASSERT( m_dialog.isValid() ) ;
}

void GGui::Control::invalidate()
{
	m_valid = false ;
}

bool GGui::Control::valid() const
{
	return m_valid ;
}

GGui::Dialog & GGui::Control::dialog() const
{
	G_ASSERT( m_valid ) ;
	return m_dialog ;
}

int GGui::Control::id() const
{
	return m_id ;
}

LRESULT GGui::Control::sendMessage( unsigned int message , WPARAM wparam , LPARAM lparam ) const
{
	return m_dialog.sendMessage( m_id , message , wparam , lparam ) ;
}

HWND GGui::Control::handle() const
{
	if( m_hwnd == 0 )
	{
		G_ASSERT( m_dialog.isValid() ) ;
		const_cast<Control*>(this)->m_hwnd = ::GetDlgItem( m_dialog.handle() , m_id ) ;
		G_DEBUG( "GGui::Control::handle: GetDlgItem(" << m_id << ") -> " << m_hwnd ) ;
		G_ASSERT( m_hwnd != 0 ) ;
	}
	return m_hwnd ;
}

GGui::Control::~Control()
{
	G_ASSERT( m_no_redraw_count == 0 ) ;
}

bool GGui::Control::subClass()
{
	G_ASSERT( handle() != 0 ) ;

	SubClassMap::Proc old = reinterpret_cast<SubClassMap::Proc>( ::GetWindowLongPtr( handle() , GWLP_WNDPROC ) ) ;

	m_dialog.map().add( handle() , old , static_cast<void*>(this) ) ;
	::SetWindowLongPtr( handle() , GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(gcontrol_wndproc_export) ) ;
	return true ;
}

LRESULT GGui::Control::wndProc( unsigned int message , WPARAM wparam , LPARAM lparam , WNDPROC super_class )
{
	bool forward = true ;
	LRESULT result = onMessage( message , wparam , lparam , super_class , forward ) ;

	if( forward )
		return (*super_class)( handle() , message , wparam , lparam ) ;
	else
		return result ;
}

LRESULT GGui::Control::onMessage( unsigned int , WPARAM , LPARAM , WNDPROC , bool &forward )
{
	forward = true ;
	return 0 ;
}

// ===

LRESULT CALLBACK gcontrol_wndproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	// get the dialog box window handle
	HWND hwnd_dialog = ::GetParent(hwnd) ;
	if( hwnd_dialog == NULL )
	{
		G_ASSERT( false ) ;
		return ::DefWindowProc( hwnd , message , wparam , lparam ) ;
	}

	// find the dialog box object
	GGui::Dialog *dialog = reinterpret_cast<GGui::Dialog*>( ::GetWindowLongPtr( hwnd_dialog , DWLP_USER ) ) ;
	if( dialog == NULL )
	{
		G_ASSERT( false ) ;
		return ::DefWindowProc( hwnd , message , wparam , lparam ) ;
	}
	G_ASSERT( dialog->isValid() ) ;

	// find the control object and the super-class window procedure
	void * context = NULL ;
	GGui::SubClassMap::Proc super_class = reinterpret_cast<GGui::SubClassMap::Proc>(dialog->map().find(hwnd,&context)) ;
	GGui::Control * control = static_cast<GGui::Control*>(context) ;
	G_ASSERT( control != NULL ) ;
	G_ASSERT( control->handle() == hwnd ) ;
	G_ASSERT( control->id() == ::GetDlgCtrlID(hwnd) ) ;

	// remove the control from the map if it is being destroyed
	if( message == WM_NCDESTROY )
		dialog->map().remove( hwnd ) ;

	return control->wndProc( message , wparam , lparam , super_class ) ;
}

// ===

GGui::Control::NoRedraw::NoRedraw( Control & control ) :
	m_control(control)
{
	m_control.m_no_redraw_count++ ;
	if( m_control.m_no_redraw_count == 1 )
		m_control.sendMessage( WM_SETREDRAW , false ) ;
}

GGui::Control::NoRedraw::~NoRedraw()
{
	m_control.m_no_redraw_count-- ;
	if( m_control.m_no_redraw_count == 0 )
		m_control.sendMessage( WM_SETREDRAW , true ) ;
}

// ===

GGui::ListBox::ListBox( Dialog &dialog , int id ) :
	Control(dialog,id)
{
}

GGui::ListBox::~ListBox()
{
}

void GGui::ListBox::set( const G::Strings &list )
{
	if( list.size() == 0U )
	{
		sendMessage( LB_RESETCONTENT , 0 , 0 ) ;
		return ;
	}

	// disable redrawing
	NoRedraw no_redraw( *this ) ;

	// clear
	sendMessage( LB_RESETCONTENT , 0 , 0 ) ;

	// add
	for( G::Strings::const_iterator string_p = list.begin() ; 
		string_p != list.end() ; ++string_p )
	{
		sendMessage( LB_ADDSTRING , 0 , 
			reinterpret_cast<LPARAM>((*string_p).c_str()) ) ;
	}
}

int GGui::ListBox::getSelection()
{
	LRESULT rc = sendMessage( LB_GETCURSEL ) ;
	return rc == LB_ERR ? -1 : static_cast<int>(rc) ;
}

void GGui::ListBox::setSelection( int index )
{
	sendMessage( LB_SETCURSEL , static_cast<WPARAM>(index) ) ;
}

std::string GGui::ListBox::getItem( int index ) const
{
	G_ASSERT( index >= 0 ) ;

	LRESULT rc = sendMessage( LB_GETTEXTLEN , static_cast<WPARAM>(index) ) ;
	if( rc == LB_ERR || rc > 0xfff0 )
		return std::string() ;

	char *buffer = new char[rc+2] ;
	G_ASSERT( buffer != NULL ) ;
	if( buffer == NULL )
		return std::string() ;

	buffer[0] = '\0' ;
	sendMessage( LB_GETTEXT , static_cast<WPARAM>(index) , reinterpret_cast<LPARAM>(static_cast<LPCSTR>(buffer)) ) ;
	std::string s( buffer ) ;
	delete [] buffer ;
	return s ;
}

unsigned int GGui::ListBox::entries() const
{
	LRESULT entries = sendMessage( LB_GETCOUNT , 0 , 0 ) ;
	if( entries == LB_ERR )
	{
		G_DEBUG( "GGui::ListBox::entries: listbox getcount error" ) ;
		entries = 0 ;
	}
	G_ASSERT( entries == static_cast<LRESULT>(static_cast<unsigned int>(entries)) ) ;
	return static_cast<unsigned int>(entries) ;
}

// ===

GGui::EditBox::EditBox( Dialog &dialog , int id ) :
	Control( dialog , id ) ,
	m_character_height(0)
{
}

GGui::EditBox::~EditBox()
{
}

void GGui::EditBox::set( const std::string & text )
{
	NoRedraw no_redraw( *this ) ;
	::SetWindowTextA( handle() , text.c_str() ) ;
}

void GGui::EditBox::set( const G::Strings & list )
{
	if( list.size() == 0U )
	{
		::SetWindowTextA( handle() , "" ) ;
	}
	else
	{
		NoRedraw no_redraw( *this ) ;

		std::string total ;
		const char *sep = "" ;
		for( G::Strings::const_iterator iter = list.begin() ; iter != list.end() ; ++iter )
		{
			total.append( sep ) ;
			sep = "\x0D\x0A" ;
			total.append( *iter ) ;
		}

		::SetWindowTextA( handle() , total.c_str() ) ;
		G_ASSERT( lines() >= list.size() ) ;
	}
}

unsigned int GGui::EditBox::lines()
{
	// handle an empty control since em_getlinecount returns one
	int length = ::GetWindowTextLength( handle() ) ;
	if( length == 0 )
		return 0 ;

	LRESULT lines = sendMessage( EM_GETLINECOUNT ) ;
	G_DEBUG( "GGui::EditBox::lines: " << lines ) ;
	G_ASSERT( lines == static_cast<LRESULT>(static_cast<unsigned int>(lines)) ) ;
	return static_cast<unsigned int>(lines) ;
}

unsigned int GGui::EditBox::linesInWindow()
{
	unsigned int text_height = characterHeight() ;
	unsigned int window_height = windowHeight() ;
	G_ASSERT( text_height != 0 ) ;
	unsigned int result = window_height / text_height ;
	G_DEBUG( "GGui::EditBox::linesInWindow: " << result ) ;
	return result ;
}

void GGui::EditBox::scrollBack( int lines )
{
	if( lines <= 0 ) 
		return ;

	LONG dy = -lines ;
	sendMessage( EM_LINESCROLL , 0 , dy ) ;
}

void GGui::EditBox::scrollToEnd()
{
	// (add ten just to make sure)
	sendMessage( EM_LINESCROLL , 0 , lines() + 10 ) ;
}

std::string GGui::EditBox::get() const
{
	int length = ::GetWindowTextLength( handle() ) ;
	char *buffer = new char[length+2] ;
	G_ASSERT( buffer != NULL ) ;
	::GetWindowTextA( handle() , buffer , length+1 ) ;
	buffer[length+1] = '\0' ;
	std::string s( buffer ) ;
	delete [] buffer ;
	return s ;
}

unsigned int GGui::EditBox::scrollPosition()
{
	LRESULT position = sendMessage( EM_GETFIRSTVISIBLELINE ) ;
	G_DEBUG( "GGui::EditBox::scrollPosition: " << position ) ;
	G_ASSERT( position == static_cast<LRESULT>(static_cast<unsigned int>(position)) ) ;
	return static_cast<unsigned int>(position) ;
}

unsigned int GGui::EditBox::scrollRange()
{
	unsigned int range = lines() ; 
	if( range <= 1 )
		range = 1 ;
	else
		range-- ;

	//range += linesInWindow() ;
	G_DEBUG( "GGui::EditBox::scrollRange: " << range ) ;
	return range ;
}

unsigned int GGui::EditBox::characterHeight() 
{
	if( m_character_height == 0 )
	{
		DeviceContext dc( handle() ) ;
		TEXTMETRIC tm ;
		::GetTextMetrics( dc() , &tm ) ;
		m_character_height = static_cast<unsigned int>( tm.tmHeight + tm.tmExternalLeading ) ;
		G_ASSERT( m_character_height != 0 ) ;
	}
	return m_character_height ;
}

unsigned int GGui::EditBox::windowHeight()
{
	RECT rect ;
	::GetWindowRect( handle() , &rect ) ;
	G_ASSERT( rect.bottom >= rect.top ) ;
	return static_cast<unsigned int>( rect.bottom - rect.top ) ;
}

// ===

GGui::CheckBox::CheckBox( Dialog &dialog , int id ) :
	Control(dialog,id)
{
}

GGui::CheckBox::~CheckBox()
{
}

bool GGui::CheckBox::get() const
{
	return !! ::IsDlgButtonChecked( dialog().handle() , id() ) ;
}

void GGui::CheckBox::set( bool b )
{
	::CheckDlgButton( dialog().handle() , id() , b ) ;
}

// ===

GGui::Button::Button( Dialog &dialog , int id ) :
	Control(dialog,id)
{
}

GGui::Button::~Button()
{
}

bool GGui::Button::enabled() const
{
	return !!::IsWindowEnabled( handle() ) ;
}

void GGui::Button::enable( bool b )
{
	::EnableWindow( handle() , !!b ) ;
}

void GGui::Button::disable()
{
	::EnableWindow( handle() , false ) ;
}

/// \file gcontrol.cpp
