#!/usr/bin/perl
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
# winbuild.pm
#
# Automakefile parser.
#
# Synopsis:
#
#  require "winbuild.pm" ;
#  winbuild::default_touchfile(...) ;
#  winbuild::find_cmake(...) ;
#  winbuild::find_msbuild(...) ;
#  winbuild::spit_out_batch_files(...) ;
#  winbuild::clean_cmake_files(...) ;
#  winbuild::clean_cmake_cache_files(...) ;
#  winbuild::deltree(...) ;
#  winbuild::run_msbuild(...) ;
#  winbuild::create_touchfile(...) ;
#  winbuild::read_makefiles(...) ;
#  winbuild::read_makefiles_imp(...) ;
#  winbuild::cache_value(...) ;
#  winbuild::find_msvc_base(...) ;
#  winbuild::fixup(...) ;
#

use strict ;
use FileHandle ;
use File::Basename ;
use File::Find ;
use File::Path ;

package winbuild ;

sub default_touchfile
{
	my ( $script ) = @_ ;
	$script =~ s/\.pl$// ;
	return "$script.ok" ;
}

sub find_cmake
{
	return "cmake" if "$^O" eq "linux" ;
	my @dirs = (
		split(";",$ENV{PATH}) ,
		"$ENV{SystemDrive}:/cmake/bin" ,
		"$ENV{ProgramFiles}/cmake/bin" ,
	) ;
	my @list = grep { -f $_ } map { "$_/cmake.exe" } grep { $_ } @dirs ;
	return scalar(@list) ? $list[0] : undef ;
}

sub find_msbuild
{
	return "make" if "$^O" eq "linux" ;
	my @list = grep { -f $_ } map { "$_/msbuild.exe" } grep { $_ } split(";",$ENV{PATH}) ;
	for my $base (
		$ENV{'ProgramFiles(x86)'}."/msbuild" ,
		$ENV{ProgramFiles} ,
		$ENV{'ProgramFiles(x86)'} )
	{
		next if !$base ;
		last if scalar(@list) ;
		print "msbuild-search=[$base]\n" ;
		File::Find::find( sub { push @list , $File::Find::name if lc($_) eq "msbuild.exe" } , $base ) ;
	}
	return scalar(@list) ? $list[0] : undef ;
}

sub spit_out_batch_files
{
	my ( @parts ) = @_ ;
	for my $part ( @parts )
	{
		my $fname = "winbuild-$part.bat" ;
		if( ! -f $fname )
		{
			my $fh = new FileHandle( $fname , "w" ) or next ;
			print $fh "runperl winbuild.pl winbuild.ok $part\n" ;
			$fh->close() ;
		}
	}
}

sub clean_cmake_files
{
	my @list = () ;
	File::Find::find( sub { push @list , $File::Find::name if $_ eq "CMakeLists.txt" } , "." ) ;
	unlink @list ;
}

sub clean_cmake_cache_files
{
	-d "CMakeFiles" && ( deltree("CMakeFiles") ) ;
	-f "CMakeCache.txt" && ( unlink "CMakeCache.txt" or die ) ;
}

sub deltree
{
	my ( $dir ) = @_ ;
	my $e ;
	File::Path::remove_tree( $dir , {safe=>1,verbose=>1,error=>\$e} ) ;
	if( $e && scalar(@$e) )
	{
		for my $x ( @$e )
		{
			my ( $f , $m ) = ( %$x ) ;
			print "warning: " . ($f?"[$f]: ":"") . $m , "\n" ;
		}
	}
}

sub run_msbuild
{
	my ( $msbuild , $project , $confname , $target ) = @_ ;
	$confname ||= "Release" ;
	my @msbuild_args = ( "/fileLogger" , "$project.sln" ) ;
	if( $target ) { push @msbuild_args , "/t:$target" }
	if( $project ) { push @msbuild_args , "/p:Configuration=$confname" }
	if( $^O eq "linux" ) { $msbuild = ("make") ; @msbuild_args = ( $target ) }
	my $rc = system( $msbuild , @msbuild_args ) ;
	print "msbuild-exit=[$rc]\n" ;
	die unless $rc == 0 ;
}

sub create_touchfile
{
	my ( $touchfile ) = @_ ;
	my $fh = new FileHandle( $touchfile , "w" ) or die ;
	$fh->close() or die ;
}

sub read_makefiles
{
	my ( $switches , $vars ) = @_ ;
	my @makefiles = () ;
	read_makefiles_imp( \@makefiles , "." , $switches , $vars ) ;
	return @makefiles ;
}

sub read_makefiles_imp
{
	my ( $makefiles , $dir , $switches , $vars ) = @_ ;
	my $m = new AutoMakeParser( "$dir/Makefile.am" , $switches , $vars ) ;
	print "makefile=[" , $m->path() , "]\n" ;
	push @$makefiles , $m ;
	for my $subdir ( $m->value("SUBDIRS") )
	{
		read_makefiles_imp( $makefiles , "$dir/$subdir" , $switches , $vars ) ;
	}
}

sub cache_value
{
	my ( $re ) = @_ ;
	my $fh = new FileHandle( "CMakeCache.txt" , "r" ) or die "error: cannot open cmake cache file\n" ;
	my $value ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		my ( $x ) = ( $line =~ m/$re/i ) ;
		if( $x )
		{
			$value = $x ;
			last ;
		}
	}
	return $value ;
}

sub find_msvc_base
{
	my $msvc_linker = _cache_value_msvc_linker() ;
	my $dir = File::Basename::dirname( $msvc_linker ) ;
	my ( $base ) = ( $dir =~ m:(.*/vc)/.*:i ) ; # could to better
	$base or die "error: install: cannot determine the msvc base directory from [$msvc_linker]\n" ;
	return $base ;
}

sub _cache_value_msvc_linker
{
	my $msvc_linker = cache_value( qr/^CMAKE_LINKER:[A-Z]+=(.*)/ ) ;
	$msvc_linker or die "error: install: cannot read linker path from CMakeCache.txt\n" ;
	return $msvc_linker ;
}

sub fixup
{
	my ( $base , $fnames , $fixes ) = @_ ;
	for my $fname ( @$fnames )
	{
		my $fh_in = new FileHandle( "$base/$fname" , "r" ) or die ;
		my $fh_out = new FileHandle( "$base/$fname.$$.tmp" , "w" ) or die ;
		while(<$fh_in>)
		{
			my $line = $_ ;
			for my $from ( keys %$fixes )
			{
				my $to = $fixes->{$from} ;
				$line =~ s/\Q$from\E/$to/g ;
			}
			print $fh_out $line ;
		}
		$fh_in->close() or die ;
		$fh_out->close() or die ;
		rename( "$base/$fname.$$.tmp" , "$base/$fname" ) or die ;
	}
}

package main ;

1 ;
