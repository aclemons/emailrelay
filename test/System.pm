#!/usr/bin/perl
#
# Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# System.pm
#
# Provides various o/s-y utilities.
#

use strict ;
use FileHandle ;
use Check ;

package System ;

our $bin_dir = ".." ;

sub cwd
{
	# Returns the cwd.
	my $s = `pwd` ; 
	chomp $s ;
	return $s ;
}

sub tempfile
{
	# Creates a temporary file with a unique name, optionally
	# using the given hint as part of the filename.
	my ( $hint_key , $dir ) = @_ ;
	$hint_key = defined($hint_key) ? $hint_key : "" ;
	$dir = defined($dir) ? $dir : cwd() ;
	return $dir . "/" . ".tmp.$hint_key." . $$ . "." . rand() ;
}

sub createFile
{
	# Creates a file, optionally containing one line of text.
	my ( $path , $line ) = @_ ;
	my $fh = new FileHandle( "> " . $path ) ;
	if( defined($line) ) { print $fh $line , "\n" }
	$fh->close() ;
}

sub createSmallMessageFile
{
	# Creates a small message file.
	my ( $dir ) = @_ ;
	return createMessageFile( $dir , 10 ) ;
}

sub createMessageFile
{
	# Creates a message file containing 'n' lines
	# of gibberish text.
	my ( $dir , $n ) = @_ ;
	$n = defined($n) ? $n : 10 ;
	my $path = tempfile("message",$dir) ;
	my $fh = new FileHandle( "> " . $path ) ;
	print $fh "Subject: test\r\n" ;
	print $fh "X-Foo: bar\r\n" ;
	print $fh "\r\n" ;
	for( my $i = 0 ; $i < $n ; $i++ )
	{
		print $fh "${i}_ddflgkjrpodfpgdsflkgjxcmselrkjwlenwoiuoiuoiuwoeiruw\r\n" ;
	}
	$fh->close() ;
	return $path ;
}

sub createSpoolDir
{
	# Creates a spool directory with open permissions.
	my ( $mode , $dir , $key ) = @_ ;
	$mode = defined($mode) ? $mode : 0777 ;
	$key = defined($key) ? $key : "spool" ;
	my $path = tempfile($key,$dir) ;
	my $old_mask = umask 0 ;
	my $ok = mkdir $path , $mode ;
	umask $old_mask ;
	Check::that( $ok , "failed to create spool directory" , $path ) ;
	return $path ;
}

sub deleteSpoolDir
{
	# Deletes valid-looking message files from a spool 
	# directory. Optionally deletes all files.
	my ( $path , $all ) = @_ ;
	$all = defined($all) ? $all : 0 ;
	if( -d $path )
	{
		system( "cd $path ; ls -1 | grep 'content\$' | xargs -r rm 2>/dev/null" ) ;
		system( "cd $path ; ls -1 | grep 'envelope\$' | xargs -r rm 2>/dev/null" ) ;
		if( $all )
		{
			system( "cd $path ; ls -1 | grep 'envelope.bad\$' | xargs -r rm 2>/dev/null" ) ;
			system( "cd $path ; ls -1 | grep 'envelope.busy\$' | xargs -r rm 2>/dev/null" ) ;
			system( "cd $path ; ls -1 | grep 'envelope.new\$' | xargs -r rm 2>/dev/null" ) ;
		}
		system( "rmdir $path" ) ;
	}
}

sub match
{
	# Returns the name of the single file that matches
	# the given filespec. Fails if not exactly one.
	my ( $filespec ) = @_ ;
	my $s = `ls $filespec` ;
	chomp $s ;
	my @lines = split( "\n" , $s ) ;
	Check::that( @lines == 0 || @lines == 1 , "too many matching files" , $filespec ) ;
	return @lines[0] ;
}

sub submitSmallMessage
{
	# Submits a small message using the "emailrelay-submit" utility.
	my ( $spool_dir , $tmp_dir ) = @_ ;
	submitMessage( $spool_dir , $tmp_dir , 10 ) ;
}

sub submitMessage
{
	# Submits a message of 'n' lines using the "emailrelay-submit" utility.
	my ( $spool_dir , $tmp_dir , $n ) = @_ ;
	my $path = createMessageFile($tmp_dir,$n) ;
	my $rc = system( "$bin_dir/emailrelay-submit --from me\@here.local " .
		"--spool-dir $spool_dir me\@there.local < $path" ) ;
	Check::that( $rc == 0 , "failed to submit" ) ;
	unlink $path ;
}

sub _proc
{
	my ( $pid , $re , $field ) = @_ ;
	my $s = `cat /proc/$pid/status | fgrep $re | head -1` ;
	chomp $s ;
	my @part = split( /\s+/ , $s ) ;
	return $part[$field] ;
}

sub effectiveUser
{
	# Returns the calling process's effective user id.
	my ( $pid ) = @_ ;
	return _proc($pid,"Uid:",2) ;
}

sub effectiveGroup
{
	# Returns the calling process's effective group id.
	my ( $pid ) = @_ ;
	return _proc($pid,"Gid:",2) ;
}

sub realUser
{
	# Returns the calling process's real user id.
	my ( $pid ) = @_ ;
	return _proc($pid,"Uid:",1) ;
}

sub realGroup
{
	# Returns the calling process's group id.
	my ( $pid ) = @_ ;
	return _proc($pid,"Gid:",1) ;
}

sub savedUser
{
	# Returns the calling process's saved user id.
	my ( $pid ) = @_ ;
	return _proc($pid,"Uid:",3) ;
}

sub uid
{
	# Returns the user id for a given account.
	my ( $name ) = @_ ;
	my ($login_,$pass_,$uid_,$gid_) = getpwnam($name) ;
	return $uid_ ;
}

sub gid
{
	# Returns the group id for a given account.
	my ( $name ) = @_ ;
	my ($login_,$pass_,$uid_,$gid_) = getpwnam($name) ;
	return $gid_ ;
}

sub drain
{
	# Waits for files to disappear from a directory.
	my ( $dir , $n , $sleep_time , $progress ) = @_ ;
	$n = defined($n) ? $n : 10 ;
	$sleep_time = defined($sleep_time) ? $sleep_time : 1 ;
	$progress = defined($progress) ? $progress : 1 ;
	for( my $i = 0 ; $i < $n ; $i++ )
	{
		my @list = `ls -1 $dir 2>/dev/null` ;
		print "." if( $progress ) ;
		if( scalar(@list) == 0 ) { return 1 }
		sleep( $sleep_time ) ;
	}
	return 0 ;
}

1 ;

