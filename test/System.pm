#!/usr/bin/perl
#
# Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
use File::Glob ;
use Cwd ;
use Check ;

package System ;

our $bin_dir = ".." ;
our $verbose = 0 ;

sub log_
{
	print STDERR "++++ ",join(" ",@_),"\n" if $verbose ;
}

sub bsd
{
	return $^O =~ m/bsd$/ ; # not mac
}

sub mac
{
	return $^O eq 'darwin' ;
}

sub linux
{
	return $^O eq 'linux' ;
}

sub unix
{
	return bsd() || mac() || linux() ; # TODO more
}

sub _dot_exe
{
	return unix() ? "" : ".exe" ;
}

sub path
{
	return join( "/" , @_ ) ;
}

sub weirdpath
{
	my $p = path( @_ ) ;
	if( !unix() ) { $p =~ s:/:\\:g }
	return $p ;
}

sub exe
{
	return path( @_ ) . _dot_exe() ;
}

sub chmod_r
{
	my ( $dir , $chmod_arg_dir , $chmod_arg_file ) = @_ ;
	$chmod_arg_dir ||= "700" ;
	$chmod_arg_file ||= "600" ;
	if( unix() )
	{
		my $xargs = mac() ? "xargs" : "xargs -r" ;
		system( "chmod $chmod_arg_dir " . $dir ) ;
		system( "cd $dir ; ls -1 | $xargs chmod $chmod_arg_file" ) ;
	}
}

sub commandline
{
	my ( $command , $args_in ) = @_ ;

	$args_in ||= {} ;
	my %args = %$args_in ;
	if(!exists($args{background})) {$args{background} = 0}
	if(!defined($args{stdout})) {$args{stdout} = ""}
	if(!defined($args{stderr})) {$args{stderr} = ""}
	if(!defined($args{prefix})) {$args{prefix} = ""} # eg. "sudo -c bin"
	if(!defined($args{gtest})) {$args{gtest} = ""}

	my $stderr = $args{stderr} ;
	if( $args{stdout} ne "" && $args{stdout} eq $args{stderr} )
	{
		$stderr = "&1" ;
	}

	return
		( System::unix() ? "" : "cmd /c \"" ) .
		( System::unix() || $args{gtest} eq "" ? "" : "set G_TEST=$args{gtest} && " ) .
		( $args{background} && !System::unix() ? "start /D. " : "" ) .
		$args{prefix} . $command . " " .
		( $args{stdout} ? ">$args{stdout} " : "" ) .
		( $args{stderr} && System::unix() ? "2>$stderr " : "" ) .
		( System::unix() ? "" : "\"" ) .
		( System::unix() && $args{background} ? "&" : "" ) ;
}

sub tempfile
{
	# Creates a temporary file with a unique name, optionally
	# using the given hint as part of the filename.
	my ( $hint_key , $dir ) = @_ ;
	$hint_key = defined($hint_key) ? $hint_key : "" ;
	$dir = defined($dir) ? $dir : Cwd::cwd() ;
	return $dir . "/" . ".tmp.$hint_key." . $$ . "." . rand() ;
}

sub createFile
{
	# Creates a file, optionally containing one line of text.
	my ( $path , $line ) = @_ ;
	my $fh = new FileHandle( "> " . $path ) or die "cannot create [$path]" ;
	if( defined($line) ) { print $fh $line , unix() ? "\n" : "\r\n" }
	$fh->close() or die "cannot write to [$path]" ;
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

sub _deleteFiles
{
	my ( $dir , $tail ) = @_ ;
	for my $path ( glob_( "$dir/*$tail" ) )
	{
		if( -f $path && $path =~ m/${tail}$/ ) # sanity check
		{
			my $ok = unlink $path ;
			Check::that( $ok , "cannot delete file" , $path ) ;
		}
	}
}

sub deleteSpoolDir
{
	# Deletes valid-looking message files from a spool 
	# directory. Optionally deletes all files.
	my ( $path , $all ) = @_ ;
	$all = defined($all) ? $all : 0 ;
	if( -d $path )
	{
		_deleteFiles( $path , "content" ) ;
		_deleteFiles( $path , "envelope" ) ;
		if( $all )
		{
			_deleteFiles( $path , "envelope.bad" ) ;
			_deleteFiles( $path , "envelope.busy" ) ;
			_deleteFiles( $path , "envelope.new" ) ;
		}
		rmdir( $path ) ;
	}
}

sub glob_
{
	# Returns the file paths that match the given glob expression.
	my ( $expr ) = @_ ;
	my @files = File::Glob::bsd_glob( $expr ) ;
	return @files ;
}

sub match
{
	# Returns the name of the single file that matches
	# the given filespec. Fails if not exactly one.
	my ( $filespec ) = @_ ;
	my @files = glob_( $filespec ) ;
	Check::that( @files == 0 || @files == 1 , "too many matching files" , $filespec ) ;
	return $files[0] ;
}

sub submitSmallMessage
{
	# Submits a small message using the "emailrelay-submit" utility.
	my ( $spool_dir , $tmp_dir , @to ) = @_ ;
	submitMessage( $spool_dir , $tmp_dir , 10 , @to ) ;
}

sub submitMessage
{
	# Submits a message of 'n' lines using the "emailrelay-submit" utility.
	my ( $spool_dir , $tmp_dir , $n , @to ) = @_ ;
	push @to , "me\@there.local" if( scalar(@to) == 0 ) ;
	my $path = createMessageFile($tmp_dir,$n) ;
	my $rc = system( weirdpath(exe($bin_dir,"emailrelay-submit")) . " --from me\@here.local " .
		"--spool-dir $spool_dir " . join(" ",@to) . " < $path" ) ;
	Check::that( $rc == 0 , "failed to submit" ) ;
	unlink $path ;
}

sub submitMessages
{
	# Submits 'n' message of 'm' lines using the "emailrelay-submit" utility.
	my ( $spool_dir , $tmp_dir , $n , $m ) = @_ ;
	for my $i ( 1 .. $n )
	{
		submitMessage( $spool_dir , $tmp_dir , $m ) ;
	}
}

sub _old_status
{
	my ( $pid , $key , $field ) = @_ ;
	# linux-specific
	my $line = `cat /proc/$pid/status | fgrep $key: | head -1` ;
	chomp $line ;
	my @part = split( /\s+/ , $line ) ;
	return $part[$field] ;
}

sub _status
{
	my ( $pid , $key , $field ) = @_ ;
	my $cmd = "ps -p $pid -o pid,ruid,uid,svuid,gid,rgid,svgid" ;
	my $fh = new FileHandle( "$cmd |" ) ;
	my $header = <$fh> ;
	chomp( my $line = <$fh> ) ;
	$line =~ s/^\s+// ;
	my ($pid_ignore,$ruid,$uid,$svuid,$gid,$rgid,$svgid) = split( /\s+/ , $line ) ;
	my $result = undef ;
	$result = $ruid if( $key eq "Uid" && $field == 1 ) ; # real
	$result = $uid if( $key eq "Uid" && $field == 2 ) ; # effective
	$result = $svuid if( $key eq "Uid" && $field == 3 ) ; # saved
	$result = $rgid if( $key eq "Gid" && $field == 1 ) ; # real
	$result = $gid if( $key eq "Gid" && $field == 2 ) ; # effective
	if( $result < 0 && ( mac() || bsd() ) )
	{
		$result += 4294967296 ;
	}
	return $result ;
}

sub effectiveUser
{
	# Returns the calling process's effective user id.
	my ( $pid ) = @_ ;
	return _status($pid,"Uid",2) ;
}

sub effectiveGroup
{
	# Returns the calling process's effective group id.
	my ( $pid ) = @_ ;
	return _status($pid,"Gid",2) ;
}

sub realUser
{
	# Returns the calling process's real user id.
	my ( $pid ) = @_ ;
	return _status($pid,"Uid",1) ;
}

sub realGroup
{
	# Returns the calling process's group id.
	my ( $pid ) = @_ ;
	return _status($pid,"Gid",1) ;
}

sub savedUser
{
	# Returns the calling process's saved user id.
	my ( $pid ) = @_ ;
	return _status($pid,"Uid",3) ;
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
	$n *= 5 if !unix() ;
	$sleep_time = defined($sleep_time) ? $sleep_time : 1 ;
	$progress = defined($progress) ? $progress : 1 ;
	for( my $i = 0 ; $i < $n ; $i++ )
	{
		my @list = glob_( "$dir/*" ) ;
		print "." if( $progress ) ;
		if( scalar(@list) == 0 ) { return 1 }
		sleep( $sleep_time ) ;
	}
	return 0 ;
}

sub sleep_cs
{
	my ( $cs ) = @_ ;
	$cs = defined($cs) ? $cs : 1 ;
	select( undef , undef , undef , 0.01 * $cs ) ;
}

sub kill_
{
	my ( $pid , $timeout_cs ) = @_ ;
	$timeout_cs = defined($timeout_cs) ? $timeout_cs : 100 ;
	return if( !defined($pid) || $pid <= 0 ) ;
	if( unix() )
	{
		kill( 15 , $pid ) ;
		sleep_cs( $timeout_cs ) ;
		kill( 9 , $pid ) ;
	}
	else
	{
		kill( 15 , $pid ) ;
		sleep_cs( $timeout_cs ) ;
		system( "taskkill /PID $pid >NUL 2>&1" ) ;
		sleep_cs( $timeout_cs ) ;
		system( "taskkill /F /PID $pid >NUL 2>&1" ) ;
	}
}

sub wait
{
	my ( $pid , $timeout_cs ) = @_ ;
	$timeout_cs = defined($timeout_cs) ? $timeout_cs : 100 ;
	for( my $i = 0 ; $i < $timeout_cs ; $i++ )
	{
		sleep_cs() ;
		if( System::processIsRunning($pid) )
		{
			next
		}
	}
}

sub processIsRunning
{
	my ( $pid ) = @_ ;
	if( unix() )
	{
		my $rc = kill 0 , $pid ;
		return defined($rc) ? $rc : 0 ;
	}
	else
	{
		if( defined($pid) && $pid > 0 )
		{
			my $fh = new FileHandle( "tasklist /FI \"PID eq $pid\" /FO csv /NH |" ) or 
				die "tasklist error" ;
			while(<$fh>)
			{
				chomp( my $line = $_ ) ;
				my ( $f_name , $f_pid ) = split( "," , $line ) ;
				return ( $f_pid eq "\"$pid\"" ? 1 : 0 ) ;
			}
		}
		return 0 ;
	}
}

sub killall
{
	my ( $match ) = @_ ;
	if( System::unix() )
	{
		# TODO
	}
	else
	{
		my $fh = new FileHandle( "tasklist /NH /FO csv |" ) or die ;
		while(<$fh>)
		{
			chomp( my $line = $_ ) ;
			next if $line eq "" ;
			$line =~ s:["]::g ;
			my ( $name , $pid ) = split( "," , $line ) ;
			if( $name =~ m/^$match/ )
			{
				log_( "killing pid $pid ($name)" ) ;
				system( "taskkill /F /PID $pid" ) ;
			}
		}
	}
}

my $port_generator = 10000 ;
sub nextPort
{
	$port_generator++ ;
	return $port_generator ;
}

1 ;

