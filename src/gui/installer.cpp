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
// installer.cpp
//

#include "gdef.h"
#include "gcominit.h"
#include "glog.h"
#include "gstr.h"
#include "gpath.h"
#include "gfile.h"
#include "gdate.h"
#include "gtime.h"
#include "gstrings.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "glink.h"
#include "gregister.h"
#include "garg.h"
#include "gunpack.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "glog.h"
#include "gexception.h"
#include "pointer.h"
#include "boot.h"
#include "mapfile.h"
#include "dir.h"
#include "installer.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>
#include <utility>
#include <map>
#include <set>
#include <list>
#include <cstdlib>

#ifdef CreateDirectory
#undef CreateDirectory
#endif

struct LinkInfo
{
	G::Path target ; // exe-or-wrapper
	G::Strings args ; // exe-or-wrapper args
	G::Path raw_target ; // exe
	G::Strings raw_args ; // exe args
	G::Path icon ;
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
	G::Path m_argv0 ;
	G::Unpack & m_unpack ;
	G::Path m_dst ;
	std::string m_ok ;
	ExtractOriginal( G::Path argv0 , G::Unpack & unpack , G::Path dst ) ;
	virtual std::string ok() const ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct CopyFrameworks : public ActionBase
{
	G::Path m_argv0 ;
	G::Path m_dst ;
	std::string m_cmd ;
	static bool active( G::Path argv0 ) ;
	CopyFrameworks( G::Path argv0 , G::Path dst ) ;
	virtual void run() ;
	virtual std::string text() const ;
	static std::string sanitised( std::string ) ;
} ;

struct CreatePointerFile : public ActionBase
{
	G::Path m_pointer_path ;
	G::Path m_gui ;
	G::StringMap m_map ;
	CreatePointerFile( const G::Path & , const G::Path & , const G::StringMap & ) ;
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
	explicit CreateBatchFile( LinkInfo ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct CreateLoggingBatchFile : public ActionBase
{
	G::Path m_bat ;
	G::Path m_exe ;
	G::Strings m_args ;
	G::Path m_log ;
	CreateLoggingBatchFile( G::Path bat , G::Path exe , G::Strings args , G::Path ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct UpdateLink : public ActionBase
{
	G::Path m_argv0 ;
	bool m_active ;
	G::Path m_link_dir ;
	G::Path m_working_dir ;
	LinkInfo m_target_link_info ;
	G::Path m_link_path ;
	UpdateLink( G::Path argv0 , bool active , std::string link_dir , G::Path working_dir , LinkInfo target_link_info ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct UpdateBootLink : public ActionBase
{
	bool m_active ;
	std::string m_ok ;
	std::string m_init_d ;
	LinkInfo m_target_link_info ;
	UpdateBootLink( bool active , std::string init_d , LinkInfo target_link_info ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct RegisterAsEventSource : public ActionBase
{
	std::string m_dir ;
	std::string m_basename ;
	RegisterAsEventSource( const std::string & dir , const std::string & basename ) ;
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
	typedef G::StringMap Map ;
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
	InstallerImp( G::Path argv0 , G::Path payload , bool installing , std::istream & ) ;
	~InstallerImp() ;
	bool next() ;
	Action & current() ;

private:
	typedef G::StringMap Map ;
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
	G::StringMap secrets() const ;
	void addSecret( G::StringMap & , const std::string & ) const ;
	void addSecret( G::StringMap & , const std::string & , const std::string & , const std::string & ) const ;
	LinkInfo targetLinkInfo() const ;
	G::Strings commandlineArgs( bool short_ = false , bool disallow_close_stderr = false ) const ;
	std::pair<std::string,Map> commandlineMap( bool short_ = false ) const ;
	void insert( ActionInterface * p ) ;

private:
	G::Path m_argv0 ;
	bool m_installing ;
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
	G::Directory directory( m_path ) ;
	if( G::File::exists(m_path) )
	{
		if( !directory.valid() )
			throw std::runtime_error( "directory path exists but not valid a directory" ) ;
		m_ok = "exists" ;
	}
	else
	{
		G::File::mkdirs( m_path , 10 ) ;
	}
	if( !directory.writeable() )
		throw std::runtime_error( "directory exists but is not writable" ) ;
}

// ==

ExtractOriginal::ExtractOriginal( G::Path argv0 , G::Unpack & unpack , G::Path dst ) :
	m_argv0(argv0) ,
	m_unpack(unpack) ,
	m_dst(dst)
{
}

void ExtractOriginal::run()
{
	// okay if not packed or a separate payload, just copy argv0
	if( m_unpack.names().empty() || m_unpack.path() != m_argv0 )
	{
		if( m_argv0 == m_dst )
		{
			m_ok = "nothing to do" ;
		}
		else
		{
			m_ok = "copied" ;
			G::File::mkdirs( m_dst.dirname() ) ;
			G::File::copy( m_argv0 , m_dst ) ;
			G::File::chmodx( m_dst ) ;
		}
	}
	else
	{
		m_unpack.unpackOriginal( m_dst ) ;
		G::File::chmodx( m_dst ) ;
	}
}

std::string ExtractOriginal::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

std::string ExtractOriginal::text() const
{
	return std::string() + "creating [" + m_dst.str() + "]" ;
}

// ==

bool CopyFrameworks::active( G::Path argv0 )
{
	// TODO -- move this test outside the installer code
	return G::File::exists( G::Path(argv0.dirname(),"../Frameworks") ) ; // ie. mac bundle
}

std::string CopyFrameworks::sanitised( std::string s )
{
	// remove shell metacharacters
	for( const char * p = "$\\\"'()[]<>|!~*?&;" ; *p ; p++ )
		G::Str::replaceAll( s , std::string(1U,*p) , "_" ) ;
	return s ;
}

CopyFrameworks::CopyFrameworks( G::Path argv0 , G::Path dst ) :
	m_argv0(argv0) ,
	m_dst(dst)
{
	G::Path frameworks( m_argv0.dirname() , "../Frameworks" ) ;
	m_cmd = std::string() + "/bin/cp -fR \"" + sanitised(frameworks.str()) + "\" \"" + sanitised(m_dst.str()) + "\"" ;
}

void CopyFrameworks::run()
{
	// k.i.s.s
	int rc = std::system( m_cmd.c_str() ) ;
	if( rc != 0 )
		throw std::runtime_error( "failed" ) ;
}

std::string CopyFrameworks::text() const
{
	return std::string() + "copying frameworks [" + m_cmd + "]" ;
}

// ==

CreatePointerFile::CreatePointerFile( const G::Path & pointer_path , const G::Path & gui , const G::StringMap & map ) :
	m_pointer_path(pointer_path) ,
	m_gui(gui) ,
	m_map(map)
{
}

void CreatePointerFile::run()
{
	std::ofstream stream( m_pointer_path.str().c_str() ) ;
	Pointer::write( stream , m_map , m_gui ) ;
	if( !stream.good() )
		throw std::runtime_error( std::string() + "cannot write to \"" + m_pointer_path.str() + "\"" ) ;

	stream.close() ;
	G::File::chmodx( m_pointer_path ) ;
}

std::string CreatePointerFile::text() const
{
	return std::string() + "creating pointer file [" + m_pointer_path.str() + "]" ;
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
	if( m_unpack.flags(m_key).find('x') != std::string::npos )
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

	// impose the new field order - remove this eventually
	{
		G::Strings result ;
		for( G::Strings::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
		{
			typedef std::string::size_type pos_t ;
			const pos_t npos = std::string::npos ;
			std::string line = *line_p ;
			pos_t pos1 = line.find_first_not_of(G::Str::ws()) ;
			pos_t pos2 = pos1 == npos ? npos : line.find_first_of(G::Str::ws(),pos1) ;
			pos_t pos3 = pos2 == npos ? npos : line.find_first_not_of(G::Str::ws(),pos2) ;
			pos_t pos4 = pos3 == npos ? npos : line.find_first_of(G::Str::ws(),pos3) ;
			if( pos4 != npos )
			{
				std::string f1 = G::Str::lower(line.substr(pos1,pos2-pos1)) ;
				std::string f2 = G::Str::lower(line.substr(pos3,pos4-pos3)) ;
				if( ( f1 == "apop" || f1 == "cram-md5" || f1 == "none" || f1 == "login" || f1 == "plain" ) &&
					( f2 == "server" || f2 == "client" ) )
				{
					line.replace( pos1 , pos4-pos1 , line.substr(pos3,pos4-pos3)+" "+line.substr(pos1,pos2-pos1) ) ;
				}
			}
			result.push_back( line ) ;
		}
		line_list = result ;
	}

	// write a header if none
	if( line_list.empty() )
	{
		line_list.push_back( "#" ) ;
		line_list.push_back( std::string() + "# " + m_path.basename() ) ;
		line_list.push_back( "#" ) ;
		line_list.push_back( "# {server|client} <mechanism> <name> <secret>" ) ;
		line_list.push_back( "#   mechanism = { CRAM-MD5 | LOGIN | APOP | NONE }" ) ;
		line_list.push_back( "#" ) ;
	}

	// assemble the new file
	for( G::StringMap::iterator map_p = m_content.begin() ; map_p != m_content.end() ; ++map_p )
	{
		bool replaced = false ;
		for( G::Strings::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
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
		G::Process::Umask umask( G::Process::Umask::Tightest ) ;
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
	file << "start \"emailrelay\" " << quote(m_link_info.raw_target.str()) << " " << str(m_link_info.raw_args) << std::endl ;
	ok = ok && file.good() ;
	file.close() ;
	if( !ok )
		throw std::runtime_error(std::string()+"cannot create \""+m_link_info.target.str()+"\"") ;
}

// ==

CreateLoggingBatchFile::CreateLoggingBatchFile( G::Path bat , G::Path exe , G::Strings args , G::Path log ) :
	m_bat(bat) ,
	m_exe(exe) ,
	m_args(args) ,
	m_log(log)
{
}

std::string CreateLoggingBatchFile::text() const
{
	return std::string() + "creating batch file [" + m_bat.str() + "]" ;
}

void CreateLoggingBatchFile::run()
{
	std::ofstream file( m_bat.str().c_str() ) ;
	bool ok = file.good() ;
	std::string log_file = quote( m_log.str() ) ;
	G::Str::replaceAll( log_file , "%" , "%%" ) ;
	file << "start \"emailrelay\" " << quote(m_exe.str()) << " " << str(m_args) << " --log-time --log-file=" << log_file << std::endl ;
	ok = ok && file.good() ;
	file.close() ;
	if( !ok )
		throw std::runtime_error(std::string()+"cannot create \""+m_bat.str()+"\"") ;
}

// ==

UpdateLink::UpdateLink( G::Path argv0 , bool active , std::string link_dir , G::Path working_dir ,
	LinkInfo target_link_info ) :
		m_argv0(argv0) ,
		m_active(active) ,
		m_link_dir(link_dir) ,
		m_working_dir(working_dir) ,
		m_target_link_info(target_link_info)
{
	std::string link_filename = GLink::filename( "E-MailRelay" ) ;
	m_link_path = G::Path( m_link_dir , link_filename ) ;
}

std::string UpdateLink::text() const
{
	return m_link_dir.str().empty() ?
		( std::string() + "updating startup link" ) :
		( std::string() + "updating link in [" + m_link_dir.str() + "]" ) ;
}

void UpdateLink::run()
{
	new GComInit ; // (leak ok)
	if( m_active )
	{
		GLink link( m_target_link_info.target , "E-MailRelay" ,
			"Starts the E-MailRelay server in the background" ,
			m_working_dir , m_target_link_info.args ,
			m_target_link_info.icon , GLink::Show_Hide ,
			"E-MailRelay" ,
			std::string() + "Generated by the E-MailRelay configuration GUI (" + m_argv0.str() + ")" ) ;

		G::Process::Umask umask( G::Process::Umask::Tightest ) ;
		G::File::mkdirs( m_link_dir , 10 ) ;
		link.saveAs( m_link_path ) ;
	}
	else
	{
		GLink::remove( m_link_path ) ;
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
	std::string s = std::string() + "updating boot time links for [" + m_target_link_info.target.basename() + "]" ;
	if( ! m_init_d.empty() )
		s.append( std::string() + " in [" + m_init_d + "]" ) ;
	return s ;
}

void UpdateBootLink::run()
{
	if( m_init_d.empty() )
	{
		m_ok = "no access" ;
	}
	else if( m_active )
	{
		if( ! Boot::install( m_init_d , m_target_link_info.target , m_target_link_info.args ) )
			throw std::runtime_error( "failed to create links" ) ;
		m_ok = "installed" ;
	}
	else
	{
		bool removed = Boot::uninstall( m_init_d , m_target_link_info.target , m_target_link_info.args ) ;
		m_ok = removed ? "removed" : "not installed" ;
	}
}

std::string UpdateBootLink::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

// ==

RegisterAsEventSource::RegisterAsEventSource( const std::string & dir , const std::string & basename ) :
	m_dir(dir) ,
	m_basename(basename)
{
}

void RegisterAsEventSource::run()
{
	GRegister::server( G::Path(m_dir,m_basename) ) ;
}

std::string RegisterAsEventSource::text() const
{
	return std::string() + "registering [" + G::Path(m_dir,m_basename).str() + "] to use the event log" ;
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
		G::File::create( m_dst ) ; // shouldnt get here if the template is in the payload
	else
		G::File::copy( m_src , m_dst ) ;
}

std::string CreateConfigFile::text() const
{
	return std::string() + "creating config file [" + m_dst.str() + "]" ;
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
	const bool do_backup = true ;

	// use a stop list for things which the init.d script does for itself
	G::StringMap stop_list ;
	stop_list["syslog"] = "" ;
	stop_list["close-stderr"] = "" ;
	stop_list["pid-file"] = "" ;
	stop_list["log"] = "" ;

	MapFile::edit( m_path , m_map , "gui-" , false , stop_list , do_backup , false , false ) ;
}

std::string EditConfigFile::text() const
{
	return std::string() + "editing config file [" + m_path.str() + "]" ;
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
	m_p->run() ;
}

// ==

InstallerImp::InstallerImp( G::Path argv0 , G::Path payload , bool installing , std::istream & ss ) :
	m_argv0(argv0) ,
	m_installing(installing) ,
	m_unpack(payload,G::Unpack::NoThrow())
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
	// read install variables
	m_map = MapFile::read( ss ) ;
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
	G::StringMapReader map( m_map ) ;
	return map.at( "gui-" + key , default_ ) ;
}

std::string InstallerImp::value( const std::string & key ) const
{
	G::StringMapReader map( m_map ) ;
	return map.at( "gui-" + key ) ;
}

bool InstallerImp::exists( const std::string & key ) const
{
	return m_map.find("gui-"+key) != m_map.end() ;
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
	if( m_installing )
	{
		insert( new CreateDirectory("install",value("dir-install")) ) ;
		insert( new CreateDirectory("configuration",value("dir-config")) ) ;
	}
	insert( new CreateDirectory("spool",value("dir-spool")) ) ;
	insert( new CreateDirectory("pid",value("dir-pid")) ) ;

	// create secrets
	//
	insert( new CreateSecrets(value("dir-config"),"emailrelay.auth",secrets()) ) ;

	// create a startup link target
	//
	LinkInfo target_link_info = targetLinkInfo() ;
	if( isWindows() )
	{
		target_link_info.target = G::Path( value("dir-install") , "emailrelay-start.bat" ) ;
		target_link_info.args = G::Strings() ;
		insert( new CreateBatchFile(target_link_info) ) ;

		insert( new CreateLoggingBatchFile( G::Path(value("dir-install"),"emailrelay-start-with-log-file.bat") ,
			target_link_info.raw_target , commandlineArgs(false,true) ,
			G::Path(value("dir-install"),"emailrelay-%d.txt") ) ) ;
	}

	// extract packed files -- do substitution for "$install", "$etc"
	// and "$init" -- see "make-setup.sh"
	//
	if( m_installing )
	{
		G::Strings name_list = m_unpack.names() ;
		G_DEBUG( "InstallerImp::insertActions: " << name_list.size() << " packed files to unpack" ) ;
		std::set<std::string> dir_set ;
		for( G::Strings::iterator p = name_list.begin() ; p != name_list.end() ; ++p )
		{
			const std::string & name = *p ;
			G::Path path ;
			{
				std::string sname = std::string() + "/" + name ;
				if( sname.find(Dir::boot().str()) == 0U )
				{
					// ("dir-boot" may not be writeable so bootcopy() allows us
					// to squirrel the files away somewhere else where Boot::install()
					// can get at them)

					G::Path dst_dir = Dir::bootcopy( value("dir-boot") , value("dir-install") ) ;
					G_DEBUG( "InstallerImp::insertActions: [" << name << "] boot-copy-> [" << dst_dir << "]" ) ;
					if( dst_dir != G::Path() )
					{
						path = G::Path::join( dst_dir , name.substr(Dir::boot().str().length()-1U) ) ;
						if( m_unpack.flags(name).find('x') != std::string::npos )
							target_link_info.target = path ; // eek!
					}
				}
				else if( sname.find(Dir::config().str()) == 0U )
				{
					path = G::Path::join( value("dir-config") , name.substr(Dir::config().str().length()-1U) ) ;
				}
				else if( sname.find(Dir::install().str()) == 0U )
				{
					path = G::Path::join( value("dir-install") , name.substr(Dir::install().str().length()-1U) ) ;
				}
				else
				{
					path = G::Path::join( value("dir-install") , name ) ;
				}
				G_DEBUG( "InstallerImp::insertActions: [" << name << "] -> [" << path << "]" ) ;
			}
			if( path != G::Path() )
			{
				if( dir_set.find(path.dirname().str()) == dir_set.end() )
				{
					dir_set.insert( path.dirname().str() ) ;
					insert( new CreateDirectory("target",path.dirname().str()) ) ;
				}
				insert( new Extract(m_unpack,name,path) ) ;
			}
		}
	}

	// extract the gui without its packed-file payload
	//
	if( m_installing )
	{
		G::Path gui = Dir::gui( value("dir-install") ) ;
		insert( new ExtractOriginal(m_argv0,m_unpack,gui) ) ;
		if( CopyFrameworks::active(m_argv0) )
			insert( new CopyFrameworks(m_argv0,gui.dirname()) ) ;
		insert( new CreatePointerFile(Pointer::file(gui.str()),gui,m_map) ) ;
	}

	// copy dlls -- note that the dlls are locked if we are re-running in the target directory
	//
	if( m_installing && isWindows() )
	{
		if( G::File::exists("mingwm10.dll") )
			insert( new Copy(value("dir-install"),"mingwm10.dll") ) ;
		if( G::File::exists("QtCore4.dll") )
			insert( new Copy(value("dir-install"),"QtCore4.dll") ) ;
		if( G::File::exists("QtGui4.dll") )
			insert( new Copy(value("dir-install"),"QtGui4.dll") ) ;
	}

	// register for using the windows event log - doing it here, hopefully as 
	// an Administrator, has a chance of avoiding the Windows 7 VirtualStore 
	// nonesense - see also glogoutput_win32.cpp
	//
	if( m_installing && isWindows() )
	{
		insert( new RegisterAsEventSource(value("dir-install"),"emailrelay") ) ;
	}

	// create startup links
	//
	G::Path working_dir = value("dir-config") ;
	const bool is_mac = yes(value("start-is-mac")) ;
	if( !is_mac )
	{
		insert( new UpdateLink(m_argv0,yes(value("start-link-desktop")),value("dir-desktop"),working_dir,target_link_info) ) ;
		insert( new UpdateLink(m_argv0,yes(value("start-link-menu")),value("dir-menu"),working_dir,target_link_info) ) ;
	}
	insert( new UpdateLink(m_argv0,yes(value("start-at-login")),value("dir-login"),working_dir,target_link_info) ) ;
	insert( new UpdateBootLink(yes(value("start-on-boot")),value("dir-boot"),target_link_info) ) ;
	if( isWindows() )
	{
		insert( new UpdateLink(m_argv0,true,value("dir-install"),working_dir,target_link_info) ) ;
	}

	// create and edit the boot-time config file
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
		std::string head = std::string() + "server NONE " + value(k) ;
		std::string tail = std::string() + " " + "trusted" ;
		map[head] = head + tail ;
	}
}

void InstallerImp::addSecret( G::StringMap & map ,
	const std::string & side , const std::string & k1 , const std::string & k2 ) const
{
	if( exists(k2+"-name") && !value(k2+"-name").empty() )
	{
		std::string head = side + " " + value(k1) + " " + value(k2+"-name") ;
		std::string tail = std::string() + " " + value(k2+"-password") ;
		map[head] = head + tail ;
	}
}

LinkInfo InstallerImp::targetLinkInfo() const
{
	G::Path target_exe = Dir::server( value("dir-install") ) ;
	G::Path icon = Dir::icon( value("dir-install") ) ;
	G::Strings args = commandlineArgs() ;

	LinkInfo link_info ;
	link_info.target = target_exe ;
	link_info.args = args ;
	link_info.raw_target = target_exe ;
	link_info.raw_args = args ;
	link_info.icon = icon ;
	return link_info ;
}

G::Strings InstallerImp::commandlineArgs( bool short_ , bool no_close_stderr ) const
{
	G::Strings result ;
	std::pair<std::string,Map> pair = commandlineMap( short_ ) ;
	Map & map = pair.second ;
	for( Map::iterator p = map.begin() ; p != map.end() ; ++p )
	{
		std::string switch_ = (*p).first ;
		std::string switch_arg = (*p).second ;
		std::string dash = switch_.length() > 1U ? "--" : "-" ;
		if( no_close_stderr && ( switch_ == "d" || switch_ == "as-server" ) )
		{
			result.push_back( short_ ? "-l" : "--log" ) ;
		}
		else if( no_close_stderr && ( switch_ == "y" || switch_ == "as-proxy" ) )
		{
			result.push_back( short_ ? "-O" : "--poll" ) ; result.push_back( "0" ) ;
			result.push_back( short_ ? "-o" : "--forward-to" ) ;
		}
		else
		{
			result.push_back( dash + switch_ ) ;
		}
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
		else if( yes(value("forward-on-disconnect")) )
		{
			out[short_?"O":"poll"] = "0" ;
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

Installer::Installer( G::Path argv0 , G::Path payload , bool installing ) :
	m_argv0(argv0) ,
	m_payload(payload) ,
	m_installing(installing) ,
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
	m_imp = new InstallerImp(m_argv0,m_payload,m_installing,s) ;
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
		throw std::runtime_error( "internal error: invalid state" ) ;
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
 #if defined(G_WIN32) || defined(G_AS_IF_WINDOWS)
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
