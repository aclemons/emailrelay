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
/// \file gstack.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gstack.h"
#include "gscope.h"
#include "gstr.h"
#include "glog.h"
#include "gassert.h"
#include <algorithm>
#include <prsht.h> // PropertySheet
#include <windowsx.h>

namespace GGui
{
	namespace StackImp
	{
		LRESULT CALLBACK wndProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;
		INT_PTR CALLBACK dlgProc( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam ) ;
		int CALLBACK sheetCallback( HWND hwnd , UINT message , LPARAM lparam ) ;
		UINT CALLBACK pageCallback( HWND hwnd , UINT message , G::nowide::PROPSHEETPAGE_type * page ) ;

		GGui::Stack * m_this = nullptr ;

		template <typename T> T convert( void * p )
		{
			return reinterpret_cast<T>( p ) ;
		}
		template <typename T> T convert( ULONG_PTR p )
		{
			return reinterpret_cast<T>( p ) ;
		}
		template <typename T> T convert( LRESULT (WINAPI *fn)(HWND,UINT,WPARAM,LPARAM) )
		{
			return reinterpret_cast<T>( fn ) ;
		}
	}
}

LRESULT CALLBACK GGui::StackImp::wndProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	try
	{
		return GGui::Stack::wndProc( hwnd , message , wparam , lparam ) ;
	}
	catch( std::exception & )
	{
		return 0 ;
	}
}

INT_PTR CALLBACK GGui::StackImp::dlgProc( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam )
{
	try
	{
		bool processed = GGui::Stack::dlgProc( hdialog , message , wparam , lparam ) ;
		return processed ? TRUE : FALSE ;
	}
	catch( std::exception & )
	{
		return FALSE ;
	}
}

int CALLBACK GGui::StackImp::sheetCallback( HWND hwnd , UINT message , LPARAM lparam )
{
	try
	{
		return GGui::Stack::sheetCallback( hwnd , message , lparam ) ;
	}
	catch( std::exception & )
	{
		return 0 ;
	}
}

UINT CALLBACK GGui::StackImp::pageCallback( HWND hwnd , UINT message , G::nowide::PROPSHEETPAGE_type * page )
{
	try
	{
		return GGui::Stack::pageCallback( hwnd , message , page ) ;
	}
	catch( std::exception & )
	{
		return 0U ;
	}
}

GGui::Stack::Stack( StackPageCallback & callback , HINSTANCE hinstance , std::pair<DWORD,DWORD> style ,
	bool set_style ) :
		WindowBase(0) ,
		m_magic(MAGIC) ,
		m_hinstance(hinstance) ,
		m_hsheet(0) ,
		m_callback(callback) ,
		m_style(style) ,
		m_set_style(set_style) ,
		m_fixed_size(false) ,
		m_notify_hwnd(0) ,
		m_notify_message(0U) ,
		m_wndproc_orig(0)
{
}

void GGui::Stack::create( HWND hparent , const std::string & title , int icon_id ,
	HWND notify_hwnd , unsigned int notify_message , bool fixed_size )
{
	G_ASSERT( handle() == 0 ) ;
	G_ASSERT( !m_hpages.empty() ) ;
	G_ASSERT( (notify_hwnd==0) == (notify_message==0U) ) ;

	G::ScopeExitSet<Stack*,nullptr> set_m_this( StackImp::m_this = this ) ;

	m_notify_hwnd = notify_hwnd ;
	m_notify_message = notify_message ;

	G::nowide::PROPSHEETHEADER_type header {} ;
	header.dwSize = sizeof(header) ;
	header.dwFlags = PSH_MODELESS | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP |
		PSH_USECALLBACK | ( icon_id ? PSH_USEICONID : 0 ) ;
	header.hwndParent = hparent ;
	header.hInstance = m_hinstance ;
	header.nPages = static_cast<UINT>(m_hpages.size()) ;
	header.nStartPage = 0 ;
	header.phpage = m_hpages.size() ? &m_hpages[0] : nullptr ;
	header.pfnCallback = StackImp::sheetCallback ;

	// create the PropertySheet() with a callback that hook()s in StackImp::wndProc()
	INT_PTR rc = G::nowide::propertySheet( &header , title , icon_id ) ;
	if( rc == 0 || rc == ID_PSREBOOTSYSTEM || rc == ID_PSRESTARTWINDOWS )
		throw std::runtime_error( "PropertySheet() failed" ) ;

	m_hsheet = reinterpret_cast<HWND>(rc) ;
	G_DEBUG( "GGui::Stack::create: hsheet=" << m_hsheet ) ;

	// redraw in case we have changed the window style
	RedrawWindow( m_hsheet , nullptr , HNULL , RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW ) ;
	m_fixed_size = fixed_size ;

	PropSheet_CancelToClose( m_hsheet ) ; // 'ok' and 'cancel' is daft -- 'close' with disabled 'cancel' is better

	if( !m_seen_wm_initdialog ) // or PropSheet_GetCurrentPageHwnd() ?
		throw std::runtime_error( "cannot load property sheet" ) ;
}

void GGui::Stack::addPage( const std::string & title , int dialog_id )
{
	int index = static_cast<int>( m_pages.size() ) ;
	m_pages.push_back( PageInfo(this,index) ) ;
	PageInfo * page_info = &m_pages.back() ;

	G::nowide::PROPSHEETPAGE_type page {} ;
	page.dwSize = sizeof(page) ;
	page.dwFlags = PSP_USECALLBACK | PSP_USETITLE ;
	page.hInstance = m_hinstance ;
	page.pszIcon = 0 ;
	page.pfnDlgProc = StackImp::dlgProc ;
	page.lParam = reinterpret_cast<LPARAM>(page_info) ;
	page.pfnCallback = StackImp::pageCallback ;

	HPROPSHEETPAGE hpage = G::nowide::createPropertySheetPage( &page , title , dialog_id ) ;
	if( hpage == HNULL )
	{
		m_pages.pop_back() ;
		throw std::runtime_error( "CreatePropertySheetPage() failed" ) ;
	}

	G_DEBUG( "GGui::Stack::addPage: new page: index=" << index << " title=[" << title << "] hpage=" << hpage ) ;
	m_hpages.push_back( hpage ) ;
}

GGui::Stack::~Stack()
{
	try
	{
		unhook() ;
	}
	catch(...) // dtor
	{
	}
	m_magic = 0 ;
}

void GGui::Stack::hook( HWND hsheet )
{
	G_ASSERT( m_hsheet == 0 ) ;
	m_hsheet = hsheet ;

	// use GGui::WindowBase as a convenience to any derived classes
	setHandle( hsheet ) ;

	if( m_set_style )
	{
		G::nowide::setWindowLong( hsheet , GWL_STYLE , m_style.first ) ;
		G::nowide::setWindowLong( hsheet , GWL_EXSTYLE , m_style.second ) ;
	}

	if( G::nowide::getWindowLongPtr( hsheet , GWLP_USERDATA ) == 0 )
	{
		G::nowide::setWindowLongPtr( hsheet , GWLP_USERDATA , StackImp::convert<ULONG_PTR>(this) ) ;
		m_wndproc_orig = G::nowide::getWindowLongPtr( hsheet , GWLP_WNDPROC ) ;
		G::nowide::setWindowLongPtr( hsheet , GWLP_WNDPROC , StackImp::convert<ULONG_PTR>(StackImp::wndProc) ) ;
	}
}

void GGui::Stack::unhook()
{
	if( G::nowide::getWindowLongPtr( m_hsheet , GWLP_USERDATA ) )
	{
		G::nowide::setWindowLongPtr( m_hsheet , GWLP_USERDATA , 0 ) ;
		G::nowide::setWindowLongPtr( m_hsheet , GWLP_WNDPROC , m_wndproc_orig ) ;
	}
}

GGui::Stack * GGui::Stack::getObjectPointer( HWND hwnd )
{
	ULONG_PTR lp = G::nowide::getWindowLongPtr( hwnd , GWLP_USERDATA ) ;
	Stack * This = StackImp::convert<Stack*>( lp ) ;
	G_ASSERT( This == nullptr || This->m_magic == MAGIC ) ;
	return This ;
}

bool GGui::Stack::stackMessage( MSG & msg )
{
	HWND hsheet = msg.hwnd ;
	if( PropSheet_IsDialogMessage( hsheet , &msg ) )
	{
		// this is the only way to know if the property page is
		// finished -- see MSDN PropertySheet() "Remarks"
		bool finished = PropSheet_GetCurrentPageHwnd( hsheet ) == HNULL ;
		if( finished )
		{
			Stack * This = getObjectPointer( hsheet ) ;
			if( This )
			{
				G_ASSERT( This->m_hsheet == hsheet ) ;
				bool is_ok = PropSheet_GetResult(hsheet) > 0 ; // ok or cancel
				This->postNotifyMessage( is_ok ? 1U : 0U ) ;
				This->unhook() ;
			}
		}
		return true ;
	}
	else
	{
		return false ;
	}
}

LRESULT GGui::Stack::wndProc( HWND hsheet , UINT message , WPARAM wparam , LPARAM lparam )
{
	Stack * This = getObjectPointer( hsheet ) ;
	if( message == WM_WINDOWPOSCHANGING && This && This->m_fixed_size && lparam )
	{
		// fiddling with the window-style doesn't always give the right
		// degree of control, so disable resizing once 'fixed'
		WINDOWPOS * pos_p = reinterpret_cast<WINDOWPOS*>(lparam) ;
		pos_p->flags |= SWP_NOSIZE ;
	}
	else if( message == WM_SYSCOMMAND && This && (wparam & 0xFFF0u) < 0xF000u )
	{
		// emit a notification message if a derived class has added to the system menu
		G_DEBUG( "GGui::Stack::wndProc: wm_syscommand " << (wparam & 0xFFF0u) ) ;
		This->postNotifyMessage( 3U , wparam & 0xFFF0u ) ;
	}

	if( This && This->m_wndproc_orig )
	{
		// call the PropertySheet window procedure
		return G::nowide::callWindowProc( This->m_wndproc_orig , hsheet , message , wparam , lparam ) ;
	}
	else
	{
		// never gets here -- dtor calls unhook()
		return G::nowide::defDlgProc( hsheet , message , wparam , lparam ) ;
	}
}

int GGui::Stack::sheetCallback( HWND hsheet , UINT message , LPARAM /*lparam*/ )
{
	G_DEBUG( "GGui::Stack::sheetCallback: hsheet=" << hsheet << " message=" << message ) ;
	if( message == PSCB_INITIALIZED )
	{
		G_ASSERT( StackImp::m_this != nullptr ) ;
		if( StackImp::m_this )
			StackImp::m_this->hook( hsheet ) ;
	}
	return 0 ;
}

unsigned int GGui::Stack::pageCallback( HWND hpage , UINT message , G::nowide::PROPSHEETPAGE_type * /*page_p*/ )
{
	G_DEBUG( "GGui::Stack::pageCallback: hpage=" << hpage << " message=message" ) ;
	if( message == PSPCB_CREATE )
		return 1U ; // allow creation
	return 0U ;
}

bool GGui::Stack::dlgProc( HWND hpage , UINT message , WPARAM /*wparam*/ , LPARAM lparam )
{
	if( message == WM_INITDIALOG )
	{
		// if we are still be inside PropertySheet() then object pointer might not
		// be set -- however the lparam parameter points at the page structure, and its
		// lParam value points to our PageInfo structure
		//
		G_DEBUG( "GGui::Stack::dlgProc: wm_initdialog: h=" << hpage << " lparam=" << lparam ) ;
		const G::nowide::PROPSHEETPAGE_type * page_p = reinterpret_cast<G::nowide::PROPSHEETPAGE_type*>(lparam) ;
		const PageInfo * page_info = page_p ? reinterpret_cast<const PageInfo*>(page_p->lParam) : nullptr ;
		if( page_info && page_info->first )
		{
			Stack * This = page_info->first ;
			G_ASSERT( This->m_magic == MAGIC ) ;
			This->m_seen_wm_initdialog = true ;
			G::nowide::setWindowLongPtr( hpage , GWLP_USERDATA , StackImp::convert<ULONG_PTR>(This) ) ;
			This->m_callback.onInit( hpage , page_info->second ) ;
		}
		return true ;
	}
	else
	{
		Stack * This = getObjectPointer( hpage ) ;
		if( This && message == WM_NOTIFY )
		{
			NMHDR * header = reinterpret_cast<NMHDR*>(lparam) ;
			if( header->code == PSN_SETACTIVE )
			{
				int index = PropSheet_HwndToIndex( This->handle() , hpage ) ;
				G_DEBUG( "GGui::Stack::dlgProc: wm_notify: psn_setactive: h=" << hpage << " index=" << index ) ;
				This->m_callback.onActive( index ) ;
			}
			else if( header->code == PSN_KILLACTIVE )
			{
				int index = PropSheet_HwndToIndex( This->handle() , hpage ) ;
				G_DEBUG( "GGui::Stack::dlgProc: wm_notify: psn_killactive: h=" << hpage ) ;
				This->m_callback.onInactive( index ) ;
			}
			else if( header->code == PSN_APPLY )
			{
				G_DEBUG( "GGui::Stack::dlgProc: wm_notify: psn_apply: h=" << hpage ) ;
				This->doOnApply( hpage ) ;
				return true ; // with MSGRESULT
			}
		}
		return false ;
	}
}

void GGui::Stack::doOnApply( HWND hpage )
{
	bool allow = m_callback.onApply() ;
	G::nowide::setWindowLong( hpage , DWLP_MSGRESULT , allow ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE ) ;
	if( !allow )
		postNotifyMessage( 2U ) ;
}

void GGui::Stack::postNotifyMessage( WPARAM wparam , LPARAM lparam )
{
	if( m_notify_message )
		G::nowide::postMessage( m_notify_hwnd , m_notify_message , wparam , lparam ) ;
}

// ==

void GGui::StackPageCallback::onInit( HWND , int )
{
}

void GGui::StackPageCallback::onActive( int )
{
}

void GGui::StackPageCallback::onInactive( int )
{
}

bool GGui::StackPageCallback::onApply()
{
	return true ;
}

GGui::StackPageCallback::~StackPageCallback()
= default ;

