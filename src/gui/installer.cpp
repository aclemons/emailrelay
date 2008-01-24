//
// Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// installer.cpp
//

#include "gdef.h"
#include "glog.h"
#include "gstr.h"
#include "gpath.h"
#include "gfile.h"
#include "gdate.h"
#include "gtime.h"
#include "gstrings.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "gcominit.h"
#include "glink.h"
#include "garg.h"
#include "gunpack.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "glog.h"
#include "gexception.h"
#include "boot.h"
#include "dir.h"
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
	G::Path target ; // exe-or-wrapper
	G::Strings args ; // exe-or-wrapper args
	G::Path raw_target ; // exe
	G::Strings raw_args ; // exe args
} ;

struct ActionInterface
{
	virtual void run() = 0 ;
	virtual std::string text() const = 0 ;
	virtual std::string ok() const = 0 ;
	protected: virtual ~ActionInterface() {}
} ;

struct Helper
{
	static bool isWindows() ;
	static std::string exe() ;
	static std::string quote( std::string , bool = false ) ;
	static std::string str( const G::Strings & list ) ;
} ;

struct ActionBase : public ActionInterface , protected Helper
{
	virtual std::string ok() const ;
} ;

struct CreateDirectory : public ActionBase
{
	std::string m_display_name ;
	std::string m_ok ;
	G::Path m_path ;
	CreateDirectory( std::string display_name , std::string path , std::string sub_path = std::string() ) ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
	virtual void run() ;
} ;

struct ExtractOriginal : public ActionBase
{
	G::Unpack & m_unpack ;
	G::Path m_dst ;
	ExtractOriginal( G::Unpack & unpack , G::Path dst ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct CreateStateFile : public ActionBase
{
	G::Path m_dst ;
	G::Path m_exe ;
	std::string m_line1 ;
	std::string m_line2 ;
	std::string m_line3 ;
	CreateStateFile( G::Path dir , std::string state_name , std::string exe_name ,
		std::string line1 , std::string line2 , std::string line3 ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct Copy : public ActionBase
{
	G::Path m_dst_dir ;
	G::Path m_src ;
	Copy( std::string install_dir , std::string name , std::string sub_dir = std::string() ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct Extract : public ActionBase
{
	G::Unpack & m_unpack ;
	std::string m_key ;
	G::Path m_dst ;
	Extract( G::Unpack & unpack , std::string key , G::Path dst ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct CreateSecrets : public ActionBase
{
	G::Path m_path ;
	G::StringMap m_content ;
	CreateSecrets( const std::string & config_dir , const std::string & filename , G::StringMap content ) ;
	virtual void run() ;
	virtual std::string text() const ;
	static bool match( std::string , std::string ) ;
} ;

struct CreateBatchFile : public ActionBase
{
	LinkInfo m_link_info ;
	std::string m_args ;
	CreateBatchFile( LinkInfo ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct UpdateLink : public ActionBase
{
	bool m_active ;
	G::Path m_link_dir ;
	G::Path m_working_dir ;
	LinkInfo m_target_link_info ;
	G::Path m_icon_path ;
	G::Path m_link_path ;
	UpdateLink( bool active , std::string link_dir , G::Path working_dir , LinkInfo target_link_info ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct UpdateBootLink : public ActionBase
{
	bool m_active ;
	std::string m_init_d ;
	LinkInfo m_target_link_info ;
	UpdateBootLink( bool active , std::string init_d , LinkInfo target_link_info ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct CreateConfigFile : public ActionBase
{
	std::string m_ok ;
	G::Path m_src ;
	G::Path m_dst ;
	CreateConfigFile( std::string dir , std::string dst_name , std::string src_dir , std::string src_name ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct EditConfigFile : public ActionBase
{
	typedef std::map<std::string,std::string> Map ;
	typedef std::list<std::string> List ;
	G::Path m_path ;
	Map m_map ;
	EditConfigFile( std::string dir , std::string name , const Map & map ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct Action
{
	ActionInterface * m_p ; // should do reference counting, but just leak for now
	explicit Action( ActionInterface * p ) ;
	std::string text() const ;
	std::string ok() const ;
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
	static std::string normalised( const std::string & ) ;
	std::string value( const std::string & key ) const ;
	std::string value( const std::string & key , const std::string & default_ ) const ;
	bool exists( const std::string & key ) const ;
	static bool yes( const std::string & ) ;
	static bool no( const std::string & ) ;
	G::StringMap secrets() const ;
	void addSecret( G::StringMap & , const std::string & ) const ;
	void addSecret( G::StringMap & , const std::string & , const std::string & , const std::string & ) const ;
	LinkInfo targetLinkInfo() const ;
	bool addIndirection( LinkInfo & link_info ) const ;
	G::Strings commandlineArgs( bool short_ = false ) const ;
	std::pair<std::string,Map> commandlineMap( bool short_ = false ) const ;
	void insert( ActionInterface * p ) ;

private:
	G::Unpack m_unpack ;
	Map m_map ;
	List m_list ;
	List::iterator m_p ;
} ;

// ==

CreateDirectory::CreateDirectory( std::string display_name , std::string path , std::string sub_path ) :
	m_display_name(display_name) ,
	m_path(sub_path.empty()?path:G::Path::join(path,sub_path))
{
}

std::string CreateDirectory::text() const
{
	return std::string() + "creating " + m_display_name + " directory [" + m_path.str() + "]" ;
}

std::string CreateDirectory::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

void CreateDirectory::run()
{
	G::Directory dir( m_path ) ;
	if( G::File::exists(m_path) )
	{
		if( !dir.valid() )
			throw std::runtime_error( "directory path exists but not valid a directory" ) ;
		m_ok = "exists" ;
	}
	else
	{
		G::File::mkdirs( m_path , 10 ) ;
	}
	if( !dir.writeable() )
		throw std::runtime_error( "directory exists but is not writable" ) ;
}

// ==

ExtractOriginal::ExtractOriginal( G::Unpack & unpack , G::Path dst ) :
	m_unpack(unpack) ,
	m_dst(dst)
{
}

void ExtractOriginal::run() 
{
	m_unpack.unpackOriginal( m_dst ) ;
	G::File::chmodx( m_dst ) ;
}

std::string ExtractOriginal::text() const 
{ 
	return std::string() + "creating [" + m_dst.str() + "]" ; 
}

// ==

CreateStateFile::CreateStateFile( G::Path dir , std::string state_name , std::string exe_name ,
	std::string line1 , std::string line2 , std::string line3 ) :
		m_dst(G::Path(dir,state_name)) ,
		m_exe(G::Path(dir,exe_name)) ,
		m_line1(line1) ,
		m_line2(line2) ,
		m_line3(line3)
{
}

void CreateStateFile::run() 
{
	std::ofstream file( m_dst.str().c_str() ) ;
	if( !isWindows() )
		file << "#!/bin/sh" << std::endl ;
	file << "INSTALLED_SPOOL_DIR=" << m_line1 << std::endl ;
	file << "INSTALLED_CONFIG_DIR=" << m_line2 << std::endl ;
	if( !m_line3.empty() ) 
		file << "INSTALLED_PID_DIR=" << m_line3 << std::endl ;
	if( !isWindows() )
		file << "exec " << m_exe << " \"$@\"" << std::endl ;
	if( !file.good() ) throw std::runtime_error( std::string() + "cannot write to \"" + m_dst.str() + "\"" ) ;
	file.close() ;
	G::File::chmodx( m_dst ) ;
}

std::string CreateStateFile::text() const 
{ 
	return std::string() + "creating state file [" + m_dst.str() + "]" ;
}

// ==

Copy::Copy( std::string install_dir , std::string name , std::string sub_dir ) :
	m_dst_dir(sub_dir.empty()?install_dir:G::Path(install_dir,sub_dir)) ,
	m_src(name)
{
}

void Copy::run()
{
	G::File::copy( m_src , G::Path(m_dst_dir,m_src.basename()) ) ;
}

std::string Copy::text() const
{
	return std::string() + "copying [" + m_src.basename() + "] -> [" + m_dst_dir.str() + "]" ;
}

// ==

Extract::Extract( G::Unpack & unpack , std::string key , G::Path dst ) :
	m_unpack(unpack) ,
	m_key(key) ,
	m_dst(dst)
{
}

void Extract::run()
{
	m_unpack.unpack( m_key , m_dst ) ;
	if( m_dst.dirname().str().find("share/") != std::string::npos ) // ick
		G::File::chmodx( m_dst ) ;
}

std::string Extract::text() const
{
	return "extracting [" + m_dst.basename() + "] to [" + m_dst.dirname().str() + "]" ;
}

// ==

CreateSecrets::CreateSecrets( const std::string & config_dir , const std::string & filename , 
	G::StringMap content ) :
		m_path(config_dir,filename) ,
		m_content(content)
{
}

std::string CreateSecrets::text() const
{
	return std::string() + "creating authentication secrets file [" + m_path.str() + "]" ;
}

bool CreateSecrets::match( std::string p1 , std::string p2 )
{
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
	G::Strings line_list ;
	if( file_exists )
	{
		std::ifstream file( m_path.str().c_str() ) ;
		while( file.good() )
		{
			std::string line = G::Str::readLineFrom( file , "\n" ) ;
			if( !file ) break ;
			line_list.push_back( line ) ;
		}
	}

	// write a header if none
	if( line_list.empty() )
	{
		line_list.push_back( "#" ) ;
		line_list.push_back( std::string() + "# " + m_path.basename() ) ;
		line_list.push_back( "#" ) ;
		line_list.push_back( "# <mechanism> {server|client} <name> <secret>" ) ;
		line_list.push_back( "# mechanism := CRAM-MD5 | LOGIN | APOP | NONE" ) ;
		line_list.push_back( "#" ) ;
	}

	// assemble the new file
	for( G::StringMap::iterator map_p = m_content.begin() ; map_p != m_content.end() ; ++map_p )
	{
		bool replaced = false ;
		for( G::Strings::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p ) // k.i.s.s
		{
			if( match( *line_p , (*map_p).first ) )
			{
				*line_p = (*map_p).second ;
				replaced = true ;
				break ;
			}
		}
		if( !replaced )
		{
			line_list.push_back( (*map_p).second ) ;
		}
	}

	// make a backup -- ignore errors for now
	if( file_exists )
	{
		G::DateTime::BrokenDownTime now = G::DateTime::local( G::DateTime::now() ) ;
		std::string timestamp = G::Date(now).string(G::Date::yyyy_mm_dd) + G::Time(now).hhmmss() ;
		G::Path backup( m_path.dirname() , m_path.basename() + "." + timestamp ) ;
		G::File::copy( m_path , backup , G::File::NoThrow() ) ;
	}

	// write the new file
	std::ofstream file( m_path.str().c_str() ) ;
	bool ok = file.good() ;
	for( G::Strings::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
	{
		file << *line_p << std::endl ;
	}
	ok = ok && file.good() ;
	file.close() ;
	if( !ok )
			throw std::runtime_error(std::string()+"cannot create \""+m_path.str()+"\"") ;
}

// ==

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

UpdateLink::UpdateLink( bool active , std::string link_dir , G::Path working_dir , LinkInfo target_link_info ) :
	m_active(active) ,
	m_link_dir(link_dir) ,
	m_working_dir(working_dir) ,
	m_target_link_info(target_link_info) ,
	m_icon_path(target_link_info.target.dirname(),"emailrelay-icon.png")
{
	if( isWindows() )
		m_icon_path = target_link_info.raw_target ; // get the icon from the exe resource

	std::string link_filename = GLink::filename( "E-MailRelay" ) ;
	m_link_path = G::Path( m_link_dir , link_filename ) ;
}

std::string UpdateLink::text() const
{
	return std::string() + "updating link in [" + m_link_dir.str() + "]" ;
}

void UpdateLink::run()
{
	new GComInit ; // (leak ok)
	if( m_active )
	{
		GLink link( m_target_link_info.target , "E-MailRelay" , "E-MailRelay server" , 
			m_working_dir , str(m_target_link_info.args) , m_icon_path , GLink::Show_Hide ) ;

		G::Process::Umask umask( G::Process::Umask::Tightest ) ;
		G::File::mkdirs( m_link_dir , 10 ) ;
		link.saveAs( m_link_path ) ;
	}
	else
	{
		G::File::remove( m_link_path , G::File::NoThrow() ) ;
	}
}

// ==

UpdateBootLink::UpdateBootLink( bool active , std::string init_d , LinkInfo target_link_info ) :
	m_active(active) ,
	m_init_d(init_d) ,
	m_target_link_info(target_link_info)
{
}

std::string UpdateBootLink::text() const
{
	return 
		std::string() + "updating boot-time links for " +
		"[" + G::Path(m_init_d,m_target_link_info.target.basename()).str() + "]" ;
}

void UpdateBootLink::run()
{
	if( m_active )
	{
		if( ! Boot::install( m_init_d , m_target_link_info.target , m_target_link_info.args ) )
			throw std::runtime_error( "failed to create links" ) ;
	}
	else
	{
		Boot::uninstall( m_init_d , m_target_link_info.target , m_target_link_info.args ) ;
	}
}

// ==

CreateConfigFile::CreateConfigFile( std::string dst_dir , std::string dst_name , 
	std::string src_dir , std::string src_name ) :
		m_src(G::Path(src_dir,src_name)) ,
		m_dst(G::Path(dst_dir,dst_name))
{
}

void CreateConfigFile::run()
{
	if( G::File::exists(m_dst) )
		m_ok = "exists" ;
	else if( !G::File::exists(m_src) )
		throw std::runtime_error( std::string() + "cannot find configuration template: \"" + m_src.str() + "\"" ) ;
	else
		G::File::copy( m_src , m_dst ) ;
}

std::string CreateConfigFile::text() const
{
	return std::string() + "creating config file \"" + m_dst.str() + "\"" ;
}

std::string CreateConfigFile::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

// ==

EditConfigFile::EditConfigFile( std::string dir , std::string name , const Map & map ) :
	m_path(G::Path(dir,name)) ,
	m_map(map)
{
}

void EditConfigFile::run()
{
	// read
	List line_list ;
	{
		std::ifstream file_in( m_path.str().c_str() ) ;
		if( !file_in.good() ) throw std::runtime_error( std::string() + "cannot read \"" + m_path.str() + "\"" ) ;
		while( file_in.good() )
		{
			std::string line = G::Str::readLineFrom( file_in , "\n" ) ;
			if( !file_in ) break ;
			line_list.push_back( line ) ;
		}
	}

	// comment-out everything
	for( List::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
	{
		std::string line = *line_p ;
		if( !line.empty() && line.at(0U) != '#' )
		{
			line = std::string(1U,'#') + line ;
			*line_p = line ;
		}
	}

	// un-comment-out (or add) values from the map
	for( Map::const_iterator map_p = m_map.begin() ; map_p != m_map.end() ; ++map_p )
	{
		bool found = false ;
		for( List::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p ) // k.i.s.s
		{
			G::StringArray part ;
			G::Str::splitIntoTokens( *line_p , part , G::Str::ws()+"#" ) ;
			if( part.size() && part[0] == (*map_p).first )
			{
				*line_p = (*map_p).first + " " + quote((*map_p).second) ;
				found = true ;
				break ;
			}
		}

		if( !found )
		{
			// dont add things that the init.d script takes care of
			const bool ignore = 
				(*map_p).first == "syslog" ||
				(*map_p).first == "close-stderr" ||
				(*map_p).first == "pid-file" ||
				(*map_p).first == "log" ;

			if( !ignore )
				line_list.push_back( (*map_p).first + " " + quote((*map_p).second) ) ;
		}
	}

	// TODO -- "--syslog" and "--no-syslog" interaction

	// make a backup -- ignore errors for now
	G::DateTime::BrokenDownTime now = G::DateTime::local( G::DateTime::now() ) ;
	std::string timestamp = G::Date(now).string(G::Date::yyyy_mm_dd) + G::Time(now).hhmmss() ;
	G::Path backup( m_path.dirname() , m_path.basename() + "." + timestamp ) ;
	G::File::copy( m_path , backup , G::File::NoThrow() ) ;

	// write
	std::ofstream file_out( m_path.str().c_str() ) ;
	for( List::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
	{
		file_out << *line_p << std::endl ;
	}
	if( !file_out.good() ) 
		throw std::runtime_error( std::string() + "cannot write \"" + m_path.str() + "\"" ) ;
}

std::string EditConfigFile::text() const
{
	return std::string() + "editing config file \"" + m_path.str() + "\"" ;
}

// ==

std::string ActionBase::ok() const
{
	return "ok" ;
}

// ==

Action::Action( ActionInterface * p ) :
	m_p(p)
{
}

std::string Action::text() const
{
	return m_p->text() ;
}

std::string Action::ok() const
{
	return m_p->ok() ;
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

		if( !ss )
			break ;

		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " =\t" ) ;
		if( part.size() == 0U )
			continue ;

		std::string value = part.size() == 1U ? std::string() : line.substr(part[0].length()+1U) ;
		value = G::Str::trimmed( value , G::Str::ws() ) ;
		if( value.length() >= 2U && value.at(0U) == '"' && value.at(value.length()-1U) == '"' )
			value = value.substr(1U,value.length()-2U) ;

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

std::string InstallerImp::normalised( const std::string & key )
{
	std::string k = key ;
	G::Str::replaceAll( k , "-" , "_" ) ;
	G::Str::toUpper( k ) ;
	return k ;
}

std::string InstallerImp::value( const std::string & key , const std::string & default_ ) const
{
	Map::const_iterator p = m_map.find(normalised(key)) ;
	return p == m_map.end() ? default_ : (*p).second ;
}

std::string InstallerImp::value( const std::string & key ) const
{
	Map::const_iterator p = m_map.find(normalised(key)) ;
	if( p == m_map.end() )
		throw std::runtime_error( std::string() + "no such value: " + key ) ;
	return (*p).second ;
}

bool InstallerImp::exists( const std::string & key ) const
{
	return m_map.find(normalised(key)) != m_map.end() ;
}

bool InstallerImp::yes( const std::string & value )
{
	return value.find_first_of("yY") == 0U ;
}

bool InstallerImp::no( const std::string & value )
{
	return !yes( value ) ;
}

void InstallerImp::insert( ActionInterface * p )
{
	m_list.push_back( Action(p) ) ;
}

void InstallerImp::insertActions()
{
	// create base directories
	//
	insert( new CreateDirectory("install",value("dir-install")) ) ;
	insert( new CreateDirectory("spool",value("dir-spool")) ) ;
	insert( new CreateDirectory("configuration",value("dir-config")) ) ;
	insert( new CreateDirectory("pid",value("dir-pid")) ) ;

	// bits and bobs
	//
	insert( new CreateSecrets(value("dir-config"),"emailrelay.auth",secrets()) ) ;
	LinkInfo target_link_info = targetLinkInfo() ;
	if( addIndirection(target_link_info) )
		insert( new CreateBatchFile(target_link_info) ) ;

	// extract packed files
	//
	G::Strings name_list = m_unpack.names() ;
	std::set<std::string> dir_set ;
	for( G::Strings::iterator p = name_list.begin() ; p != name_list.end() ; ++p )
	{
		std::string name = *p ;
		std::string base = value("dir-install") ;
		if( name.find("$etc") == 0U )
		{
			name = name.substr(4U) ;
			base = value("dir-config") ;
		}

		G::Path path = G::Path::join( base , name ) ;
		if( dir_set.find(path.dirname().str()) == dir_set.end() )
		{
			dir_set.insert( path.dirname().str() ) ;
			insert( new CreateDirectory("target",path.dirname().str()) ) ;
		}
		insert( new Extract(m_unpack,*p,path) ) ;
	}

	// extract the gui without its packed-file payload and write a state file
	//
	bool is_setup = ! name_list.empty() ;
	if( is_setup ) 
	{
		G::Path gui_dir = isWindows() ? value("dir-install") : (value("dir-install")+"/sbin") ; // TODO
		std::string gui_name = isWindows() ? m_unpack.path().basename() : "emailrelay-gui.real" ; // TODO

		// see also guimain.cpp ...
		std::string::size_type pos = gui_name.find('.') ;
		std::string state_name =
			pos == std::string::npos ?
				(gui_name + ".state") :
				G::Str::head( gui_name , pos ) ;

		insert( new ExtractOriginal(m_unpack,G::Path(gui_dir,gui_name)) ) ;
		insert( new CreateStateFile(gui_dir,state_name,gui_name,
			value("dir-spool"),value("dir-config"),std::string()) ) ;
	}

	// copy dlls -- note that the dlls are locked if we are re-running in the target directory
	//
	if( is_setup && isWindows() )
	{
		if( G::File::exists("mingwm10.dll") )
			insert( new Copy(value("dir-install"),"mingwm10.dll") ) ;
		if( G::File::exists("QtCore4.dll") )
			insert( new Copy(value("dir-install"),"QtCore4.dll") ) ;
		if( G::File::exists("QtGui4.dll") )
			insert( new Copy(value("dir-install"),"QtGui4.dll") ) ;
	}

	// create links
	//
	G::Path working_dir = value("dir-config") ;
	const bool is_mac = yes(value("start-is-mac")) ;
	if( !is_mac )
	{
		insert( new UpdateLink(yes(value("start-link-desktop")),value("dir-desktop"),working_dir,target_link_info) ) ;
		insert( new UpdateLink(yes(value("start-link-menu")),value("dir-menu"),working_dir,target_link_info) ) ;
		insert( new UpdateLink(yes(value("start-at-login")),value("dir-login"),working_dir,target_link_info) ) ;
	}
	insert( new UpdateBootLink(yes(value("start-on-boot")),value("dir-boot"),target_link_info) ) ;
	if( isWindows() )
	{
		insert( new UpdateLink(true,value("dir-install"),working_dir,target_link_info) ) ;
	}

	// edit the boot-time config file -- the ".conf" file is created from the
	// template if necessary
	//
	if( !isWindows() )
	{
		insert( new CreateConfigFile(value("dir-config"),"emailrelay.conf",
			value("dir-config"),"emailrelay.conf.template") ) ;
		insert( new EditConfigFile(value("dir-config"),"emailrelay.conf",commandlineMap().second) ) ;
	}
}

G::StringMap InstallerImp::secrets() const
{
	G::StringMap map ;
	if( yes(value("do-pop")) )
	{
		std::string mechanism = value("pop-auth-mechanism") ;
		addSecret( map , "server" , "pop-auth-mechanism" , "pop-account-1" ) ;
		addSecret( map , "server" , "pop-auth-mechanism" , "pop-account-2" ) ;
		addSecret( map , "server" , "pop-auth-mechanism" , "pop-account-3" ) ;
	}
	if( yes(value("do-smtp")) && yes(value("smtp-server-auth")) )
	{
		std::string mechanism = value("smtp-server-auth-mechanism") ;
		addSecret( map , "server" , "smtp-server-auth-mechanism" , "smtp-server-account" ) ;
		addSecret( map , "smtp-server-trust" ) ;
	}
	if( yes(value("do-smtp")) && yes(value("smtp-client-auth")) )
	{
		std::string mechanism = value("smtp-client-auth-mechanism") ;
		addSecret( map , "client" , "smtp-client-auth-mechanism" , "smtp-client-account" ) ;
	}
	return map ;
}

void InstallerImp::addSecret( G::StringMap & map , const std::string & k ) const
{
	if( exists(k) && !value(k).empty() )
	{
		std::string head = std::string() + "NONE server " + value(k) ;
		std::string tail = std::string() + " " + "trusted" ;
		map[head] = head + tail ;
	}
}

void InstallerImp::addSecret( G::StringMap & map ,
	const std::string & side , const std::string & k1 , const std::string & k2 ) const
{
	if( exists(k2+"-name") && !value(k2+"-name").empty() )
	{
		std::string head = value(k1) + " " + side + " " + value(k2+"-name") ;
		std::string tail = std::string() + " " + value(k2+"-password") ;
		map[head] = head + tail ;
	}
}

LinkInfo InstallerImp::targetLinkInfo() const
{
	G::Path target_exe( value("dir-install") , std::string() + "emailrelay" + exe() ) ;
	G::Strings args = commandlineArgs() ;

	LinkInfo link_info ;
	link_info.target = target_exe ;
	link_info.args = args ;
	link_info.raw_target = target_exe ;
	link_info.raw_args = args ;
	return link_info ;
}

bool InstallerImp::addIndirection( LinkInfo & link_info ) const
{
	// create a batch script on windows -- (the service stuff requires a batch file)
	bool use_batch_file = isWindows() ;
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

G::Strings InstallerImp::commandlineArgs( bool short_ ) const
{
	G::Strings result ;
	std::pair<std::string,Map> pair = commandlineMap( short_ ) ;
	for( Map::iterator p = pair.second.begin() ; p != pair.second.end() ; ++p )
	{
		std::string switch_ = (*p).first ;
		std::string switch_arg = (*p).second ;
		std::string dash = switch_.length() > 1U ? "--" : "-" ;
		result.push_back( dash + switch_ ) ;
		if( ! switch_arg.empty() )
		{
			// (move this?)
			bool is_commandline = 
				result.back() == "--filter" || result.back() == "-z" ||
				result.back() == "--client-filter" || result.back() == "-Y" ||
				result.back() == "--verifier" || result.back() == "-Z" ;

			result.push_back( quote(switch_arg,is_commandline) ) ;
		}
	}
	return result ;
}

std::pair<std::string,InstallerImp::Map> InstallerImp::commandlineMap( bool short_ ) const
{
	std::string auth = G::Path(value("dir-config"),"emailrelay.auth").str() ;

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
		if( yes(value("smtp-client-tls")) )
		{
			out[short_?"j":"client-tls"] ;
		}
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
	return m_reason.empty() ? m_imp->current().ok() : m_reason ;
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
	if( !done() )
		throw std::runtime_error( "internal error" ) ;
	return !m_reason.empty() ;
}

bool Installer::done() const
{
	return m_imp == NULL ;
}

// ==

std::string Helper::exe()
{
	return Dir::dotexe() ;
}

bool Helper::isWindows()
{
 #ifdef G_WIN32
	return true ;
 #else
	return false ;
 #endif
}

std::string Helper::quote( std::string s , bool escape_spaces )
{
	if( escape_spaces )
	{
		G::Str::replaceAll( s , " " , "\\ " ) ;
	}
	return s.find_first_of(" \t") == std::string::npos ? s : (std::string()+"\""+s+"\"") ;
}

std::string Helper::str( const G::Strings & list )
{
	return G::Str::join( list , " " ) ;
}

/// \file installer.cpp
