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
# qtbuild.pl
#
# Builds Qt static libraries, typically Qt version 5 on windows.
#
# (The pre-built Qt windows libraries available to download are import
# libraries with Qt DLLs and they dynamically link to the c/c++ run-time.
# To build a GUI application that can be distributed without any DLLs
# a static build of Qt libraries is needed. Older versions of Qt were
# awkward to build statically because they did not have the
# "-static-runtime" option on the configure script, so they needed
# an edit of the "mkspecs\common\msvc-desktop.conf" file to add "/MT".)
#
# Runs the Qt 'configure' script with options chosen to limit the size of
# the build, and then runs 'make' or 'nmake'.
#
# usage: qtbuild.pl [options] [<src-dir> [<build-dir>] [<install-dir>]]]
#          --qt6                       assume qt6
#          --arch={x64|x86}            x64 or x86 (windows)
#          --config={debug|release}    debug or release
#          --dynamic                   dynamic linking and associated tools
#
# On Windows use from a 'vcvars' "developer command prompt".
#
# Download qt5 source with:
#    $ git clone https://code.qt.io/qt/qt5.git qt5
#    $ git -C qt5 checkout 5.15
#    $ cd qt5 && perl init-repository --module-subset=qtbase,qttools,qttranslations
#
# Only the "qtbase" module is required to build the libraries, but
# "qttools" and "qttranslations" are needed for building the "windeployqt"
# tool, which will be needed after a non-static ("--dynamic") build on
# windows. It is best to initialise all three when downloading and then
# use "-skip" options on the "configure" command-line as appropriate.
#
# If this script is run without options then a "qt5" sub-directory is
# expected, an empty sub-directory is created for the build and the
# install goes into the qt-<arch> sub-directory:
#
#  source   - qt5/
#  build    - qt-build-<arch>-<config>/
#  install  - qt-<arch>/
#
# Delete the build directory tree for a clean build ("rmdir /q /s" on
# windows).
#

use strict ;
use Cwd ;
use FileHandle ;
use File::Find ;
use File::Basename ;
use File::Copy ;
use File::Glob ;
use Getopt::Long ;
use lib( File::Basename::dirname($0) ) ;

# parse the command-line
my $cfg_prefix = File::Basename::basename($0) ;
my %opt = () ;
GetOptions( \%opt , "qt6" , "install|i=s" , "arch=s" , "config=s" , "verbose|v" , "dry-run|n" , "quiet|q" , "dynamic" ) or die "$cfg_prefix: usage error" ;
my $cfg_qt6 = $opt{qt6} ;
my $cfg_static = !$opt{dynamic} ;
my $cfg_config = $opt{config} || "release" ;
my $cfg_build_more = $opt{dynamic} ; # build more stuff so that windeployqt works
my $cfg_arch = $opt{arch} || $ENV{Platform} || "x64" ;
my $cfg_source_dir_ = $ARGV[0] || _find($cfg_qt6?"qt6":"qt5") ;
my $cfg_build_dir = $ARGV[1] || "qt-build-${cfg_arch}-${cfg_config}" ;
my $cfg_install_dir = $ARGV[2] || "qt-${cfg_arch}" ;
my $cfg_source_dir = Cwd::realpath( $cfg_source_dir_ ) ;
my $cfg_verbose = $opt{verbose} ; # (verbose make)
my $cfg_quiet = $opt{quiet} ; # (this script)
my $cfg_dry_run = $opt{'dry-run'} ;
die "$cfg_prefix: usage error" if( $cfg_config ne "debug" && $cfg_config ne "release" ) ;

# sanity checks
die "$cfg_prefix: error: no source directory [$cfg_source_dir_]\n" if ! -d $cfg_source_dir ;
if( _windows() && !$ENV{Platform} )
{
	die "$cfg_prefix: error: not running in a developer command prompt" ;
}
my %check = (
	5 => [
		"$cfg_source_dir/qtbase/src/corelib/kernel/qobject.h" ,
		"$cfg_source_dir/qtbase/.qmake.conf" ,
		"$cfg_source_dir/qtbase/qmake/Makefile.unix.unix" , # sic
	] ,
	6 => [
		"$cfg_source_dir/qtbase/src/corelib/kernel/qobject.h" ,
	] ,
) ;
for my $check ( @{$check{$cfg_qt6?6:5}} )
{
	die "$cfg_prefix: error: missing source file [$check]\n" if ! -e $check ;
}

# start with a clean build directory
if( -d $cfg_build_dir && scalar(glob("$cfg_build_dir/*")) )
{
	die "$cfg_prefix: error: build directory is not empty [$cfg_build_dir]\n" unless $cfg_dry_run ;
}
if( !-d $cfg_build_dir )
{
	_mkdir( $cfg_build_dir ) or die "$cfg_prefix: error: cannot create build directory [$cfg_build_dir]\n" ;
}
$cfg_build_dir = Cwd::realpath( $cfg_build_dir ) ;
if( !-d $cfg_install_dir )
{
	_mkdir( $cfg_install_dir ) or die "$cfg_prefix: error: cannot create install directory [$cfg_install_dir]\n" ;
}
$cfg_install_dir = Cwd::realpath( $cfg_install_dir ) ;

print "$cfg_prefix: source: $cfg_source_dir\n" unless $cfg_quiet ;
print "$cfg_prefix: build: $cfg_build_dir\n" unless $cfg_quiet ;
print "$cfg_prefix: install: $cfg_install_dir\n" unless $cfg_quiet ;

# prepare the 'configure' options -- see qtbase/config_help.txt
my @configure_args = grep {m/./} (
		"-opensource" , "-confirm-license" ,
		"-prefix" , $cfg_install_dir ,
		"-${cfg_config}" ,
		( $cfg_static ? "-static" : "" ) ,
		( $cfg_static && _windows() ? "-static-runtime" : "" ) ,
		"-platform" , ( _windows() ? "win32-msvc" : "linux-g++" ) ,
		"-nomake" , "examples" ,
		"-nomake" , "tests" ) ;

if( $cfg_build_more )
{
	push @configure_args , grep {m/./} (
		"-opengl" ,
		"-no-openssl" ,
		"-no-dbus" ,
		"-no-gif" ,
		"-no-libpng" ,
		"-no-libjpeg" ,
		"-make" , "tools" ,
		"-make" , "libs" ,
	) ;
}
else
{
	push @configure_args , grep {m/./} (
		"-no-opengl" ,
		"-no-openssl" ,
		"-no-dbus" ,
		"-no-gif" ,
		"-no-libpng" ,
		"-no-libjpeg" ,
		"-nomake" , "tools" ,
		"-make" , "libs" ,
		"-skip" , "translations" ,
		"-skip" , "tools"
	) ;
}
if( $cfg_qt6 && _unix() )
{
	push @configure_args , (
		"-xcb" ,
		"-feature-thread" ,
		"-feature-xcb" ,
		"-feature-xkbcommon-x11" ,
	) ;
}
if( $cfg_verbose )
{
	unshift @configure_args , "-verbose" ;
}
if( $cfg_config ne "debug" )
{
	push @configure_args , (
		"-no-pch" ,
		"-optimize-size"
	) ;
}

if( $cfg_dry_run )
{
	my $sep = ( _windows() ? "\\\\" : "/" ) ;
	my $bat = ( _windows() ? ".bat" : "" ) ;
	print "cd ${cfg_build_dir} \&\& ${cfg_source_dir}${sep}configure$bat " , join(" ",@configure_args) , "\n" ;
	exit ;
}

# fix-ups
#
map { touch("$cfg_source_dir/$_/.git") } ( "qtbase" , "qttools" , "qttranslations" ) ; # see "-e" test in "qtbase/configure"
$ENV{MAKE} = "make" if( _unix() ) ; # we dont want gmake -- see "qtbase/configure"

# run 'configure'
#
if( _unix() )
{
	run( $cfg_build_dir , "configure($cfg_arch)" , "$cfg_source_dir/configure" , @configure_args ) ;
}
else
{
	$ENV{PATH} = File::Basename::dirname($^X) .";$ENV{PATH}" ; # perl on the path
	run( $cfg_build_dir , "configure($cfg_arch)" , "$cfg_source_dir\\\\configure.bat" , @configure_args ) ;
}

# run 'make'
#
run( $cfg_build_dir , "make($cfg_arch)" , make_command() ) ;

# run 'make install'
#
run( $cfg_build_dir , "make-install($cfg_arch)" , make_install_command() ) ;

## ==

sub make_command
{
	if( $cfg_qt6 )
	{
		return "cmake --build . --parallel" ;
	}
	elsif( _unix() )
	{
		return "make -j 10" ;
	}
	else
	{
		return "nmake" ;
	}
}

sub make_install_command
{
	if( $cfg_qt6 )
	{
		return "cmake --install ." ;
	}
	elsif( _unix() )
	{
		return "make install" ;
	}
	else
	{
		return "nmake install" ;
	}
}

sub run
{
	my ( $cd , $run_prefix , @cmd ) = @_ ;

	my $old_dir ;
	if( $cd )
	{
		$old_dir = Cwd::getcwd() ;
		chdir( $cd ) or die "$cfg_prefix: error: cannot cd to [$cd]" ;
	}
	print "$run_prefix: running: cmd=[".join(" ",@cmd)."] cwd=[".Cwd::getcwd()."]\n" ;
	my $rc = system( @cmd ) ;
	print "$run_prefix: rc=[$rc]\n" ;
	if( $old_dir )
	{
		chdir( $old_dir ) or die "$cfg_prefix: error: cannot cd back to [$old_dir]" ;
	}
	die "$cfg_prefix: error: command failed\n" if $rc != 0 ;
}

sub touch
{
	my ( $path ) = @_ ;
	return 1 if -e $path ;
	my $fh = new FileHandle( $path , "w" ) ;
	( $fh && $fh->close() ) or die "$cfg_prefix: error: cannot touch [$path]" ;
}

sub _find
{
	my ( $name ) = @_ ;
	my $base = File::Basename::dirname( $0 ) ;
	for my $path ( "$base/$name" , "$base/../$name" , "$base/../../$name" )
	{
		return $path if -d $path ;
	}
	return $name ;
}

sub _mkdir
{
	return 1 if( $cfg_dry_run ) ;
	return mkdir( $_[0] ) ;
}

sub _unix
{
	return $^O eq "linux" ; # could do better
}

sub _windows
{
	return !_unix() ;
}

