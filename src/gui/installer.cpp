//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
#include "gdirectory.h"
#include "gprocess.h"
#include "glink.h"
#include "gbatchfile.h"
#include "glog.h"
#include "glogoutput.h"
#include "gexception.h"
#include "gnewprocess.h"
#include "gmapfile.h"
#include "boot.h"
#include "access.h"
#include "serverconfiguration.h"
#include "installer.h"
#include "service_install.h"
#include "service_remove.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <list>
#include <cstdlib>

#ifdef CreateDirectory
#undef CreateDirectory
#endif
#ifdef CopyFile
#undef CopyFile
#endif

#ifdef G_GUI_DIR_H
#error do not use dir.h from installer.cpp
#endif

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
	static bool isMac() ;
	static bool m_is_windows ;
	static bool m_is_mac ;
} ;
bool Helper::m_is_windows = false ;
bool Helper::m_is_mac = false ;

struct ActionBase : public ActionInterface , protected Helper
{
	virtual std::string ok() const ;
} ;

struct CreateDirectory : public ActionBase
{
	std::string m_display_name ;
	std::string m_ok ;
	G::Path m_path ;
	bool m_tight_permissions ;
	CreateDirectory( std::string display_name , std::string path , bool tight_permissions = false ) ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
	virtual void run() ;
} ;

struct CreatePointerFile : public ActionBase
{
	G::Path m_pointer_file ;
	G::Path m_gui_exe ;
	G::Path m_dir_config ;
	G::Path m_dir_install ;
	CreatePointerFile( const G::Path & pointer_file , const G::Path & gui_exe , const G::Path & dir_config , const G::Path & dir_install ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct CreateFilterScript : public ActionBase
{
	G::Path m_path ;
	std::string m_type ;
	std::string m_ok ;
	CreateFilterScript( const G::Path & path , bool client ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct CopyFile : public ActionBase
{
	G::Path m_src ;
	G::Path m_dst ;
	std::string m_flags ;
	CopyFile( G::Path src , G::Path dst , std::string flags ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct CopyTree : public ActionBase
{
	G::Path m_src ;
	G::Path m_dst ;
	CopyTree( G::Path src , G::Path dst ) ;
	virtual void run() ;
	virtual std::string text() const ;
	void add( int depth , G::Path , G::Path ) const ;
} ;

struct FileGroup : public ActionBase
{
	std::string m_path ;
	std::string m_tail ;
	std::string m_ok ;
	FileGroup( const std::string & , const std::string & ) ;
	void exec( const std::string & , const std::string & ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct CreateSecrets : public ActionBase
{
	G::Path m_path ;
	G::Path m_template ;
	G::StringMap m_content ;
	CreateSecrets( const std::string & config_dir , const std::string & filename , G::Path template_ , G::StringMap content ) ;
	virtual void run() ;
	virtual std::string text() const ;
	static bool match( std::string , std::string ) ;
} ;

struct CreateBatchFile : public ActionBase
{
	G::Path m_bat ;
	G::Path m_exe ;
	G::StringArray m_args ;
	CreateBatchFile( const G::Path & bat , const G::Path & exe , const G::StringArray & args ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct UpdateLink : public ActionBase
{
	bool m_active ;
	G::Path m_link_dir ;
	G::Path m_working_dir ;
	G::Path m_target ;
	G::StringArray m_args ;
	G::Path m_icon ;
	G::Path m_link_path ;
	std::string m_ok ;
	UpdateLink( bool active , G::Path link_dir , G::Path working_dir , G::Path target , const G::StringArray & args , G::Path icon ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct UpdateBootLink : public ActionBase
{
	bool m_active ;
	std::string m_ok ;
	G::Path m_dir_boot ;
	std::string m_name ;
	G::Path m_startstop_src ;
	G::Path m_exe ;
	UpdateBootLink( bool active , G::Path dir_boot , std::string , G::Path startstop_src , G::Path exe ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct InstallService : public ActionBase
{
	bool m_active ;
	std::string m_ok ;
	G::Path m_bat ;
	G::Path m_service_wrapper ;
	InstallService( bool active , G::Path bat , G::Path wrapper ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct RegisterAsEventSource : public ActionBase
{
	G::Path m_exe ;
	explicit RegisterAsEventSource( const G::Path & ) ;
	virtual void run() ;
	virtual std::string text() const ;
} ;

struct CreateConfigFile : public ActionBase
{
	std::string m_ok ;
	G::Path m_template ;
	G::Path m_dst ;
	CreateConfigFile( G::Path dst_dir , std::string dst_name , G::Path template_ ) ;
	virtual void run() ;
	virtual std::string text() const ;
	virtual std::string ok() const ;
} ;

struct EditConfigFile : public ActionBase
{
	G::Path m_path ;
	G::MapFile m_server_config ;
	bool m_do_backup ;
	EditConfigFile( G::Path dir , std::string name , const G::MapFile & server_config , bool ) ;
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
	InstallerImp( bool installing , bool is_windows , bool is_mac , const G::Path & payload_path , std::istream & ) ;
	~InstallerImp() ;
	bool next() ;
	Action & current() ;
	G::ExecutableCommand launchCommand() const ;

private:
	InstallerImp( const InstallerImp & ) ;
	void operator=( const InstallerImp & ) ;
	void insertActions() ;
	std::string pvalue( const std::string & key , const std::string & default_ ) const ;
	std::string pvalue( const std::string & key ) const ;
	std::string ivalue( const std::string & key ) const ;
	bool exists( const std::string & key ) const ;
	static bool yes( const std::string & ) ;
	static bool no( const std::string & ) ;
	G::StringMap allSecrets() const ;
	void addSecret( G::StringMap & , const std::string & ) const ;
	void addSecret( G::StringMap & , const std::string & , const std::string & , const std::string & ) const ;
	void insert( ActionInterface * p ) ;

private:
	typedef std::list<Action> List ;
	bool m_installing ;
	G::MapFile m_installer_config ;
	G::Path m_payload ;
	G::MapFile m_pages_output ;
	G::MapFile m_var ;
	List m_list ;
	List::iterator m_p ;
} ;

// ==

CreateDirectory::CreateDirectory( std::string display_name , std::string path , bool tight_permissions ) :
	m_display_name(display_name) ,
	m_path(path) ,
	m_tight_permissions(tight_permissions)
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
	if( m_path == G::Path() )
	{
		m_ok = "nothing to do" ;
	}
	else
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
		Access::modify( m_path , m_tight_permissions ) ;
		if( !directory.writeable() )
			throw std::runtime_error( "directory exists but is not writable" ) ;
	}
}

// ==

CreatePointerFile::CreatePointerFile( const G::Path & pointer_file , const G::Path & gui_exe ,
	const G::Path & dir_config , const G::Path & dir_install ) :
		m_pointer_file(pointer_file) ,
		m_gui_exe(gui_exe) ,
		m_dir_config(dir_config) ,
		m_dir_install(dir_install)
{
}

void CreatePointerFile::run()
{
	if( m_pointer_file == G::Path() )
		return ;

	// create the directory -- probably unnecessary
	if( !G::File::isDirectory(m_pointer_file.dirname()) )
		G::File::mkdirs( m_pointer_file.dirname() , G::File::NoThrow() ) ;

	// create the file
	std::ofstream stream( m_pointer_file.str().c_str() ) ;

	// add the exec preamble
	if( !isWindows() )
	{
		stream << "#!/bin/sh\n" ;
		if( m_gui_exe != G::Path() )
			stream << "exec \"`dirname \\\"$0\\\"`/" << m_gui_exe.basename() << "\" \"$@\"\n" ;
	}

	// write the pointer variable(s)
	G::MapFile::writeItem( stream , "dir-config" , m_dir_config.str() ) ;
	G::MapFile::writeItem( stream , "dir-install" , m_dir_install.str() ) ;

	// close the file
	if( !stream.good() )
		throw std::runtime_error( std::string() + "cannot write to \"" + m_pointer_file.basename() + "\"" ) ;
	stream.close() ;

	// make both files executable
	if( !isWindows() )
	{
		G::File::chmodx( m_pointer_file ) ;
		G::File::chmodx( m_gui_exe ) ; // hopefully redundant
	}
}

std::string CreatePointerFile::text() const
{
	return m_pointer_file == G::Path() ?
		std::string("creating pointer file") :
		( "creating pointer file [" + m_pointer_file.str() + "]" ) ;
}

std::string CreatePointerFile::ok() const
{
	return m_pointer_file == G::Path() ? std::string("nothing to do") : ActionBase::ok() ;
}

// ==

CopyFile::CopyFile( G::Path src , G::Path dst , std::string flags ) :
	m_src(src) ,
	m_dst(dst) ,
	m_flags(flags)
{
}

void CopyFile::run()
{
	G_LOG( "CopyFile::run: copy file [" << m_src << "] -> [" << m_dst << "]" ) ;
	G::File::mkdirs( m_dst.dirname() , G::File::NoThrow() , 8 ) ;
	G::File::copy( m_src , m_dst ) ;

	if( m_flags.find("x") != std::string::npos ||
		G::File::executable(m_src) ||
		m_dst.extension() == "sh" || m_dst.extension() == "bat" ||
		m_dst.extension() == "exe" || m_dst.extension() == "pl" )
			G::File::chmodx( m_dst ) ;
}

std::string CopyFile::text() const
{
	return "copying [" + m_dst.basename() + "] -> [" + m_dst.dirname().str() + "]" ;
}

// ==

CopyTree::CopyTree( G::Path src , G::Path dst ) :
	m_src(src) ,
	m_dst(dst)
{
}

void CopyTree::add( int depth , G::Path src_dir , G::Path dst_dir ) const
{
	if( depth > 10 ) return ;
	G::File::mkdir( dst_dir , G::File::NoThrow() ) ;
	G_LOG( "CopyTree::add: scanning [" << src_dir << "]" ) ;
	G::Directory d( src_dir ) ;
	G::DirectoryIterator iter( d ) ;
	while( iter.more() )
	{
		if( iter.isDir() )
		{
			G_LOG( "CopyTree::add: recursion: [" << iter.filePath() << "] [" << dst_dir << "] [" << iter.fileName() << "]" ) ;
			add( depth+1 , iter.filePath() , dst_dir+iter.fileName() ) ; // recurse
		}
		else
		{
			G::Path src = iter.filePath() ;
			G::Path dst = dst_dir + iter.fileName() ;
			G_LOG( "CopyTree::add: depth=" << depth << ": copy file [" << src << "] -> [" << dst << "]" ) ;
			G::File::copy( src , dst ) ;
			if( G::File::executable(src) ||
				dst.extension() == "sh" || dst.extension() == "bat" ||
				dst.extension() == "exe" || dst.extension() == "pl" )
					G::File::chmodx( dst ) ;
		}
	}
}

void CopyTree::run()
{
	G_LOG( "CopyTree::run: copy tree [" << m_src << "] -> [" << m_dst << "]" ) ;
	add( 0 , m_src , m_dst ) ;
}

std::string CopyTree::text() const
{
	G::Path src_etc = m_src + "..." ;
	return "copying [" + src_etc.str() + "] -> [" + m_dst.str() + "]" ;
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
	G::Str::splitIntoTokens( m_tail , parts , G::Str::ws() ) ;
	if( parts.size() > 0U && !parts.at(0U).empty() ) exec( "/bin/chgrp" , parts.at(0U) + " " + m_path ) ;
	if( parts.size() > 1U ) exec( "/bin/chmod" , parts.at(1U) + " " + m_path ) ;
	if( parts.size() > 2U ) exec( "/bin/chmod" , parts.at(2U) + " " + m_path ) ;
}

void FileGroup::exec( const std::string & exe , const std::string & tail )
{
	G_LOG( "FileGroup::exec: [" << exe << "] [" << tail << "]" ) ;

	G::StringArray args ; G::Str::splitIntoTokens( tail , args , G::Str::ws() ) ;
	G::NewProcess child( exe , args ) ;
	int rc = child.wait().run().get() ;

	if( rc != 0 )
		m_ok = "failed" ;
}

std::string FileGroup::text() const
{
	return "setting group permissions [" + m_path + " " + m_tail + "]" ;
}

std::string FileGroup::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

// ==

CreateSecrets::CreateSecrets( const std::string & config_dir , const std::string & filename ,
	G::Path template_ , G::StringMap content ) :
		m_path(config_dir,filename) ,
		m_template(template_) ,
		m_content(content)
{
}

std::string CreateSecrets::text() const
{
	return std::string() + "creating authentication secrets file [" + m_path.str() + "]" ;
}

bool CreateSecrets::match( std::string p1 , std::string p2 )
{
	// true if p1 starts with p2
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
		G::StringArray result ;
		for( G::StringArray::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
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
		line_list.swap( result ) ;
	}

	// write a header if none
	if( line_list.empty() )
	{
		if( m_template != G::Path() && G::File::exists(m_template) )
		{
			std::ifstream file( m_template.str().c_str() ) ;
			while( file.good() )
			{
				std::string line = G::Str::readLineFrom( file , "\n" ) ;
				if( !file ) break ;
				line_list.push_back( line ) ;
			}
		}
		if( line_list.empty() )
		{
			line_list.push_back( "#" ) ;
			line_list.push_back( std::string() + "# " + m_path.basename() ) ;
			line_list.push_back( "#" ) ;
			line_list.push_back( "# client plain <name> <password>" ) ;
			line_list.push_back( "# client md5 <name> <password-hash>" ) ;
			line_list.push_back( "# server plain <name> <password>" ) ;
			line_list.push_back( "# server md5 <name> <password-hash>" ) ;
			line_list.push_back( "# server none <address-range> <verifier-keyword>" ) ;
			line_list.push_back( "#" ) ;
		}
	}

	// assemble the new file
	for( G::StringMap::iterator map_p = m_content.begin() ; map_p != m_content.end() ; ++map_p )
	{
		bool replaced = false ;
		for( G::StringArray::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
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
		std::string timestamp = G::Date(now).string(G::Date::Format::yyyy_mm_dd) + G::Time(now).hhmmss() ;
		G::Path backup_path( m_path.dirname() , m_path.basename() + "." + timestamp ) ;
		G::Process::Umask umask( G::Process::Umask::Mode::Tightest ) ;
		G::File::copy( m_path , backup_path , G::File::NoThrow() ) ;
	}

	// write the new file
	std::ofstream file( m_path.str().c_str() ) ;
	bool ok = file.good() ;
	for( G::StringArray::iterator line_p = line_list.begin() ; line_p != line_list.end() ; ++line_p )
	{
		file << *line_p << std::endl ;
	}
	ok = ok && file.good() ;
	file.close() ;
	if( !ok )
		throw std::runtime_error(std::string()+"cannot create \""+m_path.basename()+"\"") ;
}

// ==

CreateBatchFile::CreateBatchFile( const G::Path & bat , const G::Path & exe , const G::StringArray & args ) :
	m_bat(bat) ,
	m_exe(exe) ,
	m_args(args)
{
}

std::string CreateBatchFile::text() const
{
	return std::string() + "creating batch file [" + m_bat.str() + "]" ;
}

void CreateBatchFile::run()
{
	G::StringArray all_args = m_args ;
	all_args.insert( all_args.begin() , m_exe.str() ) ;
	G::BatchFile::write( m_bat , all_args , "emailrelay" ) ;
}

// ==

UpdateLink::UpdateLink( bool active , G::Path link_dir , G::Path working_dir ,
	G::Path target , const G::StringArray & args , G::Path icon ) :
		m_active(active) ,
		m_link_dir(link_dir) ,
		m_working_dir(working_dir) ,
		m_target(target) ,
		m_args(args) ,
		m_icon(icon)
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
		GLink link( m_target , "E-MailRelay" , "Starts the E-MailRelay server in the background" ,
			m_working_dir , m_args , m_icon , GLink::Show::Hide ,
			"E-MailRelay" , "Generated by the E-MailRelay configuration GUI" ) ;

		//G::Process::Umask umask( G::Process::Umask::Mode::Tightest ) ;
		G::File::mkdirs( m_link_dir , 10 ) ;
		link.saveAs( m_link_path ) ;
	}
	else
	{
		m_ok = GLink::remove( m_link_path ) ? "removed" : "nothing to do" ;
	}
}

std::string UpdateLink::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

// ==

InstallService::InstallService( bool active , G::Path bat , G::Path service_wrapper ) :
	m_active(active) ,
	m_bat(bat) ,
	m_service_wrapper(service_wrapper)
{
}

void InstallService::run()
{
	if( m_bat == G::Path() || m_service_wrapper == G::Path() )
	{
		m_ok = "nothing to do" ;
	}
	else if( m_active )
	{
		bool ok = Boot::install( G::Path() , "emailrelay" , m_bat , m_service_wrapper ) ;
		m_ok = ok ? "installed" : "failed" ;
	}
	else
	{
		bool ok = Boot::uninstall( G::Path() , "emailrelay" , m_bat , m_service_wrapper ) ;
		m_ok = ok ? "uninstalled" : "nothing to do" ;
	}
}

std::string InstallService::text() const
{
	return std::string(m_active?"":"un") + "installing service" ;
}

std::string InstallService::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

// ==

UpdateBootLink::UpdateBootLink( bool active , G::Path dir_boot , std::string name , G::Path startstop_src , G::Path exe ) :
	m_active(active) ,
	m_dir_boot(dir_boot) ,
	m_name(name) ,
	m_startstop_src(startstop_src) ,
	m_exe(exe)
{
}

std::string UpdateBootLink::text() const
{
	return std::string() + "updating boot configuration [" + (m_dir_boot+m_name).str() + "]" ;
}

void UpdateBootLink::run()
{
	if( m_dir_boot == G::Path() || m_startstop_src == G::Path() || m_exe == G::Path() )
	{
		m_ok = "nothing to do" ;
	}
	else if( m_active )
	{
		if( ! Boot::install( m_dir_boot , m_name , m_startstop_src , m_exe ) )
			throw std::runtime_error( "failed to create links" ) ;
		m_ok = "installed" ;
	}
	else
	{
		bool removed = Boot::uninstall( m_dir_boot , m_name , m_startstop_src , m_exe ) ;
		m_ok = removed ? "removed" : "nothing to remove" ;
	}
}

std::string UpdateBootLink::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
}

// ==

RegisterAsEventSource::RegisterAsEventSource( const G::Path & exe ) :
	m_exe(exe)
{
}

void RegisterAsEventSource::run()
{
	if( m_exe != G::Path() )
		G::LogOutput::register_( m_exe.str() ) ;
}

std::string RegisterAsEventSource::text() const
{
	return std::string() + "registering [" + m_exe.str() + "]" ;
}

// ==

CreateFilterScript::CreateFilterScript( const G::Path & path , bool client_filter ) :
	m_path(path) ,
	m_type(client_filter?"client ":"")
{
}

void CreateFilterScript::run()
{
	if( m_path == G::Path() )
	{
		m_ok = "nothing to do" ;
	}
	else if( G::File::exists(m_path) )
	{
		m_ok = "exists" ;
	}
	else
	{
		std::ofstream f( m_path.str().c_str() ) ;
		if( isWindows() )
			f << "WScript.Quit(0);\r\n" << std::flush ;
		else
			f << "#!/bin/sh\nexit 0" << std::endl ;
		if( !f.good() )
			throw std::runtime_error( std::string() + "cannot write to \"" + m_path.basename() + "\"" ) ;
		f.close() ;
		if( !isWindows() )
			G::File::chmodx( m_path ) ;
	}
}

std::string CreateFilterScript::text() const
{
	return std::string() + "creating " + m_type + "filter script [" + m_path.str() + "]" ;
}

std::string CreateFilterScript::ok() const
{
	return m_ok.empty() ? ActionBase::ok() : m_ok ;
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
		m_ok = "exists" ;
	else if( G::File::exists(m_template) )
		G::File::copy( m_template , m_dst ) ;
	else
		G::File::create( m_dst ) ;
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

EditConfigFile::EditConfigFile( G::Path dir , std::string name , const G::MapFile & server_config , bool do_backup ) :
	m_path(dir+name) ,
	m_server_config(server_config) ,
	m_do_backup(do_backup)
{
}

void EditConfigFile::run()
{
	m_server_config.editInto( m_path , m_do_backup , /*allow-read-error=*/false , /*allow-write-error=*/false ) ;
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

InstallerImp::InstallerImp( bool installing , bool is_windows , bool is_mac , const G::Path & payload , std::istream & ss ) :
	m_installing(installing) ,
	m_payload(payload) ,
	m_pages_output(ss)
{
	Helper::m_is_windows = is_windows ;
	Helper::m_is_mac = is_mac ;

	// define ivalue o/s-specific paths
	m_installer_config.add( "-authtemplate" , isWindows() ? "" : "%payload%/etc/emailrelay.auth.template" ) ;
	m_installer_config.add( "-conftemplate" , isWindows() ? "" : "%payload%/etc/emailrelay.conf.template" ) ;
	m_installer_config.add( "-bat" , isWindows() ? "%dir-config%/emailrelay-start.bat" : "" ) ; // not dir-install -- see guimain
	m_installer_config.add( "-exe" , isWindows() ? "%dir-install%/emailrelay.exe" :
		( isMac() ? "%dir-install%/E-MailRelay.app/Contents/MacOS/emailrelay" : "%dir-install%/sbin/emailrelay" ) ) ;
	m_installer_config.add( "-gui" , isWindows() ? "%dir-install%/emailrelay-gui.exe" : "%dir-install%/sbin/emailrelay-gui.real" ) ;
	m_installer_config.add( "-icon" , isWindows()?"%dir-install%/emailrelay.exe":"%dir-install%/lib/emailrelay/emailrelay-icon.png");
	m_installer_config.add( "-pointer" , isWindows() ? "%dir-install%/emailrelay-gui.cfg" : "%dir-install%/sbin/emailrelay-gui" ) ;
	m_installer_config.add( "-startstop" , isWindows() ? "" : "%dir-install%/lib/emailrelay/emailrelay-startstop.sh" ) ;
	m_installer_config.add( "-servicewrapper" , isWindows() ? "%dir-install%/emailrelay-service.exe" : "" ) ;
	m_installer_config.add( "-filtercopy" , isWindows() ?
			"%dir-install%/emailrelay-filter-copy.exe" : "%dir-install%/lib/emailrelay/emailrelay-filter-copy" ) ;

	// define some substitution variables (used for expansion of pvalues, ivalues and payload.cfg)
	m_var.add( "dir-install" , pvalue("dir-install") ) ;
	m_var.add( "dir-config" , pvalue("dir-config") ) ;
	m_var.add( "dir-run" , pvalue("dir-run") ) ;
	m_var.add( "dir-spool" , pvalue("dir-spool") ) ;
	m_var.add( "payload" , m_payload.str() ) ;

	insertActions() ;
	m_p = m_list.end() ; // sic
}

InstallerImp::~InstallerImp()
{
}

G::ExecutableCommand InstallerImp::launchCommand() const
{
	if( isWindows() )
	{
		return G::ExecutableCommand( ivalue("-bat") , G::StringArray() ) ;
	}
	else
	{
		G::Path filter_copy = ivalue("-filtercopy") ;
		G::Path target = ivalue("-exe") ;
		ServerConfiguration sc = ServerConfiguration::fromPages( m_pages_output , filter_copy ) ;
		return G::ExecutableCommand( target , sc.args() ) ;
	}
}

bool InstallerImp::next()
{
	if( m_p == m_list.end() )
		m_p = m_list.begin() ;
	else
		++m_p ;
	return m_p != m_list.end() ;
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
		insert( new CreateDirectory("install",pvalue("dir-install"),true) ) ;
		insert( new CreateDirectory("configuration",pvalue("dir-config")) ) ;
	}
	insert( new CreateDirectory("runtime",pvalue("dir-run")) ) ;
	insert( new CreateDirectory("spool",pvalue("dir-spool")) ) ;

	// create pop-by-name sub-directories
	//
	{
		G::Path spool_dir( pvalue("dir-spool") ) ;
		std::vector<std::string> names ;
		names.push_back( pvalue("pop-account-1-name") ) ;
		names.push_back( pvalue("pop-account-2-name") ) ;
		names.push_back( pvalue("pop-account-3-name") ) ;
		for( std::vector<std::string>::iterator name_p = names.begin() ; name_p != names.end() ; ++name_p )
		{
			if( (*name_p).empty() ) continue ;
			G::Path dir( spool_dir , *name_p ) ;
			insert( new CreateDirectory("pop-by-name",dir.str()) ) ;
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

		// insert the file copy tasks
		for( G::StringArray::const_iterator p = payload_map.keys().begin() ; p != payload_map.keys().end() ; ++p )
		{
			const std::string & key = *p ;
			const std::string & value = payload_map.value( key ) ;
			if( key.find("+") == 0U && key.length() > 1U && value.find("group ") == 0U && value.length() > 6U )
				insert( new FileGroup(m_var.expand(key.substr(1U)),value.substr(6U)) ) ;
			if( key.find_first_of("-+=") == 0U )
				continue ;

			std::string dst = m_var.expand( value ) ;
			bool is_directory_tree = key.size() && key.at(key.size()-1U) == '/' ;

			// allow for flags such as "+x" decorating the destination
			std::string::size_type flags_pos = dst.find_last_of("+") ;
			std::string flags = G::Str::tail( dst , flags_pos , std::string() ) ;
			dst = G::Str::trimmed( G::Str::head( dst , flags_pos , dst ) , G::Str::ws() ) ;

			G::Path src = m_payload + key ;
			if( is_directory_tree )
				insert( new CopyTree(src,dst) ) ;
			else
				insert( new CopyFile(src,dst,flags) ) ;
		}
	}

	// create secrets
	//
	G::Path authtemplate_src = m_installing ? ivalue( "-authtemplate" ) : std::string() ;
	insert( new CreateSecrets(pvalue("dir-config"),"emailrelay.auth",authtemplate_src,allSecrets()) ) ;

	// create a startup batch file
	//
	if( isWindows() )
	{
		G::Path filter_copy = ivalue("-filtercopy") ;
		G::Path exe = ivalue("-exe") ;
		G::Path bat = ivalue("-bat") ;
		G::StringArray args = ServerConfiguration::fromPages(m_pages_output,filter_copy).args() ;
		insert( new CreateBatchFile(bat,exe,args) ) ;
	}

	// create the pointer file so that the gui program can be used to re-configure
	//
	if( m_installing )
	{
		G::Path pointer_file = ivalue( "-pointer" ) ;
		G::Path gui_exe = ivalue( "-gui" ) ;
		G::Path dir_config = pvalue( "dir-config" ) ;
		G::Path dir_install = pvalue( "dir-install" ) ;
		insert( new CreatePointerFile(pointer_file,gui_exe,dir_config,dir_install) ) ;
	}

	// register for using the windows event log - doing it here since the server
	// will not have administrator privilege
	//
	if( m_installing && isWindows() )
	{
		insert( new RegisterAsEventSource(ivalue("-exe")) ) ;
	}

	// create filter scripts
	//
	if( m_installing )
	{
		if( !pvalue("filter-server").empty() && no(pvalue("pop-filter-copy")) )
		{
			insert( new CreateDirectory( "filter" , G::Path(pvalue("filter-server")).dirname().str() ) ) ;
			insert( new CreateFilterScript( pvalue("filter-server") , false ) ) ;
		}

		if( !pvalue("filter-client").empty() )
		{
			insert( new CreateDirectory( "client-filter" , G::Path(pvalue("filter-client")).dirname().str() ) ) ;
			insert( new CreateFilterScript( pvalue("filter-client") , true ) ) ;
		}
	}

	// create startup links and startup config
	//
	{
		G::Path server_exe = ivalue("-exe") ;
		G::Path working_dir = pvalue("dir-config") ;
		G::Path dir_config = pvalue( "dir-config" ) ;
		G::Path dir_install = pvalue( "dir-install" ) ;

		bool do_desktop = yes(pvalue("start-link-desktop")) && !yes(pvalue("start-is-mac")) ;
		bool do_menu = yes(pvalue("start-link-menu")) && !yes(pvalue("start-is-mac")) ;
		bool do_login = yes(pvalue("start-at-login")) ;
		bool do_boot = yes(pvalue("start-on-boot")) ;

		G::Path dir_desktop = pvalue( "dir-desktop" ) ;
		G::Path dir_menu = pvalue( "dir-menu" ) ;
		G::Path dir_login = pvalue( "dir-login" ) ;
		G::Path dir_boot = pvalue( "dir-boot" ) ;

		G::Path bat = ivalue( "-bat" ) ;
		G::Path filter_copy = ivalue("-filtercopy") ;
		G::Path target = isWindows() ? bat : server_exe ;
		G::StringArray args = isWindows() ? G::StringArray() : ServerConfiguration::fromPages(m_pages_output,filter_copy).args() ;
		G::Path icon = ivalue( "-icon" ) ;

		insert( new UpdateLink(do_desktop,dir_desktop,working_dir,target,args,icon) ) ;
		insert( new UpdateLink(do_menu,dir_menu,working_dir,target,args,icon) ) ;
		insert( new UpdateLink(do_login,dir_login,working_dir,target,args,icon) ) ;

		if( isWindows() )
		{
			insert( new UpdateLink(true,dir_install,working_dir,target,args,icon) ) ;

			G::Path service_wrapper = ivalue( "-servicewrapper" ) ;
			insert( new InstallService(do_boot,bat,service_wrapper) ) ;
		}
		else
		{
			// install the startstop script and its config file
			G::Path conftemplate_src = m_installing ? ivalue( "-conftemplate" ) : std::string() ;
			G::MapFile server_config = ServerConfiguration::fromPages(m_pages_output,filter_copy).map() ;
			insert( new UpdateBootLink(do_boot,dir_boot,"emailrelay",ivalue("-startstop"),server_exe) ) ;
			insert( new CreateConfigFile(dir_config,"emailrelay.conf",conftemplate_src) ) ;
			insert( new EditConfigFile(dir_config,"emailrelay.conf",server_config,!m_installing) ) ;
		}
	}
}

G::StringMap InstallerImp::allSecrets() const
{
	G::StringMap map ;
	if( yes(pvalue("do-pop")) )
	{
		addSecret( map , "server" , "pop-auth-mechanism" , "pop-account-1" ) ;
		addSecret( map , "server" , "pop-auth-mechanism" , "pop-account-2" ) ;
		addSecret( map , "server" , "pop-auth-mechanism" , "pop-account-3" ) ;
	}
	if( yes(pvalue("do-smtp")) && yes(pvalue("smtp-server-auth")) )
	{
		addSecret( map , "server" , "smtp-server-auth-mechanism" , "smtp-server-account" ) ;
		addSecret( map , "smtp-server-trust" ) ;
	}
	if( yes(pvalue("do-smtp")) && yes(pvalue("smtp-client-auth")) )
	{
		addSecret( map , "client" , "smtp-client-auth-mechanism" , "smtp-client-account" ) ;
	}
	return map ;
}

void InstallerImp::addSecret( G::StringMap & map , const std::string & k ) const
{
	if( exists(k) && !pvalue(k).empty() )
	{
		std::string head = "server none " + pvalue(k) ;
		std::string tail = "trusted" ;
		map[head] = head + " " + tail ;
	}
}

void InstallerImp::addSecret( G::StringMap & map ,
	const std::string & side , const std::string & k1 , const std::string & k2 ) const
{
	if( exists(k2+"-name") && !pvalue(k2+"-name").empty() )
	{
		if( side == "server" )
		{
			std::string head = side + " " + pvalue(k1) + " " + pvalue(k2+"-name") ; // eg. "server plain joe"
			std::string tail = pvalue(k2+"-password") ; // eg. "secret"
			map[head] = head + " " + tail ;
		}
		else
		{
			std::string head = side + " " + pvalue(k1) ; // eg. "client plain"
			std::string tail = pvalue(k2+"-name") + " " + pvalue(k2+"-password") ; // eg. "joe secret"
			map[head] = head + " " + tail ;
		}
	}
}

// ==

Installer::Installer( bool installing , bool is_windows , bool is_mac , const G::Path & payload ) :
	m_installing(installing) ,
	m_is_windows(is_windows) ,
	m_is_mac(is_mac&&!is_windows) ,
	m_payload(payload) ,
	m_imp(nullptr)
{
}

Installer::~Installer()
{
	delete m_imp ;
}

void Installer::start( std::istream & s )
{
	delete m_imp ;
	m_imp = new InstallerImp( m_installing , m_is_windows , m_is_mac , m_payload , s ) ;
	m_launch_command = m_imp->launchCommand() ;
	m_reason.erase() ;
}

bool Installer::next()
{
	bool more = false ;
	if( m_imp != nullptr )
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
	m_imp = nullptr ;
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

std::string Installer::reason() const
{
	return m_reason ;
}

bool Installer::done() const
{
	return m_imp == nullptr ;
}

G::ExecutableCommand Installer::launchCommand() const
{
	return m_launch_command ;
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

/// \file installer.cpp
