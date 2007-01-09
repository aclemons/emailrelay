//
// Copyright (C) 2001-2006 Graeme Walker <graeme_walker@users.sourceforge.net>
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
// tool.cpp
//
// Used by the gui to do installation/configuration steps.
//
// Reads "install.cfg".
//
// usage: tool [--show]
//

#include "gstr.h"
#include "gpath.h"
#include "gfile.h"
#include "gstr.h"
#include "gdirectory.h"
#include "gprocess.h"
#include "gcominit.h"
#include "glink.h"
#include "garg.h"
#include "package.h"
#include "ggetopt.h"
#include "glogoutput.h"
#include "gexception.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <utility>
#include <map>

typedef std::map<std::string,std::string> Map ;

struct LinkInfo
{
	G::Path target ;
	G::Strings args ;
	G::Path raw_target ;
	G::Strings raw_args ;
} ;

typedef std::list<std::pair<G::Path,std::string> > FileList ;

G_EXCEPTION( BootError , "cannot install to run at boot time" ) ;

Map read( const std::string & path ) ;
int run( int argc , char * argv [] ) ;
void show( G::Path , const Map & map ) ;
void action( G::Path , const Map & map ) ;
void secretsLine( std::ostream & stream , bool , const std::string & , 
	const Map & , const std::string & , const std::string & , const std::string & ) ;
std::string secretsFile( const Map & map , bool , const std::string & ) ;
std::string secretsFilename( const Map & map ) ;
std::string configFile( const Map & map , const std::string & ) ;
std::string configFilename( const Map & map ) ;
std::string commandlineString( const Map & map , bool , bool ) ;
G::Strings commandlineArgs( const Map & map_in , bool , bool ) ;
std::pair<std::string,Map> commandlineMap( const Map & map , bool , bool ) ;
std::string unmask( const std::string & , const std::string & ) ;
std::string value( const Map & map , const std::string & key ) ;
std::string value( const Map & map , const std::string & key , const std::string & default_ ) ;
bool exists( const Map & map , const std::string & key ) ;
bool yes( const std::string & value ) ;
bool no( const std::string & value ) ;
std::string rot13( const std::string & in ) ;
void createLink( const std::string & save_as_dir , G::Path target , const G::Strings & args , 
	G::Path working_dir , G::Path target_exe ) ;
void createLinkCore( const std::string & save_as_dir , G::Path target , const G::Strings & args , 
	G::Path working_dir , G::Path target_exe ) ;
void createBootLink( const Map & map , G::Path target , G::Strings args ) ;
void createDirectory( const Map & map , const std::string & key , const std::string & name ) ;
LinkInfo createLinkTarget( const Map & map ) ;
void createLinks( const Map & map , LinkInfo link_info ) ;
void copyFiles( G::Path , const FileList & file_list , G::Path install_dir ) ;
void installAllFiles( G::Path , const Map & map ) ;
bool unpackFiles( G::Path argv0 , G::Path install_dir ) ;
FileList fileListFromFile( G::Path , const Map & map ) ;
FileList fileListFromDirectory( G::Path , const Map & map ) ;
std::string quote( const std::string & s ) ;
std::string str( const G::Strings & ) ;
void createBootLinkUnix( G::Path boot_dir , G::Path target , G::Strings args ) ;
void createBootLinkWindows( G::Path reskit_dir , G::Path target , G::Strings args ) ;
void createBootLinkMac( G::Path target , G::Strings args ) ;

int main( int argc , char * argv [] )
{
	std::ostream & err = std::cout ;
	try
	{
		return run( argc , argv ) ;
	}
	catch( std::exception & e )
	{
		err << "** error: " << e.what() << std::endl ;
	}
	catch( ... )
	{
		err << "** error: unknown exception" << std::endl ;
	}
	err << "** failed **" << std::endl ;
	return 1 ;
}

Map read( const std::string & path )
{
	std::ifstream file( path.c_str() ) ;
	if( !file.good() )
		throw std::runtime_error( std::string() + "cannot open \"" + path + "\"" ) ;

	Map map ;
	std::string line ;
	while( file.good() )
	{
		G::Str::readLineFrom( file , "\n" , line ) ;
		if( line.empty() || line.find('#') == 0U || line.find_first_not_of(" \t\r") == std::string::npos )
			continue ;

		G::StringArray part ;
		G::Str::splitIntoTokens( line , part , " \t" ) ;
		if( part.size() == 0U )
			continue ;

		std::string value = 
			part.size() == 1U ? 
				std::string() : 
				line.substr(part[0].length()) ;

		G::Str::trim( value , G::Str::ws() ) ;

		map.insert( Map::value_type(part[0],value) ) ;
	}
	return map ;
}

int run( int argc , char * argv [] )
{
	G::Arg args( argc , argv ) ;
	G::GetOpt getopt( args , 
		"h/help/show this help text and exit/0//1|"
		"f/file/specify input file/1/input-file/1|"
		"d/debug/show debug messages if compiled-in/0//1|"
		"s/show/show what needs doing wihtout doing it/0//1" ) ;
	if( getopt.hasErrors() )
	{
		getopt.showErrors( std::cerr ) ;
		return 2 ;
	}
	if( getopt.args().c() != 1U )
	{
		throw std::runtime_error( "usage error" ) ;
	}
	if( getopt.contains("help") )
	{
		getopt.showUsage( std::cout , std::string() , false ) ;
		return 0 ;
	}
	G::LogOutput log_ouptut( getopt.contains("debug") ) ;
	bool do_show = getopt.contains("show") ;
	std::string config = getopt.contains("file") ? getopt.value("file") : std::string("install.cfg") ;

	Map map = read( config ) ;

	if( do_show )
		show( G::Path(argv[0]) , map ) ;
	else
		action( G::Path(argv[0]) , map ) ;

	return 0 ;
}

void show( G::Path , const Map & map )
{
	std::cout << "Command-line:" << std::endl ;
	std::cout << G::Str::wrap(commandlineString(map,false,false)," ","   ") << std::endl ;
	if( ! configFilename(map).empty() )
	{
		std::cout << "Startup file (" << configFilename(map) << "):" << std::endl ;
		std::cout << configFile(map," ") << std::endl ;
	}
	if( ! secretsFilename(map).empty() )
	{
		std::cout << "Secrets file (" << secretsFilename(map) << "):" << std::endl ;
		std::cout << secretsFile(map,true," ") << std::endl ;
	}
}

bool isWindows()
{
 #ifdef _WIN32
	return true ;
 #else
	return false ;
 #endif
}

bool isMac()
{
	return G::File::exists("/Library/StartupItems") ; // could do better
}

std::string exe()
{
	return isWindows() ? ".exe" : "" ;
}

void createDirectory( const Map & map , bool show , const std::string & key , const std::string & name )
{
	G::Path dir( value(map,key) ) ;
	if( ! G::File::exists(dir) )
	{
		std::cout << "creating " << name << " directory [" << dir << "]" << std::endl ;
		if( ! show )
			G::File::mkdir(dir) ;
	}
}

void createSecretsFile( const Map & map )
{
	// create the auth secrets file
	G::Path config_dir( value(map,"dir-config") ) ;
	if( ! secretsFilename(map).empty() )
	{
		std::string path = secretsFilename(map) ;
		std::cout << "creating authentication secrets file [" << path << "]" << std::endl ;
		std::ofstream file( path.c_str() ) ;
		bool ok = file.good() ;
		file << secretsFile(map,false,"") ;
		ok = ok && file.good() ;
		file.close() ;
		if( !ok )
			throw std::runtime_error(std::string()+"cannot create \""+path+"\"") ;
	}
}

FileList fileListFromFile( G::Path argv0 , const Map & )
{
	FileList file_list ;
	G::Path file_list_file( argv0.dirname() , "emailrelay-files.txt" ) ;
	if( G::File::exists(file_list_file) )
	{
		std::cout << "reading file list from [" << file_list_file.basename() << "]" << std::endl ;
		std::ifstream file_list_stream( file_list_file.str().c_str() ) ;
		while( file_list_stream.good() )
		{
			std::string line = G::Str::readLineFrom( file_list_stream ) ;
			G::StringArray part ;
			G::Str::splitIntoTokens( line , part , G::Str::ws() ) ;
			if( part.size() == 1U ) part.push_back( std::string() ) ;
			if( part.size() >= 2U && !part[0].empty() && part[0][0] != '#' )
				file_list.push_back( std::make_pair(part[0],part[1]) ) ;
		}
	}
	return file_list ;
}

FileList fileListFromDirectory( G::Path argv0 , const Map & )
{
	FileList file_list ;
	G::Directory dir( argv0.dirname() ) ;
	G::DirectoryIterator iter( dir ) ;
	while( iter.more() )
	{
		// (look in immediate child subdirectories too)
		if( iter.isDir() )
		{
			G::Directory dir_inner( iter.filePath() ) ;
			G::DirectoryIterator inner( dir_inner ) ;
			while( inner.more() )
				if( ! inner.isDir() )
					file_list.push_back( std::make_pair(inner.filePath(),iter.fileName().str())) ;
		}
		else
		{
			file_list.push_back( std::make_pair(iter.filePath(),std::string()) ) ;
		}
	}
	return file_list ;
}

void installAllFiles( G::Path argv0 , const Map & map )
{
	G::Path install_dir( value(map,"dir-install") ) ;
	G::Path src_dir = argv0.dirname() ;
	if( ! unpackFiles( argv0 , install_dir ) )
	{
		FileList file_list = fileListFromFile( argv0 , map ) ;
		if( file_list.empty() )
			file_list = fileListFromDirectory( argv0 , map ) ;
		copyFiles( argv0 , file_list , install_dir ) ;
	}
}

bool unpackFiles( G::Path argv0 , G::Path install_dir )
{
	Package package( argv0 ) ;
	int n = package.count() ;
	for( int i = 0 ; i < n ; i++ )
	{
		std::string name = package.name(i) ;
		G::Path dst( install_dir , name ) ;
		if( G::File::exists(dst) )
		{
			std::cout << "not unpacking [" << name << "] onto [" << dst << "]: file exists" << std::endl ;
		}
		else
		{
			std::cout << "unpacking [" << dst << "]" << std::endl ;
			package.unpack( install_dir , package.name(i) ) ;
		}
	}
	return n > 0 ;
}

void copyFiles( G::Path argv0 , const FileList & file_list , G::Path install_dir )
{
	for( FileList::const_iterator p = file_list.begin() ; p != file_list.end() ; ++p )
	{
		G::Path src = (*p).first ;
		G::Path dst_dir = (*p).second.empty() ? install_dir : G::Path(install_dir,(*p).second) ;
		G::Path dst( dst_dir , (*p).first.basename() ) ;

		if( G::File::exists(dst) )
		{
			std::cout << "not copying [" << src << "] to [" << dst << "]: file exists" << std::endl ;
		}
		else
		{
			if( ! G::File::exists(dst_dir) )
			{
				std::cout << "creating sub-directory [" << dst_dir << "]" << std::endl ;
				G::File::mkdir(dst_dir) ;
			}

			std::cout << "copying [" << src << "] to [" << dst << "]" << std::endl ;
			G::File::copy( src , dst ) ;
			G::File::chmodx( dst ) ; // TODO should only be scripts and binaries 
		}
	}
}

void action( G::Path argv0 , const Map & map )
{
	createDirectory( map , false , "dir-install" , "install" ) ;
	createDirectory( map , false , "dir-config" , "config" ) ;
	createDirectory( map , false , "dir-spool" , "spool" ) ;
	createSecretsFile( map ) ;
	installAllFiles( argv0 , map ) ;
	createLinks( map , createLinkTarget(map) ) ;
	std::cout << "done" << std::endl ;
}

LinkInfo createLinkTarget( const Map & map )
{
	// prepare the command-line args 
	G::Path target_dir( value(map,"dir-install") ) ;
	G::Path target_exe( value(map,"dir-install") , std::string() + "emailrelay" + exe() ) ;
	G::Strings args = commandlineArgs( map , false , false ) ;

	// create a script if the command-line is too long
	bool use_batch_file = false ;
	G::Path batch_file ;
	if( isWindows() )
	{
		if( false && (target_exe.str().length()+1+str(args).length()) > 230U )
			args = commandlineArgs( map , true , false ) ;

		use_batch_file = (target_exe.str().length()+1+str(args).length()) >= 235U ;
		if( use_batch_file )
		{
			batch_file = G::Path( value(map,"dir-install") , "emailrelay-start.bat" ) ;

			std::cout << "creating batch file [" << batch_file << "]" << std::endl ;
			std::ofstream file( batch_file.str().c_str() ) ;
			bool ok = file.good() ;
			file << quote(target_exe.str()) << " " << str(args) << std::endl ;
			ok = ok && file.good() ;
			file.close() ;
			if( !ok )
				throw std::runtime_error(std::string()+"cannot create \""+batch_file.str()+"\"") ;
		}
	}

	LinkInfo link_info ;
	link_info.target = use_batch_file ? batch_file : target_exe ;
	link_info.args = use_batch_file ? G::Strings() : args ;
	link_info.raw_target = target_exe ;
	link_info.raw_args = args ;
	return link_info ;
}

void createLinks( const Map & map , LinkInfo link_info )
{
	GComInit com_init ;

	G::Path config_dir( value(map,"dir-config") ) ;
	G::Path working_dir = config_dir ;

	if( yes(value(map,"start-link-desktop")) )
	{
		createLink( value(map,"dir-desktop") , link_info.target , link_info.args , 
			working_dir , link_info.raw_target ) ;
	}

	if( yes(value(map,"start-link-menu")) )
	{
		createLink( value(map,"dir-menu") , link_info.target , link_info.args , 
			working_dir , link_info.raw_target ) ;
	}

	if( yes(value(map,"start-at-login")) )
	{
		createLink( value(map,"dir-login") , link_info.target , link_info.args , 
			working_dir , link_info.raw_target ) ;
	}

	if( yes(value(map,"start-on-boot")) )
	{
		createBootLink( map , link_info.raw_target , link_info.raw_args ) ;
	}
}

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

void createLink( const std::string & save_as_dir , G::Path target , const G::Strings & args , 
	G::Path working_dir , G::Path target_exe )
{
	try
	{
		createLinkCore( save_as_dir , target , args , working_dir , target_exe ) ;
	}
	catch( GLink::SaveError & e )
	{
		std::cout << "** error: " << e.what() << std::endl ;
	}
}

void createLinkCore( const std::string & save_as_dir , G::Path target , const G::Strings & args , 
	G::Path working_dir , G::Path target_exe )
{
	G::Path icon_path( target.dirname() , "emailrelay-icon.png" ) ;
	GLink link( target , "E-MailRelay" , "E-MailRelay server" , working_dir , str(args) , 
		isWindows() ? target_exe : icon_path , GLink::Show_Hide ) ;

	G::Process::Umask umask( G::Process::Umask::Tightest ) ;
	G::File::mkdirs( save_as_dir ) ;
	G::Path save_as( save_as_dir , GLink::filename("E-MailRelay") ) ;
	std::cout << "creating link [" << save_as << "]" << std::endl ;
	link.saveAs( save_as ) ;
}

bool exists( const Map & map , const std::string & key )
{
	return map.find(key+":") != map.end() ;
}

std::string value( const Map & map , const std::string & key , const std::string & default_ )
{
	Map::const_iterator p = map.find(key+":") ;
	return p == map.end() ? default_ : (*p).second ;
}

std::string value( const Map & map , const std::string & key )
{
	Map::const_iterator p = map.find(key+":") ;
	if( p == map.end() )
		throw std::runtime_error( std::string() + "no such value: " + key ) ;
	return (*p).second ;
}

bool yes( const std::string & value )
{
	return value.find_first_of("yY") == 0U ;
}

bool no( const std::string & value )
{
	return !yes(value) ;
}

std::string rot13( const std::string & in )
{
	std::string s( in ) ;
	for( std::string::iterator p = s.begin() ; p != s.end() ; ++p )
	{
		if( *p >= 'a' && *p <= 'z' ) 
			*p = 'a' + ( ( ( *p - 'a' ) + 13U ) % 26U ) ;
		if( *p >= 'A' && *p <= 'Z' )
			*p = 'A' + ( ( ( *p - 'A' ) + 13U ) % 26U ) ;
	}
	return s ;
}

std::string unmask( const std::string & m , const std::string & s )
{
	if( m == "CRAM-MD5" )
	{
		return s ;
	}
	else
	{
		return rot13(s) ;
	}
}

void secretsLine( std::ostream & stream , bool show , const std::string & prefix , 
	const Map & map , const std::string & side , const std::string & k1 , const std::string & k2 )
{
	if( exists(map,k2+"-name") && !value(map,k2+"-name").empty() )
	{
		stream
			<< prefix
			<< value(map,k1) << " " << side << " " 
			<< value(map,k2+"-name") << " "
			<< (show?std::string("..."):unmask(value(map,k1),value(map,k2+"-password")))
			<< std::endl ;
	}
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

std::string secretsFile( const Map & map , bool show , const std::string & prefix )
{
	std::ostringstream ss ;
	if( yes(value(map,"do-pop")) )
	{
		std::string mechanism = value(map,"pop-auth-mechanism") ;
		secretsLine( ss , show , prefix , map , "server" , "pop-auth-mechanism" , "pop-account-1" ) ;
		secretsLine( ss , show , prefix , map , "server" , "pop-auth-mechanism" , "pop-account-2" ) ;
		secretsLine( ss , show , prefix , map , "server" , "pop-auth-mechanism" , "pop-account-3" ) ;
	}
	if( yes(value(map,"do-smtp")) && yes(value(map,"smtp-server-auth")) )
	{
		std::string mechanism = value(map,"smtp-server-auth-mechanism") ;
		secretsLine( ss , show , prefix , map , "server" , "smtp-server-auth-mechanism" , "smtp-server-account" ) ;
		if( ! value(map,"smtp-server-trust").empty() )
		{
			ss << prefix << "NONE server " << value(map,"smtp-server-trust") << " trusted" << std::endl ;
		}
	}
	if( yes(value(map,"do-smtp")) && yes(value(map,"smtp-client-auth")) )
	{
		std::string mechanism = value(map,"smtp-client-auth-mechanism") ;
		secretsLine( ss , show , prefix , map , "client" , "smtp-client-auth-mechanism" , "smtp-client-account" ) ;
	}
	return ss.str() ;
}

std::string secretsFilename( const Map & map )
{
	if( yes(value(map,"do-pop")) || 
		( yes(value(map,"do-smtp")) && yes(value(map,"smtp-server-auth")) ) ||
		( yes(value(map,"do-smtp")) && yes(value(map,"smtp-client-auth")) ) )
	{
		return G::Path(value(map,"dir-config"),"emailrelay.auth").str() ;
	}
	else
	{
		return std::string() ;
	}
}

std::string commandlineString( const Map & map_in , bool short_ , bool relative )
{
	std::ostringstream ss ;
	std::pair<std::string,Map> pair = commandlineMap( map_in , short_ , relative ) ;
	ss << pair.first << " " << str(commandlineArgs(map_in,short_,relative)) ;
	return ss.str() ;
}

std::string quote( const std::string & s )
{
	return
		s.find_first_of(" \t") == std::string::npos ?
			s :
			(std::string()+"\""+s+"\"") ;
}

G::Strings commandlineArgs( const Map & map_in , bool short_ , bool relative )
{
	G::Strings result ;
	std::pair<std::string,Map> pair = commandlineMap( map_in , short_ , relative ) ;
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

std::pair<std::string,Map> commandlineMap( const Map & map , bool short_ , bool relative )
{
	std::string auth = relative ?
		std::string("emailrelay.auth") :
		G::Path(value(map,"dir-config"),"emailrelay.auth").str() ;

	Map out ;
	std::string path = G::Path(value(map,"dir-install"),"emailrelay").str() ;
	out[short_?"s":"spool-dir"] = value(map,"dir-spool") ;
	out[short_?"l":"log"] ;
	out[short_?"e":"close-stderr"] ;
	out[short_?"r":"remote-clients"] ;
	out[short_?"i":"pid-file"] = G::Path(value(map,"dir-pid"),"emailrelay.pid").str() ;
	if( yes(value(map,"do-smtp")) )
	{
		if( yes(value(map,"forward-immediate")) )
		{
			out[short_?"m":"immediate"] ;
		}
		if( yes(value(map,"forward-poll")) )
		{
			if( value(map,"forward-poll-period") == "minute" )
				out[short_?"O":"poll"] = "60" ;
			else if( value(map,"forward-poll-period") == "second" )
				out[short_?"O":"poll"] = "1" ;
			else 
				out[short_?"O":"poll"] = "3600" ;
		}
		if( value(map,"smtp-server-port") != "25" )
		{
			out[short_?"p":"port"] = value(map,"smtp-server-port") ;
		}
		if( yes(value(map,"smtp-server-auth")) )
		{
			out[short_?"S":"server-auth"] = auth ;
		}
		out[short_?"o":"forward-to"]  = value(map,"smtp-client-host") + ":" + value(map,"smtp-client-port") ;
		if( yes(value(map,"smtp-client-auth")) )
		{
			out[short_?"C":"client-auth"] = auth ;
		}
	}
	else
	{
		out[short_?"X":"no-smtp"] ;
	}
	if( yes(value(map,"do-pop")) )
	{
		out[short_?"B":"pop"] ;
		if( value(map,"pop-port") != "110" )
		{
			out[short_?"E":"pop-port"] = value(map,"pop-port") ;
		}
		if( yes(value(map,"pop-shared-no-delete")) )
		{
			out[short_?"G":"pop-no-delete"] ;
		}
		if( yes(value(map,"pop-by-name")) )
		{
			out[short_?"J":"pop-by-name"] ;
		}
		if( yes(value(map,"pop-by-name-auto-copy")) )
		{
			out[short_?"z":"filter"] = G::Path(value(map,"dir-install"),"emailrelay-filter-copy").str() ;
		}
		out[short_?"F":"pop-auth"] = auth ;
	}
	if( yes(value(map,"logging-verbose")) )
	{
		out[short_?"v":"verbose"] ;
	}
	if( yes(value(map,"logging-debug")) )
	{
		out[short_?"d":"debug"] ;
	}
	if( yes(value(map,"logging-syslog")) )
	{
		out[short_?"k":"syslog"] ;
	}
	if( yes(value(map,"listening-remote")) )
	{
		out[short_?"r":"remote-clients"] ;
	}
	if( no(value(map,"listening-all")) && !value(map,"listening-interface").empty() )
	{
		out[short_?"I":"interface"] = value(map,"listening-interface") ;
	}
	return std::make_pair(path,out) ;
}

std::string str( const G::Strings & list )
{
	return G::Str::join( list , " " ) ;
}

