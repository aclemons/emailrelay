#!/usr/bin/perl
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
# winbuild.pm
#
# Helper functions for winbuild.pl.
#
# Synopsis:
#
#  require "winbuild.pm" ;
#  winbuild::default_touchfile(...) ;
#  winbuild::find_cmake(...) ;
#  winbuild::find_msbuild(...) ;
#  winbuild::find_qt(...) ;
#  winbuild::find_mbedtls(...) ;
#  winbuild::find_runtime(...) ;
#  winbuild::spit_out_batch_files(...) ;
#  winbuild::clean_cmake_files(...) ;
#  winbuild::clean_cmake_cache_files(...) ;
#  winbuild::deltree(...) ;
#  winbuild::run_msbuild(...) ;
#  winbuild::translate(...) ;
#  winbuild::create_touchfile(...) ;
#  winbuild::read_makefiles(...) ;
#  winbuild::read_makefiles_imp(...) ;
#  winbuild::cache_value(...) ;
#  winbuild::find_msvc_base(...) ;
#  winbuild::fixup(...) ;
#  winbuild::touch(...) ;
#  winbuild::file_copy(...) ;
#

use strict ;
use Cwd ;
use Carp ;
use FileHandle ;
use File::Basename ;
use File::Find ;
use File::Path ;
use File::Glob ':bsd_glob' ;
use lib dirname($0) , dirname($0)."/bin" ;
use AutoMakeParser ;
$AutoMakeParser::debug = 0 ;

package winbuild ;

sub find_cmake
{
	return _fcache( "cmake" ,
		_find_bypass( "cmake" ) ||
		_find_basic( "find-cmake" , "cmake.exe" , _path_dirs() ) ||
		_find_match( "find-cmake" , "cmake*/bin/cmake.exe" , undef ,
			"$ENV{SystemDrive}" ,
			"$ENV{ProgramFiles}" ) ) ;
}

sub find_msbuild
{
	return _fcache( "msbuild" ,
		_find_bypass( "msbuild" ) ||
		_find_basic( "find-msbuild" , "msbuild.exe" , _path_dirs() ) ||
		_find_under( "find-msbuild" , "msbuild.exe" ,
			$ENV{'ProgramFiles(x86)'}."/msbuild" ,
			$ENV{'ProgramFiles(x86)'}."/Microsoft Visual Studio" ,
			$ENV{'ProgramFiles(x86)'} ,
			$ENV{ProgramFiles} ) ) ;
}

sub find_qt
{
	my @dirs = (
		File::Basename::dirname($0)."/.." ,
		"$ENV{HOMEDRIVE}$ENV{HOMEPATH}/qt" ,
		"$ENV{SystemDrive}/qt" ,
	) ;

	my $x86 =
		_find_bypass( "qt" , "x86" ) ||
		_find_match( "find-qt(x86)" , "5*/msvc*/lib/cmake/qt5" , qr;/msvc\d\d\d\d/; , @dirs ) ;

	my $x64 =
		_find_bypass( "qt" , "x64" ) ||
		_find_match( "find-qt(x64)" , "5*/msvc*_64/lib/cmake/qt5" , undef , @dirs ) ;

	_fcache( "qt-x86" , $x86 ) ;
	_fcache( "qt-x64" , $x64 ) ;
	return { x86 => $x86 , x64 => $x64 } ;
}

sub find_mbedtls
{
	return _fcache( "mbedtls" ,
		_find_bypass( "mbedtls" ) ||
		_find_match( "find-mbedtls" , "mbedtls*" , undef ,
			File::Basename::dirname($0)."/.." ,
			"$ENV{HOMEDRIVE}$ENV{HOMEPATH}" ,
			"$ENV{SystemDrive}" ) ) ;
}

sub find_runtime
{
	my ( $msvc_base , $arch , @names ) = @_ ;
	my $search_base = "$msvc_base/redist" ;
	my %runtime = () ;
	for my $name ( @names )
	{
		my @paths = grep { m:/$arch/:i } _find_all_under( $name , $search_base ) ;
		if( @paths )
		{
			# pick the shortest, as a heuristic
			my @p = sort { length($a) <=> length($b) } @paths ;
			my $path = $p[0] ;
			print "runtime: [$name] for [$arch] is [$path]\n" ;

			$runtime{$name} = { path => $path , name => $name } ;
		}
		else
		{
			print "runtime: [$name] not found under [$search_base]\n" ;
		}
	}
	return \%runtime ;
}

sub default_touchfile
{
	my ( $script ) = @_ ;
	$script =~ s/\.pl$// ;
	return "$script.ok" ;
}

sub _path_dirs
{
	my $path = $ENV{PATH} ;
	my $sep = ( $path =~ m/;/ ) ? ";" : ":" ;
	return split( $sep , $path ) ;
}

sub _sanepath
{
	my ( $path ) = @_ ;
	$path =~ s:\\:/:g ;
	return $path ;
}

sub _find_basic
{
	my ( $logname , $fname , @dirs ) = @_ ;
	my $result ;
	for my $dir ( map {_sanepath($_)} @dirs )
	{
		my $path = "$dir/$fname" ;
		if( -e $path ) { $result = Cwd::realpath($path) ; last }
		print "$logname: not $path\n" ;
	}
	print "$logname=[$result]\n" if $result ;
	return $result ;
}

sub _find_under
{
	my ( $logname , $fname , @dirs ) = @_ ;
	my $result ;
	for my $dir ( map {_sanepath($_)} @dirs )
	{
		next if !$dir ;
		my @find_list = () ;
		File::Find::find( sub { push @find_list , $File::Find::name if lc($_) eq $fname } , $dir ) ;
		if( @find_list ) { $result = Cwd::realpath($find_list[0]) ; last }
		print "$logname: not under $dir\n" ;
	}
	print "$logname=[$result]\n" if $result ;
	return $result ;
}

sub _find_all_under
{
	my ( $fname , $dir ) = @_ ;
	my @result = () ;
	File::Find::find( sub { push @result , $File::Find::name if lc($_) eq $fname } , $dir ) ;
	return @result ;
}

my %fcache = () ;
sub _fcache
{
	my ( $key , $value ) = @_ ;
	$fcache{$key} = $value ;
	return $value ;
}

sub fcache_delete
{
	my $path = join( "/" , File::Basename::dirname(Cwd::realpath($0)) , "winbuild.cfg" ) ;
	rename( "$path" , "${path}.old" ) ;
	unlink( $path ) ;
}

sub fcache_write
{
	my $path = join( "/" , File::Basename::dirname(Cwd::realpath($0)) , "winbuild.cfg" ) ;
	my $fh = new FileHandle( $path , "w" ) or die "error: install: cannot create [$path]\n" ;
	for my $k ( sort keys %fcache )
	{
		print $fh "$k $fcache{$k}\n" ;
	}
	$fh->close() or die ;
}

sub _find_bypass
{
	my ( $name , $arch ) = @_ ;

	# winbuild.cfg
	# eg.
	# cmake c:/cmake/bin/cmake.exe
	# mbedtls c:/mbedtls-2.99
	# msbuild c:/msbuild/msbuild.exe
	# qt-x64 c:/qt/5.0/msvc_64/lib/cmake/Qt5
	# qt-x86 c:/qt/5.0/msvc/lib/cmake/Qt5

	my $fh = new FileHandle( "winbuild.cfg" , "r" ) ;
	return undef if !$fh ;
	my $key = $arch ? "$name-$arch" : $name ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		my ( $k , $v ) = ( $line =~ m/(\S+)\s+(.*)/ ) ;
		return $v if( $k eq $key ) ;
	}
	return undef ;
}

sub _find_match
{
	my ( $logname , $glob , $re , @dirs ) = @_ ;
	$re = qr;.; if !defined($re) ;
	my $result ;
	for my $dir ( map {_sanepath($_)} @dirs )
	{
		my @glob_match = () ;
		push @glob_match , grep { -e $_ && $_ =~ m/$re/ } File::Glob::bsd_glob( "$dir/$glob" ) ;
		if( @glob_match ) { $result = Cwd::realpath($glob_match[0]) ; last }
		print "$logname: no match for $dir/$glob\n" ;
	}
	print "$logname=[$result]\n" if $result ;
	return $result ;
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
	my ( $base_dir ) = @_ ;
	$base_dir ||= "." ;
	my @list = () ;
	File::Find::find( sub { push @list , $File::Find::name if $_ eq "CMakeLists.txt" } , $base_dir ) ;
	unlink @list ;
}

sub clean_cmake_cache_files
{
	my ( $base_dir , $opt ) = @_ ;
	my $verbose = $opt->{verbose} || 0 ;
	$base_dir ||= "." ;
	my @tree_list = () ;
	my @file_list = () ;
	File::Find::find( sub { push @tree_list , $File::Find::name if $_ eq "CMakeFiles" } , $base_dir ) ;
	File::Find::find( sub { push @file_list , $File::Find::name if $_ eq "CMakeCache.txt" } , $base_dir ) ;
	map { print "cmake: cleaning [$base_dir/$_]\n" if $verbose } ( @tree_list , @file_list ) ;
	map { deltree($_) } @tree_list ;
	map { unlink $_ or die } @file_list ;
}

sub deltree
{
	my ( $dir ) = @_ ;
	my $e ;
	File::Path::remove_tree( $dir , {safe=>1,verbose=>0,error=>\$e} ) ;
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
	my ( $msbuild , $project , $arch , $confname , $target ) = @_ ;
	$arch ||= "x64" ;
	$confname ||= "Release" ;
	my $build_dir = $arch ;
	my @msbuild_args = ( "/fileLogger" , "$build_dir/$project.sln" ) ;
	push @msbuild_args , "/t:$target" if $target ;
	push @msbuild_args , "/p:Configuration=$confname" ;
	if( $^O eq "linux" ) { $msbuild = ("make") ; @msbuild_args = ( $target ) }
	my $rc = system( $msbuild , @msbuild_args ) ;
	print "msbuild-exit=[$rc]\n" ;
	die unless $rc == 0 ;
}

sub translate
{
	my ( $arch , $qt_dirs , $xx_XX , $xx ) = @_ ;
	my $dir = $qt_dirs->{$arch} ;
	$dir = File::Basename::dirname( $dir ) ;
	$dir = File::Basename::dirname( $dir ) ;
	$dir = File::Basename::dirname( $dir ) ;
	my $tool = join( "/" , $dir , "bin" , "lrelease.exe" ) ;
	my $rc = system( $tool , "src/gui/emailrelay_tr.$xx_XX.ts" , "-qm" , "src/gui/emailrelay.$xx.qm" ) ;
	print "lrelease-exit=[$rc]\n" ;
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
	my $verbose = 1 ;
	return AutoMakeParser::readall( "." , $switches , $vars , $verbose ) ;
}

sub cache_value
{
	my ( $arch , $re ) = @_ ;
	my $fh = new FileHandle( "$arch/CMakeCache.txt" , "r" ) or Carp::confess "error: cannot open cmake cache file\n" ;
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
	my ( $arch ) = @_ ;
	return _find_bypass("msvc") if _find_bypass("msvc") ;
	my $msvc_linker = _cache_value_msvc_linker( $arch ) ;
	my $dir = File::Basename::dirname( $msvc_linker ) ;
	my ( $base ) = ( $dir =~ m:(.*/vc)/.*:i ) ; # could to better
	$base or die "error: install: cannot determine the msvc base directory from [$msvc_linker]\n" ;
	return _fcache( "msvc" , $base ) ;
}

sub _cache_value_msvc_linker
{
	my ( $arch ) = @_ ;
	my $msvc_linker = cache_value( $arch , qr/^CMAKE_LINKER:[A-Z]+=(.*)/ ) ;
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

sub touch
{
	my ( $path ) = @_ ;
	if( ! -f $path )
	{
		my $fh = new FileHandle( $path , "w" ) or die ;
		$fh->close() or die ;
	}
}

sub file_copy
{
	my ( $src , $dst ) = @_ ;

	my $to_crlf = undef ;
	for my $ext ( "txt" , "js" , "pl" , "pm" )
	{
		$to_crlf = 1 if( ( ! -d $dst && ( $dst =~ m/$ext$/ ) ) || ( -d $dst && ( $src =~ m/$ext$/ ) ) ) ;
	}

	if( $to_crlf )
	{
		if( -d $dst ) { $dst = "$dst/".File::Basename::basename($src) }
		my $fh_in = new FileHandle( $src , "r" ) ;
		my $fh_out = new FileHandle( $dst , "w" ) ;
		( $fh_in && $fh_out ) or die "error: failed to copy [$src] to [$dst]\n" ;
		while(<$fh_in>)
		{
			chomp( my $line = $_ ) ;
			print $fh_out $line , "\r\n" ;
		}
		$fh_out->close() or die "error: failed to copy [$src] to [$dst]\n" ;
	}
	else
	{
		File::Copy::copy( $src , $dst ) or die "error: failed to copy [$src] to [$dst]\n" ;
	}
}

package main ;

1 ;
