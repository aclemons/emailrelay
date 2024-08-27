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
/// \file gnewprocess_win32.cpp
///

#include "gdef.h"
#include "gnowide.h"
#include "gnewprocess.h"
#include "gexception.h"
#include "gstr.h"
#include "gpath.h"
#include "gtest.h"
#include "gbuffer.h"
#include "gconvert.h"
#include "glog.h"
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <process.h>
#include <io.h>
#include <algorithm>
#include <utility>
#include <array>

namespace G
{
	namespace NewProcessWindowsImp
	{
		struct Pipe
		{
			Pipe() ;
			~Pipe() ;
			HANDLE hread() const ;
			HANDLE hwrite() const ;
			static std::size_t read( HANDLE read , char * buffer , std::size_t buffer_size ) noexcept ;
			void close() ;
			private:
			static void create( HANDLE & read , HANDLE & write ) ;
			static void uninherited( HANDLE h ) ;
			HANDLE m_read ;
			HANDLE m_write ;
		} ;
		#if GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX
			struct AttributeList
			{
				G_EXCEPTION( Error , tx("AttributeList error") )
				using pointer_type = LPPROC_THREAD_ATTRIBUTE_LIST ;
				explicit AttributeList( const std::array<HANDLE,4U> & ) ;
				~AttributeList() ;
				pointer_type ptr() ;
				private:
				void cleanup() noexcept ;
				G::Buffer<char> m_buffer ;
				std::array<HANDLE,4U> m_handles ;
				pointer_type m_ptr {NULL} ;
			} ;
		#else
			struct AttributeList
			{
				using pointer_type = void* ;
				explicit AttributeList( const std::array<HANDLE,4U> & ) {}
				pointer_type ptr() { return nullptr ; }
			} ;
		#endif
		struct StartupInfo
		{
			nowide::STARTUPINFO_REAL_type m_startup_info ;
			nowide::STARTUPINFO_BASE_type * m_ptr ;
			DWORD m_flags ;
			StartupInfo( AttributeList & attribute_list , HANDLE hstdin , HANDLE hstdout , HANDLE hstderr )
			{
				#if GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX
					nowide::STARTUPINFO_REAL_type zero {} ;
					m_startup_info = zero ;
					m_startup_info.StartupInfo.cb = sizeof(m_startup_info) ;
					m_startup_info.StartupInfo.dwFlags = STARTF_USESTDHANDLES ;
					m_startup_info.StartupInfo.hStdInput = hstdin ;
					m_startup_info.StartupInfo.hStdOutput = hstdout ;
					m_startup_info.StartupInfo.hStdError = hstderr ;
					m_startup_info.lpAttributeList = attribute_list.ptr() ;
					m_ptr = reinterpret_cast<nowide::STARTUPINFO_BASE_type*>(&m_startup_info) ;
					m_flags = CREATE_NO_WINDOW | nowide::STARTUPINFO_flags ;
				#else
					GDEF_IGNORE_PARAMS( attribute_list ) ;
					nowide::STARTUPINFO_BASE_type zero {} ;
					m_startup_info = zero ;
					m_startup_info.cb = sizeof(m_startup_info) ;
					m_startup_info.dwFlags = STARTF_USESTDHANDLES ;
					m_startup_info.hStdInput = hstdin ;
					m_startup_info.hStdOutput = hstdout ;
					m_startup_info.hStdError = hstderr ;
					m_ptr = &m_startup_info ;
					m_flags = CREATE_NO_WINDOW | nowide::STARTUPINFO_flags ;
				#endif
			}
		} ;
		HANDLE fdhandle( int fd ) ;
		void closeHandle( HANDLE h ) noexcept
		{
			if( h != HNULL )
				CloseHandle( h ) ;
		}
	}
}

class G::NewProcessImp
{
public:
	using Pipe = NewProcessWindowsImp::Pipe ;
	using AttributeList = NewProcessWindowsImp::AttributeList ;
	using StartupInfo = NewProcessWindowsImp::StartupInfo ;
	using Fd = NewProcess::Fd ;

	NewProcessImp( const Path & , const StringArray & , const NewProcess::Config & ) ;
		// Constructor. Spawns the new process.

	~NewProcessImp() ;
		// Destructor. Kills the process if it is still running.

	NewProcessWaitable & waitable() noexcept ;
		// Returns a reference to the Waitable sub-object to allow
		// the caller to wait for the process to finish.

	void kill() noexcept ;
		// Tries to kill the spawned process.

	int id() const noexcept ;
		// Returns the process id.

	static bool valid( HANDLE h ) noexcept ;
		// Returns true if a valid handle.

public:
	NewProcessImp( const NewProcessImp & ) = delete ;
	NewProcessImp( NewProcessImp && ) = delete ;
	NewProcessImp & operator=( const NewProcessImp & ) = delete ;
	NewProcessImp & operator=( NewProcessImp && ) = delete ;

private:
	static std::pair<std::string,std::string> commandLine( std::string exe , StringArray args ) ;
	static std::pair<HANDLE,DWORD> createProcessImp( const std::string & exe ,
		const std::string & command_line , const Environment & ,
		HANDLE hpipe , HANDLE keep_handle_1 , HANDLE keep_handle_2 ,
		Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , bool with_cd , const Path & cd ) ;
	static void dequote( std::string & ) ;
	static std::string withQuotes( const std::string & ) ;
	static bool isSpaced( const std::string & ) ;
	static bool isSimplyQuoted( const std::string & ) ;
	static std::string windowsPath() ;
	static std::string cscript() ;
	static std::string powershell() ;

private:
	NewProcess::Config m_config ;
	HANDLE m_hprocess ;
	DWORD m_pid ;
	bool m_killed ;
	Pipe m_pipe ;
	NewProcessWaitable m_waitable ;
} ;


G::NewProcess::NewProcess( const Path & exe , const StringArray & args , const Config & config ) :
	m_imp(std::make_unique<NewProcessImp>(exe,args,config))
{
}

G::NewProcess::~NewProcess()
= default ;

G::NewProcessWaitable & G::NewProcess::waitable() noexcept
{
	return m_imp->waitable() ;
}

int G::NewProcess::id() const noexcept
{
	return m_imp->id() ;
}

bool G::NewProcessImp::valid( HANDLE h ) noexcept
{
	return h != HNULL && h != INVALID_HANDLE_VALUE ;
}

void G::NewProcess::kill( bool yield ) noexcept
{
	m_imp->kill() ;
	if( yield )
	{
		G::threading::yield() ;
		SleepEx( 0 , FALSE ) ;
	}
}

// ==

G::NewProcessImp::NewProcessImp( const Path & exe , const StringArray & args , const NewProcess::Config & config ) :
	m_config(config) ,
	m_hprocess(0) ,
	m_killed(false) ,
	m_waitable(HNULL,HNULL,0)
{
	G_DEBUG( "G::NewProcessImp::ctor: exe=[" << exe << "] args=[" << Str::join("],[",args) << "]" ) ;

	bool one_pipe = config.stdout == Fd::pipe() || config.stderr == Fd::pipe() ;
	bool stdin_ok = config.stdin.m_null || config.stdin.m_fd >= 0 ;
	if( !one_pipe || !stdin_ok )
		throw NewProcess::Error( "invalid parameters" ) ;

	auto command_line_pair = commandLine( exe.str() , args ) ;

	std::pair<HANDLE,DWORD> pair = createProcessImp( command_line_pair.first , command_line_pair.second ,
		config.env , m_pipe.hwrite() , config.keep_handle_1 , config.keep_handle_2 ,
		config.stdin , config.stdout , config.stderr , !config.cd.empty() , config.cd ) ;

	if( !valid(pair.first) )
	{
		DWORD e = pair.second ;
		std::string s ;
		if( m_config.exec_error_format_fn )
		{
			s = (config.exec_error_format_fn)(config.exec_error_format,e) ;
		}
		else if( !m_config.exec_error_format.empty() )
		{
			s = m_config.exec_error_format ;
			G::Str::replaceAll( s , "__""errno""__" , std::to_string(e) ) ;
			G::Str::replaceAll( s , "__""strerror""__" , Process::errorMessage(e) ) ;
		}
		else
		{
			s = "error " + std::to_string(e) ;
		}
		throw NewProcess::CreateProcessError( s ) ;
	}

	m_hprocess = pair.first ;
	m_pid = pair.second ;

	m_pipe.close() ; // close write end, now used by child process
	m_waitable.assign( m_hprocess , m_pipe.hread() , 0 ) ;
}

void G::NewProcessImp::kill() noexcept
{
	if( !m_killed && valid(m_hprocess) )
		TerminateProcess( m_hprocess , 127 ) ;
	m_killed = true ;
}

G::NewProcessImp::~NewProcessImp()
{
	namespace imp = NewProcessWindowsImp ;
	imp::closeHandle( m_hprocess ) ;
}

G::NewProcessWaitable & G::NewProcessImp::waitable() noexcept
{
	return m_waitable ;
}

int G::NewProcessImp::id() const noexcept
{
	return static_cast<int>(m_pid) ;
}

std::pair<HANDLE,DWORD> G::NewProcessImp::createProcessImp( const std::string & exe ,
	const std::string & command_line , const Environment & env ,
	HANDLE hpipe , HANDLE keep_handle_1 , HANDLE keep_handle_2 ,
	Fd fd_stdin , Fd fd_stdout , Fd fd_stderr , bool with_cd , const Path & cd_path )
{
	namespace imp = NewProcessWindowsImp ;
	G_DEBUG( "G::NewProcessImp::createProcessImp: exe=[" << exe << "] command-line=[" << command_line << "]" ) ;

	HANDLE hstdin = INVALID_HANDLE_VALUE ;
	if( fd_stdin.m_fd >= 0 )
		hstdin = imp::fdhandle( fd_stdin.m_fd ) ;

	HANDLE hstdout = INVALID_HANDLE_VALUE ;
	if( fd_stdout == Fd::pipe() )
		hstdout = hpipe ;
	else if( fd_stdout.m_fd >= 0 )
		hstdout = imp::fdhandle( fd_stdout.m_fd ) ;

	HANDLE hstderr = INVALID_HANDLE_VALUE ;
	if( fd_stderr == Fd::pipe() )
		hstderr = hpipe ;
	else if( fd_stderr.m_fd >= 0 )
		hstderr = imp::fdhandle( fd_stderr.m_fd ) ;

	// redirect stdout or stderr onto the read end of our pipe
	imp::AttributeList attribute_list({ hstdin , hpipe , keep_handle_1 , keep_handle_2 }) ;
	imp::StartupInfo startup_info( attribute_list , hstdin , hstdout , hstderr ) ;

	std::string env_char_block = env.block() ;
	std::wstring env_wchar_block = env.block( &G::Convert::widen ) ;

	PROCESS_INFORMATION info {} ;
	DWORD e = 0 ;

	BOOL rc = nowide::createProcess( exe , command_line ,
		env.empty() ? nullptr : env_char_block.data() ,
		env.empty() ? nullptr : env_wchar_block.data() ,
		with_cd ? &cd_path : nullptr ,
		startup_info.m_flags , startup_info.m_ptr ,
		&info ) ;

	if( rc == 0 || !valid(info.hProcess) )
	{
		e = GetLastError() ;
		G_DEBUG( "G::NewProcessImp::createProcessImp: error=" << e ) ;
		imp::closeHandle( info.hThread ) ;
		return { info.hProcess , e } ;
	}
	else
	{
		imp::closeHandle( info.hThread ) ;
		G_DEBUG( "G::NewProcessImp::createProcessImp: process-id=" << info.dwProcessId ) ;
		G_DEBUG( "G::NewProcessImp::createProcessImp: thread-id=" << info.dwThreadId ) ;
		return { info.hProcess , info.dwProcessId } ;
	}
}

std::pair<std::string,std::string> G::NewProcessImp::commandLine( std::string exe , StringArray args )
{
	// there is no correct way to do this because every target program
	// will parse its command-line differently -- quotes, spaces and
	// empty arguments are best avoided

	// in this implementation: all quotes are deleted(!) unless
	// an exe; executable paths with a space are quoted; empty
	// arguments and arguments with a space are quoted (unless
	// a batch file that has been quoted)

	if( isSimplyQuoted(exe) )
		dequote( exe ) ;

	for( auto & arg : args )
		dequote( arg ) ;

	std::string type = Str::lower( G::Path(exe).extension() ) ;
	if( type == "exe" || type == "bat" )
	{
		// we can run CreateProcess() directly -- but note
		// that CreateProcess() with a batch file runs
		// "cmd.exe /c" internally
	}
	else if( type == "ps1" )
	{
		args.insert( args.begin() , exe ) ;
		args.insert( args.begin() , "-File" ) ;
		args.insert( args.begin() , "-NoLogo" ) ;
		exe = powershell() ;
	}
	else
	{
		args.insert( args.begin() , exe ) ;
		args.insert( args.begin() , "//B" ) ;
		args.insert( args.begin() , "//nologo" ) ;
		exe = cscript() ;
	}

	std::string command_line = isSpaced(exe) ? withQuotes(exe) : exe ;
	for( auto & arg : args )
	{
		if( ( arg.empty() || isSpaced(arg) ) && isSpaced(exe) && type == "bat" )
		{
			G_WARNING_ONCE( "G::NewProcessImp::commandLine: batch file path contains a space so arguments cannot be quoted" ) ;
			command_line.append(1U,' ').append(arg) ; // this fails >-: cmd /c "a b.bat" "c d"
		}
		else if( arg.empty() || isSpaced(arg) )
		{
			command_line.append(1U,' ').append(withQuotes(arg)) ;
		}
		else
		{
			command_line.append(1U,' ').append(arg) ;
		}
	}
	return { exe , command_line } ;
}

void G::NewProcessImp::dequote( std::string & s )
{
	if( isSimplyQuoted(s) )
	{
		s = s.substr( 1U , s.length()-2U ) ;
	}
	else if( s.find( '\"' ) != std::string::npos )
	{
		G::Str::removeAll( s , '\"' ) ;
		G_WARNING_ONCE( "G::NewProcessImp::dequote: quotes removed when building command-line" ) ;
	}
}

bool G::NewProcessImp::isSimplyQuoted( const std::string & s )
{
	static constexpr char q = '\"' ;
	return
		s.length() > 1U && s.at(0U) == q && s.at(s.length()-1U) == q &&
		s.find(q,1U) == (s.length()-1U) ;
}

bool G::NewProcessImp::isSpaced( const std::string & s )
{
	return s.find(' ') != std::string::npos ;
}

std::string G::NewProcessImp::withQuotes( const std::string & s )
{
	return std::string(1U,'\"').append(s).append(1U,'\"') ;
}

std::string G::NewProcessImp::windowsPath()
{
	std::string result = nowide::windowsPath() ;
	if( result.empty() )
		throw NewProcess::SystemError( "GetWindowsDirectory failed" ) ;
	return result ;
}

std::string G::NewProcessImp::cscript()
{
	return windowsPath().append("\\system32\\cscript.exe") ;
}

std::string G::NewProcessImp::powershell()
{
	return windowsPath().append("\\System32\\WindowsPowerShell\\v1.0\\powershell.exe") ;
}

// ==

#if GCONFIG_HAVE_WINDOWS_STARTUP_INFO_EX
G::NewProcessWindowsImp::AttributeList::AttributeList( const std::array<HANDLE,4U> & handles_in ) :
	m_handles(handles_in)
{
	auto end = std::partition( m_handles.begin() , m_handles.end() ,
		[](HANDLE h){return h!=0 && h!=INVALID_HANDLE_VALUE;} ) ;
	std::size_t handles_size = std::distance( m_handles.begin() , end ) ;
	if( handles_size )
	{
		SIZE_T buffer_size = 0 ;
		InitializeProcThreadAttributeList( NULL , 1 , 0 , &buffer_size ) ;
		if( buffer_size == 0 || buffer_size > 100000 )
			throw Error() ;
		m_buffer.resize( buffer_size ) ;

		auto ptr = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(m_buffer.data()) ;
		BOOL ok = InitializeProcThreadAttributeList( ptr , 1 , 0 , &buffer_size ) ;
		if( !ok || buffer_size != m_buffer.size() )
			throw Error() ;

		ok = UpdateProcThreadAttribute( ptr , 0 , PROC_THREAD_ATTRIBUTE_HANDLE_LIST ,
			m_handles.data() , handles_size * sizeof(HANDLE) , NULL , NULL ) ;
		if( !ok )
		{
			cleanup() ;
			throw Error() ;
		}
		m_ptr = ptr ;
	}
}

G::NewProcessWindowsImp::AttributeList::~AttributeList()
{
	cleanup() ;
}

void G::NewProcessWindowsImp::AttributeList::cleanup() noexcept
{
	if( m_ptr )
		DeleteProcThreadAttributeList( m_ptr ) ;
	m_ptr = NULL ;
}

G::NewProcessWindowsImp::AttributeList::pointer_type G::NewProcessWindowsImp::AttributeList::ptr()
{
	return m_ptr ;
}
#endif

// ==

G::NewProcessWindowsImp::Pipe::Pipe() :
	m_read(HNULL) ,
	m_write(HNULL)
{
	create( m_read , m_write ) ;
	uninherited( m_read ) ;
}

G::NewProcessWindowsImp::Pipe::~Pipe()
{
	closeHandle( m_read ) ;
	closeHandle( m_write ) ;
}

void G::NewProcessWindowsImp::Pipe::create( HANDLE & h_read , HANDLE & h_write )
{
	SECURITY_ATTRIBUTES attributes {} ;
	attributes.nLength = sizeof(attributes) ;
	attributes.lpSecurityDescriptor = nullptr ;
	attributes.bInheritHandle = TRUE ;

	h_read = HNULL ;
	h_write = HNULL ;
	DWORD buffer_size_hint = 0 ;
	BOOL rc = CreatePipe( &h_read , &h_write , &attributes , buffer_size_hint ) ;
	if( rc == 0 )
	{
		DWORD error = GetLastError() ;
		G_ERROR( "G::NewProcessWindowsImp::Pipe::create: pipe error: create: " << error ) ;
		throw NewProcess::PipeError( "create" ) ;
	}
}

void G::NewProcessWindowsImp::Pipe::uninherited( HANDLE h )
{
	if( ! SetHandleInformation( h , HANDLE_FLAG_INHERIT , 0 ) )
	{
		DWORD error = GetLastError() ;
		closeHandle( h ) ;
		G_ERROR( "G::NewProcessWindowsImp::Pipe::uninherited: uninherited error " << error ) ;
		throw NewProcess::PipeError( "uninherited" ) ;
	}
}

HANDLE G::NewProcessWindowsImp::Pipe::hwrite() const
{
	return m_write ;
}

HANDLE G::NewProcessWindowsImp::Pipe::hread() const
{
	return m_read ;
}

void G::NewProcessWindowsImp::Pipe::close()
{
	closeHandle( m_write ) ;
	m_write = HNULL ;
}

std::size_t G::NewProcessWindowsImp::Pipe::read( HANDLE hread , char * buffer , std::size_t buffer_size_in ) noexcept
{
	// (worker thread - keep it simple)
	if( hread == HNULL ) return 0U ;
	DWORD buffer_size = static_cast<DWORD>(buffer_size_in) ;
	DWORD nread = 0U ;
	BOOL ok = ReadFile( hread , buffer , buffer_size , &nread , nullptr ) ;
	//DWORD error = GetLastError() ;
	nread = ok ? std::min( nread , buffer_size ) : DWORD(0) ;
	return static_cast<std::size_t>(nread) ;
}

// ==

G::NewProcessWaitable::NewProcessWaitable() :
	m_hprocess(HNULL),
	m_hpipe(HNULL) ,
	m_pid(0),
	m_rc(0),
	m_status(0),
	m_error(0) ,
	m_test_mode(G::Test::enabled("waitpid-slow"))
{
}

G::NewProcessWaitable::NewProcessWaitable( HANDLE hprocess , HANDLE hpipe , int ) :
	m_buffer(1024U*4U) ,
	m_hprocess(hprocess),
	m_hpipe(hpipe),
	m_pid(0),
	m_rc(0),
	m_status(0),
	m_error(0) ,
	m_test_mode(G::Test::enabled("waitpid-slow"))
{
}

void G::NewProcessWaitable::assign( HANDLE hprocess , HANDLE hpipe , int )
{
	m_buffer.resize( 1024U * 4U ) ;
	m_data_size = 0U ;
	m_hprocess = hprocess ;
	m_hpipe = hpipe ;
	m_pid = 0 ;
	m_rc = 0 ;
	m_status = 0 ;
	m_error = 0 ;
}

void G::NewProcessWaitable::waitp( std::promise<std::pair<int,std::string>> p ) noexcept
{
	try
	{
		wait() ;
		p.set_value( std::make_pair(get(),output()) ) ;
	}
	catch(...)
	{
		try { p.set_exception( std::current_exception() ) ; } catch(...) {}
	}
}

G::NewProcessWaitable & G::NewProcessWaitable::wait()
{
	// (worker thread - keep it simple)
	m_data_size = 0U ;
	m_error = 0 ;
	std::array<char,64U> discard_buffer {} ;
	char * discard = discard_buffer.data() ;
	std::size_t discard_size = discard_buffer.size() ;
	char * read_p = m_buffer.data() ;
	std::size_t space = m_buffer.size() ;
	for(;;)
	{
		HANDLE handles[2] ;
		DWORD nhandles = 0 ;
		if( NewProcessImp::valid(m_hprocess) )
			handles[nhandles++] = m_hprocess ;
		if( m_hpipe != HNULL )
			handles[nhandles++] = m_hpipe ;
		if( nhandles == 0 )
			break ;

		// wait on both handles to avoid the pipe-writer from blocking if the pipe fills
		DWORD rc = WaitForMultipleObjects( nhandles , handles , FALSE , INFINITE ) ;
		HANDLE h = rc == WAIT_OBJECT_0 ? handles[0] : (rc==(WAIT_OBJECT_0+1)?handles[1]:HNULL) ;
		if( h == m_hprocess && m_hprocess )
		{
			DWORD exit_code = 127 ;
			GetExitCodeProcess( m_hprocess , &exit_code ) ;
			m_status = static_cast<int>(exit_code) ;
			m_hprocess = HNULL ;
		}
		else if( h == m_hpipe && m_hpipe )
		{
			using Pipe = NewProcessWindowsImp::Pipe ;
			std::size_t nread = Pipe::read( m_hpipe , space?read_p:discard , space?space:discard_size ) ;
			if( space && nread <= space )
			{
				read_p += nread ;
				space -= nread ;
				m_data_size += nread ;
			}
			if( nread == 0U )
				m_hpipe = HNULL ;
		}
		else
		{
			m_error = 1 ;
			break ;
		}
	}
	if( m_test_mode )
		Sleep( 10000U ) ;
	return *this ;
}

int G::NewProcessWaitable::get() const
{
	if( m_error )
		throw NewProcess::WaitError() ;
	return m_status ;
}

int G::NewProcessWaitable::get( std::nothrow_t , int ec ) const noexcept
{
	return m_error ? ec : m_status ;
}

std::string G::NewProcessWaitable::output() const
{
	return m_buffer.empty() ? std::string() : std::string(m_buffer.data(),m_data_size) ;
}

// ==

HANDLE G::NewProcessWindowsImp::fdhandle( int fd )
{
	// beware "parameter validation" -- maybe use _set_thread_local_invalid_parameter_handler()
	if( fd < 0 )
		return INVALID_HANDLE_VALUE ;
	intptr_t h = _get_osfhandle( fd ) ;
	if( h == -1 || h == -2 )
		return INVALID_HANDLE_VALUE ;
	return reinterpret_cast<HANDLE>(h) ;
}

