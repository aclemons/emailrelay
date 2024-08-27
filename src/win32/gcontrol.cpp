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
/// \file gcontrol.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gdialog.h"
#include "gcontrol.h"
#include "gconvert.h"
#include "glog.h"
#include "gassert.h"
#include "gdc.h"
#include <type_traits>
#include <limits>
#include <algorithm>
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

HWND GGui::Control::hdialog() const noexcept
{
	static_assert( noexcept(m_dialog->handle()) , "" ) ;
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
	return G::nowide::sendMessage( handle(std::nothrow) , message , wparam , lparam ) ;
}

LRESULT GGui::Control::sendMessageString( unsigned int message , WPARAM wparam , const std::string & s ) const
{
	return G::nowide::sendMessageString( handle() , message , wparam , s ) ;
}

std::string GGui::Control::sendMessageGetString( unsigned int message , WPARAM wparam ) const
{
	return G::nowide::sendMessageGetString( handle() , message , wparam ) ;
}

HWND GGui::Control::handle( std::nothrow_t ) const noexcept
{
	static_assert( noexcept(hdialog()) , "" ) ;
	return m_hwnd ?  m_hwnd : GetDlgItem( hdialog() , m_id ) ;
}

HWND GGui::Control::handle() const
{
	if( m_hwnd == 0 )
	{
		HWND hdialog_ = hdialog() ;
		const_cast<Control*>(this)->m_hwnd = GetDlgItem( hdialog_ , m_id ) ;
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
	SubClassMap::Proc old = reinterpret_cast<SubClassMap::Proc>( G::nowide::getWindowLongPtr( handle() , GWLP_WNDPROC ) ) ;
	map.add( handle() , old , static_cast<void*>(this) ) ;
	G::nowide::setWindowLongPtr( handle() , GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(gcontrol_wndproc_export) ) ;
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
			return G::nowide::defWindowProc( hwnd , message , wparam , lparam ) ;
		}

		// find the dialog box object
		GGui::Dialog * dialog = reinterpret_cast<GGui::Dialog*>( G::nowide::getWindowLongPtr( hwnd_dialog , DWLP_USER ) ) ;
		if( dialog == nullptr )
		{
			G_ASSERT( false ) ;
			return G::nowide::defWindowProc( hwnd , message , wparam , lparam ) ;
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
		return G::nowide::defWindowProc( hwnd , message , wparam , lparam ) ;
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
		sendMessageString( LB_ADDSTRING , 0 , *string_p ) ;
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

	return sendMessageGetString( LB_GETTEXT , static_cast<WPARAM>(index) ) ;
}

unsigned int GGui::ListBox::entries() const
{
	LRESULT entries = sendMessage( LB_GETCOUNT , 0 , 0 ) ;
	if( entries == LB_ERR )
		entries = 0 ;
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

void GGui::ListView::set( const G::StringArray & list , unsigned int columns , unsigned int width )
{
	NoRedraw no_redraw( *this ) ;
	std::size_t i = 0U ;
	for( unsigned int c = 0U ; c < columns ; c++ )
	{
		G::nowide::sendMessageInsertColumn( handle() , c , list.at(i++) , width?width:100U ) ;
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
			G::nowide::sendMessageInsertItem( handle() , item , c , list.at(i++) ) ;
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

void GGui::EditBox::set( const std::string & text )
{
	NoRedraw no_redraw( *this ) ;
	G::nowide::setWindowText( handle() , text ) ;
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
		for( G::StringArray::const_iterator iter = list.begin() ; iter != list.end() ; ++iter )
		{
			total.append( "\r\n" , (iter==list.begin()?0U:2U) ) ;
			total.append( *iter ) ;
		}
		G::nowide::setWindowText( handle() , total ) ;
		G_ASSERT( lines() >= list.size() ) ;
	}
}

unsigned int GGui::EditBox::lines()
{
	// handle an empty control since em_getlinecount returns one
	int length = G::nowide::getWindowTextLength( handle() ) ;
	if( length == 0 )
		return 0U ;

	LRESULT lines = sendMessage( EM_GETLINECOUNT ) ;
	return static_cast<unsigned int>(lines) ;
}

unsigned int GGui::EditBox::linesInWindow()
{
	unsigned int text_height = characterHeight() ;
	unsigned int window_height = windowHeight() ;
	G_ASSERT( text_height != 0U ) ;
	return window_height / std::max(1U,text_height) ;
}

void GGui::EditBox::scrollBack( int lines )
{
	static_assert( std::is_signed<LONG>::value && sizeof(LONG) >= sizeof(int) , "" ) ;
	LONG dy = -static_cast<LONG>(lines) ;
	if( dy < 0 )
		sendMessage( EM_LINESCROLL , 0 , dy ) ;
}

void GGui::EditBox::scrollToEnd()
{
	// (add ten just to make sure)
	sendMessage( EM_LINESCROLL , 0 , LPARAM(lines()) + 10U ) ;
}

std::string GGui::EditBox::get() const
{
	return G::nowide::getWindowText( handle() ) ;
}

unsigned int GGui::EditBox::scrollPosition()
{
	static_assert( sizeof(LRESULT) >= sizeof(unsigned) , "" ) ;
	LRESULT position = sendMessage( EM_GETFIRSTVISIBLELINE ) ;
	return static_cast<unsigned int>( std::min( LRESULT(std::numeric_limits<unsigned>::max()) , position ) ) ;
}

unsigned int GGui::EditBox::scrollRange()
{
	unsigned int range = lines() ;
	if( range <= 1U )
		range = 1U ;
	else
		range-- ;
	return range ;
}

unsigned int GGui::EditBox::characterHeight()
{
	if( m_character_height == 0U )
	{
		DeviceContext dc( handle() ) ;
		m_character_height = G::nowide::getTextMetricsHeight( dc() ) ;
		G_ASSERT( m_character_height != 0U ) ;
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
		reinterpret_cast<LPARAM>(tabs.data()) ) ;

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

