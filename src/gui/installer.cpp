//
// Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// installer.cpp
//

#include "gdef.h"
#include "glog.h"
#include "gstr.h"
#include "gpath.h"
#include "gfile.h"
#include "gstr.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "gcominit.h"
#include "glink.h"
#include "garg.h"
#include "gunpack.h"
#include "package.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "glog.h"
#include "gexception.h"
#include "installer.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <map>
#include <set>
#include <list>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

struct LinkInfo
{
	G::Path target ;
	G::Strings args ;
	G::Path raw_target ;
	G::Strings raw_args ;
} ;

struct ActionInterface
{
	virtual void run() = 0 ;
	virtual std::string text() const = 0 ;
	protected: virtual ~ActionInterface() {}
} ;

struct Helper
{
	static bool isWindows() ;
	static bool isMac() ;
	static std::string exe() ;
	static std::string quote( const std::string & ) ;
	static std::string str( const G::Strings & list ) ;
} ;

struct ActionBase : public ActionInterface , protected Helper
{
} ;

// ==
struct CreateDirectory : public ActionBase
{
	std::string m_display_name ;
	G::Path m_path ;
	CreateDirectory( std::string display_name , std::string path , std::string sub_path = std::string() ) ;
	std::string text() const ;
	void run() ;
} ;
CreateDirectory::CreateDirectory( std::string display_name , std::string path , std::string sub_path ) :
	m_display_name(display_name) ,
	m_path(sub_path.empty()?path:G::Path::join(path,sub_path))
{
}
std::string CreateDirectory::text() const
{
	return std::string() + "creating " + m_display_name + " directory [" + m_path.str() + "]" ;
}
void CreateDirectory::run()
{
	G::Directory dir( m_path ) ;
	if( G::File::exists(m_path) )
	{
		if( !dir.valid() )
			throw std::runtime_error( "directory path exists but not valid a directory" ) ;
	}
	else
	{
		G::File::mkdirs( m_path , 10 ) ;
	}
	if( !dir.writeable() )
		throw std::runtime_error( "directory exists but is not writable" ) ;
}
// ==
struct ExtractOriginal : public ActionBase
{
	G::Unpack & m_unpack ;
	G::Path m_src ;
	G::Path m_dst_dir ;
	G::Path m_dst ;
	ExtractOriginal( G::Unpack & unpack , std::string install_dir ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;
ExtractOriginal::ExtractOriginal( G::Unpack & unpack , std::string install_dir ) :
	m_unpack(unpack) ,
	m_dst_dir(install_dir)
{
	m_dst = G::Path( m_dst_dir , m_unpack.path().basename() ) ;
}
void ExtractOriginal::run() 
{
	m_unpack.unpackOriginal( m_dst ) ;
}
std::string ExtractOriginal::text() const 
{ 
	return std::string() + "creating [" + m_dst.str() + "]" ; 
}
// ==
struct Copy : public ActionBase
{
	G::Path m_dst_dir ;
	std::string m_name ;
	Copy( std::string install_dir , std::string name , std::string sub_dir = std::string() ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;
Copy::Copy( std::string install_dir , std::string name , std::string sub_dir ) :
	m_dst_dir(sub_dir.empty()?install_dir:G::Path(install_dir,sub_dir)) ,
	m_name(name)
{
}
void Copy::run()
{
	G::File::copy( m_name , G::Path(m_dst_dir,m_name) ) ;
}
std::string Copy::text() const
{
	return std::string() + "copying [" + m_name + "] -> [" + m_dst_dir.str() + "]" ;
}
// ==
struct Extract : public ActionBase
{
	G::Unpack & m_unpack ;
	G::Path m_dst_dir ;
	G::Path m_name ;
	Extract( G::Unpack & unpack , std::string install_dir , G::Path name ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;
Extract::Extract( G::Unpack & unpack , std::string install_dir , G::Path name ) :
	m_unpack(unpack) ,
	m_dst_dir(install_dir) ,
	m_name(name)
{
}
void Extract::run()
{
	m_unpack.unpack( m_dst_dir , m_name.str() ) ;
}
std::string Extract::text() const
{
	G::Path path = G::Path::join( m_dst_dir , m_name ) ;
	return "extracting [" + path.basename() + "] to [" + path.dirname().str() + "]" ;
}
// ==
struct CreateSecrets : public ActionBase
{
	G::Path m_path ;
	std::string m_content ;
	CreateSecrets( const std::string & config_dir , const std::string & filename , const std::string & content ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;
CreateSecrets::CreateSecrets( const std::string & config_dir , const std::string & filename , 
	const std::string & content ) :
		m_path(config_dir,filename) ,
		m_content(content)
{
}
std::string CreateSecrets::text() const
{
	return std::string() + "creating authentication secrets file [" + m_path.str() + "]" ;
}
void CreateSecrets::run()
{
	std::ofstream file( m_path.str().c_str() ) ;
	bool ok = file.good() ;
	file << m_content ;
	ok = ok && file.good() ;
	file.close() ;
	if( !ok )
			throw std::runtime_error(std::string()+"cannot create \""+m_path.str()+"\"") ;
}
// ==
struct CreateBatchFile : public ActionBase
{
	LinkInfo m_link_info ;
	std::string m_args ;
	CreateBatchFile( LinkInfo ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;
CreateBatchFile::CreateBatchFile( LinkInfo link_info ) :
	m_link_info(link_info)
{
}
std::string CreateBatchFile::text() const
{
	return std::string() + "creating batch file [" + m_link_info.target.str() + "]" ;
}
void CreateBatchFile::run()
{
	std::ofstream file( m_link_info.target.str().c_str() ) ;
	bool ok = file.good() ;
	file << quote(m_link_info.raw_target.str()) << " " << str(m_link_info.raw_args) << std::endl ;
	ok = ok && file.good() ;
	file.close() ;
	if( !ok )
		throw std::runtime_error(std::string()+"cannot create \""+m_link_info.target.str()+"\"") ;
}
// ==
struct CreateLink : public ActionBase
{
	G::Path m_link_dir ;
	G::Path m_working_dir ;
	LinkInfo m_target_link_info ;
	G::Path m_icon_path ;
	CreateLink( std::string link_dir , G::Path working_dir , LinkInfo target_link_info ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;
CreateLink::CreateLink( std::string link_dir , G::Path working_dir , LinkInfo target_link_info ) :
	m_link_dir(link_dir) ,
	m_working_dir(working_dir) ,
	m_target_link_info(target_link_info) ,
	m_icon_path(target_link_info.target.dirname(),"emailrelay-icon.png")
{
	if( isWindows() )
		m_icon_path = target_link_info.raw_target ; // get the icon from the exe resource
}
std::string CreateLink::text() const
{
	return std::string() + "creating link in [" + m_link_dir.str() + "]" ;
}
void CreateLink::run()
{
	new GComInit ;

	std::string link_filename = GLink::filename( "E-MailRelay" ) ;
	G::Path link_path( m_link_dir , link_filename ) ;

	GLink link( m_target_link_info.target , "E-MailRelay" , "E-MailRelay server" , 
		m_working_dir , str(m_target_link_info.args) , m_icon_path , GLink::Show_Hide ) ;

	G::Process::Umask umask( G::Process::Umask::Tightest ) ;
	G::File::mkdirs( m_link_dir , 10 ) ;
	link.saveAs( link_path ) ;
}
// ==

struct Action
{
	ActionInterface * m_p ; // sould do reference counting, but just leak for now
	explicit Action( ActionInterface * p ) ;
	std::string text() const ;
	void run() ;
} ;

class InstallerImp : private Helper 
{
public:
	InstallerImp( G::Path argv0 , std::istream & ) ;
	~InstallerImp() ;
	bool next() ;
	Action & current() ;

private:
	typedef std::map<std::string,std::string> Map ;
	typedef std::list<Action> List ;

private:
	InstallerImp( const InstallerImp & ) ;
	void operator=( const InstallerImp & ) ;
	void read( std::istream & ) ;
	void insertActions() ;
	std::string value( const std::string & key ) const ;
	std::string value( const std::string & key , const std::string & default_ ) const ;
	bool exists( const std::string & key ) const ;
	static bool yes( const std::string & ) ;
	static bool no( const std::string & ) ;
	std::string secrets() const ;
	void secretsLine( std::ostream & , const std::string & , const std::string & , const std::string & ) const ;
	LinkInfo targetLinkInfo() const ;
	bool addIndirection( LinkInfo & link_info ) const ;
	G::Strings commandlineArgs( bool short_ , bool relative ) const ;
	std::pair<std::string,Map> commandlineMap( bool short_ , bool relative ) const ;

private:
	G::Unpack m_unpack ;
	Map m_map ;
	List m_list ;
	List::iterator m_p ;
} ;

// ==

Action::Action( ActionInterface * p ) :
	m_p(p)
{
}

std::string Action::text() const
{
	return m_p->text() ;
}

void Action::run()
{
	return m_p->run() ;
}

// ==

InstallerImp::InstallerImp( G::Path argv0 , std::istream & ss ) :
	m_unpack(argv0,G::Unpack::NoThrow())
{
	read( ss ) ;
	insertActions() ;
	m_p = m_list.end() ; // sic
}

InstallerImp::~InstallerImp()
{
}

void InstallerImp::read( std::istream & ss )
{
	std::string line ;
	while( ss.good() )
	{
		G::Str::readLineFrom( ss , "\n" , line ) ;
		if( line.empty() || line.find('#') == 0U || line.find_first_not_of(" \t\r") == std::string::npos )
			continue ;

		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " \t" ) ;
		if( part.size() == 0U )
			continue ;

		std::string value = part.size() == 1U ? std::string() : line.substr(part[0].length()) ;
		value = G::Str::trimmed( value , G::Str::ws() ) ;
		std::string key = part[0] ;
		G_DEBUG( "InstallerImp::read: \"" << key << "\" = \"" << value << "\"" ) ;
		m_map.insert( Map::value_type(key,value) ) ;
	}
}

bool InstallerImp::next()
{
	if( m_p == m_list.end() )
		m_p = m_list.begin() ;
	else if( m_p != m_list.end() )
		++m_p ;
	return m_p != m_list.end() ;
}

Action & InstallerImp::current()
{
	return *m_p ;
}

std::string InstallerImp::value( const std::string & key , const std::string & default_ ) const
{
	Map::const_iterator p = m_map.find(key+":") ;
	return p == m_map.end() ? default_ : (*p).second ;
}

std::string InstallerImp::value( const std::string & key ) const
{
	Map::const_iterator p = m_map.find(key+":") ;
	if( p == m_map.end() )
		throw std::runtime_error( std::string() + "no such value: " + key ) ;
	return (*p).second ;
}

bool InstallerImp::exists( const std::string & key ) const
{
	return m_map.find(key+":") != m_map.end() ;
}

bool InstallerImp::yes( const std::string & value )
{
	return value.find_first_of("yY") == 0U ;
}

bool InstallerImp::no( const std::string & value )
{
	return !yes( value ) ;
}

void InstallerImp::insertActions()
{
	// create base directories
	//
	m_list.push_back( Action(new CreateDirectory("install",value("dir-install"))) ) ;
	m_list.push_back( Action(new CreateDirectory("spool",value("dir-spool"))) ) ;
	m_list.push_back( Action(new CreateDirectory("configuration",value("dir-config"))) ) ;
	m_list.push_back( Action(new CreateDirectory("pid",value("dir-pid"))) ) ;

	// bits and bobs
	//
	m_list.push_back( Action(new CreateSecrets(value("dir-config"),"emailrelay.auth",secrets()) ) ) ;
	LinkInfo target_link_info = targetLinkInfo() ;
	if( addIndirection(target_link_info) )
		m_list.push_back( Action(new CreateBatchFile(target_link_info) ) ) ;

	// extract the gui without the packed files
	//
	G::Strings name_list = m_unpack.names() ;
	bool is_setup = ! name_list.empty() ;
	if( is_setup ) 
	{
		m_list.push_back( Action(new ExtractOriginal(m_unpack,value("dir-install"))) ) ;
	}

	// extract packed files
	//
	std::set<std::string> dir_set ;
	for( G::Strings::iterator p = name_list.begin() ; p != name_list.end() ; ++p )
	{
		G::Path path( *p ) ;
		if( dir_set.find(path.dirname().str()) == dir_set.end() )
		{
			dir_set.insert( path.dirname().str() ) ;
			m_list.push_back( Action(new CreateDirectory("target",value("dir-install"),path.dirname().str())) ) ;
		}
		m_list.push_back( Action(new Extract(m_unpack,value("dir-install"),path)) ) ;
	}

	// copy dlls -- note that the dlls are locked if we are re-running in the target directory
	//
	if( is_setup && isWindows() )
	{
		if( G::File::exists("mingwm10.dll") )
			m_list.push_back( Action(new Copy(value("dir-install"),"mingwm10.dll")) ) ;
		if( G::File::exists("QtCore4.dll") )
			m_list.push_back( Action(new Copy(value("dir-install"),"QtCore4.dll")) ) ;
		if( G::File::exists("QtGui4.dll") )
			m_list.push_back( Action(new Copy(value("dir-install"),"QtGui4.dll")) ) ;
	}

	// create links
	//
	G::Path working_dir = value("dir-config") ;
	if( yes(value("start-link-desktop")) )
		m_list.push_back( Action(new CreateLink(value("dir-desktop"),working_dir,target_link_info)) ) ;
	if( yes(value("start-link-menu")) )
		m_list.push_back( Action(new CreateLink(value("dir-menu"),working_dir,target_link_info)) ) ;
	if( yes(value("start-at-login")) )
		m_list.push_back( Action(new CreateLink(value("dir-login"),working_dir,target_link_info)) ) ;
	if( isWindows() )
		m_list.push_back( Action(new CreateLink(value("dir-install"),working_dir,target_link_info)) ) ;
	//if( yes(value("start-on-boot")) )
		//m_list.push_back( Action(new CreateBootLink()) ) ;
}

std::string InstallerImp::secrets() const
{
	std::ostringstream ss ;
	if( yes(value("do-pop")) )
	{
		std::string mechanism = value("pop-auth-mechanism") ;
		secretsLine( ss , "server" , "pop-auth-mechanism" , "pop-account-1" ) ;
		secretsLine( ss , "server" , "pop-auth-mechanism" , "pop-account-2" ) ;
		secretsLine( ss , "server" , "pop-auth-mechanism" , "pop-account-3" ) ;
	}
	if( yes(value("do-smtp")) && yes(value("smtp-server-auth")) )
	{
		std::string mechanism = value("smtp-server-auth-mechanism") ;
		secretsLine( ss , "server" , "smtp-server-auth-mechanism" , "smtp-server-account" ) ;
		if( ! value("smtp-server-trust").empty() )
		{
			ss << "NONE server " << value("smtp-server-trust") << " trusted" << std::endl ;
		}
	}
	if( yes(value("do-smtp")) && yes(value("smtp-client-auth")) )
	{
		std::string mechanism = value("smtp-client-auth-mechanism") ;
		secretsLine( ss , "client" , "smtp-client-auth-mechanism" , "smtp-client-account" ) ;
	}
	return ss.str() ;
}

void InstallerImp::secretsLine( std::ostream & stream ,
	const std::string & side , const std::string & k1 , const std::string & k2 ) const
{
	if( exists(k2+"-name") && !value(k2+"-name").empty() )
	{
		stream
			<< value(k1) << " " << side << " " 
			<< value(k2+"-name") << " "
			<< value(k2+"-password") << std::endl ;
	}
}

std::string Helper::exe()
{
	return isWindows() ? ".exe" : "" ;
}

LinkInfo InstallerImp::targetLinkInfo() const
{
	G::Path target_exe( value("dir-install") , std::string() + "emailrelay" + exe() ) ;
	G::Strings args = commandlineArgs( false , false ) ;

	LinkInfo link_info ;
	link_info.target = target_exe ;
	link_info.args = args ;
	link_info.raw_target = target_exe ;
	link_info.raw_args = args ;
	return link_info ;
}

bool InstallerImp::addIndirection( LinkInfo & link_info ) const
{
	// create a batch script if the command-line is too long
	//bool long_commandline = (link_info.target.str().length()+1+str(link_info.args).length()) >= 235U ;
	bool use_batch_file = isWindows() ; // && long_commandline ;
	if( use_batch_file )
	{
		link_info.target = G::Path( value("dir-install") , "emailrelay-start.bat" ) ;
		link_info.args = G::Strings() ;
		return true ;
	}
	else
	{
		return false ;
	}
}

G::Strings InstallerImp::commandlineArgs( bool short_ , bool relative ) const
{
	G::Strings result ;
	std::pair<std::string,Map> pair = commandlineMap( short_ , relative ) ;
	for( Map::iterator p = pair.second.begin() ; p != pair.second.end() ; ++p )
	{
		std::string switch_ = (*p).first ;
		std::string switch_arg = (*p).second ;
		std::string dash = switch_.length() > 1U ? "--" : "-" ;
		result.push_back( dash + switch_ ) ;
		if( ! switch_arg.empty() )
			result.push_back( quote(switch_arg) ) ;
	}
	return result ;
}

std::pair<std::string,InstallerImp::Map> InstallerImp::commandlineMap( bool short_ , bool relative ) const
{
	std::string auth = relative ?
		std::string("emailrelay.auth") :
		G::Path(value("dir-config"),"emailrelay.auth").str() ;

	Map out ;
	std::string path = G::Path(value("dir-install"),"emailrelay").str() ;
	out[short_?"s":"spool-dir"] = value("dir-spool") ;
	out[short_?"l":"log"] ;
	out[short_?"e":"close-stderr"] ;
	out[short_?"r":"remote-clients"] ;
	out[short_?"i":"pid-file"] = G::Path(value("dir-pid"),"emailrelay.pid").str() ;
	if( yes(value("do-smtp")) )
	{
		if( yes(value("forward-immediate")) )
		{
			out[short_?"m":"immediate"] ;
		}
		if( yes(value("forward-poll")) )
		{
			if( value("forward-poll-period") == "minute" )
				out[short_?"O":"poll"] = "60" ;
			else if( value("forward-poll-period") == "second" )
				out[short_?"O":"poll"] = "1" ;
			else 
				out[short_?"O":"poll"] = "3600" ;
		}
		if( value("smtp-server-port") != "25" )
		{
			out[short_?"p":"port"] = value("smtp-server-port") ;
		}
		if( yes(value("smtp-server-auth")) )
		{
			out[short_?"S":"server-auth"] = auth ;
		}
		out[short_?"o":"forward-to"] = value("smtp-client-host") + ":" + value("smtp-client-port") ;
		if( yes(value("smtp-client-auth")) )
		{
			out[short_?"C":"client-auth"] = auth ;
		}
	}
	else
	{
		out[short_?"X":"no-smtp"] ;
	}
	if( yes(value("do-pop")) )
	{
		out[short_?"B":"pop"] ;
		if( value("pop-port") != "110" )
		{
			out[short_?"E":"pop-port"] = value("pop-port") ;
		}
		if( yes(value("pop-shared-no-delete")) )
		{
			out[short_?"G":"pop-no-delete"] ;
		}
		if( yes(value("pop-by-name")) )
		{
			out[short_?"J":"pop-by-name"] ;
		}
		if( yes(value("pop-by-name-auto-copy")) )
		{
			std::string filter = std::string() + "emailrelay-filter-copy" + exe() ;
			out[short_?"z":"filter"] = G::Path(value("dir-install"),filter).str() ;
		}
		out[short_?"F":"pop-auth"] = auth ;
	}
	if( yes(value("logging-verbose")) )
	{
		out[short_?"v":"verbose"] ;
	}
	if( yes(value("logging-debug")) )
	{
		out[short_?"d":"debug"] ;
	}
	if( yes(value("logging-syslog")) )
	{
		out[short_?"k":"syslog"] ;
	}
	if( yes(value("listening-remote")) )
	{
		out[short_?"r":"remote-clients"] ;
	}
	if( no(value("listening-all")) && !value("listening-interface").empty() )
	{
		out[short_?"I":"interface"] = value("listening-interface") ;
	}
	return std::make_pair(path,out) ;
}

// ==

Installer::Installer( G::Path argv0 ) :
	m_argv0(argv0) ,
	m_imp(NULL)
{
}

Installer::~Installer()
{
	delete m_imp ;
}

void Installer::start( std::istream & s )
{
	delete m_imp ;
	m_imp = new InstallerImp(m_argv0,s) ;
	m_reason.erase() ;
}

bool Installer::next()
{
	bool more = false ;
	if( m_imp != NULL )
		more = m_imp->next() ;
	if( !more ) 
		cleanup() ;
	return more ;
}

void Installer::cleanup( const std::string & reason )
{
	if( m_reason.empty() )
	{
		G_DEBUG( "Installer::cleanup: [" << reason << "]" ) ;
		m_reason = reason ;
	}
	InstallerImp * imp = m_imp ;
	m_imp = NULL ;
	delete imp ;
}

std::string Installer::beforeText()
{
	return m_imp->current().text() ;
}

std::string Installer::afterText()
{
	return m_reason.empty() ? "ok" : m_reason ;
}

void Installer::run()
{
	try
	{
		m_imp->current().run() ;
	}
	catch( std::exception & e )
	{
		cleanup( e.what() ) ;
	}
}

bool Installer::failed() const
{
	if( !done() ) throw std::runtime_error( "internal error" ) ;
	return !m_reason.empty() ;
}

bool Installer::done() const
{
	return m_imp == NULL ;
}

// ==

bool Helper::isWindows()
{
 #ifdef _WIN32
	return true ;
 #else
	return false ;
 #endif
}

bool Helper::isMac()
{
	return G::File::exists("/Library/StartupItems") ; // could do better
}

std::string Helper::quote( const std::string & s )
{
	return s.find_first_of(" \t") == std::string::npos ? s : (std::string()+"\""+s+"\"") ;
}

std::string Helper::str( const G::Strings & list )
{
	return G::Str::join( list , " " ) ;
}


#if 0

void createBootLink( const Map & map , G::Path target , G::Strings args )
{
	if( isWindows() )
		createBootLinkWindows( value(map,"dir-reskit",std::string()) , target , args ) ;
	else if( isMac() )
		createBootLinkMac( target , args ) ;
	else
		createBootLinkUnix( value(map,"dir-boot",std::string()) , target , args ) ;
}

void createBootLinkMac( G::Path , G::Strings )
{
	throw BootError( "createBootLinkMac: not implemented" ) ; // TODO could do better
}

void createBootLinkUnix( G::Path boot_dir , G::Path , G::Strings )
{
	G::Path boot_script( boot_dir , "emailrelay" ) ;
	if( ! G::File::exists(boot_script) )
		throw BootError(std::string()+"cannot find "+boot_script.str()+": have you run \"make install\"?") ;

	G::Path install_tool_lsb( "/usr/lib/lsb/install_initd" ) ; // or "/sbin/insserv"
	if( G::File::exists(install_tool_lsb) )
	{
		std::string cmd = install_tool_lsb.str() + " " + boot_script.str() ;
		std::cout << "installing as a boot service using [" << cmd << "]" << std::endl ;

		G::Strings args ;
		args.push_back( boot_script.str() ) ;
		int rc = G::Process::spawn(install_tool_lsb,args).wait() ;
		if( rc )
			throw BootError( install_tool_lsb.str() + " failed" ) ;
	}
	else
	{
		throw BootError( "cannot find a boot-script installation tool" ) ; // could do better
	}
}

void createBootLinkWindows( G::Path reskit_dir , G::Path target , G::Strings args )
{
	if( reskit_dir.str().empty() )
		reskit_dir = G::Path( "c:/program files/resource kit" ) ;

	G::Path wrapper( reskit_dir , "srvany.exe" ) ;
	G::Path install_tool( reskit_dir , "instsrv.exe" ) ;
	if( ! G::File::exists(install_tool) )
		throw std::runtime_error( std::string() + "cannot run \"" + install_tool.str() + "\": no such file" ) ;

	args.push_front( target.str() ) ;
	args.push_front( "E-MailRelay" ) ;
	args.push_back( "-H" ) ; // hidden window
	args.push_back( "-t" ) ; // --no-daemon

	std::cout << "installing as a service using [" << quote(install_tool.str()) << "]" << std::endl ;
	int rc = G::Process::spawn(install_tool.str(),args).wait() ;
	if( rc != 0 )
		throw std::runtime_error( std::string() + "cannot run \"" + install_tool.str() + "\"" ) ;
}

std::string configFile( const Map & map_in , const std::string & prefix )
{
	std::ostringstream ss ;
	std::pair<std::string,Map> pair = commandlineMap( map_in , false , false ) ;
	for( Map::iterator p = pair.second.begin() ; p != pair.second.end() ; ++p )
	{
		ss << prefix << (*p).first ;
		if( ! (*p).second.empty() )
			ss << " " << (*p).second ;
		ss << std::endl ;
	}
	return ss.str() ;
}

std::string configFilename( const Map & map_in )
{
	return isWindows() ? std::string() : G::Path(value(map_in,"dir-config"),"emailrelay.conf").str() ;
}

std::string commandlineString( const Map & map_in , bool short_ , bool relative )
{
	std::ostringstream ss ;
	std::pair<std::string,Map> pair = commandlineMap( map_in , short_ , relative ) ;
	ss << pair.first << " " << str(commandlineArgs(map_in,short_,relative)) ;
	return ss.str() ;
}

#endif
/// \file installer.cpp
