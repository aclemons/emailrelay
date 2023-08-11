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
/// \file installer.cpp
///

#include "gdef.h"
#include "gcominit.h"
#include "gstr.h"
#include "gstringmap.h"
#include "gpath.h"
#include "gfile.h"
#include "gdate.h"
#include "gtest.h"
#include "gtime.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "guilink.h"
#include "gbatchfile.h"
#include "gxtext.h"
#include "gbase64.h"
#include "glog.h"
#include "glogoutput.h"
#include "gexception.h"
#include "gexecutablecommand.h"
#include "gnewprocess.h"
#include "gmapfile.h"
#include "guiboot.h"
#include "guiaccess.h"
#include "serverconfiguration.h"
#include "installer.h"
#include <functional>
#include <iterator>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <list>
#include <cstdlib>

// internationalisation shenanigans
#if 1
#include "gqt.h"
using trstring = QString ;
static std::string from_trstring( const trstring & qs )
{
	return GQt::stdstr( qs , GQt::Utf8 ) ;
}
static bool empty( const trstring & s )
{
	return s.isEmpty() ;
}
struct TrError : std::runtime_error
{
	trstring m_text ;
	std::string m_subject ;
	explicit TrError( trstring text , std::string subject = std::string() ) :
		std::runtime_error(from_trstring(text)) ,
		m_text(text) ,
		m_subject(subject)
	{
	}
} ;
#else
#define Q_DECLARE_TR_FUNCTIONS(name)
using trstring = std::string ;
struct Proxy
{
	Proxy( const std::string & s ) :
		m_s(s) ,
		m_arg(1)
	{
	}
	Proxy & arg( const std::string & value )
	{
		std::string key = "%" + G::Str::fromInt(m_arg++) ;
		G::Str::replaceAll( m_s , key , value ) ;
		return *this ;
	}
	operator std::string() const
	{
		return m_s ;
	}
	std::string operator+( const std::string & s ) const
	{
		return m_s + s ;
	}
	std::string m_s ;
	int m_arg ;
} ;
struct QCoreApplication
{
	static Proxy translate( const char * , const std::string & s )
	{
		return Proxy(s) ;
	}
} ;
static Proxy tr( const std::string & s )
{
	return Proxy(s) ;
}
static std::string from_trstring( const trstring & s )
{
	return s ;
}
static bool empty( const trstring & s )
{
	return s.empty() ;
}
struct TrError : std::runtime_error
{
	std::string m_text ;
	std::string m_subject ;
	explicit TrError( const std::string & text , const std::string & subject = std::string() ) :
		std::runtime_error(text+(subject.empty()?"":": ")+subject) ,
		m_text(text) ,
		m_subject(subject)
	{
	}
} ;
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

struct ActionInterface
{
	virtual void run() = 0 ;
	virtual trstring text() const = 0 ;
	virtual std::string subject() const = 0 ;
	virtual trstring ok() const = 0 ;
	virtual ~ActionInterface() = default ;
} ;

struct Helper
{
	static bool isWindows() ;
	static bool isMac() ;
	static bool m_is_windows ;
	static bool m_is_mac ;
} ;
bool Helper::m_is_windows = false ;
bool Helper::m_is_mac = false ;

struct ActionBase : public ActionInterface , protected Helper
{
	trstring ok() const override ;
} ;

struct CreateDirectory : public ActionBase
{
	trstring m_display_name ;
	trstring m_ok ;
	G::Path m_path ;
	bool m_tight_permissions ;
	CreateDirectory( trstring display_name , std::string path , bool tight_permissions = false ) ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	void run() override ;
	Q_DECLARE_TR_FUNCTIONS(CreateDirectory)
} ;

struct CreatePointerFile : public ActionBase
{
	G::Path m_pointer_file ;
	G::Path m_gui_exe ;
	G::Path m_dir_config ;
	G::Path m_dir_install ;
	G::Path m_dir_tr ;
	CreatePointerFile( const G::Path & pointer_file , const G::Path & gui_exe , const G::Path & dir_config , const G::Path & dir_install , const G::Path & dir_tr ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	Q_DECLARE_TR_FUNCTIONS(CreatePointerFile)
} ;

struct CreateFilterScript : public ActionBase
{
	G::Path m_path ;
	bool m_client_filter ;
	trstring m_ok ;
	CreateFilterScript( const G::Path & path , bool client ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	Q_DECLARE_TR_FUNCTIONS(CreateFilterScript)
} ;

struct CopyPayloadFile : public ActionBase
{
	G::Path m_src ;
	G::Path m_dst ;
	std::string m_flags ;
	CopyPayloadFile( G::Path src , G::Path dst , std::string flags ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	Q_DECLARE_TR_FUNCTIONS(CopyPayloadFile)
} ;

struct CopyPayloadTree : public ActionBase
{
	G::Path m_src ;
	G::Path m_dst ;
	CopyPayloadTree( G::Path src , G::Path dst ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	void add( int depth , G::Path , G::Path ) const ;
	Q_DECLARE_TR_FUNCTIONS(CopyPayloadTree)
} ;

struct FileGroup : public ActionBase
{
	std::string m_path ;
	std::string m_tail ;
	trstring m_ok ;
	FileGroup( const std::string & , const std::string & ) ;
	void exec( const std::string & , const std::string & ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	Q_DECLARE_TR_FUNCTIONS(FileGroup)
} ;

struct CreateSecrets : public ActionBase
{
	G::Path m_path ;
	G::Path m_template ;
	struct Item
	{
		std::string key ; // "client plain:b" or "server plain:b BOB==" or "server none"
		std::string key2 ; // "client plain" or "server plain b+0B" or ""
		std::string line ; // "client plain:b ALICE== PWD==" or "server plain:b BOB== PWD==" or "server none 127.0.0.1 trusted"
	} ;
	std::vector<Item> m_content ;
	CreateSecrets( const std::string & config_dir , const std::string & filename , G::Path template_ , const G::MapFile & pvalues ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	static bool match( std::string , std::string ) ;
	static bool yes( const std::string & ) ;
	void addSecret( const G::MapFile & , const std::string & k ) ;
	void addSecret( const G::MapFile & , const std::string & side , const std::string & k1 , const std::string & k2 ) ;
	Q_DECLARE_TR_FUNCTIONS(CreateSecrets)
} ;

struct CreateBatchFile : public ActionBase
{
	G::Path m_bat ;
	G::Path m_exe ;
	G::StringArray m_args ;
	CreateBatchFile( const G::Path & bat , const G::Path & exe , const G::StringArray & args ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	Q_DECLARE_TR_FUNCTIONS(CreateBatchFile)
} ;

struct UpdateLink : public ActionBase
{
	enum class LinkType
	{
		Desktop ,
		StartMenu ,
		AutoStart ,
		BatchFile
	} ;
	LinkType m_link_type ;
	bool m_active ;
	G::Path m_link_dir ;
	G::Path m_working_dir ;
	G::Path m_target ;
	G::StringArray m_args ;
	G::Path m_icon ;
	G::Path m_link_path ;
	trstring m_ok ;
	UpdateLink( LinkType , bool active , G::Path link_dir , G::Path working_dir , G::Path target , const G::StringArray & args , G::Path icon ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	Q_DECLARE_TR_FUNCTIONS(UpdateLink)
} ;

struct UpdateBootLink : public ActionBase
{
	bool m_active ;
	bool m_start_on_boot ;
	trstring m_ok ;
	std::string m_name ;
	G::Path m_startstop_src ;
	G::Path m_exe ;
	UpdateBootLink( bool active , bool start_on_boot , std::string , G::Path startstop_src , G::Path exe ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	Q_DECLARE_TR_FUNCTIONS(UpdateBootLink)
} ;

struct InstallService : public ActionBase
{
	bool m_active ;
	bool m_start_on_boot ;
	trstring m_ok ;
	G::Path m_bat ;
	G::Path m_service_wrapper ;
	InstallService( bool active , bool start_on_boot , G::Path bat , G::Path wrapper ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	Q_DECLARE_TR_FUNCTIONS(InstallService)
} ;

struct RegisterAsEventSource : public ActionBase
{
	G::Path m_exe ;
	explicit RegisterAsEventSource( const G::Path & ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	Q_DECLARE_TR_FUNCTIONS(RegisterAsEventSource)
} ;

struct CreateConfigFile : public ActionBase
{
	trstring m_ok ;
	G::Path m_template ;
	G::Path m_dst ;
	CreateConfigFile( G::Path dst_dir , std::string dst_name , G::Path template_ ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	Q_DECLARE_TR_FUNCTIONS(CreateConfigFile)
} ;

struct EditConfigFile : public ActionBase
{
	G::Path m_path ;
	G::MapFile m_server_config ;
	bool m_do_backup ;
	EditConfigFile( G::Path dir , std::string name , const G::MapFile & server_config , bool ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	Q_DECLARE_TR_FUNCTIONS(EditConfigFile)
} ;

struct GenerateKey : public ActionBase
{
	GenerateKey( G::Path path_out , const std::string & issuer ) ;
	static G::Path exe( bool is_windows ) ; // public & used early
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	G::Path m_exe ;
	G::Path m_path_out ;
	std::string m_issuer ;
	Q_DECLARE_TR_FUNCTIONS(GenerateKey)
} ;

struct Launcher : public ActionBase
{
	Launcher( bool as_service , const G::Path & bat , const G::Path & exe , const G::Path & conf ) ;
	void run() override ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	bool m_as_service ;
	trstring m_text ;
	std::string m_subject ;
	trstring m_ok ;
	G::ExecutableCommand m_cmd ;
	Q_DECLARE_TR_FUNCTIONS(Launcher)
} ;

struct JustTesting : public ActionBase
{
	JustTesting() ;
	trstring text() const override ;
	std::string subject() const override ;
	trstring ok() const override ;
	void run() override ;
	trstring m_ok ;
	Q_DECLARE_TR_FUNCTIONS(JustTesting)
} ;

struct Action
{
	std::shared_ptr<ActionInterface> m_p ;
	explicit Action( ActionInterface * p ) ;
	trstring text() const ;
	std::string subject() const ;
	trstring ok() const ;
	void run() ;
} ;

class InstallerImp : private Helper
{
public:
	InstallerImp( bool installing , bool is_windows , bool is_mac , const G::Path & payload_path , std::istream & ) ;
	~InstallerImp() ;
	bool next() ;
	bool done() ;
	void back() ;
	void run() ;
	Installer::Output output() ;
	bool failed() const ;
	G::Path addLauncher() ;

public:
	InstallerImp( const InstallerImp & ) = delete ;
	InstallerImp( InstallerImp && ) = delete ;
	InstallerImp & operator=( const InstallerImp & ) = delete ;
	InstallerImp & operator=( InstallerImp && ) = delete ;

private:
	Action & current() ;
	std::string pvalue( const std::string & key , const std::string & default_ ) const ;
	std::string pvalue( const std::string & key ) const ;
	std::string ivalue( const std::string & key ) const ;
	bool exists( const std::string & key ) const ;
	static bool yes( const std::string & ) ;
	static bool no( const std::string & ) ;
	void addActions() ;
	void addAction( ActionInterface * p ) ;

private:
	using List = std::list<Action> ;
	bool m_installing ;
	G::MapFile m_installer_config ;
	G::Path m_payload ;
	G::MapFile m_pages_output ;
	G::MapFile m_var ;
	List m_list ;
	List::iterator m_p ;
	bool m_have_run ;
	Installer::Output m_output ;
	Q_DECLARE_TR_FUNCTIONS(InstallerImp)
} ;

// ==

CreateDirectory::CreateDirectory( trstring display_name , std::string path , bool tight_permissions ) :
	m_display_name(display_name) ,
	m_path(path) ,
	m_tight_permissions(tight_permissions)
{
}

trstring CreateDirectory::text() const
{
	return tr("creating %1 directory").arg(m_display_name) ;
}

std::string CreateDirectory::subject() const
{
	return m_path.str() ;
}

trstring CreateDirectory::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

void CreateDirectory::run()
{
	if( m_path.empty() )
	{
		m_ok = tr("nothing to do") ;
	}
	else
	{
		G::Directory directory( m_path ) ;
		if( G::File::exists(m_path) )
		{
			if( !directory.valid() )
				throw TrError( tr("directory path exists but not valid a directory") ) ;
			m_ok = tr("exists") ;
		}
		else
		{
			G::File::mkdirs( m_path , 10 ) ;
		}
		Gui::Access::modify( m_path , m_tight_permissions ) ;
		if( !directory.writeable() )
			throw TrError( tr("directory exists but is not writable") ) ;
	}
}

// ==

CreatePointerFile::CreatePointerFile( const G::Path & pointer_file , const G::Path & gui_exe ,
	const G::Path & dir_config , const G::Path & dir_install , const G::Path & dir_tr ) :
		m_pointer_file(pointer_file) ,
		m_gui_exe(gui_exe) ,
		m_dir_config(dir_config) ,
		m_dir_install(dir_install) ,
		m_dir_tr(dir_tr)
{
}

void CreatePointerFile::run()
{
	if( m_pointer_file.empty() )
		return ;

	// create the directory -- probably unnecessary
	if( !G::File::isDirectory(m_pointer_file.dirname(),std::nothrow) )
		G::File::mkdirs( m_pointer_file.dirname() , std::nothrow ) ;

	// create the file
	std::ofstream stream ;
	G::File::open( stream , m_pointer_file , G::File::Text() ) ;

	// add the exec preamble
	if( !isWindows() )
	{
		stream << "#!/bin/sh\n" ;
		if( !m_gui_exe.empty() )
			stream << "exec \"`dirname \\\"$0\\\"`/" << m_gui_exe.basename() << "\" --qmdir=\"" << m_dir_tr << "\" \"$@\"\n" ;
	}

	// write the pointer variable(s)
	G::MapFile::writeItem( stream , "dir-config" , m_dir_config.str() ) ;
	G::MapFile::writeItem( stream , "dir-install" , m_dir_install.str() ) ;

	// close the file
	if( !stream.good() )
		throw TrError( tr("cannot write to file") , m_pointer_file.basename() ) ;
	stream.close() ;

	// make both files executable
	if( !isWindows() )
	{
		G::File::chmodx( m_pointer_file ) ;
		G::File::chmodx( m_gui_exe , std::nothrow ) ; // hopefully redundant
	}
}

trstring CreatePointerFile::text() const
{
	return tr("creating pointer file") ;
}

std::string CreatePointerFile::subject() const
{
	return m_pointer_file.str() ; // possibly empty
}

trstring CreatePointerFile::ok() const
{
	if( m_pointer_file.empty() )
		return tr("nothing to do") ;
	else
		return ActionBase::ok() ;
}

// ==

CopyPayloadFile::CopyPayloadFile( G::Path src , G::Path dst , std::string flags ) :
	m_src(src) ,
	m_dst(dst) ,
	m_flags(flags)
{
}

void CopyPayloadFile::run()
{
	G_LOG( "CopyPayloadFile::run: copy file [" << m_src << "] -> [" << m_dst << "]" ) ;
	G::File::mkdirs( m_dst.dirname() , std::nothrow , 8 ) ;
	G::File::copy( m_src , m_dst ) ;

	if( m_flags.find('x') != std::string::npos ||
		G::File::isExecutable(m_src,std::nothrow) ||
		m_dst.extension() == "sh" || m_dst.extension() == "bat" ||
		m_dst.extension() == "exe" || m_dst.extension() == "pl" )
			G::File::chmodx( m_dst ) ;
}

trstring CopyPayloadFile::text() const
{
	return tr("copying payload file") ;
}

std::string CopyPayloadFile::subject() const
{
	return m_dst.str() ;
}

// ==

CopyPayloadTree::CopyPayloadTree( G::Path src , G::Path dst ) :
	m_src(src) ,
	m_dst(dst)
{
}

void CopyPayloadTree::add( int depth , G::Path src_dir , G::Path dst_dir ) const
{
	if( depth > 10 ) return ;
	G::File::mkdir( dst_dir , std::nothrow ) ;
	G_LOG( "CopyPayloadTree::add: scanning [" << src_dir << "]" ) ;
	G::Directory d( src_dir ) ;
	G::DirectoryIterator iter( d ) ;
	while( iter.more() )
	{
		if( iter.isDir() )
		{
			G_LOG( "CopyPayloadTree::add: recursion: [" << iter.filePath() << "] [" << dst_dir << "] [" << iter.fileName() << "]" ) ;
			add( depth+1 , iter.filePath() , dst_dir+iter.fileName() ) ; // recurse
		}
		else
		{
			G::Path src = iter.filePath() ;
			G::Path dst = dst_dir + iter.fileName() ;
			G_LOG( "CopyPayloadTree::add: depth=" << depth << ": copy file [" << src << "] -> [" << dst << "]" ) ;
			G::File::copy( src , dst ) ;
			if( G::File::isExecutable(src,std::nothrow) ||
				dst.extension() == "sh" || dst.extension() == "bat" ||
				dst.extension() == "exe" || dst.extension() == "pl" )
					G::File::chmodx( dst ) ;
		}
	}
}

void CopyPayloadTree::run()
{
	G_LOG( "CopyPayloadTree::run: copy tree [" << m_src << "] -> [" << m_dst << "]" ) ;
	add( 0 , m_src , m_dst ) ;
}

trstring CopyPayloadTree::text() const
{
	return tr("copying payload directory") ;
}

std::string CopyPayloadTree::subject() const
{
	return m_dst.str() ;
}

// ==

FileGroup::FileGroup( const std::string & path , const std::string & tail ) :
	m_path(path) ,
	m_tail(tail)
{
}

void FileGroup::run()
{
	G::StringArray parts ;
	G::Str::splitIntoTokens( m_tail , parts , G::Str::ws() ) ; // eg: "daemon 755 g+s"
	if( !parts.empty() && !parts.at(0U).empty() )
		m_ok = G::File::chgrp( m_path , parts.at(0U) , std::nothrow ) ? "" : "failed" ;
	if( parts.size() > 1U )
		G::File::chmod( m_path , parts.at(1U) ) ;
	if( parts.size() > 2U )
		G::File::chmod( m_path , parts.at(2U) ) ;
}

trstring FileGroup::text() const
{
	return tr("setting group permissions") ;
}

std::string FileGroup::subject() const
{
	return m_path + " " + m_tail ;
}

trstring FileGroup::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

// ==

CreateSecrets::CreateSecrets( const std::string & config_dir , const std::string & filename ,
	G::Path template_ , const G::MapFile & p ) :
		m_path(config_dir,filename) ,
		m_template(template_)
{
	if( yes(p.value("do-pop")) )
	{
		addSecret( p , "server" , "pop-auth-mechanism" , "pop-account-1" ) ;
		addSecret( p , "server" , "pop-auth-mechanism" , "pop-account-2" ) ;
		addSecret( p , "server" , "pop-auth-mechanism" , "pop-account-3" ) ;
	}
	if( yes(p.value("do-smtp")) && yes(p.value("smtp-server-auth")) )
	{
		addSecret( p , "server" , "smtp-server-auth-mechanism" , "smtp-server-account" ) ;
		addSecret( p , "smtp-server-trust" ) ;
	}
	if( yes(p.value("do-smtp")) && yes(p.value("smtp-client-auth")) )
	{
		addSecret( p , "client" , "smtp-client-auth-mechanism" , "smtp-client-account" ) ;
	}
}

bool CreateSecrets::yes( const std::string & s )
{
	return G::Str::isPositive( s ) ;
}

void CreateSecrets::addSecret( const G::MapFile & p , const std::string & k )
{
	if( !p.value(k).empty() )
	{
		std::string address_range = p.value( k ) ;
		m_content.push_back( {
			"" ,
			"" ,
			"server none " + address_range + " trusted" } ) ;
	}
}

void CreateSecrets::addSecret( const G::MapFile & p ,
	const std::string & side , const std::string & /*k1*/ , const std::string & k2 )
{
	if( !p.value(k2+"-name").empty() )
	{
		// see pages.cpp -- mechanism is "plain", name and secret are base64
		//std::string mechanism = p.value( k1 ) ; // plain
		std::string name_base64 = p.value( k2+"-name" ) ;
		std::string secret_base64 = p.value( k2+"-password" ) ;
		std::string name_xtext = G::Xtext::encode( G::Base64::decode(name_base64) ) ;
		std::string secret_xtext = G::Xtext::encode( G::Base64::decode(secret_base64) ) ;
		if( side == "server" )
		{
			m_content.push_back( {
				"server plain:b " + name_base64 ,
				"server plain " + name_xtext ,
				"server plain:b " + name_base64 + " " + secret_base64 } ) ;
		}
		else
		{
			m_content.push_back( {
				"client plain:b" ,
				"client plain" ,
				"client plain:b " + name_base64 + " " + secret_base64 } ) ;
		}
	}
}

trstring CreateSecrets::text() const
{
	return tr("creating authentication secrets file") ;
}

std::string CreateSecrets::subject() const
{
	return m_path.str() ;
}

bool CreateSecrets::match( std::string p1 , std::string p2 )
{
	// true if p1 starts with p2
	if( p2.empty() ) return false ;
	G::Str::replaceAll( p1 , "\t" , " " ) ;
	while( G::Str::replaceAll( p1 , "  " , " " ) ) ;
	G::Str::trimLeft( p1 , G::Str::ws() ) ;
	G::Str::toLower( p1 ) ;
	G::Str::toLower( p2 ) ;
	return p1.find( p2 ) == 0U ;
}

void CreateSecrets::run()
{
	bool file_exists = G::File::exists(m_path) ;

	// read the old file
	G::StringArray line_list ;
	if( file_exists )
	{
		std::ifstream file ;
		G::File::open( file , m_path , G::File::Text() ) ;
		while( file.good() )
		{
			std::string line = G::Str::readLineFrom( file ) ;
			G::Str::trimRight( line , {"\r",1U} ) ;
			if( !file ) break ;
			line_list.push_back( line ) ;
		}
	}

	// write a header if none
	if( line_list.empty() )
	{
		if( !m_template.empty() && G::File::exists(m_template) )
		{
			std::ifstream file( m_template.cstr() ) ;
			while( file.good() )
			{
				std::string line = G::Str::readLineFrom( file ) ;
				G::Str::trimRight( line , {"\r",1U} ) ;
				if( !file ) break ;
				line_list.push_back( line ) ;
			}
		}
		if( line_list.empty() )
		{
			line_list.push_back( "#" ) ;
			line_list.push_back( "# " + m_path.basename() ) ;
			line_list.push_back( "#" ) ;
			line_list.push_back( "# client plain <name(xtext)> <password(xtext)>" ) ;
			line_list.push_back( "# client plain:b <name(base64)> <password(base64)>" ) ;
			line_list.push_back( "# client md5 <name(xtext)> <password-hash>" ) ;
			line_list.push_back( "# server plain <name(xtext)> <password(xtext)>" ) ;
			line_list.push_back( "# server plain:b <name(base64)> <password(base64)>" ) ;
			line_list.push_back( "# server md5 <name(xtext)> <password-hash>" ) ;
			line_list.push_back( "# server none <address-range> <verifier-keyword>" ) ;
			line_list.push_back( "#" ) ;
		}
	}

	// assemble the new file
	for( const auto & map_item : m_content )
	{
		bool replaced = false ;
		for( auto & line : line_list )
		{
			if( match(line,map_item.key) || match(line,map_item.key2) )
			{
				line = map_item.line ;
				replaced = true ;
				break ;
			}
		}
		if( !replaced )
		{
			line_list.push_back( map_item.line ) ;
		}
	}

	// make a backup -- ignore errors for now
	if( file_exists )
	{
		G::BrokenDownTime now = G::SystemTime::now().local() ;
		std::string timestamp = G::Date(now).str(G::Date::Format::yyyy_mm_dd) + G::Time(now).hhmmss() ;
		G::Path backup_path( m_path.dirname() , m_path.basename() + "." + timestamp ) ;
		G::Process::Umask umask( G::Process::Umask::Mode::Tightest ) ;
		G::File::copy( m_path , backup_path , std::nothrow ) ;
	}

	// write the new file
	std::ofstream file ;
	G::File::open( file , m_path, G::File::Text() ) ;
	bool ok = file.good() ;
	for( const auto & line : line_list )
	{
		file << line << std::endl ;
	}
	ok = ok && file.good() ;
	file.close() ;
	if( !ok )
		throw TrError( tr("cannot create file") , m_path.basename() ) ;
}

// ==

CreateBatchFile::CreateBatchFile( const G::Path & bat , const G::Path & exe , const G::StringArray & args ) :
	m_bat(bat) ,
	m_exe(exe) ,
	m_args(args)
{
}

trstring CreateBatchFile::text() const
{
	return tr("creating startup batch file") ;
}

std::string CreateBatchFile::subject() const
{
	return m_bat.str() ;
}

void CreateBatchFile::run()
{
	G::StringArray all_args = m_args ;
	all_args.insert( all_args.begin() , m_exe.str() ) ;
	G::BatchFile::write( m_bat , all_args , "emailrelay" ) ;
}

// ==

UpdateLink::UpdateLink( LinkType link_type , bool active , G::Path link_dir , G::Path working_dir ,
	G::Path target , const G::StringArray & args , G::Path icon ) :
		m_link_type(link_type) ,
		m_active(active) ,
		m_link_dir(link_dir) ,
		m_working_dir(working_dir) ,
		m_target(target) ,
		m_args(args) ,
		m_icon(icon)
{
	std::string link_filename = Gui::Link::filename( "E-MailRelay" ) ;
	m_link_path = G::Path( m_link_dir , link_filename ) ;
}

trstring UpdateLink::text() const
{
	if( m_link_type == LinkType::Desktop ) return tr("updating destkop link") ;
	if( m_link_type == LinkType::StartMenu ) return tr("updating start menu link") ;
	if( m_link_type == LinkType::AutoStart ) return tr("updating autostart link") ;
	if( m_link_type == LinkType::BatchFile ) return tr("updating program-files link") ;
	return tr("updating link" ) ;
}

std::string UpdateLink::subject() const
{
	return m_link_dir.str() ; // possibly empty
}

void UpdateLink::run()
{
	GDEF_UNUSED GComInit com_init ;
	if( m_active )
	{
		Gui::Link link( m_target , "E-MailRelay" , "Starts the E-MailRelay server in the background" ,
			m_working_dir , m_args , m_icon , Gui::Link::Show::Hide ,
			"E-MailRelay" , "Generated by the E-MailRelay configuration GUI" ) ;

		//G::Process::Umask umask( G::Process::Umask::Mode::Tightest ) ;
		G::File::mkdirs( m_link_dir , 10 ) ;
		link.saveAs( m_link_path ) ;
	}
	else
	{
		m_ok = Gui::Link::remove( m_link_path ) ? tr("removed") : tr("nothing to do") ;
	}
}

trstring UpdateLink::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

// ==

InstallService::InstallService( bool active , bool start_on_boot , G::Path bat , G::Path service_wrapper ) :
	m_active(active) ,
	m_start_on_boot(start_on_boot) ,
	m_bat(bat) ,
	m_service_wrapper(service_wrapper)
{
}

void InstallService::run()
{
	if( !m_active )
	{
		m_ok = tr("not possible") ; // see Gui::Boot::installable()
	}
	else if( m_bat.empty() || m_service_wrapper.empty() )
	{
		m_ok = tr("nothing to do") ;
	}
	else if( m_start_on_boot )
	{
		Gui::Boot::install( "emailrelay" , m_bat , m_service_wrapper ) ;
	}
	else
	{
		bool ok = Gui::Boot::uninstall( "emailrelay" , m_bat , m_service_wrapper ) ;
		m_ok = ok ? tr("uninstalled") : tr("nothing to do") ;
	}
}

trstring InstallService::text() const
{
	return (!m_active || m_start_on_boot) ? tr("installing service") : tr("uninstalling service") ;
}

std::string InstallService::subject() const
{
	return std::string() ;
}

trstring InstallService::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

// ==

UpdateBootLink::UpdateBootLink( bool active , bool start_on_boot , std::string name , G::Path startstop_src , G::Path exe ) :
	m_active(active) ,
	m_start_on_boot(start_on_boot) ,
	m_name(name) ,
	m_startstop_src(startstop_src) ,
	m_exe(exe)
{
}

trstring UpdateBootLink::text() const
{
	return tr("updating boot configuration") ;
}

std::string UpdateBootLink::subject() const
{
	return m_name ;
}

void UpdateBootLink::run()
{
	if( !m_active )
	{
		m_ok = tr("not possible") ; // see Gui::Boot::installable()
	}
	else if( m_startstop_src.empty() || m_exe.empty() )
	{
		m_ok = tr("nothing to do") ;
	}
	else if( m_start_on_boot )
	{
		Gui::Boot::install( m_name , m_startstop_src , m_exe ) ;
	}
	else
	{
		bool removed = Gui::Boot::uninstall( m_name , m_startstop_src , m_exe ) ;
		m_ok = removed ? tr("removed") : tr("nothing to remove") ;
	}
}

trstring UpdateBootLink::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

// ==

RegisterAsEventSource::RegisterAsEventSource( const G::Path & exe ) :
	m_exe(exe)
{
}

void RegisterAsEventSource::run()
{
	if( !m_exe.empty() )
		G::LogOutput::register_( m_exe.str() ) ;
}

trstring RegisterAsEventSource::text() const
{
	return tr("registering as a source for event viewer logging") ;
}

std::string RegisterAsEventSource::subject() const
{
	return m_exe.str() ;
}

// ==

CreateFilterScript::CreateFilterScript( const G::Path & path , bool client_filter ) :
	m_path(path) ,
	m_client_filter(client_filter)
{
}

void CreateFilterScript::run()
{
	if( m_path.empty() ||
		G::Str::headMatch( m_path.str() , "copy:" ) ||
		G::Str::headMatch( m_path.str() , "spam-edit:" ) )
	{
		m_ok = tr("nothing to do") ;
	}
	else if( G::File::exists(m_path) )
	{
		m_ok = tr("exists") ;
	}
	else
	{
		std::ofstream f ;
		G::File::open( f , m_path , G::File::Text() ) ;
		if( isWindows() )
			f << "WScript.Quit(0);" << std::endl ;
		else
			f << "#!/bin/sh\nexit 0" << std::endl ;
		if( !f.good() )
			throw TrError( tr("cannot write to file") , m_path.basename() ) ;
		f.close() ;
		if( !isWindows() )
			G::File::chmodx( m_path ) ;
	}
}

trstring CreateFilterScript::text() const
{
	if( m_client_filter )
		return tr("creating client filter script") ;
	else
		return tr("creating filter script") ;
}

std::string CreateFilterScript::subject() const
{
	return m_path.str() ;
}

trstring CreateFilterScript::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

// ==

CreateConfigFile::CreateConfigFile( G::Path dst_dir , std::string dst_name , G::Path template_ ) :
	m_template(template_) ,
	m_dst(dst_dir+dst_name)
{
}

void CreateConfigFile::run()
{
	if( G::File::exists(m_dst) )
		m_ok = tr("exists") ;
	else if( G::File::exists(m_template) )
		G::File::copy( m_template , m_dst ) ;
	else
		G::File::create( m_dst ) ;
}

trstring CreateConfigFile::text() const
{
	return tr("creating configuration file") ;
}

std::string CreateConfigFile::subject() const
{
	return m_dst.str() ;
}

trstring CreateConfigFile::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

// ==

EditConfigFile::EditConfigFile( G::Path dir , std::string name , const G::MapFile & server_config , bool do_backup ) :
	m_path(dir+name) ,
	m_server_config(server_config) ,
	m_do_backup(do_backup)
{
}

void EditConfigFile::run()
{
	const bool allow_read_error = false ;
	const bool allow_write_error = false ;
	m_server_config.editInto( m_path , m_do_backup , allow_read_error , allow_write_error ) ;
}

trstring EditConfigFile::text() const
{
	return tr("editing configuration file") ;
}

std::string EditConfigFile::subject() const
{
	return m_path.str() ;
}

// ==

GenerateKey::GenerateKey( G::Path path_out , const std::string & issuer ) :
	m_exe(exe(isWindows())) ,
	m_path_out(path_out) ,
	m_issuer(issuer)
{
}

G::Path GenerateKey::exe( bool is_windows )
{
	// (guimain.cpp tests for this binary and tells the gui 'smtp server' page)

	std::string this_exe = G::Process::exe() ;
	if( this_exe.empty() )
		return {} ;

	G::Path dir = G::Path(this_exe).dirname() ;
	std::string filename = is_windows ? "emailrelay-keygen.exe" : "emailrelay-keygen" ;

	if( G::File::exists(dir+"programs"+filename) )
		return dir + "programs" + filename ;
	else
		return dir + filename ;
}

void GenerateKey::run()
{
	G::NewProcess task( m_exe , {m_issuer,m_path_out.str()} ,
		G::NewProcess::Config()
			.set_stdout( G::NewProcess::Fd::devnull() )
			.set_stderr( G::NewProcess::Fd::pipe() )
			.set_exec_error_format( "failed to execute ["+m_exe.str()+"]: __""strerror""__" ) ) ;

	int rc = task.waitable().wait().get() ;
	std::string output = G::Str::printable( G::Str::trimmed(task.waitable().output(),G::Str::ws()) ) ;
	if( rc != 0 && output.empty() )
		output = "exit " + G::Str::fromInt(rc) ;

	if( rc != 0 )
		throw std::runtime_error( output ) ;
}

trstring GenerateKey::text() const
{
	return tr("generating tls server key") ;
}

std::string GenerateKey::subject() const
{
	return m_path_out.str() ;
}

// ==

Launcher::Launcher( bool as_service , const G::Path & bat , const G::Path & exe , const G::Path & config_file ) :
	m_as_service(as_service)
{
	if( m_as_service )
	{
		m_text = tr("starting service") ;
	}
	else if( isWindows() )
	{
		m_cmd = G::ExecutableCommand( bat , G::StringArray() ) ;
		m_text = tr("running startup batch file") ;
		m_subject = bat.str() ;
		m_ok = tr("done") ; // since not necessarily 'ok'
	}
	else
	{
		m_cmd = G::ExecutableCommand( exe , {config_file.str()} ) ;
		m_text = tr("running") ;
		m_subject = exe.str() + " " + config_file.str() ;
	}
}

void Launcher::run()
{
	if( m_as_service )
	{
		Gui::Boot::launch( "emailrelay" ) ;
	}
	else
	{
		// (keep it simple -- a console window will pop up to show
		// problems running the batch file and the emailrelay log
		// file is tail-ed by ProgressPage -- using G::NewProcess
		// doesn't work well because stderr is not always inherited
		// and then closed cleanly)
		std::string s = m_cmd.exe().str() ;
		(void) system( s.c_str() ) ; // NOLINT
	}
}

trstring Launcher::text() const
{
	return m_text ;
}

std::string Launcher::subject() const
{
	return m_subject ;
}

trstring Launcher::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

// ==

JustTesting::JustTesting()
= default ;

trstring JustTesting::ok() const
{
	return empty(m_ok) ? ActionBase::ok() : m_ok ;
}

trstring JustTesting::text() const
{
	//: random text used in testing
	return tr("doing something") ;
}

std::string JustTesting::subject() const
{
	return
		G::Test::enabled("installer-test-subject") ?
			std::string("/some/directory") :
			std::string() ;
}

void JustTesting::run()
{
	if( G::Test::enabled("installer-test-nop") )
		//: random text used in testing
		m_ok = tr("nothing to do") ;
	if( G::Test::enabled("installer-test-throw") )
		//: random text used in testing
		throw TrError( tr("some error") ) ;
	if( G::Test::enabled("installer-test-throw-with-subject") )
		//: random text used in testing
		throw TrError( tr("another error") , "/some/file" ) ;
	if( G::Test::enabled("installer-test-throw-native") )
		throw std::runtime_error( "failed to do something to /some/file" ) ;
}

// ==

trstring ActionBase::ok() const
{
	return QCoreApplication::translate( "Installer" , "ok" ) ;
}

// ==

Action::Action( ActionInterface * p ) :
	m_p(p)
{
}

trstring Action::text() const
{
	return m_p->text() ;
}

std::string Action::subject() const
{
	return m_p->subject() ;
}

trstring Action::ok() const
{
	return m_p->ok() ;
}

void Action::run()
{
	m_p->run() ;
}

// ==

InstallerImp::InstallerImp( bool installing , bool is_windows , bool is_mac , const G::Path & payload , std::istream & ss ) :
	m_installing(installing) ,
	m_payload(payload) ,
	m_pages_output(ss) ,
	m_have_run(false)
{
	Helper::m_is_windows = is_windows ;
	Helper::m_is_mac = is_mac ;

	// define ivalue o/s-specific paths
	m_installer_config.add( "-authtemplate" , isWindows() ? "" : "%payload%/usr/lib/emailrelay/emailrelay.auth.in" ) ;
	m_installer_config.add( "-conftemplate" , isWindows() ? "" : "%payload%/usr/lib/emailrelay/emailrelay.conf.in" ) ;
	m_installer_config.add( "-bat" , isWindows() ? "%dir-config%/emailrelay-start.bat" : "" ) ; // not dir-install -- see guimain
	m_installer_config.add( "-exe" , isWindows() ? "%dir-install%/emailrelay.exe" :
		( isMac() ? "%dir-install%/E-MailRelay.app/Contents/MacOS/emailrelay" : "%dir-install%/sbin/emailrelay" ) ) ;
	m_installer_config.add( "-gui" , isWindows() ? "%dir-install%/emailrelay-gui.exe" : "%dir-install%/sbin/emailrelay-gui.real" ) ;
	m_installer_config.add( "-icon" , isWindows()?"%dir-install%/emailrelay.exe":"%dir-install%/share/emailrelay/emailrelay-icon.png");
	m_installer_config.add( "-trdir" , isWindows()?"%dir-install%/translations":"%dir-install%/share/emailrelay") ;
	m_installer_config.add( "-pointer" , isWindows() ? "%dir-install%/emailrelay-gui.cfg" : "%dir-install%/sbin/emailrelay-gui" ) ;
	m_installer_config.add( "-startstop" , isWindows() ? "" : "%dir-install%/etc/init.d/emailrelay" ) ;
	m_installer_config.add( "-servicewrapper" , isWindows() ? "%dir-install%/emailrelay-service.exe" : "" ) ;

	// define some substitution variables (used for expansion of pvalues, ivalues and payload.cfg)
	m_var.add( "dir-install" , pvalue("dir-install") ) ;
	m_var.add( "dir-config" , pvalue("dir-config") ) ;
	m_var.add( "dir-run" , pvalue("dir-run") ) ;
	m_var.add( "dir-spool" , pvalue("dir-spool") ) ;
	m_var.add( "payload" , m_payload.str() ) ;

	addActions() ;
	m_p = m_list.end() ;
}

InstallerImp::~InstallerImp()
= default;

bool InstallerImp::next()
{
	m_output = Installer::Output() ;
	if( m_list.empty() )
	{
		return false ;
	}
	else if( m_p == m_list.end() ) // inc. new
	{
		m_p = m_list.begin() ;
		m_output.action_utf8 = from_trstring( (*m_p).text() ) ;
		m_output.subject = (*m_p).subject() ;
		return true ;
	}
	else
	{
		++m_p ;
		if( m_p != m_list.end() )
		{
			m_output.action_utf8 = from_trstring( (*m_p).text() ) ;
			m_output.subject = (*m_p).subject() ;
		}
		return m_p != m_list.end() ;
	}
}

void InstallerImp::back()
{
	m_output = Installer::Output() ;
	if( m_list.empty() || m_p == m_list.begin() )
	{
	}
	else if( m_p == m_list.end() )
	{
		m_p = m_list.begin() ;
		std::advance( m_p , m_list.size()-1U ) ;
		m_output.action_utf8 = from_trstring( (*m_p).text() ) ;
		m_output.subject = (*m_p).subject() ;
	}
	else
	{
		--m_p ;
		m_output.action_utf8 = from_trstring( (*m_p).text() ) ;
		m_output.subject = (*m_p).subject() ;
	}
}

bool InstallerImp::failed() const
{
	return m_have_run && ( !m_output.error.empty() || !m_output.error_utf8.empty() ) ;
}

bool InstallerImp::done()
{
	return m_p == m_list.end() ;
}

Action & InstallerImp::current()
{
	return *m_p ;
}

std::string InstallerImp::pvalue( const std::string & key , const std::string & default_ ) const
{
	return m_var.expand( m_pages_output.value( key , default_ ) ) ;
}

std::string InstallerImp::pvalue( const std::string & key ) const
{
	return m_var.expand( m_pages_output.value( key ) ) ;
}

std::string InstallerImp::ivalue( const std::string & key ) const
{
	return m_var.expand( m_installer_config.value( key ) ) ;
}

bool InstallerImp::exists( const std::string & key ) const
{
	return m_pages_output.contains( key ) ;
}

bool InstallerImp::yes( const std::string & value )
{
	return G::Str::isPositive(value) ;
}

bool InstallerImp::no( const std::string & value )
{
	return !yes( value ) ;
}

void InstallerImp::addAction( ActionInterface * p )
{
	m_list.push_back( Action(p) ) ;
}

void InstallerImp::addActions()
{
	// create base directories
	//
	if( m_installing )
	{
		addAction( new CreateDirectory(tr("install"),pvalue("dir-install"),true) ) ;
		addAction( new CreateDirectory(tr("configuration"),pvalue("dir-config")) ) ;
	}
	addAction( new CreateDirectory(tr("runtime"),pvalue("dir-run")) ) ;
	addAction( new CreateDirectory(tr("spool"),pvalue("dir-spool")) ) ;

	// create pop-by-name sub-directories
	//
	{
		G::Path spool_dir( pvalue("dir-spool") ) ;
		std::vector<std::string> names ;
		names.push_back( G::Xtext::encode( G::Base64::decode(pvalue("pop-account-1-name")) ) ) ;
		names.push_back( G::Xtext::encode( G::Base64::decode(pvalue("pop-account-2-name")) ) ) ;
		names.push_back( G::Xtext::encode( G::Base64::decode(pvalue("pop-account-3-name")) ) ) ;
		for( const auto & name : names )
		{
			if( name.empty() ) continue ;
			G::Path dir( spool_dir , name ) ;
			addAction( new CreateDirectory(tr("pop-by-name"),dir.str()) ) ;
		}
	}

	// process the payload -- the payload is a directory including a
	// config file ("payload.cfg") like this:
	//
	//   pkgdir/filename= %dir-install%/bin/filename +x
	//   pkgdir/subdir/= %dir-install%/subdir/
	//   +%dir-install%/foo group daemon 775 g+s
	//
	if( m_installing )
	{
		// read the contents
		G::MapFile payload_map( m_payload + "payload.cfg" ) ;

		// add the file copy tasks
		G::StringArray keys = payload_map.keys() ;
		for( const auto & key : keys )
		{
			const std::string & value = payload_map.value( key ) ;
			if( key.find('+') == 0U && key.length() > 1U && value.find("group ") == 0U && value.length() > 6U )
				addAction( new FileGroup(m_var.expand(key.substr(1U)),value.substr(6U)) ) ;
			if( key.find_first_of("-+=") == 0U )
				continue ;

			std::string dst = m_var.expand( value ) ;
			bool is_directory_tree = !key.empty() && key.at(key.size()-1U) == '/' ;

			// allow for flags such as "+x" decorating the destination
			std::string::size_type flags_pos = dst.find_last_of('+') ;
			std::string flags = G::Str::tail( dst , flags_pos , std::string() ) ;
			dst = G::Str::trimmed( G::Str::head( dst , flags_pos , dst ) , G::Str::ws() ) ;

			G::Path src = m_payload + key ;
			if( is_directory_tree )
				addAction( new CopyPayloadTree(src,dst) ) ;
			else
				addAction( new CopyPayloadFile(src,dst,flags) ) ;
		}
	}

	// create secrets
	//
	// (this would be better using a separate file for the pop secrets)
	//
	G::Path authtemplate_src = m_installing ? ivalue( "-authtemplate" ) : std::string() ;
	addAction( new CreateSecrets(pvalue("dir-config"),"emailrelay.auth",authtemplate_src,m_pages_output) ) ;

	// create the pointer file so that the gui program can be used to re-configure
	//
	if( m_installing )
	{
		G::Path pointer_file = ivalue( "-pointer" ) ;
		G::Path gui_exe = ivalue( "-gui" ) ;
		G::Path dir_config = pvalue( "dir-config" ) ;
		G::Path dir_install = pvalue( "dir-install" ) ;
		G::Path dir_tr = ivalue( "-trdir" ) ;
		addAction( new CreatePointerFile(pointer_file,gui_exe,dir_config,dir_install,dir_tr) ) ;
	}

	// register for using the windows event log - doing it here since the server
	// will not have administrator privilege
	//
	if( m_installing && isWindows() )
	{
		addAction( new RegisterAsEventSource(ivalue("-exe")) ) ;
	}

	// create filter scripts
	//
	if( m_installing )
	{
		if( !pvalue("filter-server").empty() )
		{
			addAction( new CreateDirectory( tr("filter") , G::Path(pvalue("filter-server")).dirname().str() ) ) ;
			addAction( new CreateFilterScript( pvalue("filter-server") , false ) ) ;
		}

		if( !pvalue("filter-client").empty() )
		{
			addAction( new CreateDirectory( tr("client-filter") , G::Path(pvalue("filter-client")).dirname().str() ) ) ;
			addAction( new CreateFilterScript( pvalue("filter-client") , true ) ) ;
		}
	}

	// generate tls certificates
	if( m_installing )
	{
		bool server_tls = yes(pvalue("smtp-server-tls")) || yes(pvalue("smtp-server-tls-connection")) ;
		if( server_tls && pvalue("smtp-server-tls-certificate").empty() )
		{
			G::Path path_out = G::Path(pvalue("dir-config")) + "emailrelay-install.pem" ;
			addAction( new GenerateKey(path_out,"CN=example.com") ) ;
			m_pages_output.add( "smtp-server-tls-certificate" , path_out.str() , true ) ;
		}
	}

	// update the configuration
	//
	if( isWindows() )
	{
		G::Path exe = ivalue( "-exe" ) ;
		G::Path bat = ivalue( "-bat" ) ;
		G::Path dir_install = pvalue( "dir-install" ) ;
		G::Path working_dir = pvalue( "dir-config" ) ;
		G::Path target = ivalue( "-bat" ) ;
		G::Path icon = ivalue( "-icon" ) ;
		G::StringArray args = ServerConfiguration::fromPages(m_pages_output).args() ;
		addAction( new CreateBatchFile(bat,exe,args) ) ;
		addAction( new UpdateLink(UpdateLink::LinkType::BatchFile,true,dir_install,working_dir,target,args,icon) ) ;
	}
	else
	{
		G::Path dir_config = pvalue( "dir-config" ) ;
		G::Path conftemplate_src = m_installing ? ivalue( "-conftemplate" ) : std::string() ;
		G::MapFile server_config = ServerConfiguration::fromPages(m_pages_output).map() ;
		addAction( new CreateConfigFile(dir_config,"emailrelay.conf",conftemplate_src) ) ;
		addAction( new EditConfigFile(dir_config,"emailrelay.conf",server_config,!m_installing) ) ;
	}

	// create startup links
	//
	if( yes(pvalue("start-page")) ) // if the startup page was presented at all -- see guimain.cpp
	{
		G::Path server_exe = ivalue( "-exe" ) ;
		G::Path working_dir = pvalue( "dir-config" ) ;

		G::Path dir_desktop = pvalue( "dir-desktop" ) ;
		G::Path dir_menu = pvalue( "dir-menu" ) ;
		G::Path dir_login = pvalue( "dir-login" ) ;

		G::Path bat = ivalue( "-bat" ) ;
		G::Path target = isWindows() ? bat : server_exe ;
		G::StringArray args = isWindows() ? G::StringArray() : ServerConfiguration::fromPages(m_pages_output).args() ;
		G::Path icon = ivalue( "-icon" ) ;

		bool desktop_state = yes(pvalue("start-link-desktop")) && !yes(pvalue("start-is-mac")) ;
		bool menu_state = yes(pvalue("start-link-menu")) && !yes(pvalue("start-is-mac")) ;
		bool login_state = yes(pvalue("start-at-login")) ;
		bool do_boot_update = yes(pvalue("start-on-boot-enabled")) ; // see Gui::Boot::installable()
		bool boot_state = yes(pvalue("start-on-boot")) ;

		addAction( new UpdateLink(UpdateLink::LinkType::Desktop,desktop_state,dir_desktop,working_dir,target,args,icon) ) ;
		addAction( new UpdateLink(UpdateLink::LinkType::StartMenu,menu_state,dir_menu,working_dir,target,args,icon) ) ;
		addAction( new UpdateLink(UpdateLink::LinkType::AutoStart,login_state,dir_login,working_dir,target,args,icon) ) ;

		if( isWindows() )
		{
			G::Path service_wrapper = ivalue( "-servicewrapper" ) ;
			addAction( new InstallService(do_boot_update,boot_state,bat,service_wrapper) ) ;
		}
		else
		{
			addAction( new UpdateBootLink(do_boot_update,boot_state,"emailrelay",ivalue("-startstop"),server_exe) ) ;
		}
	}

	// testing
	if( G::Test::enabled("installer-test") )
		addAction( new JustTesting ) ;
}

G::Path InstallerImp::addLauncher()
{
	G::Path bat = ivalue( "-bat" ) ;
	G::Path exe = ivalue( "-exe" ) ;
	G::Path dir_config = pvalue( "dir-config" ) ;
	G::Path config_file = dir_config + "emailrelay.conf" ;
	bool as_service = yes( pvalue("start-on-boot") ) ;

	std::size_t list_size = m_list.size() ;
	addAction( new Launcher( as_service , bat , exe , config_file ) ) ;
	m_p = list_size ? m_list.begin() : m_list.end() ;
	if( list_size )
		std::advance( m_p , list_size-1U ) ;

	std::string log = pvalue( "logging-file" ) ;
	G::Str::replaceAll( log , "%d" , G::Date(G::Date::LocalTime()).str(G::Date::Format::yyyy_mm_dd) ) ;
	return log ;
}

void InstallerImp::run()
{
	try
	{
		m_have_run = true ;
		m_output.result_utf8.clear() ;
		m_output.error.clear() ;
		m_output.error_utf8.clear() ;
		current().run() ;
		m_output.result_utf8 = from_trstring( current().ok() ) ;
	}
	catch( TrError & e )
	{
		m_output.error_utf8 = from_trstring( e.m_text ) ;
		m_output.error = e.m_subject ;
	}
	catch( std::exception & e )
	{
		m_output.error = e.what() ;
	}
}

Installer::Output InstallerImp::output()
{
	return m_output ;
}

// ==

Installer::Installer( bool installing , bool is_windows , bool is_mac , const G::Path & payload ) :
	m_installing(installing) ,
	m_is_windows(is_windows) ,
	m_is_mac(is_mac&&!is_windows) ,
	m_payload(payload)
{
}

Installer::~Installer()
= default ;

void Installer::start( std::istream & input_stream )
{
	m_imp = std::make_unique<InstallerImp>( m_installing , m_is_windows , m_is_mac , m_payload , input_stream ) ;
}

bool Installer::next()
{
	if( !m_imp || failed() )
		return false ;
	return m_imp->next() ;
}

void Installer::back( int n )
{
	for( int i = 0 ; m_imp && i < n ; i++ )
		m_imp->back() ;
}

Installer::Output Installer::output()
{
	return m_imp ? m_imp->output() : Output() ;
}

void Installer::run()
{
	if( m_imp )
		m_imp->run() ;
}

bool Installer::failed() const
{
	return m_imp && m_imp->failed() ;
}

bool Installer::done() const
{
	return failed() || const_cast<Installer*>(this)->doneImp() ;
}

bool Installer::doneImp()
{
	return m_imp && m_imp->done() ;
}

bool Installer::canGenerateKey()
{
	return G::File::exists( GenerateKey::exe(m_is_windows) ) ;
}

G::Path Installer::addLauncher()
{
	return m_imp ? m_imp->addLauncher() : G::Path() ;
}

std::string Installer::failedText() const
{
	trstring failed = QCoreApplication::translate( "Installer" , "** failed **" ) ;
	return from_trstring(failed) ;
}

std::string Installer::finishedText() const
{
	trstring finished = QCoreApplication::translate( "Installer" , "== finished ==" ) ;
	return from_trstring(finished) ;
}

// ==

bool Helper::isWindows()
{
	return m_is_windows ;
}

bool Helper::isMac()
{
	return m_is_mac ;
}

