#!/usr/bin/env perl
#
# Copyright (C) 2001-2022 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# "bin/winbuild.pm". A configuration file "winbuild.cfg" can be used
# to give explicit locations if necessary. The "find" sub-task, ie.
# "winbuild-find.bat", can be used to run the find routines and create
# a "winbuild.cfg" configuration file.
#
# The "install" sub-task, which is not run by default, assembles binaries
# and their dependencies in a directory tree ready for zipping and
# distribution. The dependencies for Qt are assembled by the Qt dependency
# tool, "windeployqt".
#

use strict ;
use Cwd ;
use FileHandle ;
use File::Find ;
use File::Basename ;
use File::Copy ;
use lib dirname($0) , dirname($0)."/bin" ;
require "winbuild.pm" ;

# configuration ...

# cmake command-line options
my $cmake_args = {
	x64 => [
		# try these in turn...
		[ "-G" , "Visual Studio 16 2019" , "-A" , "x64" ] ,
		[ "-A" , "x64" ] ,
	] ,
	x86 => [
		# try these in turn...
		[ "-G" , "Visual Studio 16 2019" , "-A" , "Win32" ] ,
		[ "-A" , "Win32" ] ,
		[]
	] ,
} ;

# version
chomp( my $version = eval { FileHandle->new("VERSION")->gets() } || "2.4" ) ;

# makefile conditionals
my %switches = (
	GCONFIG_BSD => 0 ,
	GCONFIG_EPOLL => 0 ,
	GCONFIG_GUI => 1 , # << zero if no qt libraries
	GCONFIG_INSTALL_HOOK => 0 ,
	GCONFIG_INTERFACE_NAMES => 1 ,
	GCONFIG_IPV6 => 1 ,
	GCONFIG_MAC => 0 ,
	GCONFIG_PAM => 0 ,
	GCONFIG_TESTING => 1 ,
	GCONFIG_TLS_USE_MBEDTLS => 1 , # << zero if no mbedtls source
	GCONFIG_TLS_USE_OPENSSL => 0 ,
	GCONFIG_TLS_USE_BOTH => 0 ,
	GCONFIG_TLS_USE_NONE => 0 ,
	GCONFIG_WINDOWS => 1 ,
) ;

# makefile expansion variables -- many are required but not relevant
my %vars = (
	top_srcdir => "." ,
	top_builddir => "." ,
	sbindir => "." ,
	mandir => "." ,
	localedir => "." ,
	e_spooldir => "c:/windows/spool/emailrelay" ,
	e_docdir => "c:/emailrelay" ,
	e_initdir => "c:/emailrelay" ,
	e_bsdinitdir => "c:/emailrelay" ,
	e_rundir => "c:/emailrelay" ,
	e_icondir => "c:/emailrelay" ,
	e_trdir => "c:/emailrelay" ,
	e_examplesdir => "c:/emailrelay" ,
	e_libexecdir => "c:/emailrelay" ,
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

# find stuff ...

my $need_mbedtls = ( $switches{GCONFIG_TLS_USE_MBEDTLS} || $switches{GCONFIG_TLS_USE_BOTH} ) ;
my $need_qt = $switches{GCONFIG_GUI} ;

my $cmake = winbuild::find_cmake() ;
my $msbuild = winbuild::find_msbuild() ;
my $qt_dirs = $need_qt ? winbuild::find_qt() : undef ; # hash-ref keyed by architecture
my $mbedtls = $need_mbedtls ? winbuild::find_mbedtls() : undef ;

my $no_cmake = !$cmake ;
my $no_msbuild = !$msbuild ;
my $no_qt = ( $need_qt && !$qt_dirs->{x86} && !$qt_dirs->{x64} ) ;
my $no_mbedtls = ( $need_mbedtls && !$mbedtls ) ;

warn "error: cannot find cmake.exe: please download from cmake.org\n" if $no_cmake ;
warn "error: cannot find msbuild.exe: please install visual studio\n" if $no_msbuild ;
warn "error: cannot find qt libraries: please download from wwww.qt.io or unset GCONFIG_GUI\n" if $no_qt ;
warn "error: cannot find mbedtls source: please download from tls.mbed.org " .
	"or unset GCONFIG_TLS_USE_MBEDTLS\n" if $no_mbedtls ;

if( $no_cmake || $no_msbuild || $no_qt || $no_mbedtls )
{
	warn "error: missing prerequisites: please install the missing components" ,
		( ($no_qt||$no_mbedtls) ? " or unset configuration items in winbuild.pl\n" : "\n" ) ;
	die "error: missing prerequisites\n" unless ( scalar(@ARGV) > 0 && $ARGV[0] eq "mingw" ) ;
}

# choose what to run ...

my @default_parts =
	$need_mbedtls ?
		qw( batchfiles generate mbedtls cmake msbuild ) :
		qw( batchfiles generate cmake msbuild ) ;

# run stuff ...

my $project = "emailrelay" ;
my $install_x64 = "$project-$version-w64" ;
my $install_x86 = "$project-$version-w32" ;
my $install_mingw = "$project-$version-winxp" ;

my @run_parts = scalar(@ARGV) ? @ARGV : @default_parts ;
for my $part ( @run_parts )
{
	if( $part eq "find" )
	{
		winbuild::fcache_delete() ;
		winbuild::find_cmake() ;
		winbuild::find_msbuild() ;
		winbuild::find_qt() if $need_qt ;
		winbuild::find_mbedtls() if $need_mbedtls ;
		winbuild::fcache_write() ;
	}
	elsif( $part eq "batchfiles" )
	{
		winbuild::spit_out_batch_files( qw(
			find generate cmake msbuild
			debug-build debug-test test
			mbedtls clean vclean install ) ) ;
	}
	elsif( $part eq "generate" )
	{
		run_generate( $project , \%switches , \%vars ) ;
	}
	elsif( $part eq "mbedtls" )
	{
		run_mbedtls_cmake( $mbedtls , $cmake , "x64" ) ;
		run_mbedtls_cmake( $mbedtls , $cmake , "x86" ) ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x64" , "Debug" ) ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x64" , "Release" ) ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x86" , "Debug" ) ;
		run_mbedtls_msbuild( $mbedtls , $msbuild , "x86" , "Release" ) ;
	}
	elsif( $part eq "cmake" )
	{
		run_cmake( $cmake , $mbedtls , $qt_dirs , "x64" ) ;
		run_cmake( $cmake , $mbedtls , $qt_dirs , "x86" ) ;
	}
	elsif( $part eq "msbuild" )
	{
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Release" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Release" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Debug" ) ;
	}
	elsif( $part eq "debug-build" )
	{
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" ) ;
	}
	elsif( $part eq "clean" )
	{
		clean_test_files() ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Release" , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Debug" , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Release" , "Clean" ) ;
	}
	elsif( $part eq "vclean" )
	{
		clean_test_files() ;
		winbuild::clean_cmake_files() ;
		winbuild::clean_cmake_cache_files() ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Debug" , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x64" , "Release" , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Debug" , "Clean" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" , "Release" , "Clean" ) ;
		winbuild::deltree( $install_x64 ) ;
		winbuild::deltree( $install_x86 ) ;
		winbuild::deltree( "$mbedtls/x64" ) ;
		winbuild::deltree( "$mbedtls/x86" ) ;
		winbuild::fcache_delete() ;
	}
	elsif( $part eq "install" )
	{
		install( $install_x64 , "x64" , $qt_dirs , $switches{GCONFIG_GUI} ) ;
		install( $install_x86 , "x86" , $qt_dirs , $switches{GCONFIG_GUI} ) ;
	}
	elsif( $part eq "mingw" )
	{
		install_mingw() ;
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
if( (grep {$_ eq "msbuild"} @run_parts) && !(grep {$_ eq "install"} @run_parts) )
{
	print "build finished -- try winbuild-install.bat for packaging\n"
}

# ==

sub create_cmake_file
{
	my ( $project , $m , $switches ) = @_ ;

	my $path = join( "/" , dirname($m->path()) , "CMakeLists.txt" ) ;

	print "cmake-file=[$path]\n" ;
	my $fh = new FileHandle( $path , "w" ) or die ;

	print $fh "# $path -- generated by $0\n" ;
	if( $project )
	{
		print $fh "cmake_minimum_required(VERSION 3.1.0)\n" ; # was 2.8.11
		print $fh "project($project)\n" ;
		if( $switches{GCONFIG_GUI} )
		{
			# see https://doc.qt.io/qt-5/cmake-get-started.html
			print $fh "find_package(Qt5 COMPONENTS Widgets Gui Core OpenGL REQUIRED)\n" ;
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

	# force static or dynamic linking of the c++ runtime by
	# switching between /MD and /MT -- use static linking
	# by default but keep the gui dynamically linked to
	# avoid separate runtime states in Qt and non-Qt code --
	# note that the gui code is self-contained by virtue
	# of "glibsources.cpp"
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

	my $definitions = join( " " , "G_WINDOWS=1" , $m->definitions() ) ;
	my $includes = join( " " , "." , ".." , $m->includes($m->top()) , '"${MBEDTLS_INCLUDE_DIRS}"' ) ;

	my @libraries = $m->libraries() ;
	for my $library ( @libraries )
	{
		my $sources = join( " " , $m->sources( $library ) ) ;

		( my $library_key = $library ) =~ s/\.a$// ; $library_key =~ s/^lib// ;
		print $fh "\n" ;
		print $fh "add_library($library_key $sources)\n" ;
		print $fh "target_include_directories($library_key PUBLIC $includes)\n" ;
		print $fh "target_compile_definitions($library_key PUBLIC $definitions)\n" ;
	}

	my $tls_libs_fixed ;
	my @programs = $m->programs() ;
	for my $program ( @programs )
	{
		my $sources = join( " " , $m->sources( $program ) ) ;
		my $our_libs = join( " " , $m->our_libs( $program ) ) ;
		my $sys_libs = join( " " , $m->sys_libs( $program ) ) ;

		my $tls_libs = "" ;
		if( ( $our_libs =~ m/gssl/ ) &&
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

		my $qt_libs = ( $m->path() =~ m/gui/ ) ? "Qt5::OpenGL Qt5::Widgets Qt5::Gui Qt5::Core" : "" ;

		my $win32 = ( $m->path() =~ m/gui/ || $program eq "emailrelay" ) ? "WIN32 " : "" ;
		$win32 = "" if ( $program =~ m/textmode/ ) ;

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
	my ( $project , $switches , $vars ) = @_ ;

	my @makefiles = winbuild::read_makefiles( $switches , $vars ) ;
	my $first = 1 ;
	for my $m ( @makefiles )
	{
		create_cmake_file( $first?$project:undef , $m , $switches ) ;
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
	my ( $project , $switches , $vars ) = @_ ;
	create_cmake_files( $project , $switches , $vars ) ;
	create_gconfig_header() ;
}

sub run_cmake
{
	my ( $cmake , $mbedtls , $qt_dirs , $arch ) = @_ ;
	$mbedtls ||= "." ;
	$arch ||= "x64" ;

	# (only full paths work here)
	my $mbedtls_dir = Cwd::realpath( $mbedtls ) ;
	my $mbedtls_include_dir = "$mbedtls_dir/include" ;
	my $mbedtls_lib_dir = "$mbedtls_dir/$arch/library/Release" ; # fixed up to Debug elsewhere
	my $qt_dir = Cwd::realpath( $qt_dirs->{$arch} ) ;
	my $module_path = Cwd::realpath( "." ) ;

	my @arch_args = @{$cmake_args->{$arch}} ;
	my $rc ;
	for my $arch_args ( @arch_args )
	{
		winbuild::clean_cmake_cache_files( $arch , {verbose=>0} ) ;

		my @args = @$arch_args ;
		unshift @args , "-DCMAKE_MODULE_PATH:FILEPATH=$module_path" ;
		unshift @args , "-DCMAKE_INCLUDE_PATH:FILEPATH=$mbedtls_include_dir" ;
		unshift @args , "-DCMAKE_LIBRARY_PATH:FILEPATH=$mbedtls_lib_dir" ;
		unshift @args , "-DQt5_DIR:FILEPATH=$qt_dir" ;
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
		print "mbedtls-msbuild($arch,confname): already got [$arch/$output_file]: not running msbuild\n" ;
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
	my ( $install , $arch , $qt_dirs , $with_gui ) = @_ ;

	my $msvc_base = winbuild::find_msvc_base( $arch ) ;
	print "msvc-base=[$msvc_base]\n" ;

	my $runtime = winbuild::find_runtime( $msvc_base , $arch , "vcruntime140.dll" , "msvcp140.dll" ) ;
	if( scalar(keys %$runtime) != 2 )
	{
		die "error: install: cannot find msvc [$arch] runtime dlls under [$msvc_base]\n" ;
	}

	install_core( "$arch/src/main/Release" , $install ) ;

	if( $with_gui )
	{
		install_copy( "$arch/src/gui/Release/emailrelay-gui.exe" , "$install/emailrelay-setup.exe" ) ;
		install_copy( "$arch/test/Release/emailrelay-test-keygen.exe" , "$install" ) ;

		install_mkdir( "$install/payload" ) ;
		install_payload_cfg( "$install/payload/payload.cfg" ) ;
		install_core( "$arch/src/main/Release" , "$install/payload/files" ) ;

		install_copy( "$arch/src/gui/Release/emailrelay-gui.exe" , "$install/payload/files" ) ;
		install_copy( "$arch/test/Release/emailrelay-test-keygen.exe" , "$install/payload/files" ) ;

		install_gui_dependencies( $msvc_base , $arch ,
			{ exe => "$install/emailrelay-setup.exe" } ,
			{ exe => "$install/payload/files/emailrelay-gui.exe" } ) ;

		winbuild::translate( $arch , $qt_dirs , "no_NO" , "no" ) ;
		install_copy( "src/gui/emailrelay.no.qm" , "$install/translations" ) ;
		install_copy( "src/gui/emailrelay.no.qm" , "$install/payload/files/translations" ) ;
	}

	install_runtime( $runtime , $arch , $install ) ;
	install_runtime( $runtime , $arch , "$install/payload/files" ) ;

	print "$arch distribution in [$install]\n" ;
}

sub install_mingw
{
	install_core( "src/main" , $install_mingw ) ;
	{
		my $fh = new FileHandle( "$install_mingw/emailrelay-start.bat" , "w" ) or die ;
		print $fh "start \"emailrelay\" emailrelay.exe \@app/config.txt\r\n" ;
		$fh->close() or die ;
	}
	{
		my $fh = new FileHandle( "$install_mingw/config.txt" , "w" ) or die ;
		print $fh "#\r\n# config.txt\r\n#\r\n\r\n" ;
		print $fh "# For demo purposes this does loopback forwarding on startup\r\n" ;
		print $fh "# only. Change 'forward' to 'forward-on-disconnect' when you\r\n" ;
		print $fh "# have set the 'forward-to' address.\r\n" ;
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
		my $fh = new FileHandle( "$install_mingw/popauth.txt" , "w" ) or die ;
		print $fh "#\r\n# popauth.txt\r\n#\r\n" ;
		print $fh "server plain postmaster postmaster\r\n" ;
		$fh->close() or die ;
	}
	{
		my $fh = new FileHandle( "$install_mingw/emailrelay-service.cfg" , "w" ) or die ;
		print $fh "dir-config=\"\@app\"\r\n" ;
		$fh->close() or die ;
	}
	{
		my $fh = new FileHandle( "$install_mingw/emailrelay-submit-test.bat" , "w" ) or die ;
		my $cmd = "\@echo off\r\n" ;
		$cmd .= "emailrelay-submit.exe -n -s \@app --from postmaster " ;
		$cmd .= "-C U3ViamVjdDogdGVzdA== " ; # subject
		$cmd .= "-C = " ;
		$cmd .= "-C VGVzdCBtZXNzYWdl " ; # body
		$cmd .= "-d -F -t " ;
		$cmd .= "postmaster" ; # to
		print $fh "$cmd\r\n" ;
		print $fh "pause\r\n" ;
		$fh->close() or die ;
	}
	system( "zip -q -r $install_mingw.zip $install_mingw" ) == 0 or die ;
	print "mingw distribution in [$install_mingw]\n" ;
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
	my ( $msvc_base , $arch , @tasks ) = @_ ;

	my $qt_bin = find_qt_bin( $arch ) ;
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
	my ( $arch ) = @_ ;
	my $qt_core = cache_value_qt_core( $arch ) ;
	my $dir = $qt_core ;
	for( 1..10 )
	{
		last if -f "$dir/bin/windeployqt.exe" ;
		$dir = dirname( $dir ) ;
	}
	$dir or die "error: install: cannot determine the qt bin directory from [$qt_core]\n" ;
	return "$dir/bin" ;
}

sub cache_value_qt_core
{
	my ( $arch ) = @_ ;
	my $qt5_core_dir = winbuild::cache_value( $arch , qr/^Qt5Core_DIR:[A-Z]+=(.*)/ ) ;
	$qt5_core_dir or die "error: install: cannot read qt path from CMakeCache.txt\n" ;
	return $qt5_core_dir ;
}

sub install_payload_cfg
{
	my ( $file ) = @_ ;
	my $fh = new FileHandle( $file , "w" ) or die ;
	print $fh 'files/= %dir-install%/' , "\n" ;
	$fh->close() or die ;
}

sub install_core
{
	my ( $main_bin_dir , $root ) = @_ ;

	install_mkdir( $root ) ;

	my @tree = qw(
		doc
		doc/doxygen
		examples
	) ;
	map { install_mkdir("$root/$_") } @tree ;

	my %copy = qw(
		README readme.txt
		COPYING copying.txt
		AUTHORS authors.txt
		LICENSE license.txt
		NEWS news.txt
		ChangeLog changelog.txt
		doc/doxygen-missing.html doc/doxygen/index.html
		__main_bin_dir__/emailrelay-service.exe .
		__main_bin_dir__/emailrelay.exe .
		__main_bin_dir__/emailrelay-submit.exe .
		__main_bin_dir__/emailrelay-filter-copy.exe .
		__main_bin_dir__/emailrelay-passwd.exe .
		__main_bin_dir__/emailrelay-textmode.exe .
		bin/emailrelay-bcc-check.pl examples
		bin/emailrelay-service-install.js .
		bin/emailrelay-edit-content.js examples
		bin/emailrelay-edit-envelope.js examples
		bin/emailrelay-resubmit.js examples
		bin/emailrelay-set-from.pl examples
		bin/emailrelay-set-from.js examples
		doc/authentication.png doc
		doc/forwardto.png doc
		doc/whatisit.png doc
		doc/serverclient.png doc
    	doc/*.html doc
    	doc/developer.txt doc
    	doc/reference.txt doc
    	doc/userguide.txt doc
    	doc/windows.txt doc
		doc/windows.txt readme-windows.txt
		doc/doxygen-missing.html doc/doxygen/index.html
	) ;
	while( my ($src,$dst) = each %copy )
	{
		$dst = "" if $dst eq "." ;
		$src =~ s:__main_bin_dir__:$main_bin_dir:g ;
		map { install_copy( $_ , "$root/$dst" ) } glob( $src ) ;
	}
	winbuild::fixup( $root ,
		[ "readme.txt" , "license.txt" ] ,
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
	my $script = "test/emailrelay-test.pl" ;
	## $script = ( $script."_") if ! -f $script ;
	system( "perl -Itest \"$script\" $dash_v -d \"$main_bin_dir\" -x \"$test_bin_dir\" -c \"test/certificates\"" ) ;
}

