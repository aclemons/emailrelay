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

std::list<HWND> GGui::Stack::m_list ;

namespace GGui
{
	namespace StackImp
	{
		LRESULT CALLBACK sheet_wndproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam ) ;
		INT_PTR CALLBACK dlgproc_export( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam ) ;
		int CALLBACK sheet_callback_export( HWND hwnd , UINT message , LPARAM lparam ) ;
		UINT CALLBACK page_callback_export( HWND hwnd , UINT message , G::nowide::PROPSHEETPAGE_type * page ) ;

		GGui::Stack * m_this = nullptr ;

		template <typename T> T convert( void * p )
		{
			return reinterpret_cast<T>( p ) ;
		}
		template <typename T> T convert( LONG_PTR p )
		{
			return reinterpret_cast<T>( p ) ;
		}
		template <typename T> T convert( LRESULT (WINAPI *fn)(HWND,UINT,WPARAM,LPARAM) )
		{
			return reinterpret_cast<T>( fn ) ;
		}
	}
}

LRESULT CALLBACK GGui::StackImp::sheet_wndproc_export( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	try
	{
		return GGui::Stack::wndProc( hwnd , message , wparam , lparam ) ;
	}
	catch(...) // callback
	{
		return 0 ;
	}
}

INT_PTR CALLBACK GGui::StackImp::dlgproc_export( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam )
{
	try
	{
		return GGui::Stack::dlgProc( hdialog , message , wparam , lparam ) ;
	}
	catch(...) // callback
	{
		return 0 ;
	}
}

int CALLBACK GGui::StackImp::sheet_callback_export( HWND hwnd , UINT message , LPARAM lparam )
{
	try
	{
		return GGui::Stack::sheetProc( hwnd , message , lparam ) ;
	}
	catch(...) // callback
	{
		return 0 ;
	}
}

UINT CALLBACK GGui::StackImp::page_callback_export( HWND hwnd , UINT message , G::nowide::PROPSHEETPAGE_type * page )
{
	try
	{
		return GGui::Stack::pageProc( hwnd , message , page ) ;
	}
	catch(...) // callback
	{
		return 0 ;
	}
}

GGui::Stack::Stack( StackPageCallback & callback , HINSTANCE hinstance , std::pair<DWORD,DWORD> style ,
	bool set_style ) :
		WindowBase(0) ,
		m_magic(MAGIC) ,
		m_hinstance(hinstance) ,
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
	header.pfnCallback = StackImp::sheet_callback_export ;

	INT_PTR rc = G::nowide::propertySheet( &header , title , icon_id ) ;
	if( rc == 0 || rc == ID_PSREBOOTSYSTEM || rc == ID_PSRESTARTWINDOWS )
		throw std::runtime_error( "PropertySheet() failed" ) ;

	HWND hsheet = reinterpret_cast<HWND>(rc) ;
	G_DEBUG( "GGui::Stack::create: hsheet=" << hsheet ) ;

	// redraw in case we have changed the window style
	RedrawWindow( hsheet , nullptr , HNULL , RDW_FRAME | RDW_INVALIDATE | RDW_ERASENOW ) ;
	m_fixed_size = fixed_size ;

	PropSheet_CancelToClose( hsheet ) ; // 'ok' and 'cancel' is daft -- 'close' with disabled 'cancel' is better
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
	page.pfnDlgProc = StackImp::dlgproc_export ;
	page.lParam = reinterpret_cast<LPARAM>(page_info) ;
	page.pfnCallback = StackImp::page_callback_export ;

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
	unhook() ;
	m_magic = 0 ;
}

void GGui::Stack::hook( HWND hwnd )
{
	G_DEBUG( "GGui::Stack::hook: hwnd=" << hwnd << " ptr=" << this ) ;
	G_ASSERT( hwnd ) ; if( hwnd == 0 ) return ;

	setHandle( hwnd ) ; // GGui::WindowBase

	m_list.push_back( hwnd ) ;

	if( m_set_style )
	{
		G::nowide::setWindowLong( hwnd , GWL_STYLE , m_style.first ) ;
		G::nowide::setWindowLong( hwnd , GWL_EXSTYLE , m_style.second ) ;
	}

	if( G::nowide::getWindowLongPtr( hwnd , GWLP_USERDATA ) == 0 )
	{
		G::nowide::setWindowLongPtr( hwnd , GWLP_USERDATA , StackImp::convert<LONG_PTR>(this) ) ;
		m_wndproc_orig = G::nowide::getWindowLongPtr( hwnd , GWLP_WNDPROC ) ;
		G::nowide::setWindowLongPtr( hwnd , GWLP_WNDPROC , StackImp::convert<LONG_PTR>(StackImp::sheet_wndproc_export) ) ;
	}
}

void GGui::Stack::unhook()
{
	HWND hwnd = handle() ;
	G_ASSERT( hwnd ) ; if( hwnd == 0 ) return ;
	G_DEBUG( "GGui::Stack::unhook: hwnd=" << hwnd << " ptr=" << this ) ;

	if( G::nowide::getWindowLongPtr( hwnd , GWLP_USERDATA ) )
	{
		G::nowide::setWindowLongPtr( hwnd , GWLP_USERDATA , 0 ) ;
		G::nowide::setWindowLongPtr( hwnd , GWLP_WNDPROC , m_wndproc_orig ) ;
	}

	m_list.erase( std::remove(m_list.begin(),m_list.end(),hwnd) , m_list.end() ) ;
}

GGui::Stack * GGui::Stack::getObjectPointer( HWND hwnd )
{
	LONG_PTR lp = G::nowide::getWindowLongPtr( hwnd , GWLP_USERDATA ) ;
	Stack * This = StackImp::convert<Stack*>( lp ) ;
	return This && This->m_magic == MAGIC ? This : nullptr ;
}

bool GGui::Stack::stackMessage( MSG & msg )
{
	using List = std::list<HWND> ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		HWND hwnd = *p ;
		if( PropSheet_IsDialogMessage( hwnd , &msg ) )
		{
			// this is the only way to know if the property page is
			// finished -- see MSDN PropertySheet() "Remarks"
			bool finished = PropSheet_GetCurrentPageHwnd(hwnd) == HNULL ;
			if( finished )
			{
				Stack * This = getObjectPointer(hwnd) ;
				if( This )
				{
					bool is_ok = PropSheet_GetResult(hwnd) > 0 ; // ok or cancel
					This->postNotifyMessage( is_ok ) ;
					This->unhook() ;
				}
			}
			return true ;
		}
	}
	return false ;
}

LRESULT GGui::Stack::wndProc( HWND hwnd , UINT message , WPARAM wparam , LPARAM lparam )
{
	Stack * This = getObjectPointer( hwnd ) ;
	if( This )
	{
		bool call_default = true ;
		LRESULT result = This->wndProc( message , wparam , lparam , call_default ) ;

		return ( call_default && This->m_wndproc_orig ) ?
			G::nowide::callWindowProc( This->m_wndproc_orig , hwnd , message , wparam , lparam ) :
			result ;
	}
	else
	{
		// never gets here
		return G::nowide::defDlgProc( hwnd , message , wparam , lparam ) ;
	}
}

LRESULT GGui::Stack::wndProc( UINT message , WPARAM wparam , LPARAM lparam , bool & call_default )
{
	if( message == WM_SYSCOMMAND && wparam < 0xf000 )
	{
		unsigned int id = static_cast<unsigned int>(wparam) ;
		postNotifyMessage( 3U , id ) ;
		m_callback.onSysCommand( id ) ;
		call_default = false ;
	}
	else if( message == WM_WINDOWPOSCHANGING && m_fixed_size && lparam )
	{
		// fiddling with the window-style doesn't always give the right
		// degree of control, so disable the resize once 'fixed'
		WINDOWPOS * pos_p = reinterpret_cast<WINDOWPOS*>(lparam) ;
		pos_p->flags |= SWP_NOSIZE ;
	}
	return 0 ;
}

int GGui::Stack::sheetProc( HWND hsheet , UINT message , LPARAM /*lparam*/ )
{
	if( message == PSCB_INITIALIZED )
	{
		G_ASSERT( StackImp::m_this != nullptr ) ;
		if( StackImp::m_this )
			StackImp::m_this->hook( hsheet ) ;
	}
	return 0 ;
}

unsigned int GGui::Stack::pageProc( HWND , UINT message , G::nowide::PROPSHEETPAGE_type * /*page_p*/ )
{
	unsigned int result = 0U ;
	if( message == PSPCB_CREATE )
		result = 1U ; // allow creation
	return result ;
}

bool GGui::Stack::dlgProc( HWND hpage , UINT message , WPARAM /*wparam*/ , LPARAM lparam )
{
	if( message == WM_INITDIALOG )
	{
		// if we are still be inside PropertySheet() then object pointer might not
		// be set -- however the lparam parameter points at the page structure, and its
		// lParam value points to our PageInfo structure
		//
		G_DEBUG( "GGui::Stack::dlgProc: WM_INITDIALOG: h=" << hpage << " lparam=" << lparam ) ;
		const G::nowide::PROPSHEETPAGE_type * page_p = reinterpret_cast<G::nowide::PROPSHEETPAGE_type*>(lparam) ;
		const PageInfo * page_info = page_p ? reinterpret_cast<const PageInfo*>(page_p->lParam) : nullptr ;
		if( page_info && page_info->first )
		{
			Stack * This = page_info->first ;
			G::nowide::setWindowLongPtr( hpage , GWLP_USERDATA , StackImp::convert<LONG_PTR>(This) ) ;
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
				G_DEBUG( "GGui::Stack::dlgProc: WM_NOTIFY: PSN_SETACTIVE: h=" << hpage << " index=" << index ) ;
				This->m_callback.onActive( index ) ;
			}
			else if( header->code == PSN_KILLACTIVE )
			{
				int index = PropSheet_HwndToIndex( This->handle() , hpage ) ;
				G_DEBUG( "GGui::Stack::dlgProc: WM_NOTIFY: PSN_KILLACTIVE: h=" << hpage ) ;
				This->m_callback.onInactive( index ) ;
			}
			else if( header->code == PSN_APPLY )
			{
				G_DEBUG( "GGui::Stack::dlgProc: WM_NOTIFY: PSN_APPLY: h=" << hpage ) ;
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
		PostMessage( m_notify_hwnd , m_notify_message , wparam , lparam ) ;
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

void GGui::StackPageCallback::onSysCommand( unsigned int /*id*/ )
{
}

GGui::StackPageCallback::~StackPageCallback()
{
}

