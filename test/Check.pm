#!/usr/bin/perl
#
# Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Check.pm
#

use strict ;
use Carp ;
use FileHandle ;

package Check ;

sub that
{
	my ( $ok , @args ) = @_ ;
	if( !$ok )
	{
		Carp::croak( "[ " . join(": ",@args) . " ]" ) ;
	}
}

sub ok
{
	that( @_ ) ;
}

sub fileEmpty
{
	my ( $path , $more ) = @_ ;
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($path);
	Check::that( -f $path && $size == 0 , "file not empty" , $path , $more ) ;
}

sub fileExists
{
	my ( $path , $more ) = @_ ;
	Check::that( -f $path , "file does not exist" , $path , $more ) ;
}

sub fileNotEmpty
{
	my ( $path , $more ) = @_ ;
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($path);
	Check::that( -f $path && $size > 0 , "file missing or empty" , $path , $more ) ;
}

sub running
{
	my ( $pid , $more ) = @_ ;
	my $n = defined($pid) ? kill(0,$pid) : -1 ;
	Check::that( $n == 1 , "process not running" , $pid , $more ) ;
}

sub notRunning
{
	my ( $pid , $more ) = @_ ;
	my $n = kill 0 , $pid ;
	Check::that( $n == 0 , "process still running" , $pid , $more ) ;
}

sub numeric
{
	my ( $s ) = @_ ;
	Check::that( defined($s) && $s =~ m/[[:digit:]]+/ , "invalid numeric string" , $s ) ;
}

sub fileDeleted
{
	my ( $path , $more ) = @_ ;
	Check::that( ! -f $path , "file still exists" , $path , $more ) ;
}

sub fileMatchCount
{
	my ( $expr , $count , $more ) = @_ ;
	my $output = `ls $expr 2>/dev/null` ;
	chomp $output ;
	my @lines = split("\n",$output) ;
	Check::that( scalar(@lines) == $count , "unexpected number of matching files" , $more ) ;
}

sub fileOwner
{
	my ( $path , $name , $more ) = @_ ;
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($path);
	my $expected = System::uid($name) ;
	Check::that( $uid == $expected , "unexpected file owner" , $path , $uid."!=".$expected , $more ) ;
}

sub fileGroup
{
	my ( $path , $name , $more ) = @_ ;
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($path);
	my $expected = System::gid($name) ;
	Check::that( $gid == $expected , "unexpected file group" , $path , $gid."!=".$expected , $more ) ;
}

sub fileMode
{
	my ( $path , $mode_ , $more ) = @_ ;
	my ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,$ctime,$blksize,$blocks) = stat($path);
	$mode &= 0777 ;
	Check::that( $mode == $mode_ , "unexpected file permissions" , $path , $mode."!=".$mode_ , $more ) ;
}

sub fileLineCount
{
	my ( $path , $count , $string , $more ) = @_ ;
	my $f = new FileHandle( $path ) ;
	my $n = 0 ;
	while( <$f> )
	{
		my $line = $_ ;
		chomp $line ;
		if( !defined($string) || $line =~ m/$string/ ) { $n++ }
	}
	Check::that( $n == $count , "invalid matching line count" , $path , $n."!=".$count , $more ) ;
}

sub fileContains
{
	my ( $path , $string , $more ) = @_ ;
	my $f = new FileHandle( $path ) ;
	my $n = 0 ;
	while( <$f> )
	{
		my $line = $_ ;
		chomp $line ;
		if( !defined($string) || $line =~ m/$string/ ) { $n++ }
	}
	Check::that( $n > 0 , "file does not contain expected string" , $path , "[$string]" , $more ) ;
}

sub fileDoesNotContain
{
	my ( $path , $string , $more ) = @_ ;
	my $f = new FileHandle( $path ) ;
	my $n = 0 ;
	while( <$f> )
	{
		my $line = $_ ;
		chomp $line ;
		if( !defined($string) || $line =~ m/$string/ ) { $n++ }
	}
	Check::that( $n <= 0 , "file contains unexpected string" , $path , "[$string]" , $more ) ;
}

sub match
{
	my ( $s , $re , $more ) = @_ ;
	my $ok = ( $s =~ $re ) ;
	Check::that( $ok , "string does not match regexp [$re]: got [$s]" , $more ) ;
}

1 ;

