#!/usr/bin/env perl
#
# Copyright (C) 2001-2018 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Parses automake files throughout the source tree, generates
# cmake files for windows, runs cmake to generate visual
# studio project files, and then runs msbuild.
#
# usage: winbuild.pl [<subtask> [<subtask> ...]]
#
# Requires cmake and msbuild to be on the path or somewhere
# obvious, and cmake requires visual studio.
#
# By default the build will need headers and libraries for
# mbedtls and qt. The cmake "find_package()" mechanism is
# used to find them. The mbedtls source tree should ideally
# be in a sibling directory.
#
# Also spits out batch files (like "winbuild-whatever.bat")
# for doing sub-tasks, including "winbuild-install.bat".
#
# The "install" sub-task, which is not run by default, assembles
# binaries and their dependencies in a directory tree ready
# for zipping and distribution. The dependencies for Qt are
# assembled by the Qt dependency tool, "windeployqt".
#

use strict ;
use Cwd ;
use FileHandle ;
use File::Find ;
use File::Basename ;
use File::Spec ;
use File::Copy ;
unshift @INC , dirname($0) ;
require "winbuild-parser.pm" ;
require "winbuild.pm" ;
$AutoMakeParser::verbose = 0 ;

my $project = "emailrelay" ;
my $version = "2.0.1" ;
my $install_x64 = "$project-$version-w64" ;
my $install_x86 = "$project-$version-w32" ;

my $cmake = winbuild::find_cmake() ;
my $msbuild = winbuild::find_msbuild() ;
my $touchfile = winbuild::default_touchfile($0) ;
my $qt_dirs = winbuild::find_qt("c:/","c:/data") ; # eg. {x86=>"c:/data/qt/5.10/msvc2015/lib/cmake/qt5",x64=>...}

die "error: cannot find cmake.exe\n" if !$cmake ;
die "error: cannot find msbuild.exe\n" if !$msbuild ;

# conditionals in the automake files, canonically determined by 'configure'
my %switches = (
	GCONFIG_GUI => 1 ,
	GCONFIG_IPV6 => 1 ,
	GCONFIG_MAC => 0 ,
	GCONFIG_WINDOWS => 1 ,
	GCONFIG_TESTING => 1 ,
	GCONFIG_TLS_USE_OPENSSL => 0 ,
	GCONFIG_TLS_USE_MBEDTLS => 1 ,
	GCONFIG_TLS_USE_BOTH => 0 ,
	GCONFIG_TLS_USE_NONE => 0 ,
	GCONFIG_INSTALL_HOOK => 0 ,
	GCONFIG_ICONV => 0 ,
	GCONFIG_PAM => 0 ,
) ;

# makefile expansion variables -- many are required but not relevant
my %vars = (
	top_srcdir => "." ,
	top_builddir => "." ,
	sbindir => "." ,
	mandir => "." ,
	e_spooldir => "c:/windows/spool/emailrelay" ,
	e_docdir => "c:/emailrelay" ,
	e_initdir => "c:/emailrelay" ,
	e_icondir => "c:/emailrelay" ,
	e_spooldir => "c:/emailrelay" ,
	e_examplesdir => "c:/emailrelay" ,
	e_libexecdir => "c:/emailrelay" ,
	e_pamdir => "c:/emailrelay" ,
	e_sysconfdir => "c:/emailrelay" ,
	GCONFIG_WINDRES => "windres" ,
	GCONFIG_WINDMC => "mc" ,
	GCONFIG_CONFIGURATION => "windows" ,
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

print "cmake=[$cmake]\n" ;
print "msbuild=[$msbuild]\n" ;

my $need_mbedtls = ( $switches{GCONFIG_TLS_USE_MBEDTLS} || $switches{GCONFIG_TLS_USE_BOTH} ) ;
my @default_parts =
	$need_mbedtls ?
		qw( batchfiles generate mbedtls cmake msbuild ) :
		qw( batchfiles generate cmake msbuild ) ;

my @run_parts = scalar(@ARGV) ? @ARGV : @default_parts ;
for my $part ( @run_parts )
{
	if( $part eq "batchfiles" )
	{
		winbuild::spit_out_batch_files( qw( generate cmake msbuild debug-build debug-test test mbedtls clean vclean install ) ) ;
	}
	elsif( $part eq "generate" )
	{
		run_generate( $project , \%switches , \%vars ) ;
	}
	elsif( $part eq "mbedtls" )
	{
		build_mbedtls( $cmake , $msbuild , "Release" ) ;
		build_mbedtls( $cmake , $msbuild , "Debug" ) ;
	}
	elsif( $part eq "cmake" )
	{
		run_cmake( $cmake , "x64" ) ;
		run_cmake( $cmake , "x86" ) ;
	}
	elsif( $part eq "msbuild" )
	{
		winbuild::run_msbuild( $msbuild , $project , "x64" ) ;
		winbuild::run_msbuild( $msbuild , $project , "x86" ) ;
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
	}
	elsif( $part eq "install" )
	{
		install( $install_x64 , "x64" , $switches{GCONFIG_GUI} ) ;
		install( $install_x86 , "x86" , $switches{GCONFIG_GUI} ) ;
	}
	elsif( $part eq "debug-test" )
	{
		run_tests( "x64/src/main/Debug" , "x64/test/Debug" ) ;
	}
	elsif( $part eq "test" )
	{
		run_tests( "x64/src/main/Release" , "x64/test/Release" ) ;
	}
}
winbuild::create_touchfile( $touchfile ) ;

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
		print $fh "cmake_minimum_required(VERSION 2.8.11)\n" ;
		print $fh "project($project)\n" ;
		if( $switches{GCONFIG_GUI} )
		{
			print $fh "find_package(Qt5 CONFIG REQUIRED Widgets Gui Core OpenGL)\n" ;
			#print $fh "find_package(Qt5OpenGL REQUIRED)\n" ;
			#print $fh "find_package(Qt5Widgets REQUIRED)\n" ;
			#print $fh "find_package(Qt5Gui REQUIRED)\n" ;
			#print $fh "find_package(Qt5Core REQUIRED)\n" ;
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
	# switching between /MD and /MT -- keep the gui dynamically
	# linked to avoid separate runtime states in Qt and
	# non-Qt code
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

	my $definitions = join( " " , "G_WIN32=1" , $m->definitions() ) ;
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

		my $qt_includes = ( $m->path() =~ m/gui/ ) ? "Qt5::Widgets Qt5::Gui Qt5::Core" : "" ;
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
			$resources = "messages.mc emailrelay-gui.rc emailrelay-gui.exe.manifest" ;
			$resource_includes = "../main/icon" ;
		}

		my $program_sources = join(" ",split(' ',"$win32 $resources $sources")) ;
		my $program_includes = join(" ",split(' ',"$resource_includes $includes $qt_includes")) ;
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
	print $fh 'find_package_handle_standard_args(MBEDTLS DEFAULT_MSG MBEDTLS_INCLUDE_DIRS MBEDTLS_LIBRARY MBEDX509_LIBRARY MBEDCRYPTO_LIBRARY)' , "\n" ;
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
	my ( $cmake , $arch ) = @_ ;
	$arch ||= "x64" ;

	# (only full paths work here)
	my $mbed_dir = Cwd::realpath( "../mbedtls" ) ;
	my $mbed_include_dir = "$mbed_dir/include" ;
	my $mbed_lib_dir = "$mbed_dir/$arch/library/Release" ; # fixed up to Debug elsewhere
	my $module_path = Cwd::realpath( "." ) ;

	my @args = $arch eq "x86" ? () : ( "-A" , $arch ) ;
	unshift @args , "-DCMAKE_MODULE_PATH:FILEPATH=$module_path" ; # so cmake can find FindMbedTLS.cmake
	unshift @args , "-DCMAKE_INCLUDE_PATH:FILEPATH=$mbed_include_dir" ; # used by find_path() in FindMbedTLS.cmake
	unshift @args , "-DCMAKE_LIBRARY_PATH:FILEPATH=$mbed_lib_dir" ; # used by find_library() in FindMbedTLS.cmake
	unshift @args , "-DQt5_DIR:FILEPATH=$$qt_dirs{$arch}" ;
	unshift @args , "-DCMAKE_MC_COMPILER:FILEPATH=mc" ;

	my $build_dir = $arch ;
	mkdir $build_dir ;
	my $base_dir = getcwd() or die ;
	chdir $build_dir or die ;
	print "cmake: cwd=[".getcwd()."] exe=[$cmake] args=[".join("][",@args)."][..]\n" ;
	my $rc = system( $cmake , ( @args , ".." ) ) ;
	chdir $base_dir or die ;

	print "cmake-exit=[$rc]\n" ;
	die unless $rc == 0 ;
}

sub create_gconfig_header
{
	my $path = "src/gconfig_defs.h" ;
	if( ! -f $path )
	{
		my $fh = new FileHandle( $path , "w" ) or die ;
		$fh->close() or die ;
	}
}

sub build_mbedtls
{
	my ( $cmake , $msbuild , $confname ) = @_ ;
	$confname ||= "Release" ;

	my $mbed_dir = "../mbedtls" ; # could do better
	my $base_dir = getcwd() ;

	if( ! -d $mbed_dir ) { die "error: no mbedtls source directory: please download from tls.mbed.org\n" }

	my $map = {
		x86 => [] ,
		x64 => [ "-A" , "x64" ] ,
	} ;

	for my $arch ( keys %$map )
	{
		mkdir "$mbed_dir/$arch" ;
		chdir "$mbed_dir/$arch" or die ;

		my @args = (
			"-DCMAKE_C_FLAGS_DEBUG=/MTd" ,
			"-DCMAKE_C_FLAGS_RELEASE=/MT" ,
			@{$map->{$arch}}
		) ;

		my $mbed_project = "mbed TLS" ;
		my $cmake_output_file = "$mbed_project.sln" ;
		if( ! -f $cmake_output_file )
		{
			print "mbedtls-cmake: cwd=[".getcwd()."] exe=[$cmake] args=[".join("][",@args)."][..]\n" ;
			my $rc = system( $cmake , @args , ".." ) ;
			print "mbedtls-cmake: [$arch]: exit=[$rc]\n" ;
			die unless $rc == 0 ;
		}

		my $build_output_file = "library/$confname/mbedtls.lib" ;
		if( ! -f $build_output_file )
		{
			my @msbuild_args = ( "/fileLogger" , "/p:Configuration=$confname" , "\"$mbed_project.sln\"" ) ;
			my $rc = system( $msbuild , @msbuild_args ) ;
			print "mbedtls-msbuild: [$arch][$confname]: exit=[$rc]\n" ;
			die unless $rc == 0 ;
		}

		chdir $base_dir or die ;
	}
}

sub install
{
	my ( $install , $arch , $with_gui ) = @_ ;

	my $msvc_base = winbuild::find_msvc_base( $arch ) ;
	print "msvc-base=[$msvc_base]\n" ;

	install_core( $arch , $install ) ;
	install_copy( "$arch/src/gui/Release/emailrelay-gui.exe" , "$install/emailrelay-setup.exe" ) if $with_gui ;

	if( $with_gui )
	{
		install_mkdir( "$install/payload" ) ;
		install_payload_cfg( "$install/payload/payload.cfg" ) ;
		install_core( $arch , "$install/payload/files" ) ;
		install_copy( "$arch/src/gui/Release/emailrelay-gui.exe" , "$install/payload/files" ) ;
		install_gui_dependencies( $msvc_base , $arch ,
			{ exe => "$install/emailrelay-setup.exe" } ,
			{ exe => "$install/payload/files/emailrelay-gui.exe" } ) ;
	}

	install_runtime_dependencies( $msvc_base , $arch , $install ) ;
	install_runtime_dependencies( $msvc_base , $arch , "$install/payload/files" ) ;

	print "$arch distribution in [$install]\n" ;
}

sub install_runtime_dependencies
{
	my ( $msvc_base , $arch , $dst_dir ) = @_ ;
	my @dll_list = ( "vcruntime140.dll" , "msvcp140.dll" ) ;
	my $locale = "en" ;
	for my $dll ( @dll_list )
	{
		my $src = "$msvc_base/redist/$arch/Microsoft.VC140.CRT/$dll" ; # todo use File::Find (not glob!)
		my $dst = "$dst_dir/$dll" ;
		if( ! -f $dst )
		{
			copy( $src , $dst ) or die "error: install: failed to copy [$src] to [$dst]\n" ;
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
		if( $exe_in ne $exe ) { copy( $exe_in , $exe ) or die "error: install: failed to copy [$exe_in] to [$exe]\n" }
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
	$dir or die "error: install: cannot determins the qt bin directory from [$qt_core]\n" ;
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
	my ( $arch , $root ) = @_ ;

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
		__arch__/src/main/Release/emailrelay-service.exe .
		__arch__/src/main/Release/emailrelay.exe .
		__arch__/src/main/Release/emailrelay-submit.exe .
		__arch__/src/main/Release/emailrelay-filter-copy.exe .
		__arch__/src/main/Release/emailrelay-passwd.exe .
		__arch__/src/main/Release/emailrelay-textmode.exe .
		bin/emailrelay-bcc-check.pl examples
		bin/emailrelay-service-install.js .
		bin/emailrelay-edit-content.js examples
		bin/emailrelay-edit-envelope.js examples
		bin/emailrelay-resubmit.js examples
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
		$src =~ s:__arch__:$arch:g ;
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

	my $convert = undef ;
	for my $ext ( "txt" , "js" , "pl" , "pm" )
	{
		$convert = 1 if( ( ! -d $dst && ( $dst =~ m/$ext$/ ) ) || ( -d $dst && ( $src =~ m/$ext$/ ) ) ) ;
	}

	if( $convert )
	{
		if( -d $dst ) { $dst = "$dst/".basename($src) }
		my $fh_in = new FileHandle( $src , "r" ) ;
		my $fh_out = new FileHandle( $dst , "w" ) ;
		( $fh_in && $fh_out ) or die "error: install: failed to copy [$src] to [$dst]\n" ;
		while(<$fh_in>)
		{
			chomp( my $line = $_ ) ;
			print $fh_out $line , "\r\n" ;
		}
		$fh_out->close() or die "error: install: failed to copy [$src] to [$dst]\n" ;
	}
	else
	{
		copy( $src , $dst ) or die "error: install: failed to copy [$src] to [$dst]\n" ;
	}
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
	my $dash_v = "-v" ; # for now
	my $script = "test/emailrelay-test.pl" ;
	## $script = ( $script."_") if ! -f $script ;
	system( "perl -Itest \"$script\" $dash_v -d \"$main_bin_dir\" -x \"$test_bin_dir\" -c \"test/certificates\"" ) ;
}

