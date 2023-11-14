#!/usr/bin/env perl
#
# Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
# ===
#
# winbuild.pl
#
# Parses automake files throughout the source tree, generates cmake files
# for windows, runs cmake to generate visual studio project files, and
# then runs msbuild.
#
# usage: winbuild.pl [<subtask> [<subtask> ...]]
#
# Also spits out batch files (like "winbuild-whatever.bat") for doing
# sub-tasks, including "winbuild-install.bat".
#
# Requires "cmake" and "msbuild" to be on the path or somewhere obvious,
# and expects mbedtls source to be in a sibling directory, and expects
# Qt (libraries, headers and tools) to be in its default install location.
# These components are found at run-time by the find_*() routines in
# "libexec/winbuild.pm". A configuration file "winbuild.cfg" can be used
# to give explicit locations if necessary. The "find" sub-task, ie.
# "winbuild-find.bat", can be used to run the find routines and create
# a "winbuild.cfg" configuration file.
#
# The "install" sub-task, which is not run by default, assembles binaries
# and their dependencies in a directory tree ready for zipping and
# distribution. The dependencies for Qt are assembled by the Qt dependency
# tool, "windeployqt".
#
# On linux the "install_winxp" sub-task can be used after a mingw cross
# build to assemble an installation zip file.
#

use strict ;
use Cwd ;
use FileHandle ;
use File::Find ;
use File::Basename ;
use File::Copy ;
use lib dirname($0) , dirname($0)."/libexec" ;
require "winbuild.pm" ;

# configuration ...

my $cfg_x64 = 1 ;
my $cfg_x86 = 0 ;
die unless ($cfg_x64 || $cfg_x86) ;
my $cfg_debug = 0 ;
my $cfg_add_runtime = 1 ;

# cmake command-line options
my $cmake_args = {
	x64 => [
		# try these in turn...
		[ "-G" , "Visual Studio 17 2022" , "-A" , "x64" ] ,
		[ "-G" , "Visual Studio 16 2019" , "-A" , "x64" ] ,
		[ "-A" , "x64" ] ,
	] ,
	x86 => [
		# try these in turn...
		[ "-G" , "Visual Studio 17 2022" , "-A" , "Win32" ] ,
		[ "-G" , "Visual Studio 16 2019" , "-A" , "Win32" ] ,
		[ "-A" , "Win32" ] ,
		[]
	] ,
} ;

# project version
chomp( my $version = eval { FileHandle->new("VERSION")->gets() } || "2.5.1" ) ;
my $project = "emailrelay" ;
my $install_x64 = "$project-$version-w64" ;
my $install_x86 = "$project-$version-w32" ;
my $install_winxp = "$project-$version-winxp" ;

# makefile conditionals
my %switches = (
	GCONFIG_BSD => 0 ,
	GCONFIG_DNSBL => 1 ,
	GCONFIG_EPOLL => 0 ,
	GCONFIG_GETTEXT => 0 ,
	GCONFIG_GUI => 1 , # << zero if no qt libraries
	GCONFIG_ICONV => 0 ,
	GCONFIG_INSTALL_HOOK => 0 ,
	GCONFIG_INTERFACE_NAMES => 1 ,
	GCONFIG_MAC => 0 ,
	GCONFIG_PAM => 0 ,
	GCONFIG_POP => 1 ,
	GCONFIG_ADMIN => 1 ,
	GCONFIG_TESTING => 1 ,
	GCONFIG_TLS_USE_MBEDTLS => 1 , # << zero if no mbedtls source
	GCONFIG_TLS_USE_OPENSSL => 0 ,
	GCONFIG_TLS_USE_BOTH => 0 ,
	GCONFIG_TLS_USE_NONE => 0 ,
	GCONFIG_UDS => 0 ,
	GCONFIG_WINDOWS => 1 ,
) ;

# makefile expansion variables -- many are required but not relevant
my %vars = (
	top_srcdir => "." ,
	top_builddir => "." ,
	sbindir => "." ,
	mandir => "." ,
	localedir => "." ,
	e_spooldir => "c:/emailrelay" , # passed as -D but not used -- see src/gstore/gfilestore_win32.cpp
	e_docdir => "c:/emailrelay" ,
	e_initdir => "c:/emailrelay" ,
	e_bsdinitdir => "c:/emailrelay" ,
	e_rundir => "c:/emailrelay" ,
	e_icondir => "c:/emailrelay" ,
	e_trdir => "c:/emailrelay" ,
	e_examplesdir => "c:/emailrelay" ,
	e_libdir => "c:/emailrelay" ,
	e_pamdir => "c:/emailrelay" ,
	e_sysconfdir => "c:/emailrelay" ,
	GCONFIG_WINDRES => "windres" ,
	GCONFIG_WINDMC => "mc" ,
	GCONFIG_QT_LIBS => "" ,
	GCONFIG_QT_CFLAGS => "" ,
	GCONFIG_QT_MOC => "" ,
	GCONFIG_TLS_LIBS => "" ,
	GCONFIG_STATIC_START => "" ,
	GCONFIG_STATIC_END => "" ,
	VERSION => $version ,
	RPM_ARCH => "x86" ,
	RPM_ROOT => "rpm" ,
) ;

$switches{GCONFIG_TLS_USE_NONE} = 1 if (
	$switches{GCONFIG_TLS_USE_MBEDTLS} == 0 &&
	$switches{GCONFIG_TLS_USE_OPENSSL} == 0 ) ;

$switches{GCONFIG_TLS_USE_BOTH} = 1 if (
	$switches{GCONFIG_TLS_USE_MBEDTLS} == 1 &&
	$switches{GCONFIG_TLS_USE_OPENSSL} == 1 ) ;

# pre-find ...

winbuild::spit_out_batch_files( "find" ) ;

# find stuff ...

my $want_mbedtls = ( $switches{GCONFIG_TLS_USE_MBEDTLS} || $switches{GCONFIG_TLS_USE_BOTH} ) && $^O ne "linux" ;
my $want_qt = $switches{GCONFIG_GUI} && $^O ne "linux" ;

my $msbuild = winbuild::find_msbuild() ;
my $cmake = winbuild::find_cmake( $msbuild ) ;
my $qt_info = $want_qt ? winbuild::find_qt() : undef ;
my $mbedtls = $want_mbedtls ? winbuild::find_mbedtls() : undef ;

my $missing_cmake = !$cmake ;
my $missing_msbuild = !$msbuild ;
my $missing_qt = ( $want_qt && $cfg_x86 && !$qt_info->{x86} ) || ( $want_qt && $cfg_x64 && !$qt_info->{x64} ) ;
my $missing_mbedtls = ( $want_mbedtls && !$mbedtls ) ;

warn "error: cannot find cmake.exe: please download from cmake.org\n" if $missing_cmake ;
warn "error: cannot find msbuild.exe: please install visual studio\n" if $missing_msbuild ;
warn "error: cannot find qt libraries: please download from wwww.qt.io or unset GCONFIG_GUI\n" if $missing_qt ;
warn "error: cannot find mbedtls source: please download from tls.mbed.org " .
	"or unset GCONFIG_TLS_USE_MBEDTLS\n" if $missing_mbedtls ;

if( $missing_cmake || $missing_msbuild || $missing_qt || $missing_mbedtls )
{
	winbuild::fcache_create() ; # if none
	warn "error: missing prerequisites: please install the missing components " ,
		"or " . (-f "winbuild.cfg"?"edit the":"use a") . " winbuild.cfg configuration file" ,
		( ($missing_qt||$missing_mbedtls) ? " or unset configuration items in winbuild.pl\n" : "\n" ) ;
	die "error: missing prerequisites\n" ;
}

# choose what to run ...

my @default_parts =
	$^O eq "linux" ? qw( install_winxp ) : ( $want_mbedtls ?
		qw( batchfiles generate mbedtls cmake build ) :
		qw( batchfiles generate cmake build ) ) ;

# run stuff ...

my @run_parts = scalar(@ARGV) ? @ARGV : @default_parts ;
for my $part ( @run_parts )
{
	if( $part eq "find" )
	{
		winbuild::fcache_write() ;
	}
	elsif( $part eq "batchfiles" )
	{
		winbuild::spit_out_batch_files( qw(
			find generate cmake build
			debug-build debug-test test
			mbedtls clean vclean install ) ) ;
	}
	elsif( $part eq "generate" )
	{
		run_generate( $project , \%switches , \%vars , $qt_info ) ;
	}
	elsif( $part eq "mbedtls" )
	{
		run_mbedtls_cmake( $mbedtls , $cmake , "x64" ) if $cfg_x64 ;
		run_mbedtls_cmake( $mbedtls , $cmake , "x86" ) if $cfg_x86 ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x64" , "Debug" ) if ( $cfg_x64 && $cfg_debug ) ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x64" , "Release" ) if $cfg_x64 ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x86" , "Debug" ) if ( $cfg_x86 && $cfg_debug ) ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x86" , "Release" ) if $cfg_x86 ;
	}
	elsif( $part eq "cmake" )
	{
		run_cmake( $cmake , $mbedtls , $qt_info , "x64" ) if $cfg_x64 ;
		run_cmake( $cmake , $mbedtls , $qt_info , "x86" ) if $cfg_x86 ;
	}
	elsif( $part eq "build" )
	{
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Release" ) if $cfg_x64 ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" ) if ( $cfg_x64 && $cfg_debug ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Release" , "Win32" ) if $cfg_x86 ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Debug" , "Win32" ) if ( $cfg_x86 && $cfg_debug ) ;
	}
	elsif( $part eq "debug-build" )
	{
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" ) ;
	}
	elsif( $part eq "clean" )
	{
		clean_test_files() ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" , undef , "Clean" ) if ( $cfg_x64 && $cfg_debug ) ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Release" , undef , "Clean" ) if $cfg_x64 ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Debug" , "Win32" , "Clean" ) if ( $cfg_x86 && $cfg_debug ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Release" , "Win32" , "Clean" ) if $cfg_x86 ;
	}
	elsif( $part eq "vclean" )
	{
		clean_test_files() ;
		winbuild::clean_cmake_files() ;
		winbuild::clean_cmake_cache_files() ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" , undef , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Release" , undef , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Debug" , "Win32" , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Release" , "Win32" , "Clean" ) ;
		winbuild::deltree( $install_x64 ) if $cfg_x64 ;
		winbuild::deltree( $install_x86 ) if $cfg_x86 ;
		winbuild::deltree( "$mbedtls/x64" ) if $cfg_x64 ;
		winbuild::deltree( "$mbedtls/x86" ) if $cfg_x86 ;
		winbuild::fcache_cleanup() ;
	}
	elsif( $part eq "install" )
	{
		my $with_gui = $switches{GCONFIG_GUI} ;
		my $with_mbedtls = $switches{GCONFIG_TLS_USE_MBEDTLS} || $switches{GCONFIG_TLS_USE_BOTH} ;
		install( $install_x64 , "x64" , $qt_info , $with_gui , $with_mbedtls , $cfg_add_runtime ) if $cfg_x64 ;
		install( $install_x86 , "x86" , $qt_info , $with_gui , $with_mbedtls , $cfg_add_runtime ) if $cfg_x86 ;
	}
	elsif( $part eq "install_winxp" )
	{
		run_install_winxp() ;
	}
	elsif( $part eq "debug-test" )
	{
		run_tests( "x64/src/main/Debug" , "x64/test/Debug" ) ;
	}
	elsif( $part eq "test" )
	{
		run_tests( "x64/src/main/Release" , "x64/test/Release" ) ;
	}
	else
	{
		die "usage error\n" ;
	}
}

# signal success to the batch file if we have not died
winbuild::create_touchfile( winbuild::default_touchfile($0) ) ;

# show a helpful message
if( (grep {$_ eq "build"} @run_parts) && !(grep {$_ eq "install"} @run_parts) )
{
	print "build finished -- try winbuild-install.bat for packaging\n"
}

# ==

sub create_cmake_file
{
	my ( $project , $m , $switches , $qt_info ) = @_ ;

	my $path = join( "/" , dirname($m->path()) , "CMakeLists.txt" ) ;

	print "cmake-file=[$path]\n" ;
	my $fh = new FileHandle( $path , "w" ) or die ;

	print $fh "# $path -- generated by $0\n" ;
	if( $project )
	{
		print $fh "cmake_minimum_required(VERSION 3.1.0)\n" ;
		print $fh "project($project)\n" ;
		if( $switches{GCONFIG_GUI} && $qt_info->{v} == 5 )
		{
			# see https://doc.qt.io/qt-5/cmake-get-started.html
			print $fh "find_package(Qt5 COMPONENTS Widgets REQUIRED)\n" ;
		}
		elsif( $switches{GCONFIG_GUI} )
		{
			# see https://doc.qt.io/qt-6/cmake-get-started.html
			print $fh "find_package(Qt6 REQUIRED COMPONENTS Widgets)\n" ;
		}
		if( $switches{GCONFIG_TLS_USE_MBEDTLS} || $switches{GCONFIG_TLS_USE_BOTH} )
		{
			print $fh "find_package(MbedTLS REQUIRED)\n" ;
		}
		print $fh "find_program(CMAKE_MC_COMPILER mc)\n" ;
	}

	if( $m->path() =~ m/gui/ )
	{
		print $fh "set(CMAKE_AUTOMOC ON)\n" ;
		print $fh "set(CMAKE_INCLUDE_CURRENT_DIR ON)\n" ;
	}

	# force static or dynamic linking of the c++ runtime by switching
	# between /MD and /MT -- use static linking by default but keep
	# the gui dynamically linked so that it can use the Qt binary
	# distribution -- for public distribution of a statically linked
	# gui program use "emailrelay-gui.pro" -- note that the gui build
	# is self-contained by virtue of "glibsources.cpp"
	#
	my $dynamic_runtime = ( $m->path() =~ m/gui/ ) ;
	{
		print $fh '# choose dynamic or static linking of the c++ runtime' , "\n" ;
		print $fh 'set(CompilerFlags' , "\n" ;
		print $fh '    CMAKE_CXX_FLAGS' , "\n" ;
		print $fh '    CMAKE_CXX_FLAGS_DEBUG' , "\n" ;
		print $fh '    CMAKE_CXX_FLAGS_RELEASE' , "\n" ;
		print $fh '    CMAKE_C_FLAGS' , "\n" ;
		print $fh '    CMAKE_C_FLAGS_DEBUG' , "\n" ;
		print $fh '    CMAKE_C_FLAGS_RELEASE' , "\n" ;
		print $fh ')' , "\n" ;
		print $fh 'foreach(CompilerFlag ${CompilerFlags})' , "\n" ;
		if( $dynamic_runtime )
		{
			print $fh '    string(REPLACE "/MT" "/MD" ${CompilerFlag} "${${CompilerFlag}}")' , "\n" ;
		}
		else
		{
			print $fh '    string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")' , "\n" ;
		}
		print $fh 'endforeach()' , "\n" ;
	}

	print $fh "\n" ;
	for my $subdir ( $m->subdirs() )
	{
		print $fh "add_subdirectory($subdir)\n" ;
	}

	my $definitions = join( " " , "G_WINDOWS=1" , grep {!m/G_LIB_SMALL/} $m->definitions() ) ;
	my $includes = join( " " , "." , ".." , $m->includes($m->base()) , '"${MBEDTLS_INCLUDE_DIRS}"' ) ;

	my @libraries = $m->libraries() ;
	for my $library ( @libraries )
	{
		my $sources = join( " " , $m->sources( $library ) ) ;
		if( $sources )
		{
			( my $library_key = $library ) =~ s/\.a$// ; $library_key =~ s/^lib// ;
			if( $library_key !~ m/extra$/ )
			{
				print $fh "\n" ;
				print $fh "add_library($library_key $sources)\n" ;
				print $fh "target_include_directories($library_key PUBLIC $includes)\n" ;
				print $fh "target_compile_definitions($library_key PUBLIC $definitions)\n" ;
			}
		}
	}

	my $tls_libs_fixed ;
	my @programs = $m->programs() ;
	for my $program ( @programs )
	{
		my $sources = join( " " , $m->sources( $program ) ) ;
		my $our_libs = join( " " , $m->our_libs( $program ) ) ;
		my $sys_libs = join( " " , $m->sys_libs( $program ) ) ;

		my $tls_libs = "" ;
		if( ( $our_libs =~ m/gssl/ || $our_libs =~ m/keygen/ ) &&
			( $switches->{GCONFIG_TLS_USE_MBEDTLS} || $switches->{GCONFIG_TLS_USE_BOTH} ) )
		{
			if( ! $tls_libs_fixed )
			{
				print $fh '    string(REPLACE "/Release" "/Debug" MBEDTLS_LIBRARY_DEBUG "${MBEDTLS_LIBRARY}")' , "\n" ;
				print $fh '    string(REPLACE "/Release" "/Debug" MBEDX509_LIBRARY_DEBUG "${MBEDX509_LIBRARY}")' , "\n" ;
				print $fh '    string(REPLACE "/Release" "/Debug" MBEDCRYPTO_LIBRARY_DEBUG "${MBEDCRYPTO_LIBRARY}")' , "\n" ;
				$tls_libs_fixed = 1 ;
			}
			$tls_libs =
				'optimized ${MBEDTLS_LIBRARY} debug ${MBEDTLS_LIBRARY_DEBUG} ' .
				'optimized ${MBEDX509_LIBRARY} debug ${MBEDX509_LIBRARY_DEBUG} ' .
				'optimized ${MBEDCRYPTO_LIBRARY} debug ${MBEDCRYPTO_LIBRARY_DEBUG}' ;
		}

		my $qt_libs = ( $m->path() =~ m/gui/ ) ? $qt_info->{libs} : "" ;
		my $win32 = ( $m->path() =~ m/gui/ || $program eq "emailrelay" ) ? "WIN32 " : "" ;

		my $resources = "" ;
		my $resource_includes = "" ;
		if( $program eq "emailrelay" )
		{
			$resources = "messages.mc emailrelay.rc emailrelay.exe.manifest" ;
			$resource_includes = "icon" ;
		}
		if( $program eq "emailrelay-service" )
		{
			$resources = "emailrelay-service.exe.manifest" ;
		}
		if( $program =~ m/emailrelay.gui/ )
		{
			$resources = "messages.mc emailrelay-gui.rc $program.exe.manifest" ;
			$resource_includes = "../main/icon" ;
		}

		my $program_sources = join(" ",split(' ',"$win32 $resources $sources")) ;
		my $program_includes = join(" ",split(' ',"$resource_includes $includes")) ;
		my $program_libs = join(" ",split(' ',"$our_libs $qt_libs $tls_libs $sys_libs")) ;

		( my $program_key = $program ) =~ s/\.real$// ;
		print $fh "\n" ;
		print $fh "add_executable($program_key $program_sources)\n" ;
		print $fh "target_include_directories($program_key PUBLIC $program_includes)\n" ;
		print $fh "target_compile_definitions($program_key PUBLIC $definitions)\n" ;
		print $fh "target_link_libraries($program_key $program_libs)\n" ;
		if( $resources =~ /messages.mc/ )
		{
			print $fh 'add_custom_command(TARGET '."$program_key".' PRE_BUILD COMMAND "${CMAKE_MC_COMPILER}" "${CMAKE_CURRENT_SOURCE_DIR}/messages.mc" VERBATIM)' , "\n" ;
		}
		if( $resources =~ m/manifest/ )
		{
			# the uac stanza is in manifest file, so stop the linker from adding it too
			print $fh "set_target_properties($program_key PROPERTIES LINK_FLAGS \"/MANIFESTUAC:NO\")\n" ;
		}
	}

	$fh->close() or die ;
}

sub create_cmake_files
{
	my ( $project , $switches , $vars , $qt_info ) = @_ ;

	my @makefiles = winbuild::read_makefiles( $switches , $vars ) ;
	my $first = 1 ;
	for my $m ( @makefiles )
	{
		create_cmake_file( $first?$project:undef , $m , $switches , $qt_info ) ;
		$first = 0 ;
	}
	create_cmake_find_mbedtls_file() ;
}

sub create_cmake_find_mbedtls_file
{
	# from  github/Kitware/CMake/Utilities/cmcurl/CMake/FindMbedTLS.cmake
	my $fh = new FileHandle( "FindMbedTLS.cmake" , "w" ) or die ;
	print $fh 'find_path(MBEDTLS_INCLUDE_DIRS mbedtls/ssl.h)' , "\n" ;
	print $fh 'find_library(MBEDTLS_LIBRARY mbedtls)' , "\n" ;
	print $fh 'find_library(MBEDX509_LIBRARY mbedx509)' , "\n" ;
	print $fh 'find_library(MBEDCRYPTO_LIBRARY mbedcrypto)' , "\n" ;
	print $fh 'set(MBEDTLS_LIBRARIES "${MBEDTLS_LIBRARY}" "${MBEDX509_LIBRARY}" "${MBEDCRYPTO_LIBRARY}")' , "\n" ;
	print $fh 'include(FindPackageHandleStandardArgs)' , "\n" ;
	print $fh 'find_package_handle_standard_args(MbedTLS DEFAULT_MSG MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)' , "\n" ;
	print $fh 'mark_as_advanced(MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)' , "\n" ;
	$fh->close() or die ;
}

sub clean_test_files
{
	my @list = () ;
	File::Find::find( sub { push @list , $File::Find::name if $_ =~ m/^.tmp/ } , "." ) ;
	unlink grep { -f $_ } @list ;
	rmdir grep { -d $_ } @list ;
}

sub run_generate
{
	my ( $project , $switches , $vars , $qt_info ) = @_ ;
	create_cmake_files( $project , $switches , $vars , $qt_info ) ;
	create_gconfig_header() ;
}

sub run_cmake
{
	my ( $cmake , $mbedtls , $qt_info , $arch ) = @_ ;
	$mbedtls ||= "." ;
	$arch ||= "x64" ;

	# (only full paths work here)
	my $mbedtls_dir = Cwd::realpath( $mbedtls ) ;
	my $mbedtls_include_dir = "$mbedtls_dir/include" ;
	my $mbedtls_lib_dir = "$mbedtls_dir/$arch/library/Release" ; # fixed up to Debug elsewhere
	my $qt_dir = defined($qt_info) ? Cwd::realpath( $qt_info->{$arch} ) : "." ;
	my $module_path = Cwd::realpath( "." ) ; # see create_cmake_find_mbedtls_file()

	my @arch_args = @{$cmake_args->{$arch}} ;
	my $rc ;
	for my $arch_args ( @arch_args )
	{
		winbuild::clean_cmake_cache_files( $arch , {verbose=>0} ) ;

		my @args = @$arch_args ;
		unshift @args , "-DCMAKE_MODULE_PATH:FILEPATH=$module_path" ;
		unshift @args , "-DCMAKE_INCLUDE_PATH:FILEPATH=$mbedtls_include_dir" ;
		unshift @args , "-DCMAKE_LIBRARY_PATH:FILEPATH=$mbedtls_lib_dir" ;
		if( $qt_info->{v} == 5 )
		{
			unshift @args , "-DQt5_DIR:FILEPATH=$qt_dir" ;
		}
		else
		{
			unshift @args , "-DQt6_DIR:FILEPATH=$qt_dir" ;
			unshift @args , "-DQt6CoreTools_DIR:FILEPATH=".Cwd::realpath("$qt_dir/../qt6coretools") ;
			unshift @args , "-DQt6GuiTools_DIR:FILEPATH=".Cwd::realpath("$qt_dir/../qt6guitools") ;
		}
		unshift @args , "-DCMAKE_MC_COMPILER:FILEPATH=mc" ;

		my $build_dir = $arch ;
		mkdir $build_dir ;
		my $base_dir = getcwd() or die ;
		chdir $build_dir or die ;
		print "cmake: cwd=[".getcwd()."] exe=[$cmake] args=[".join("][",@args)."][..]\n" ;
		$rc = system( $cmake , ( @args , ".." ) ) ;
		chdir $base_dir or die ;
		last if $rc == 0 ;
	}
	print "cmake-exit=[$rc]\n" ;
	die "cmake failed: check error messages above and maybe tweak cmake_args in winbuild.pl\n" unless $rc == 0 ;
}

sub create_gconfig_header
{
	winbuild::touch( "src/gconfig_defs.h" ) ;
}

sub run_mbedtls_cmake
{
	my ( $mbedtls_dir , $cmake , $arch ) = @_ ;

	my $mbedtls_project = "mbed TLS" ;
	my $base_dir = getcwd() ;
	mkdir "$mbedtls_dir/$arch" ;
	chdir "$mbedtls_dir/$arch" or die ;

	# run cmake to generate .sln file
	my $output_file = "$mbedtls_project.sln" ;
	if( -f $output_file )
	{
		print "mbedtls-cmake($arch): already got [$arch/$output_file]: not running cmake\n" ;
	}
	else
	{
		my @arch_args = @{$cmake_args->{$arch}} ;
		my $rc ;
		for my $arch_args ( @arch_args )
		{
			my @args = (
				@$arch_args ,
				"-DENABLE_TESTING=Off" ,
				"-DCMAKE_C_FLAGS_DEBUG=/MTd" ,
				"-DCMAKE_C_FLAGS_RELEASE=/MT" ,
			) ;
			print "mbedtls-cmake($arch): cwd=[".getcwd()."] exe=[$cmake] args=[".join("][",@args)."][..]\n" ;
			$rc = system( $cmake , @args , ".." ) ;
			print "mbedtls-cmake($arch): exit=[$rc]\n" ;
			last if( $rc == 0 ) ;
		}
		die unless $rc == 0 ;
	}
	chdir $base_dir or die ;
}

sub run_mbedtls_msbuild
{
	my ( $mbedtls_dir , $msbuild , $arch , $confname ) = @_ ;

	my $mbedtls_project = "mbed TLS" ;
	my $base_dir = getcwd() ;
	mkdir "$mbedtls_dir/$arch" ;
	chdir "$mbedtls_dir/$arch" or die ;

	# run msbuild to generate mbedtls.lib
	my $output_file = "library/$confname/mbedtls.lib" ;
	if( -f $output_file )
	{
		print "mbedtls-msbuild($arch,$confname): already got [$arch/$output_file]: not running msbuild\n" ;
	}
	else
	{
		my @msbuild_args = ( "/fileLogger" , "/p:Configuration=$confname" , "\"$mbedtls_project.sln\"" ) ;
		print "mbedtls-msbuild($arch,$confname): running msbuild\n" ;
		my $rc = system( $msbuild , @msbuild_args ) ;
		print "mbedtls-msbuild($arch,$confname): exit=[$rc]\n" ;
		die unless $rc == 0 ;
	}
	chdir $base_dir or die ;
}

sub clean_mbedtls_files
{
	my ( $mbedtls_dir , $arch ) = @_ ;
	if( defined($mbedtls) && -d $mbedtls_dir )
	{
		winbuild::deltree( "$mbedtls_dir/$arch" ) ;
	}
}

sub install
{
	my ( $install , $arch , $qt_info , $with_gui , $with_mbedtls , $add_runtime ) = @_ ;

	my $msvc_base = winbuild::find_msvc_base( $arch ) ;
	$msvc_base or die "error: install: cannot determine the msvc base directory\n" ;
	print "msvc-base=[$msvc_base]\n" ;

	my $runtime = $add_runtime ? winbuild::find_runtime( $msvc_base , $arch , "vcruntime140.dll" , "msvcp140.dll" ) : undef ;
	if( $add_runtime && scalar(keys %$runtime) != 2 )
	{
		die "error: install: cannot find msvc [$arch] runtime dlls under [$msvc_base]\n" ;
	}

	# copy the core files -- the main programs are always statically linked so they
	# can go into a "programs" sub-directory -- the gui/setup executable may be
	# dynamically linked so it must go alongside the run-time dlls
	#
	install_core( "$arch/src/main/Release" , $install ) ;

	if( $with_gui )
	{
		install_copy( "$arch/src/gui/Release/emailrelay-gui.exe" , "$install/emailrelay-setup.exe" ) ;

		install_copy( "$arch/src/main/Release/emailrelay-keygen.exe" , "$install/programs/" ) if $with_mbedtls ;

		install_payload_cfg( "$install/payload/payload.cfg" ) ;
		install_core( "$arch/src/main/Release" , "$install/payload/files" , 1 ) ;

		install_copy( "$arch/src/gui/Release/emailrelay-gui.exe" , "$install/payload/files/gui/" ) ;
		install_copy( "$arch/src/main/Release/emailrelay-keygen.exe" , "$install/payload/files/programs/" ) if $with_mbedtls ;

		if( $add_runtime )
		{
			install_gui_dependencies( $msvc_base , $arch , $qt_info->{v} ,
				{ exe => "$install/emailrelay-setup.exe" } ,
				{ exe => "$install/payload/files/gui/emailrelay-gui.exe" } ) ;
		}

		winbuild::translate( $arch , $qt_info , "no_NO" , "no" ) ;
		install_copy( "src/gui/emailrelay.no.qm" , "$install/translations/" ) ;
		install_copy( "src/gui/emailrelay.no.qm" , "$install/payload/files/gui/translations/" ) ;
	}

	if( $add_runtime )
	{
		install_runtime( $runtime , $arch , $install ) ;
		install_runtime( $runtime , $arch , "$install/payload/files/gui" ) if $with_gui ;
	}

	print "$arch distribution in [$install]\n" ;
}

sub run_install_winxp
{
	install_core( "src/main" , $install_winxp , 0 , "." ) ;
	{
		my $fh = new FileHandle( "$install_winxp/emailrelay-start.bat" , "w" ) or die ;
		print $fh "start \"emailrelay\" emailrelay.exe \@app/config.txt\r\n" ;
		$fh->close() or die ;
	}
	{
		my $fh = new FileHandle( "$install_winxp/config.txt" , "w" ) or die ;
		print $fh "#\r\n# config.txt\r\n#\r\n\r\n" ;
		print $fh "# Use emailrelay-start.bat to run emailrelay with this config.\r\n" ;
		print $fh "# For demo purposes this only does forwarding once on startup.\r\n" ;
		print $fh "# Change 'forward' to 'forward-on-disconnect' once you\r\n" ;
		print $fh "# have set a valid 'forward-to' address.\r\n" ;
		for my $item ( qw(
			show=window,tray
			log
			verbose
			log-time
			log-file=@app/log-%d.txt
			syslog
			close-stderr
			spool-dir=@app
			port=25
			interface=0.0.0.0
			forward
			forward-to=127.0.0.1:25
			pop
			pop-port=110
			pop-auth=@app/popauth.txt
			) )
		{
			print $fh "$item\r\n" ;
		}
		print $fh "\r\n" ;
		$fh->close() or die ;
	}
	{
		my $fh = new FileHandle( "$install_winxp/popauth.txt" , "w" ) or die ;
		print $fh "#\r\n# popauth.txt\r\n#\r\n" ;
		print $fh "server plain postmaster postmaster\r\n" ;
		$fh->close() or die ;
	}
	{
		my $fh = new FileHandle( "$install_winxp/emailrelay-service.cfg" , "w" ) or die ;
		print $fh "dir-config=\"\@app\"\r\n" ;
		$fh->close() or die ;
	}
	{
		my $fh = new FileHandle( "$install_winxp/emailrelay-submit-test.bat" , "w" ) or die ;
		my $cmd = "\@echo off\r\n" ;
		$cmd .= "emailrelay-submit.exe -N -n -s \@app --from postmaster " ;
		$cmd .= "-C U3ViamVjdDogdGVzdA== " ; # subject
		$cmd .= "-C = " ;
		$cmd .= "-C VGVzdCBtZXNzYWdl " ; # body
		$cmd .= "-d -F -t " ;
		$cmd .= "postmaster" ; # to
		print $fh "$cmd\r\n" ;
		print $fh "pause\r\n" ;
		$fh->close() or die ;
	}
	system( "zip -q -r $install_winxp.zip $install_winxp" ) == 0 or die ;
	print "winxp mingw distribution in [$install_winxp]\n" ;
}

sub install_runtime
{
	my ( $runtime , $arch , $dst_dir ) = @_ ;
	for my $key ( keys %$runtime )
	{
		my $path = $runtime->{$key}->{path} ;
		my $name = $runtime->{$key}->{name} ;
		my $src = $path ;
		my $dst = "$dst_dir/$name" ;
		if( ! -f $dst )
		{
			File::Copy::copy( $src , $dst ) or die "error: install: failed to copy [$src] to [$dst]\n" ;
		}
	}
}

sub install_gui_dependencies
{
	my ( $msvc_base , $arch , $qtv , @tasks ) = @_ ;

	my $qt_bin = find_qt_bin( $arch , $qtv ) ;
	print "qt-bin=[$qt_bin]\n" ;

	$ENV{VCINSTALLDIR} = $msvc_base ;
	for my $task ( @tasks )
	{
		my $exe_in = $task->{exe} ;
		my $exe = exists($task->{dir}) ? ("$$task{dir}/".basename($exe_in)) : $exe_in ;
		if( $exe_in ne $exe )
		{
			File::Copy::copy( $exe_in , $exe ) or die "error: install: failed to copy [$exe_in] to [$exe]\n" ;
		}
		my $rc = system( "$qt_bin/windeployqt.exe" , $exe ) ;
		if( $exe_in ne $exe ) { unlink( $exe ) or die }
		$rc == 0 or die "error: install: failed running [$qt_bin/windeployqt.exe] [$exe]" ;
	}
}

sub find_qt_bin
{
	my ( $arch , $qtv ) = @_ ;
	my $qt_core = cmake_cache_value_qt_core( $arch , $qtv ) ;
	my $dir = $qt_core ;
	for( 1..10 )
	{
		last if -f "$dir/bin/windeployqt.exe" ;
		$dir = dirname( $dir ) ;
	}
	$dir or die "error: install: cannot determine the qt bin directory from [$qt_core]\n" ;
	return "$dir/bin" ;
}

sub cmake_cache_value_qt_core
{
	my ( $arch , $qtv ) = @_ ;
	my $qt_core_dir = winbuild::cmake_cache_value( $arch , qr/^Qt[56]Core_DIR:[A-Z]+=(.*)/ ) ;
	$qt_core_dir or die "error: install: cannot read qt path from CMakeCache.txt\n" ;
	return $qt_core_dir ;
}

sub install_payload_cfg
{
	my ( $file ) = @_ ;
	File::Path::make_path( File::Basename::dirname($file) ) ;
	my $fh = new FileHandle( $file , "w" ) or die "error: install: cannot create [$file]\n" ;
	print $fh "files/programs/=\%dir-install\%/\n" ;
	print $fh "files/examples/=\%dir-install\%/examples/\n" ;
	print $fh "files/doc/=\%dir-install\%/doc/\n" ;
	print $fh "files/txt/=\%dir-install\%/\n" ;
	print $fh "files/gui/=\%dir-install\%/\n" ;
	$fh->close() or die ;
}

sub install_core
{
	my ( $src_main_bin_dir , $root , $is_payload , $programs ) = @_ ;

	$programs ||= "programs" ;
	my $txt = $is_payload ? "txt" : "." ;
	my %copy = qw(
		README __txt__/readme.txt
		COPYING __txt__/copying.txt
		AUTHORS __txt__/authors.txt
		LICENSE __txt__/license.txt
		NEWS __txt__/news.txt
		ChangeLog __txt__/changelog.txt
		doc/doxygen-missing.html doc/doxygen/index.html
		__src_main_bin_dir__/emailrelay-service.exe __programs__/
		__src_main_bin_dir__/emailrelay.exe __programs__/
		__src_main_bin_dir__/emailrelay-submit.exe __programs__/
		__src_main_bin_dir__/emailrelay-passwd.exe __programs__/
		__src_main_bin_dir__/emailrelay-textmode.exe __programs__/
		bin/emailrelay-service-install.js __programs__/
		bin/emailrelay-bcc-check.pl examples/
		bin/emailrelay-edit-content.js examples/
		bin/emailrelay-edit-envelope.js examples/
		bin/emailrelay-resubmit.js examples/
		bin/emailrelay-set-from.pl examples/
		bin/emailrelay-set-from.js examples/
		doc/authentication.png doc/
		doc/forwardto.png doc/
		doc/whatisit.png doc/
		doc/serverclient.png doc/
		doc/*.html doc/
		doc/developer.txt doc/
		doc/reference.txt doc/
		doc/userguide.txt doc/
		doc/windows.txt doc/
		doc/windows.txt __txt__/readme-windows.txt
		doc/doxygen-missing.html doc/doxygen/index.html
	) ;
	while( my ($src,$dst) = each %copy )
	{
		$dst = "" if $dst eq "." ;
		$src =~ s:__src_main_bin_dir__:$src_main_bin_dir:g ;
		$dst =~ s:__programs__:$programs:g ;
		$dst =~ s:__txt__:$txt:g ;
		map { install_copy( $_ , "$root/$dst" ) } glob( $src ) ;
	}
	# fix up inter-document references in readme.txt and license.txt
	winbuild::fixup( $root ,
		[ "$txt/readme.txt" , "$txt/license.txt" ] ,
		{
			README => 'readme.txt' ,
			COPYING => 'copying.txt' ,
			AUTHORS => 'authors.txt' ,
			INSTALL => 'install.txt' ,
			ChangeLog => 'changelog.txt' ,
		} ) ;
}

sub install_copy
{
	my ( $src , $dst ) = @_ ;
	winbuild::file_copy( $src , $dst ) ;
}

sub install_mkdir
{
	my ( $dir ) = @_ ;
	return if -d $dir ;
	mkdir( $dir ) or die "error: install: failed to mkdir [$dir]\n" ;
}

sub run_tests
{
	my ( $main_bin_dir , $test_bin_dir ) = @_ ;
	my $dash_v = "" ; # or "-v"
	my $script = "test/emailrelay_test.pl" ;
	system( "perl -Itest \"$script\" $dash_v -d \"$main_bin_dir\" -x \"$test_bin_dir\" -c \"test/certificates\"" ) ;
}

