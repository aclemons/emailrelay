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
# Parses automake files throughout the emailrelay source tree to generate
# windows-specific cmake files, then uses cmake to generate mbedtls and
# emailrelay makefiles, and finally uses "cmake --build" to build the mbedtls
# libraries and emailrelay executables.
#
# usage: winbuild.pl [<subtask> [<subtask> ...]]
#
# Spits out batch files (like "winbuild-whatever.bat") for doing sub-tasks,
# including "winbuild-install.bat". See "winbuild.bat".
#
# The generated emailrelay cmake files specify static linkage of the run-time
# library ("/MT"), with the exception of the emailrelay GUI which is built with
# "/MD" iff Qt DLLs are found.
#
# Requires "cmake" to be on the path or somewhere obvious (see find_cmake() in
# "winbuild.pm").
#
# Looks for mbedtls source code in a sibling or child directory (see
# winbuild::find_mbedtls_src()) and builds it into the emailrelay build tree
# using cmake. The mbedtls configuration header is copied and edited as
# necessary.
#
# Looks for Qt libraries in various places (see winbuild::find_qt_x64()).
# For a fully static GUI build the Qt libraries will have to have been built
# from source, for example by "qtbuild.pl".
#
# The "install" sub-task, which is not run by default, assembles binaries
# and their dependencies in a directory tree ready for zipping and
# distribution. Extra dependencies and translations are assembled by the
# Qt deployment tool, "windeployqt", although this only works for a
# non-static GUI build.
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
use File::Glob ;
use lib dirname($0) , dirname($0)."/libexec" ;
use BuildInfo ;
require "make2cmake" ;
require "winbuild.pm" ;
require "mbedtlsbuild.pl" ;

# configuration
my @cfg_run_parts = @ARGV ;
my %cfg_options = (
	verbose => 1 ,
	debug => 0 ,
	with_mbedtls => 1 ,
	with_openssl => 0 ,
	with_gui => 1 ,
	x64 => 1 ,
	x86 => 0 ,
	cmake => undef ,
	mbedtls_src => undef ,
	mbedtls_tls13 => undef , # still 'experimental'
	openssl_x64 => undef ,
	openssl_x86 => undef ,
	qt_x64 => undef ,
	qt_x86 => undef ,
	qt_static => undef ,
) ;
{
	my $fh = new FileHandle( "winbuild.cfg" , "r" ) ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		$line =~ s/#.*// ;
		$line =~ s/\s*$// ;
		my ( $key , $value ) = ( $line =~ m/(\S+)[\s=]+(.*)/ ) ;
		$key =~ s/-/_/g ;
		$value =~ s/\s*$// ;
		$value =~ s/^"// ;
		$value =~ s/"$// ;
		$cfg_options{$key} = $value ;
	}
}
my $cfg_with_gui = $cfg_options{with_gui} ;
my $cfg_with_mbedtls = $cfg_options{with_mbedtls} ;
my $cfg_with_openssl = $cfg_options{with_openssl} ;
my $cfg_path_openssl_x64 = $cfg_options{openssl_x64} ;
my $cfg_path_openssl_x86 = $cfg_options{openssl_x86} ;
my $cfg_verbose = $cfg_options{verbose} ;
my $cfg_opt_debug = $cfg_options{debug} ;
my $cfg_opt_x86 = $cfg_options{x86} ;
my $cfg_opt_x64 = $cfg_options{x64} ;
my $cfg_path_cmake = $cfg_options{cmake} ;
my $cfg_path_mbedtls_src = $cfg_options{mbedtls_src} ;
my $cfg_mbedtls_tls13 = $cfg_options{mbedtls_tls13} ;
my $cfg_path_qt_x64 = $cfg_options{qt_x64} ;
my $cfg_path_qt_x86 = $cfg_options{qt_x86} ;
my $cfg_qt_static_override = $cfg_options{qt_static} ;
die unless ($cfg_opt_x64 || $cfg_opt_x86) ;
if( scalar(@cfg_run_parts) == 0 )
{
	@cfg_run_parts = $^O eq "linux" ? qw( install_winxp ) : ( $cfg_with_mbedtls ?
		qw( batchfiles generate mbedtls cmake build ) :
		qw( batchfiles generate cmake build ) ) ;
}

# find stuff ...

if( !$cfg_path_cmake ) { $cfg_path_cmake = winbuild::find_cmake() }
if( !$cfg_path_mbedtls_src && $cfg_with_mbedtls ) { $cfg_path_mbedtls_src = winbuild::find_mbedtls_src() }
if( !$cfg_path_qt_x64 && $cfg_with_gui && $cfg_opt_x64 ) { $cfg_path_qt_x64 = winbuild::find_qt_x64() }
if( !$cfg_path_qt_x86 && $cfg_with_gui && $cfg_opt_x86 ) { $cfg_path_qt_x86 = winbuild::find_qt_x86() }
if( ! -e "winbuild.cfg" )
{
	my $fh = new FileHandle( "winbuild.cfg" , "w" ) ;
	if( $fh )
	{
		my $mbedtls_src = -d $cfg_path_mbedtls_src ? $cfg_path_mbedtls_src : "c:/mbedtls-2.28.0" ;
		my $openssl_x64 = -d $cfg_path_openssl_x64 ? $cfg_path_openssl_x64 : "c:/libressl-3.8.2/build-x64" ;
		my $openssl_x86 = -d $cfg_path_openssl_x64 ? $cfg_path_openssl_x64 : "c:/libressl-3.8.2" ;
		my $qt_x64 = -d $cfg_path_qt_x64 ? $cfg_path_qt_x64 : "c:/qt/5.15.2/msvc2019_64" ;
		my $qt_x86 = -d $cfg_path_qt_x86 ? $cfg_path_qt_x86 : "c:/qt/5.15.2/msvc2019" ;

		print $fh "# winbuild.cfg -- created by winbuild.pl -- edit as required\n" ;
		print $fh "\n" ;
		print $fh "#with_mbedtls 0\n" ;
		print $fh "#mbedtls_src $mbedtls_src\n" ;
		print $fh "\n" ;
		print $fh "#with_openssl 0\n" ;
		print $fh "#openssl_x64 $openssl_x64\n" ;
		print $fh "\n" ;
		print $fh "#with_gui 0\n" ;
		print $fh "#qt_static 0\n" ;
		print $fh "#qt_x64 $qt_x64\n" ;
		print $fh "\n" ;
		print $fh "#x86 0\n" ;
		print $fh "#qt_x86 $qt_x86\n" ;
		print $fh "#openssl_x86 $openssl_x86\n" ;
	}
}
my $missing_cmake = ( !$cfg_path_cmake || !-e $cfg_path_cmake ) ;
my $missing_qt =
	( $cfg_with_gui && $cfg_opt_x86 && ( !$cfg_path_qt_x86 || !-d "$cfg_path_qt_x86/lib" ) ) ||
	( $cfg_with_gui && $cfg_opt_x64 && ( !$cfg_path_qt_x64 || !-d "$cfg_path_qt_x64/lib" ) ) ;
my $missing_mbedtls = ( $cfg_with_mbedtls && ( !$cfg_path_mbedtls_src || !-d "$cfg_path_mbedtls_src/include" ) ) ;
warn "error: cannot find cmake.exe: please download from cmake.org\n" if $missing_cmake ;
warn "error: cannot find qt libraries: please download from wwww.qt.io or set qt_x64 and/or qt_x86 or with_gui=0 in winbuild.cfg\n" if $missing_qt ;
warn "error: cannot find mbedtls source: please download from tls.mbed.org or set mbedtls or with_mbedtls=0 in winbuild.cfg\n" if $missing_mbedtls ;
if( $missing_cmake || $missing_qt || $missing_mbedtls )
{
	warn "error: missing prerequisites: please install the missing components " ,
		"or edit the winbuild.cfg configuration file" , "\n" ;
	die "winbuild: error: missing prerequisites\n" ;
}
if( $cfg_with_mbedtls )
{
	$cfg_path_mbedtls_src = Cwd::realpath( $cfg_path_mbedtls_src ) ;
}

# qt info
if( $cfg_with_gui && $cfg_opt_x86 && $cfg_opt_x64 &&
	( BuildInfo::qt_version($cfg_path_qt_x86) != BuildInfo::qt_version($cfg_path_qt_x64) ) )
{
	die "winbuild: error: qt version different between x86 and x64\n" ;
}
if( $cfg_with_gui && $cfg_opt_x86 && $cfg_opt_x64 &&
	( BuildInfo::qt_is_static($cfg_path_qt_x86) != BuildInfo::qt_is_static($cfg_path_qt_x64) ) )
{
	die "winbuild: error: qt staticness different between x86 and x64\n" ;
}
my $qt_info = {
	v => BuildInfo::qt_version( $cfg_path_qt_x86 ? $cfg_path_qt_x86 : $cfg_path_qt_x64 ) ,
	static => BuildInfo::qt_is_static( $cfg_path_qt_x86 ? $cfg_path_qt_x86 : $cfg_path_qt_x64 ) ,
	x86 => $cfg_path_qt_x86 ,
	x64 => $cfg_path_qt_x64 ,
} ;
$qt_info->{static} = $cfg_qt_static_override if defined($cfg_qt_static_override) ;

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
		] ,
	} ;

# project version
chomp( my $version = eval { FileHandle->new("VERSION")->gets() } || "2.5.2" ) ;
my $project = "emailrelay" ;
my $install_x64 = "$project-$version-w64" ;
my $install_x86 = "$project-$version-w32" ;
my $install_winxp = "$project-$version-winxp" ;

# run stuff ...

for my $part ( @cfg_run_parts )
{
	if( $part eq "batchfiles" )
	{
		winbuild::spit_out_batch_files( qw(
			generate cmake build
			debug-build debug-test test
			mbedtls clean vclean install ) ) ;
	}
	elsif( $part eq "generate" )
	{
		run_generate( $project , $qt_info ) ;
	}
	elsif( $part eq "mbedtls" )
	{
		run_mbedtls_cmake( "x64" ) if $cfg_opt_x64 ;
		run_mbedtls_cmake( "x86" ) if $cfg_opt_x86 ;
		run_mbedtls_build( "x64" , "Debug" ) if ( $cfg_opt_x64 && $cfg_opt_debug ) ;
		run_mbedtls_build( "x64" , "Release" ) if $cfg_opt_x64 ;
		run_mbedtls_build( "x86" , "Debug" ) if ( $cfg_opt_x86 && $cfg_opt_debug ) ;
		run_mbedtls_build( "x86" , "Release" ) if $cfg_opt_x86 ;
	}
	elsif( $part eq "cmake" )
	{
		run_cmake( "x64" ) if $cfg_opt_x64 ;
		run_cmake( "x86" ) if $cfg_opt_x86 ;
	}
	elsif( $part eq "build" )
	{
		run_build( "x64" , "Release" ) if $cfg_opt_x64 ;
		run_build( "x64" , "Debug" ) if ( $cfg_opt_x64 && $cfg_opt_debug ) ;
		run_build( "x86" , "Release" ) if $cfg_opt_x86 ;
		run_build( "x86" , "Debug" ) if ( $cfg_opt_x86 && $cfg_opt_debug ) ;
	}
	elsif( $part eq "debug-build" )
	{
		run_mbedtls_cmake( "x64" ) if $cfg_with_mbedtls ;
		run_mbedtls_build( "x64" , "Debug" ) if $cfg_with_mbedtls ;
		run_build( "x64" , "Debug" ) ;
	}
	elsif( $part eq "clean" )
	{
		clean_test_files() ;
		run_build( "x64" , "Debug" , "clean" ) if ( $cfg_opt_x64 && $cfg_opt_debug ) ;
		run_build( "x64" , "Release" , "clean" ) if $cfg_opt_x64 ;
		run_build( "x86" , "Debug" , "clean" ) if ( $cfg_opt_x86 && $cfg_opt_debug ) ;
		run_build( "x86" , "Release" , "clean" ) if $cfg_opt_x86 ;
	}
	elsif( $part eq "vclean" )
	{
		clean_test_files() ;
		winbuild::deltree( "x64" ) ;
		winbuild::deltree( "x86" ) ;
		winbuild::deltree( mbedtls_build_dir("x64") ) ;
		winbuild::deltree( mbedtls_build_dir("x86") ) ;
		winbuild::clean_cmake_files() ;
		winbuild::deltree( $install_x64 ) if $cfg_opt_x64 ;
		winbuild::deltree( $install_x86 ) if $cfg_opt_x86 ;
	}
	elsif( $part eq "install" )
	{
		my $add_gui_runtime = $cfg_with_gui && !$qt_info->{static} ; # windeployqt

		install( $install_x64 , "x64" , $qt_info , $cfg_with_gui , $cfg_with_mbedtls ,
			$add_gui_runtime ) if $cfg_opt_x64 ;

		install( $install_x86 , "x86" , $qt_info , $cfg_with_gui , $cfg_with_mbedtls ,
			$add_gui_runtime ) if $cfg_opt_x86 ;
	}
	elsif( $part eq "install_winxp" )
	{
		run_install_winxp() ;
	}
	elsif( $part eq "debug-test" )
	{
		my $test_arch = ( $cfg_opt_x86 && !$cfg_opt_x64 ) ? "x86" : "x64" ;
		run_tests( "$test_arch/src/main/Debug" , "$test_arch/test/Debug" ) ;
	}
	elsif( $part eq "test" )
	{
		my $test_arch = ( $cfg_opt_x86 && !$cfg_opt_x64 ) ? "x86" : "x64" ;
		run_tests( "$test_arch/src/main/Release" , "$test_arch/test/Release" ) ;
	}
	else
	{
		die "winbuild: usage error\n" ;
	}
}

# signal success to the batch file if we have not died
winbuild::create_touchfile( winbuild::default_touchfile($0) ) ;

# show a helpful message
if( (grep {$_ eq "build"} @cfg_run_parts) && !(grep {$_ eq "install"} @cfg_run_parts) )
{
	print "winbuild: finished -- try winbuild-install.bat for packaging\n"
}

# ==

sub create_cmake_files
{
	my ( $project , $qt_info ) = @_ ;

	my @makefiles = BuildInfo::read_makefiles( "." , "winbuild: reading: " ,
		{
			windows_mbedtls => $cfg_with_mbedtls ,
			windows_openssl => $cfg_with_openssl ,
			windows_gui => $cfg_with_gui ,
			qt_version => $qt_info->{v} ,
		}
	) ;

	$make2cmake::cfg_static_gui = $qt_info->{static} ;
	for my $m ( @makefiles )
	{
		make2cmake::create_cmake_file( $m ) ;
	}
}

sub run_generate
{
	my ( $project , $qt_info ) = @_ ;
	create_cmake_files( $project , $qt_info ) ;
}

sub run_cmake
{
	my ( $arch ) = @_ ;
	$arch ||= "x64" ;

	my @arch_args = @{$cmake_args->{$arch}} ;
	my $rc ;
	my $i = 0 ;
	for my $arch_args ( @arch_args )
	{
		my @args = @$arch_args ;
		if( $cfg_with_mbedtls )
		{
			my $mbedtls_build_dir = mbedtls_build_dir( $arch ) ;
			unshift @args , "-DMBEDTLS_INC=$mbedtls_build_dir/include" ;
			unshift @args , "-DMBEDTLS_RLIB=$mbedtls_build_dir/library/release" ;
			unshift @args , "-DMBEDTLS_DLIB=$mbedtls_build_dir/library/debug" ;
		}
		if( $cfg_with_openssl )
		{
			my $openssl_dir = $arch eq "x64" ? $cfg_path_openssl_x64 : $cfg_path_openssl_x86 ;
			unshift @args , "-DOPENSSL_INC=$openssl_dir/include" ;
			unshift @args , "-DOPENSSL_RLIB=$openssl_dir/library/release" ;
			unshift @args , "-DOPENSSL_DLIB=$openssl_dir/library/debug" ;
		}
		if( $cfg_with_gui )
		{
			my $qt_dir = defined($qt_info) ? Cwd::realpath( $qt_info->{$arch} ) : "." ;
			unshift @args , "-DQT_DIR=$qt_dir" ;
			unshift @args , "-DQT_MOC_DIR=$qt_dir\\bin\\" ;
		}
		push @args , ( "-S" , "." ) ;
		push @args , ( "-B" , $arch ) ;

		print "winbuild: cmake($arch): running: [",join("][",$cfg_path_cmake,@args),"]\n" ;
		$rc = system( $cfg_path_cmake , @args ) ;

		if( $rc != 0 )
		{
			print "winbuild: cmake($arch): cmake cleanup: [$arch]\n" ;
			unlink "$arch/CMakeCache.txt" ;
			File::Path::remove_tree( "$arch/CMakeFiles" , {safe=>1,verbose=>0} ) ;
		}

		last if $rc == 0 ;

		$i++ ;
		my $final = ( $i == scalar(@arch_args) ) ;
		print "winbuild: cmake($arch): cannot use that cmake generator: trying another\n" if !$final ;
	}
	print "winbuild: cmake-exit=[$rc]\n" ;
	die "winbuild: error: cmake failed: check error messages above and maybe tweak cmake_args in winbuild.pl\n" unless $rc == 0 ;
}

sub run_build
{
	my ( $arch , $confname , $target ) = @_ ;

	my @args = (
		"--build" , $arch ,
		"--config" , $confname ) ;
	push @args , ( "--target" , $target ) if $target ;
	push @args , "-v" if $cfg_verbose ;

	print "winbuild: build($arch,$confname): running: [",join("][",$cfg_path_cmake,@args),"]\n" ;
	my $rc = system( $cfg_path_cmake , @args ) ;
	print "winbuild: build($arch,$confname): exit=[$rc]\n" ;
	die unless $rc == 0 ;
}

sub mbedtls_build_dir
{
	my ( $arch ) = @_ ;
	return Cwd::realpath(dirname($0)) . "/mbedtls-$arch" ;
}

sub run_mbedtls_cmake
{
	my ( $arch ) = @_ ;

	my $src_dir = $cfg_path_mbedtls_src ;
	my $build_dir = mbedtls_build_dir( $arch ) ;
	mkdir_( $build_dir ) ;

	# no-op if we already have the .sln file
	if( -f "$build_dir/mbed tls.sln" && -f "$build_dir/CMakeCache.txt" )
	{
		print "winbuild: mbedtls-cmake($arch): already got [$build_dir/mbed tls.sln]: not running cmake\n" ;
		return ;
	}

	# copy headers and edit the configuration header file
	MbedtlsBuild::copy_headers( $src_dir , $build_dir ) ;
	my $config_file = MbedtlsBuild::config_file( $build_dir ) ;
	MbedtlsBuild::configure( $config_file , $cfg_mbedtls_tls13 ) ;

	# test for the static-runtime option
	my $have_static_runtime_option ; # not in 2.28.x
	{
		my $fh = new FileHandle( "$src_dir/library/CMakeLists.txt" ) or die ;
		my $x = eval { local $/ ; <$fh> } ;
		$have_static_runtime_option = ( $x =~ m/MSVC_STATIC_RUNTIME/ ) ;
	}

	# try each cmake generator in turn until one works
	my $rc ;
	for my $arch_args ( @{$cmake_args->{$arch}} )
	{
		my @cmake_args = () ;
		push @cmake_args , @$arch_args ;
		push @cmake_args , ( "-B" , $build_dir ) ;
		push @cmake_args , ( "-S" , $src_dir ) ;
		push @cmake_args , $have_static_runtime_option ? ( "-DMSVC_STATIC_RUNTIME=On" ) :
			( "-DCMAKE_C_FLAGS_DEBUG=-MTd -Ob0 -Od -RTC1" , # must use dashes here
			"-DCMAKE_C_FLAGS_RELEASE=-MT -O2 -Ob1 -DNDEBUG" ) ;
		push @cmake_args , "-DENABLE_TESTING=Off" ;
		push @cmake_args , "-DENABLE_PROGRAMS=Off" ;
		push @cmake_args , "-DMBEDTLS_FATAL_WARNINGS=Off" ; # for eg. v3.5.1 with TLS1.3 enabled
		push @cmake_args , "-DMBEDTLS_CONFIG_FILE=$config_file" if $config_file ;

		print "winbuild: mbedtls-cmake($arch): running: [",join("][",$cfg_path_cmake,@cmake_args),"]\n" ;
		$rc = system( $cfg_path_cmake , @cmake_args ) ;
		print "winbuild: mbedtls-cmake($arch): exit=[$rc]\n" ;

		if( $rc != 0 )
		{
			print "winbuild: mbedtls-cmake($arch): cmake cleanup: [$build_dir]\n" ;
			unlink "$build_dir/CMakeCache.txt" ;
			File::Path::remove_tree( "$build_dir/CMakeFiles" , {safe=>1,verbose=>0} ) ;
		}

		last if( $rc == 0 ) ;
	}
	die unless $rc == 0 ;
}

sub run_mbedtls_build
{
	my ( $arch , $confname ) = @_ ;

	my @args = (
		"--build" , mbedtls_build_dir($arch) ,
		"--target" , "mbedtls" ,
		"--target" , "mbedcrypto" ,
		"--target" , "mbedx509" ,
		"--config" , $confname ,
	) ;

	print "winbuild: mbedtls-build($arch,$confname): running: [",join("][",$cfg_path_cmake,@args),"]\n" ;
	my $rc = system( $cfg_path_cmake , @args ) ;
	print "winbuild: mbedtls-build($arch,$confname): exit=[$rc]\n" ;
	die unless $rc == 0 ;
}

sub install
{
	my ( $install , $arch , $qt_info , $with_gui , $with_mbedtls , $add_gui_runtime ) = @_ ;

	my $msvc_base = winbuild::msvc_base( $arch ) ;
	$msvc_base or die "winbuild: error: install: cannot determine the msvc base directory\n" ;
	print "winbuild: msvc-base=[$msvc_base]\n" ;

	# copy the core files -- the main programs are always statically linked so they
	# can go into a "programs" sub-directory -- the gui/setup executable may be
	# dynamically linked in which case it should have run-time dlls copied
	# alongside
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

		# optionally use windeployqt to install compiler compiler runtime installer,
		# Qt library DLLs, Qt plugin DLLs, and Qt qtbase translations -- this
		# is only possible if the GUI is dynamically linked -- if statically linked
		# then the plugins are automatically linked in at build-time and windeployqt
		# fails -- unfortunately that means that qtbase translations then need to
		# be copied by hand
		#
		if( $add_gui_runtime ) # ie. windeployqt -- only works if dynamically linked
		{
			install_gui_dependencies( $msvc_base , $arch , $qt_info ,
				"$install/emailrelay-setup.exe" ,
				"$install/payload/files/gui/emailrelay-gui.exe" ) ;

			my $runtime = winbuild::find_runtime( $msvc_base , $arch , "vcruntime140.dll" , "msvcp140.dll" ) ;
			install_runtime( $runtime , $arch , $install ) ;
			install_runtime( $runtime , $arch , "$install/payload/files/gui" ) if $with_gui ;
		}

		map { File::Copy::copy( $_ , "$install/translations/" ) } File::Glob::bsd_glob("src/gui/*.qm") ;
		map { File::Copy::copy( $_ , "$install/payload/files/gui/translations/" ) } File::Glob::bsd_glob("src/gui/*.qm") ;
	}

	print "winbuild: $arch distribution in [$install]\n" ;
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
	print "winbuild: winxp mingw distribution in [$install_winxp]\n" ;
}

sub install_gui_dependencies
{
	my ( $msvc_base , $arch , $qt_info , @exes ) = @_ ;

	$ENV{VCINSTALLDIR} = $msvc_base ; # used by windeployqt to copy runtime files
	my $qt_bin = "$$qt_info{$arch}/bin" ;
	if( ! -d $qt_bin ) { die "winbuild: error: install: no qt bin directory for $arch: [$qt_bin]\n" }
	my $deploy_tool = "$qt_bin/windeployqt.exe" ;
	if( ! -x $deploy_tool ) { die "winbuild: error: install: no windeployqt executable\n" }

	for my $exe ( @exes )
	{
		my $rc = system( $deploy_tool , $exe ) ;
		$rc == 0 or die "winbuild: error: install: failed running [$deploy_tool] [$exe]" ;
	}
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

sub install_payload_cfg
{
	my ( $file ) = @_ ;
	File::Path::make_path( File::Basename::dirname($file) ) ;
	my $fh = new FileHandle( $file , "w" ) or die "winbuild: error: install: cannot create [$file]\n" ;
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
		map { install_copy( $_ , "$root/$dst" ) } File::Glob::bsd_glob( $src ) ;
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

sub mkdir_
{
	my ( $dir ) = @_ ;
	return if -d $dir ;
	return mkdir( $dir ) ;
}

sub run_tests
{
	my ( $main_bin_dir , $test_bin_dir ) = @_ ;
	my $dash_v = "" ; # or "-v"
	my $script = "test/emailrelay_test.pl" ;
	system( "perl -Itest \"$script\" $dash_v -d \"$main_bin_dir\" -x \"$test_bin_dir\" -c \"test/certificates\"" ) ;
}

sub clean_test_files
{
	my @file_list = () ;
	my @dir_list = () ;
	File::Find::find( sub { push @file_list , $File::Find::name if( -f $_ && $_ =~ m/^e\./ ) } , "." ) ;
	File::Find::find( sub { push @dir_list , $File::Find::name if $_ =~ m/^e\..*\.spool$/ } , "." ) ;
	unlink( @file_list ) ;
	map { winbuild::deltree($_) } @dir_list ;
}

