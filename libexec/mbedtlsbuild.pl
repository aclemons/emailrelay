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
# mbedtlsbuild.pl
#
# Builds mbedtls using just compile and link commands. It does not use
# cmake, which means it can be used from a raw windows "WinDev" build
# server.
#
# usage: mbedtlsbuild.pl [--arch <arch>] [--config <config>] [--tls13] [<src-dir> [<build-dir>]]
#          --arch      x64 or x86
#          --config    release or debug
#          --tls13     enable TLS 1.3 ('experimental')
#          <src-dir>   defaults to mbedtls in dirname($0) or its parent or grandparent
#          <build-dir> defaults to mbedtls-<arch>
#
# Eg:
#    mbedtlssbuild.pl --config=release mbedtls-9.9 mbedtls-x64
#
# The the list of source files is taken straight from the mbedtls makefile.
#
# All mbedtls include files are copied into the build tree so that the
# mbedtls configuration header (mbedtls_config.h) can be edited to
# enable TLS 1.3 and so that client code can build with reference to
# a single base directory.
#
# Libraries end up in in <build-dir>/library/<config> and headers under
# <build-dir>/include/{mbedtls,psa}.
#
# The net result is similar to doing this:
#    mkdir mbedtls-x64
#    cd mbedtls-x64
#    mkdir include
#    xcopy /y /e ..\include\* include\
#    edit include/mbedtls/mbedtls_config.h [v3.x]
#    cmake -G "NMake Makefiles"
#       -DCMAKE_BUILD_TYPE=Release
#       -DMSVC_STATIC_RUNTIME=On
#       -DMBEDTLS_CONFIG_FILE=`pwd`/include/mbedtls/mbedtls_config.h
#       -S .. -B .
#    nmake lib
#
# On Windows run from a 'vcvars' "developer command prompt".
#
# Download mbedtls source with:
#    $ git clone https://github.com/Mbed-TLS/mbedtls.git mbedtls
#    $ git -C mbedtls checkout -q "mbedtls-2.28"
#
# Synopsis:
#   require "mbedtlsbuild.pl" ;
#   MbedtlsBuild::copy_headers( $src_dir , $build_dir ) ;
#   my $config_file = MbedtlsBuild::config_file( $build_dir ) ;
#   MbedtlsBuild::configure( $config_file , $tls13 ) ;
#   system( "cmake -B $build_dir -S $src_dir ..." ) ;
#

use strict ;
use FileHandle ;
use Getopt::Long ;
use File::Basename ;
use File::Copy ;
use Cwd ;

package MbedtlsBuild ;

sub new
{
	my ( $classname , $src_dir , $build_dir , $arch , $config , $tls13 , $prefix , $quiet , $os , $cflags_extra ) = @_ ;

	$arch ||= "x64" ;
	$config ||= "release" ;
	$prefix ||= "mbedtlsbuild" ;
	$os = ( $^O eq "linux" ? "unix" : "windows" ) if !defined($os) ;
	$cflags_extra ||= "" ;

	my $this = bless {
		m_prefix => $prefix ,
		m_quiet => $quiet ,
		m_arch => $arch ,
		m_config => $config ,
		m_tls13 => $tls13 ,
		m_src_dir => $src_dir ,
		m_build_dir => $build_dir ,
		m_os => $os ,
		m_dot_obj => ( $os eq "windows" ? ".obj" : ".o" ) ,
		m_dot_lib => ( $os eq "windows" ? ".lib" : ".a" ) ,
		m_lib_prefix => ( $os eq "windows" ? "" : "lib" ) ,
		m_cflags_extra => $cflags_extra ,
		m_compile => {
			windows => {
				debug => "cl /nologo " .
					"/I__BUILD_DIR__/include /I__BUILD_DIR__/library " .
					"/DWIN32 /D_WINDOWS /D_DEBUG /W3 /WX /MTd /Zi /Ob0 /Od /RTC1 /utf-8 " .
					"__CFLAGS_EXTRA__ " .
					"/Fd__BUILD_DIR__/library/__CONFIG__/__LIBNAME__.pdb " .
					"/Fo__OBJECT__ " .
					"/c __SOURCE__" ,
				release => "cl /nologo " .
					"/I__BUILD_DIR__/include /I__BUILD_DIR__/library " .
					"/DWIN32 /D_WINDOWS /DNDEBUG /W2 /MT /O2 /Ob2 /utf-8 " .
					"__CFLAGS_EXTRA__ " .
					"/Fd__BUILD_DIR__/library/__CONFIG__/__LIBNAME__.pdb " .
					"/Fo__OBJECT__ " .
					"/c __SOURCE__" ,
			} ,
			unix => {
				debug => "gcc -D_DEBUG " .
					"-I__BUILD_DIR__/include -I__BUILD_DIR__/library " .
					"-Wall -Wextra -Wshadow -std=c99 " .
					"__CFLAGS_EXTRA__ " .
					"-o __OBJECT__ -c __SOURCE__" ,
				release => "gcc -DNDEBUG " .
					"-I__BUILD_DIR__/include -I__BUILD_DIR__/library " .
					"-Wall -Wextra -Wshadow -std=c99 " .
					"__CFLAGS_EXTRA__ " .
					"-o __OBJECT__ -c __SOURCE__" ,
			} ,
		} ,
		m_link => {
			windows => "link /lib /nologo /OUT:__LIBPATH__ __OBJECTS__" ,
			unix => "ar -cr __LIBPATH__ __OBJECTS__" ,
		} ,
	} , $classname ;
	_init( $this ) ;
	return $this ;
}

sub _init
{
	my ( $this ) = @_ ;
	-f "$$this{m_src_dir}/library/aes.c" or die "$$this{m_prefix}: error: invalid mbedtls source directory [$$this{m_src_dir}]\n" ;
	map { -d $_ || mkdir $_ or die "mkdir($_)" } (
		File::Basename::dirname($this->{m_build_dir}) ,
		"$$this{m_build_dir}" ,
		"$$this{m_build_dir}/library" ,
		"$$this{m_build_dir}/library/$$this{m_config}" ,
		#"$$this{m_build_dir}/include" ,
		#"$$this{m_build_dir}/include/mbedtls" ,
		#"$$this{m_build_dir}/include/psa" ,
	) ;
}

sub find
{
	my ( $name ) = @_ ;
	my $base = File::Basename::dirname( $0 ) ;
	for my $path ( "$base/$name" , "$base/../$name" , "$base/../../$name" )
	{
		return $path if -d $path && -e "$path/include/mbedtls/ssl.h" && -e "$path/library/version.c" ;
	}
	return $name ;
}

sub _read_objects
{
	my ( $this , $key ) = @_ ;
	my $fh = new FileHandle( "$$this{m_src_dir}/library/Makefile" ) or die ;
	my $x ; { local $/ = undef ; $x = <$fh> ; }
	my ( $obj ) = ( $x =~ m/OBJS_${key}\s*=\s*([^#]*)/m ) ;
	return $obj
		=~ s/[\n\t\\]/ /rg
		=~ s/\s+/ /rg ;
}

sub _objects
{
	my ( $this , $lib ) = @_ ;
	return split( " " , $this->_read_objects(uc($lib)) ) ;
}

sub _sources
{
	my ( $this , $lib ) = @_ ;
	return map { $_ =~ s/\.o/.c/g ; $_ } split(" ",$this->_read_objects(uc($lib))) ;
}

sub copy_headers_
{
	my ( $this ) = @_ ;
	copy_headers( $this->{m_src_dir} , $this->{m_build_dir} , !$this->{m_quiet} ) ;
}

sub copy_headers
{
	# see also winbuild.pl
	my ( $src_dir , $build_dir , $verbose , @subdirs ) = @_ ;
	push @subdirs , ( "mbedtls" , "psa" ) if( scalar(@subdirs) == 0 ) ;
	map { _copy_headers_imp( "$src_dir/include" , "$build_dir/include" , $_ , $verbose ) } @subdirs ;
}

sub _copy_headers_imp
{
	my ( $src_inc_dir , $dst_inc_dir , $subdir , $verbose ) = @_ ;

	mkdir( $dst_inc_dir ) || die if ! -d $dst_inc_dir ;
	mkdir( "$dst_inc_dir/$subdir" ) || die if ! -d "$dst_inc_dir/$subdir" ;

	print "copy " , Cwd::realpath("$src_inc_dir/$subdir") , "/*.h " ,
		"$dst_inc_dir/$subdir\n" if $verbose ;

	for my $header ( glob("$src_inc_dir/$subdir/*.h") )
	{
		if( -f $header )
		{
			File::Copy::copy( $header , "$dst_inc_dir/$subdir/" ) or die ;
		}
	}
}

sub config_file
{
	my ( $base_dir ) = @_ ;
	my $path = "$base_dir/include/mbedtls/mbedtls_config.h" ; # mbedtls v3.x
	$path = undef if ! -e $path ;
	return $path ;
}

sub configure_
{
	my ( $this ) = @_ ;
	my $config_file = config_file( $this->{m_build_dir} ) ;
	configure( $config_file , $this->{m_tls13} ) ;
}

sub configure
{
	my ( $config_file , $tls13 ) = @_ ;

	my $config_file_in = $config_file ;
	my $config_file_out = $config_file ;
	( $config_file_in , $config_file_out ) = @$config_file if ref($config_file) ;

	return if !$config_file_in ;

	my $fh = new FileHandle( $config_file_in ) or die ;
	my $config_in = eval { local $/ ; <$fh> } ;
	$fh->close() or die ;

	my $message = "added by $0:" ;
	my $config_out = $config_in ;

	if( $tls13 )
	{
		$config_out =~ s;^//\s*(#define\s+MBEDTLS_SSL_PROTO_TLS1_3) *([\r]?)$;//$1$2\n// $message$2\n$1$2\n;m ;
		$config_out =~ s;^//\s*(#define\s+MBEDTLS_SSL_TLS1_3_COMPATIBILITY_MODE) *([\r]?)$;//$1$2\n// $message$2\n$1$2\n;m ;
	}

	if( $config_file_in eq $config_file_out && $config_in eq $config_out )
	{
		# no-op
	}
	else
	{
		$fh = new FileHandle( $config_file_out , "w" ) or die ;
		print $fh $config_out , "\n" ;
		$fh->close() or die ;
	}
}

sub _build_commands
{
	my ( $this ) = @_ ;

	my @commands = () ;
	for my $lib ( "crypto" , "x509" , "tls" )
	{
		my $libname = "mbed${lib}" ;
		my $libfile = $this->{m_lib_prefix} . $libname . $this->{m_dot_lib} ;
		my $libpath = "$$this{m_build_dir}/library/$$this{m_config}/$libfile" ;

		my @obj_paths = () ;
		for my $csrc ( $this->_sources( uc($lib) ) )
		{
			my $obj = $csrc =~ s/.c$/$$this{m_dot_obj}/r ;
			push @obj_paths , "$$this{m_build_dir}/library/$$this{m_config}/$obj" ;

			my $compile_cmd = $this->{m_compile}->{$$this{m_os}}->{$$this{m_config}} ;
			$compile_cmd =~ s;__SOURCE__;$$this{m_src_dir}/library/$csrc;g ;
			$compile_cmd =~ s;__OBJECT__;$$this{m_build_dir}/library/$$this{m_config}/$obj;g ;
			$compile_cmd =~ s/__SOURCE_DIR__/$$this{m_src_dir}/g ;
			$compile_cmd =~ s/__BUILD_DIR__/$$this{m_build_dir}/g ;
			$compile_cmd =~ s/__CONFIG__/$$this{m_config}/g ;
			$compile_cmd =~ s/__LIBNAME__/$libname/g ;
			$compile_cmd =~ s/__CFLAGS_EXTRA__/$$this{m_cflags_extra}/g ;
			push @commands , $compile_cmd ;
		}

		{
			my $link_cmd = $this->{m_link}->{$$this{m_os}} ;
			my $objects = join( " " , @obj_paths ) ;
			$link_cmd =~ s/__LIBNAME__/$libname/g ;
			$link_cmd =~ s/__LIBFILE__/$libfile/g ;
			$link_cmd =~ s/__LIBPATH__/$libpath/g ;
			$link_cmd =~ s/__OBJECTS__/$objects/g ;
			push @commands , $link_cmd ;
		}
	}
	return @commands ;
}

sub build
{
	my ( $this ) = @_ ;
	my @commands = $this->_build_commands() ;
	my $ok = 1 ;
	for my $cmd ( @commands )
	{
		print "$cmd\n" unless $this->{m_quiet} ;
		my $rc = system( $cmd ) ;
		$ok = 0 if $rc != 0 ;
	}
	return $ok ;
}

1 ;

# ==

package main ;
use Getopt::Long ;

if( basename($0) eq "mbedtlsbuild.pl" )
{
	my $prefix = File::Basename::basename($0) ;

	my %opt = () ;
	if( !GetOptions( \%opt , "help|h" , "config=s" , "arch=s" , "tls13" , "quiet|q" , "cflags-extra=s" , "as-windows" ) ||
		$opt{help} )
	{
		print "usage: $prefix: [--config={debug|release}] [--arch={x64|x86}] [--tls13] [<source-dir> [<build-dir>]]\n" ;
		exit( $opt{help} ? 0 : 1 ) ;
	}

	my $arch = $opt{arch} || $ENV{Platform} || "x64" ;
	my $config = $opt{config} || "release" ;
	my $tls13 = $opt{tls13} ;
	my $src_dir = $ARGV[0] || MbedtlsBuild::find("mbedtls") ;
	my $build_dir = $ARGV[1] || "mbedtls-${arch}" ;
	my $quiet = $opt{quiet} ;
	my $os = ( $opt{'as-windows'} ? "windows" : undef ) ;
	my $cflags_extra = $opt{'cflags-extra'} ;

	#die "$prefix: error: only runs on windows\n" if( $^O eq "linux" ) ;
	die "$prefix: error: no vcvars environment\n" if( !$arch ) ;
	die if( $arch ne "x64" && $arch ne "x86" ) ;
	die if( $config ne "debug" && $config ne "release" ) ;

	print "$prefix: source: $src_dir\n" unless $quiet ;
	print "$prefix: build: $build_dir\n" unless $quiet ;

	my $b = new MbedtlsBuild( $src_dir , $build_dir , $arch , $config , $tls13 , $prefix , $quiet , $os , $cflags_extra ) ;
	$b->copy_headers_() ;
	$b->configure_() ;
	my $ok = $b->build() ;
	print "$prefix: error: failed\n" if( !$ok && !$quiet ) ;
	exit( $ok ? 0 : 1 ) ;
}
else
{
	1 ;
}

