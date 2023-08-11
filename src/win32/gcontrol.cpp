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
/// \file gcontrol.cpp
///

#include "gdef.h"
#include "gdialog.h"
#include "gcontrol.h"
#include "gconvert.h"
#include "glog.h"
#include "gassert.h"
#include "gdc.h"
#include <vector>
#include <commctrl.h>
#include <prsht.h> // PropertySheet

LRESULT CALLBACK gcontrol_wndproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;

GGui::Control::Control( const Dialog & dialog , int id ) :
	m_dialog(&dialog) ,
	m_hdialog(HNULL) ,
	m_valid(true) ,
	m_id(id) ,
	m_hwnd(HNULL) ,
	m_no_redraw_count(0)
{
}

GGui::Control::Control( HWND hdialog , int id , HWND hcontrol ) :
	m_dialog(nullptr) ,
	m_hdialog(hdialog) ,
	m_valid(true) ,
	m_id(id) ,
	m_hwnd(hcontrol) ,
	m_no_redraw_count(0)
{
	G_ASSERT( m_hdialog != HNULL ) ;
}

void GGui::Control::load()
{
	InitCommonControls() ;
}

void GGui::Control::load( DWORD types )
{
 #if GCONFIG_HAVE_WINDOWS_INIT_COMMON_CONTROLS_EX
	INITCOMMONCONTROLSEX controls{} ;
	controls.dwSize = sizeof(controls) ;
	controls.dwICC = types ; // ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES ;
	if( ! InitCommonControlsEx( &controls ) )
	{
		G_ERROR( "GGui::Control::load: InitCommonControlsEx() failed: " << types ) ;
		//throw std::runtime_error( "InitCommonControlsEx() failed" ) ;
	}
 #else
	InitCommonControls() ;
 #endif
}

HWND GGui::Control::hdialog() const
{
	G_ASSERT( m_hdialog != HNULL || m_dialog != nullptr ) ;
	return m_hdialog ? m_hdialog : m_dialog->handle() ; // WindowBase::handle()
}

void GGui::Control::invalidate()
{
	m_valid = false ;
}

bool GGui::Control::valid() const
{
	return m_valid ;
}

int GGui::Control::id() const
{
	return m_id ;
}

LRESULT GGui::Control::sendMessage( unsigned int message , WPARAM wparam , LPARAM lparam ) const
{
	return SendMessage( handle() , message , wparam , lparam ) ;
}

HWND GGui::Control::handle() const
{
	if( m_hwnd == 0 )
	{
		HWND hdialog_ = hdialog() ;
		const_cast<Control*>(this)->m_hwnd = GetDlgItem( hdialog_ , m_id ) ;
		G_DEBUG( "GGui::Control::handle: GetDlgItem(" << m_id << ") -> " << m_hwnd ) ;
		if( m_hwnd == HNULL )
		{
			std::ostringstream ss ;
			ss << "dialog box error: no window for control id " << m_id << " in " << hdialog_ ;
			G_ERROR( "GGui::Control::handle: " << ss.str() ) ;
			throw std::runtime_error( ss.str() ) ;
		}
	}
	return m_hwnd ;
}

GGui::Control::~Control()
{
	G_ASSERT( m_no_redraw_count == 0 ) ;
}

void GGui::Control::subClass( SubClassMap & map )
{
	G_ASSERT( handle() != 0 ) ;
	SubClassMap::Proc old = reinterpret_cast<SubClassMap::Proc>( GetWindowLongPtr( handle() , GWLP_WNDPROC ) ) ;
	map.add( handle() , old , static_cast<void*>(this) ) ;
	SetWindowLongPtr( handle() , GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(gcontrol_wndproc_export) ) ;
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

// ==

LRESULT CALLBACK gcontrol_wndproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	try
	{
		// get the dialog box window handle
		HWND hwnd_dialog = GetParent( hwnd ) ;
		if( hwnd_dialog == HNULL )
		{
			G_ASSERT( false ) ;
			return DefWindowProc( hwnd , message , wparam , lparam ) ;
		}

		// find the dialog box object
		GGui::Dialog * dialog = reinterpret_cast<GGui::Dialog*>( GetWindowLongPtr( hwnd_dialog , DWLP_USER ) ) ;
		if( dialog == nullptr )
		{
			G_ASSERT( false ) ;
			return DefWindowProc( hwnd , message , wparam , lparam ) ;
		}
		G_ASSERT( dialog->isValid() ) ;

		// find the control object and the super-class window procedure
		void * context = nullptr ;
		auto super_class = reinterpret_cast<GGui::SubClassMap::Proc>( dialog->map().find(hwnd,&context) ) ;
		GGui::Control * control = static_cast<GGui::Control*>(context) ;
		G_ASSERT( control != nullptr ) ;
		G_ASSERT( control->handle() == hwnd ) ;
		G_ASSERT( control->id() == GetDlgCtrlID(hwnd) ) ;

		// remove the control from the map if it is being destroyed
		if( message == WM_NCDESTROY )
			dialog->map().remove( hwnd ) ;

		return control->wndProc( message , wparam , lparam , super_class ) ;
	}
	catch(...) // callback
	{
		return DefWindowProc( hwnd , message , wparam , lparam ) ;
	}
}

// ==

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

// ==

GGui::ListBox::ListBox( Dialog & dialog , int id ) :
	Control(dialog,id)
{
}

GGui::ListBox::~ListBox()
{
}

void GGui::ListBox::set( const G::StringArray & list )
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
	for( G::StringArray::const_iterator string_p = list.begin() ;
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
	if( rc == LB_ERR || rc > 0xfff0 || rc <= 0 )
		return std::string() ;

	std::vector<char> buffer( static_cast<std::size_t>(rc+2) ) ;
	buffer[0] = '\0' ;
	sendMessage( LB_GETTEXT , static_cast<WPARAM>(index) , reinterpret_cast<LPARAM>(static_cast<LPCSTR>(&buffer[0])) ) ;
	buffer[buffer.size()-1U] = '\0' ;
	return std::string( &buffer[0] ) ;
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

// ==

GGui::ListView::ListView( Dialog & dialog , int id ) :
	Control(dialog,id)
{
	load( ICC_LISTVIEW_CLASSES ) ;
}

GGui::ListView::ListView( HWND hdialog , int id , HWND hcontrol ) :
	Control(hdialog,id,hcontrol)
{
	load( ICC_LISTVIEW_CLASSES ) ;
}

GGui::ListView::~ListView()
{
}

LPTSTR GGui::ListView::ptext( const std::string & s )
{
	return reinterpret_cast<LPTSTR>( const_cast<char*>(s.c_str()) ) ;
}

void GGui::ListView::set( const G::StringArray & list , unsigned int columns , unsigned int width )
{
	NoRedraw no_redraw( *this ) ;
	std::size_t i = 0U ;
	for( unsigned int c = 0U ; c < columns ; c++ )
	{
		LVCOLUMN column ;
		column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;
		column.iSubItem = c ;
		column.pszText = ptext(list.at(i++)) ;
		column.cx = width ? width : 100U ;
		column.fmt = LVCFMT_LEFT ;
		sendMessage( LVM_INSERTCOLUMN , c , reinterpret_cast<LPARAM>(&column) ) ;
	}
	if( columns != 0U )
		update( list , columns ) ;
}

void GGui::ListView::update( const G::StringArray & list , unsigned int columns )
{
	G_ASSERT( columns != 0U ) ;
	G_ASSERT( list.size() >= static_cast<std::size_t>(columns) ) ;
	NoRedraw no_redraw( *this ) ;
	sendMessage( LVM_DELETEALLITEMS ) ;
	sendMessage( LVM_SETITEMCOUNT , (list.size()-columns)/columns ) ;
	std::size_t i = columns ;
	for( unsigned int item = 0U ; i < list.size() ; item++ )
	{
		for( unsigned int c = 0U ; c < columns ; c++ )
		{
			LVITEM lvitem ;
			lvitem.mask = LVIF_TEXT ;
			lvitem.iItem = item ;
			lvitem.iSubItem = c ;
			lvitem.pszText = ptext(list.at(i++)) ;
			sendMessage( c == 0 ? LVM_INSERTITEM : LVM_SETITEM , 0 , reinterpret_cast<LPARAM>(&lvitem) ) ;
		}
	}
}

// ==

GGui::EditBox::EditBox( Dialog & dialog , int id ) :
	Control( dialog , id ) ,
	m_character_height(0)
{
}

GGui::EditBox::~EditBox()
{
}

void GGui::EditBox::set( const std::string & text , int /*utf8_overload*/ )
{
	NoRedraw no_redraw( *this ) ;
	std::wstring wtext ;
	G::Convert::convert( wtext , G::Convert::utf8(text) ) ;
	SetWindowTextW( handle() , wtext.c_str() ) ;
}

void GGui::EditBox::set( const std::string & text )
{
	NoRedraw no_redraw( *this ) ;
	SetWindowTextA( handle() , text.c_str() ) ;
}

void GGui::EditBox::set( const G::StringArray & list )
{
	if( list.size() == 0U )
	{
		set( std::string() ) ;
	}
	else
	{
		NoRedraw no_redraw( *this ) ;

		std::string total ;
		const char *sep = "" ;
		for( G::StringArray::const_iterator iter = list.begin() ; iter != list.end() ; ++iter )
		{
			total.append( sep ) ;
			sep = "\x0D\x0A" ;
			total.append( *iter ) ;
		}

		SetWindowTextA( handle() , total.c_str() ) ;
		G_ASSERT( lines() >= list.size() ) ;
	}
}

unsigned int GGui::EditBox::lines()
{
	// handle an empty control since em_getlinecount returns one
	int length = GetWindowTextLength( handle() ) ;
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
	sendMessage( EM_LINESCROLL , 0 , LPARAM(lines()) + 10U ) ;
}

std::string GGui::EditBox::get() const
{
	int length = GetWindowTextLengthA( handle() ) ;
	if( length <= 0 ) return std::string() ;
	std::vector<char> buffer( static_cast<std::size_t>(length)+2U ) ;
	GetWindowTextA( handle() , &buffer[0] , length+1 ) ;
	buffer[buffer.size()-1U] = '\0' ;
	return std::string( &buffer[0] ) ;
}

std::string GGui::EditBox::get( int /*utf8_overload*/ ) const
{
	int length = GetWindowTextLengthW( handle() ) ;
	if( length <= 0 ) return std::string() ;
	std::vector<wchar_t> buffer( static_cast<std::size_t>(length)+2U ) ;
	GetWindowTextW( handle() , &buffer[0] , length+1 ) ;
	buffer[buffer.size()-1U] = '\0' ;
	G::Convert::utf8 result ;
	G::Convert::convert( result , std::wstring(&buffer[0]) ) ;
	return result.s ;
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
		GetTextMetrics( dc() , &tm ) ;
		m_character_height = static_cast<unsigned int>( tm.tmHeight + tm.tmExternalLeading ) ;
		G_ASSERT( m_character_height != 0 ) ;
	}
	return m_character_height ;
}

unsigned int GGui::EditBox::windowHeight()
{
	RECT rect ;
	GetWindowRect( handle() , &rect ) ;
	G_ASSERT( rect.bottom >= rect.top ) ;
	return static_cast<unsigned int>( rect.bottom - rect.top ) ;
}

void GGui::EditBox::setTabStops( const std::vector<int> & tabs )
{
	// see also Edit_SetTabStops()
	LRESULT rc = sendMessage( EM_SETTABSTOPS , static_cast<WPARAM>(tabs.size()) ,
		reinterpret_cast<LPARAM>(&tabs[0]) ) ;

	if( rc != TRUE )
	{
		G_DEBUG( "GGui::EditBox::setTabeStops: failed to set edit box tab stops" ) ;
	}
}

// ==

GGui::CheckBox::CheckBox( Dialog & dialog , int id ) :
	Control(dialog,id)
{
}

GGui::CheckBox::~CheckBox()
{
}

bool GGui::CheckBox::get() const
{
	return !! IsDlgButtonChecked( hdialog() , id() ) ;
}

void GGui::CheckBox::set( bool b )
{
	CheckDlgButton( hdialog() , id() , b ) ;
}

// ==

GGui::Button::Button( Dialog & dialog , int id ) :
	Control(dialog,id)
{
}

GGui::Button::~Button()
{
}

bool GGui::Button::enabled() const
{
	return !!IsWindowEnabled( handle() ) ;
}

void GGui::Button::enable( bool b )
{
	EnableWindow( handle() , !!b ) ;
}

void GGui::Button::disable()
{
	EnableWindow( handle() , false ) ;
}

