//
// Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// gstack.cpp
//

#include "gdef.h"
#include "gstack.h"
#include "gstr.h"
#include "gwindowhidden.h"
#include "glog.h"
#include "gassert.h"
#include <prsht.h> // PropertySheet

#if defined(G_MINGW)
#ifndef PSM_GETRESULT
#define PSM_GETRESULT (WM_USER+135)
#endif
#ifndef PSM_HWNDTOINDEX
#define PSM_HWNDTOINDEX (WM_USER+129)
#endif
#ifndef PSCB_BUTTONPRESSED
#define PSCB_BUTTONPRESSED 3
#endif
#ifndef PSPCB_ADDREF
#define PSPCB_ADDREF 0
#endif
#ifndef PSH_NOCONTEXTHELP
#define PSH_NOCONTEXTHELP 0
#endif
#ifndef PropSheet_IsDialogMessage
#define PropSheet_IsDialogMessage(hDlg,pMsg) (WINBOOL)::SendMessage(hDlg,PSM_ISDIALOGMESSAGE,0,(LPARAM)pMsg)
#endif
#ifndef PropSheet_HwndToIndex
#define PropSheet_HwndToIndex(hDlg,hwnd) (int)::SendMessage(hDlg,PSM_HWNDTOINDEX,(WPARAM)(hwnd),0)
#endif
#ifndef PropSheet_GetCurrentPageHwnd
#define PropSheet_GetCurrentPageHwnd(hDlg) (HWND)::SendMessage(hDlg,PSM_GETCURRENTPAGEHWND,(WPARAM)0,(LPARAM)0)
#endif
#ifndef PropSheet_GetResult
#define PropSheet_GetResult(hDlg) ::SendMessage(hDlg,PSM_GETRESULT,0,0)
#endif
#ifndef PropSheet_CancelToClose
#define PropSheet_CancelToClose(hDlg) ::SendMessage(hDlg,PSM_CANCELTOCLOSE,0,0)
#endif
#endif

std::list<HWND> GGui::Stack::m_list ;

class GGui::StackImp
{
public:
	static int sheetProc( HWND hwnd , UINT message , LPARAM lparam ) ;
	static unsigned int pageProc( HWND hwnd , UINT message , PROPSHEETPAGE * page ) ;
	static bool dlgProc( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam ) ;
} ;

// ==

int CALLBACK gstack_sheet_export( HWND hwnd , UINT message , LPARAM lparam )
{
	return GGui::StackImp::sheetProc( hwnd , message , lparam ) ;
}
int GGui::StackImp::sheetProc( HWND hwnd , UINT message , LPARAM lparam )
{
	return Stack::sheetProc( hwnd , message , lparam ) ;
}

UINT CALLBACK gstack_page_export( HWND hwnd , UINT message , PROPSHEETPAGE * page )
{
	return GGui::StackImp::pageProc( hwnd , message , page ) ;
}
UINT GGui::StackImp::pageProc( HWND hwnd , UINT message , PROPSHEETPAGE * page )
{
	return Stack::pageProc( hwnd , message , page ) ;
}

INT_PTR CALLBACK gstack_dlgproc_export( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam )
{
	return GGui::StackImp::dlgProc( hdialog , message , wparam , lparam ) ;
}
bool GGui::StackImp::dlgProc( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam )
{
	return GGui::Stack::dlgProc( hdialog , message , wparam , lparam ) ;
}

// ==

GGui::Stack::Stack( StackPageCallback & callback , HINSTANCE hinstance ) :
	WindowBase(0) ,
	m_magic(MAGIC) ,
	m_hinstance(hinstance) ,
	m_callback(callback)
{
}

void GGui::Stack::addPage( const std::string & title , int dialog_id , int icon_id )
{
	static PROPSHEETPAGE page_zero ;
	PROPSHEETPAGE page ;
	page = page_zero ;
	page.dwSize = sizeof(PROPSHEETPAGE) ;
	page.dwFlags = PSP_USECALLBACK | (icon_id ? PSP_USEICONID : 0 ) | PSP_USETITLE ;
	page.hInstance = m_hinstance ;
	page.pszTemplate = dialog_id ? MAKEINTRESOURCE(dialog_id) : 0 ;
	page.pszIcon = icon_id ? MAKEINTRESOURCE(icon_id) : 0 ;
	page.pfnDlgProc = gstack_dlgproc_export ;
	page.pszTitle = static_cast<LPCSTR>(title.c_str()) ;
	page.lParam = to_lparam(this) ; // PROPSHEETPAGE.lParam - see WM_INITDIALOG and PSPCB_CREATE
	page.pfnCallback = gstack_page_export ;

	HPROPSHEETPAGE hpage = CreatePropertySheetPage( &page ) ;
	if( hpage == NULL )
		throw std::runtime_error( "CreatePropertySheetPage() failed" ) ;

	m_pages.push_back(hpage) ;
}

void GGui::Stack::create( HWND hparent , const std::string & title , int icon_id , unsigned int notify_message )
{
	G_DEBUG( "GGui::Stack::create: hparent=" << hparent ) ;
	m_notify_hwnd = hparent ;
	m_notify_message = notify_message ;

	static PROPSHEETHEADER header_zero ;
	PROPSHEETHEADER header = header_zero ;

	header.dwSize = sizeof(PROPSHEETHEADER) ;
	header.dwFlags = PSH_MODELESS | PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP | PSH_USECALLBACK | ( icon_id ? PSH_USEICONID : 0 ) ;
	header.hwndParent = hparent ;
	header.hInstance = m_hinstance ;
	header.pszIcon = icon_id ? MAKEINTRESOURCE(icon_id) : 0 ;
	header.pszCaption = static_cast<LPCSTR>(title.c_str()) ;
	header.nPages = static_cast<UINT>(m_pages.size()) ;
	header.nStartPage = 0 ;
	header.phpage = m_pages.size() ? &m_pages[0] : NULL ;
	header.pfnCallback = gstack_sheet_export ;

	INT_PTR rc = PropertySheet( &header ) ;
	if( rc == 0 || rc == ID_PSREBOOTSYSTEM || rc == ID_PSRESTARTWINDOWS )
		throw std::runtime_error( "PropertySheet() failed" ) ;

	HWND hsheet = reinterpret_cast<HWND>(rc) ;
	G_DEBUG( "GGui::Stack::create: hsheet=" << hsheet ) ;
	setHandle( hsheet ) ; // GGui::WindowBase
	setptr( hsheet , this ) ;

	PropSheet_CancelToClose( hsheet ) ; // 'ok' and 'cancel' makes no sense -- 'close' with disabled 'cancel' is marginally better
}

void GGui::Stack::onCompleteImp( bool b )
{
	G_DEBUG( "GGui::Stack::onCompleteImp: b=" << b << " notify=" << m_notify_hwnd << " msg=" << m_notify_message ) ;
	setptr( handle() , 0 ) ;
	if( m_notify_message )
		::PostMessage( m_notify_hwnd , m_notify_message , b?1U:0U , reinterpret_cast<LPARAM>(handle()) ) ;
}

GGui::Stack::~Stack()
{
	typedef std::list<HWND> List ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( handle() == *p )
		{
			m_list.erase( p ) ;
			break ;
		}
	}
	setptr( handle() , NULL ) ;
	m_magic = 0 ;
}

GGui::Stack * GGui::Stack::from_lparam( LPARAM lparam )
{
	Stack * p = reinterpret_cast<Stack*>(reinterpret_cast<void*>(lparam)) ;
	return p && p->m_magic == MAGIC ? p : NULL ;
}

LPARAM GGui::Stack::to_lparam( const Stack * p )
{
	return reinterpret_cast<LPARAM>(reinterpret_cast<void*>(const_cast<Stack*>(p))) ;
}

void GGui::Stack::setptr( HWND hwnd , const Stack * This )
{
	::SetWindowLongPtr( hwnd , GWLP_USERDATA , to_lparam(This) ) ;
}

GGui::Stack * GGui::Stack::getptr( HWND hwnd )
{
	return from_lparam( ::GetWindowLongPtr( hwnd , GWLP_USERDATA ) ) ;
}

bool GGui::Stack::stackMessage( MSG & msg )
{
	typedef std::list<HWND> List ;
	for( List::iterator p = m_list.begin() ; p != m_list.end() ; ++p )
	{
		if( PropSheet_IsDialogMessage( *p , &msg ) )
		{
			// this is the only way to know if the property page is finished -- see
			// MSDN PropertySheet() "Remarks" section -- to get an suitable event
			// post to a hidden window
			//
			Stack * This = getptr(*p) ;
			if( This && PropSheet_GetCurrentPageHwnd(*p) == NULL )
			{
				bool ok_button = PropSheet_GetResult(*p) > 0 ; // ok or cancel
				This->onCompleteImp( ok_button ) ;
			}
			return true ;
		}
	}
	return false ;
}

bool GGui::Stack::dlgProc( HWND hdialog , UINT message , WPARAM wparam , LPARAM lparam )
{
	// see also GGui::Dialog
	if( message == WM_INITDIALOG )
	{
		// see MSDN "How to Create Wizards, Custom Page Data"
		PROPSHEETPAGE * page_p = reinterpret_cast<PROPSHEETPAGE*>(lparam) ;
		if( page_p )
		{
			Stack * This = from_lparam( page_p->lParam ) ;
			if( This )
			{
				std::string title = page_p->pszTitle ;
				G_DEBUG("GGui::Stack::dlgProc: WM_INITDIALOG: " << title ) ;
				setptr( hdialog , This ) ;
				This->m_callback.onInit( hdialog , title ) ;
			}
		}
		return true ;
	}
	else
	{
		Stack * This = getptr( hdialog ) ;
		if( This )
		{
			if( message == WM_CLOSE )
			{
				G_DEBUG( "GGui::Stack::dlgProc: WM_CLOSE: h=" << hdialog ) ;
				This->m_callback.onClose( hdialog ) ;
				return true ;
			}
			if( message == WM_DESTROY )
			{
				G_DEBUG( "GGui::Stack::dlgProc: WM_DESTROY: h=" << hdialog ) ;
				This->m_callback.onDestroy( hdialog ) ;
				return true ;
			}
			else if( message == WM_NOTIFY )
			{
				NMHDR * header = reinterpret_cast<NMHDR*>(lparam) ;
				if( header->code == PSN_SETACTIVE )
				{
					int index = PropSheet_HwndToIndex( This->handle() , hdialog ) ;
					G_DEBUG( "GGui::Stack::dlgProc: WM_NOTIFY: PSN_SETACTIVE: h=" << hdialog << " index=" << index ) ;
					HWND hfrom = header->hwndFrom ; G_IGNORE_VARIABLE(hfrom) ;
					//PSHNOTIFY * p = reinterpret_cast<PSHNOTIFY*>(lparam) ; // 'derived' structure
					This->m_callback.onActive( hdialog , index ) ;
					::SetWindowLong( hdialog , DWLP_MSGRESULT , 0 ) ; // accept activation
				}
				if( header->code == PSN_KILLACTIVE )
				{
					int index = PropSheet_HwndToIndex( This->handle() , hdialog ) ;
					G_DEBUG( "GGui::Stack::dlgProc: WM_NOTIFY: PSN_KILLACTIVE: h=" << hdialog << " index=" << index ) ;
					HWND hfrom = header->hwndFrom ; G_IGNORE_VARIABLE(hfrom) ;
					//PSHNOTIFY * p = reinterpret_cast<PSHNOTIFY*>(lparam) ; // 'derived' structure
					This->m_callback.onInactive( hdialog , index ) ;
				}
				if( header->code == PSN_APPLY )
				{
					HWND hcontrol = header->hwndFrom ; G_IGNORE_VARIABLE(hcontrol) ;
					UINT_PTR control_id = header->idFrom ; G_IGNORE_VARIABLE(control_id) ;
					G_DEBUG( "GGui::Stack::dlgProc: WM_NOTIFY: PSN_APPLY: h=" << hdialog ) ;
					// ...
				}
			}
			else if( message == WM_NCDESTROY )
			{
				G_DEBUG( "GGui::Stack::dlgProc: WM_NCDESTROY: h=" << hdialog ) ;
				setptr( hdialog , 0 ) ;
				This->m_callback.onNcDestroy( hdialog ) ;
				return true ;
			}
		}
		return false ;
	}
}

int GGui::Stack::sheetProc( HWND hwnd , UINT message , LPARAM lparam )
{
	if( message == PSCB_BUTTONPRESSED )
	{
		// never gets here
		//if( lparam == PSBTN_OK ) ;
		//if( lparam == PSBTN_CANCEL ) ;
		//if( lparam == PSBTN_APPLYNOW ) ;
		//if( lparam == PSBTN_FINISH ) ;
	}
	else if( message == PSCB_INITIALIZED )
	{
		m_list.push_back( hwnd ) ;
	}
	else if( message == PSCB_PRECREATE )
	{
	}
	return 0 ;
}

unsigned int GGui::Stack::pageProc( HWND /*not available*/ , UINT message , PROPSHEETPAGE * page_p )
{
	//Stack * This = from_lparam( page_p ? page_p->lParam : 0 ) ;
	unsigned int result = 0U ;
	if( message == PSPCB_ADDREF )
	{
	}
	else if( message == PSPCB_CREATE )
	{
		result = 1U ; // allow creation
	}
	else if( message == PSPCB_RELEASE )
	{
	}
	return result ;
}

// ==

void GGui::StackPageCallback::onInit( HWND /*hdialog*/ , const std::string & )
{
}

void GGui::StackPageCallback::onClose( HWND /*hdialog*/ )
{
}

void GGui::StackPageCallback::onDestroy( HWND /*hdialog*/ )
{
}

void GGui::StackPageCallback::onNcDestroy( HWND /*hdialog*/ )
{
}

void GGui::StackPageCallback::onActive( HWND /*hdialog*/ , int /*index*/ )
{
}

void GGui::StackPageCallback::onInactive( HWND /*hdialog*/ , int /*index*/ )
{
}

GGui::StackPageCallback::~StackPageCallback()
{
}

/// \file gstack.cpp
