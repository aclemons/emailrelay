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
# libresslbuild.pl
#
# Builds libressl on Windows using nmake. See also libresslbuild.mak.
#
# usage: libresslbuild.pl [--arch <arch>] [--config <config>] [<src-dir> [<build-dir>]]
#          --arch      x64 or x86
#          --config    release or debug
#          --makefile  libresslbuild.mak path
#          <src-dir>   defaults to liberessl in dirname($0) or its parent or grandparent
#          <build-dir> defaults to libressl-<arch>
#
# Eg:
#    libresslbuild.pl --config=release libressl-9.9 libressl-x64
#
# Run from a 'vcvars' "developer command prompt".
#
# Libraries end up in in <build-dir>/library/<config> and headers under
# <build-dir>/include.
#
# Synopsis:
#    require "libresslbuild.pl" ;
#    my $src_dir = LibresslBuild::find() ;
#    LibresslBuild::copy_headers( $src_dir , $build_dir ) ;
#    my $b = new LibresslBuild( $src_dir , $build_dir , $makefile , $arch , $config ) ;
#    $b->copy_headers_() ;
#    $b->build() or die ;
#

use strict ;
use FileHandle ;
use Getopt::Long ;
use File::Basename ;
use File::Copy ;
use Cwd ;

package LibresslBuild ;

sub new
{
	my ( $classname , $src_dir , $build_dir , $makefile , $arch , $config , $cflags_extra , $prefix , $quiet ) = @_ ;

	$arch ||= "x64" ;
	$config ||= "release" ;
	$prefix ||= "libresslbuild" ;
	$cflags_extra ||= "" ;

	my $this = bless {
		m_prefix => $prefix ,
		m_quiet => $quiet ,
		m_arch => $arch ,
		m_config => $config ,
		m_src_dir => $src_dir ,
		m_build_dir => $build_dir ,
		m_makefile => $makefile ,
		m_cflags_extra => $cflags_extra ,
	} , $classname ;
	_init( $this ) ;
	return $this ;
}

sub _init
{
	my ( $this ) = @_ ;
	-f "$$this{m_src_dir}/ssl/ssl_init.c" or die "$$this{m_prefix}: error: invalid libressl source directory [$$this{m_src_dir}]\n" ;
	map { -d $_ || mkdir $_ or die "mkdir($_)" } (
		File::Basename::dirname($this->{m_build_dir}) ,
		"$$this{m_build_dir}" ,
		#"$$this{m_build_dir}/library" ,
		#"$$this{m_build_dir}/library/$$this{m_config}" ,
		#"$$this{m_build_dir}/include" ,
	) ;
}

sub find
{
	my ( $name ) = @_ ;
	$name ||= "libressl" ;
	my $base = File::Basename::dirname( $0 ) ;
	for my $path ( "$base/$name" , "$base/../$name" , "$base/../../$name" )
	{
		return $path if -d $path ;
	}
	return $name ;
}

sub copy_headers_
{
	my ( $this ) = @_ ;
	copy_headers( $this->{m_src_dir} , $this->{m_build_dir} , !$this->{m_quiet} ) ;
}

sub copy_headers
{
	my ( $src_dir , $build_dir , $verbose , @subdirs ) = @_ ;
	push @subdirs , ( "openssl" ) if( scalar(@subdirs) == 0 ) ;
	map { _copy_headers_imp( "$src_dir/include" , "$build_dir/include" , $_ , $verbose ) } @subdirs ;
}

sub _copy_headers_imp
{
	my ( $src_inc_dir , $dst_inc_dir , $subdir , $verbose ) = @_ ;

	mkdir( $dst_inc_dir ) || die $dst_inc_dir if ! -d $dst_inc_dir ;
	mkdir( "$dst_inc_dir/$subdir" ) || die $subdir if ! -d "$dst_inc_dir/$subdir" ;

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

sub build
{
	my ( $this ) = @_ ;
	die if $^O eq "linux" ;
	my @commands = ( "nmake /f $$this{m_makefile} " .
		"ARCH=$$this{m_arch} " .
		"CONFIG=$$this{m_config} " .
		"SRC_DIR=$$this{m_src_dir} " .
		"BUILD_DIR=$$this{m_build_dir} " .
		"CFLAGS_EXTRA=$$this{m_cflags_extra}" ) ;
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

if( basename($0) eq "libresslbuild.pl" )
{
	my $prefix = File::Basename::basename($0) ;

	my %opt = () ;
	if( !GetOptions( \%opt , "help|h" , "makefile=s" , "config=s" , "arch=s" , "quiet|q" , "cflags-extra=s" , "as-windows" ) ||
		$opt{help} )
	{
		print "usage: $prefix: [--config={debug|release}] [--arch={x64|x86}] [<source-dir> [<build-dir>]]\n" ;
		exit( $opt{help} ? 0 : 1 ) ;
	}

	my $arch = $opt{arch} || $ENV{Platform} || "x64" ;
	my $config = $opt{config} || "release" ;
	my $src_dir = $ARGV[0] || LibresslBuild::find("libressl") ;
	my $build_dir = $ARGV[1] || "libressl-${arch}" ;
	my $quiet = $opt{quiet} ;
	my $makefile = $opt{makefile} || (dirname($0)."/libresslbuild.mak") ;
	my $cflags_extra = $opt{'cflags-extra'} ;

	#die "$prefix: error: only runs on windows\n" if( $^O eq "linux" ) ;
	die "$prefix: error: no vcvars environment\n" if( !$arch ) ;
	die if( $arch ne "x64" && $arch ne "x86" ) ;
	die if( $config ne "debug" && $config ne "release" ) ;

	print "$prefix: source: $src_dir\n" unless $quiet ;
	print "$prefix: build: $build_dir\n" unless $quiet ;
	print "$prefix: makefile: $makefile\n" unless $quiet ;

	my $b = new LibresslBuild( $src_dir , $build_dir , $makefile , $arch , $config , $cflags_extra , $prefix , $quiet ) ;
	$b->copy_headers_() ;
	my $ok = $b->build() ;
	print "$prefix: error: failed\n" if( !$ok && !$quiet ) ;
	exit( $ok ? 0 : 1 ) ;
}
else
{
	1 ;
}

