#!/usr/bin/perl
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
# System.pm
#
# Provides various o/s-y utilities.
#

use strict ;
use FileHandle ;
use Fcntl qw(:seek :flock LOCK_EX);
use File::Glob ;
use Cwd ;
use Check ;

package System ;

our $bin_dir = ".." ;
our $verbose = 0 ;
our $keep = 0 ;
our $ages = 30 ;
our $localhost = "127.0.0.1" ;

sub log_
{
	print join(" ",@_),"\n" if $verbose ;
}

sub bsd
{
	return $^O =~ m/bsd$/ ; # false for mac
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
	return bsd() || mac() || linux() ;
}

sub windows
{
	return !unix() && ( $^O eq "MSWin32" ) ; # disallow msys elsewhere
}

sub amRoot
{
	my $id = `id -u` ;
	return $id == 0 ;
}

sub _haveSudo
{
	return `sudo -n echo x 2>/dev/null` =~ m/x/ ;
}

sub _haveSu
{
	return `su root -c \"echo x\" 2>/dev/null </dev/null` =~ m/x/ ;
}

sub haveSudo
{
	return _haveSudo() || ( amRoot() && _haveSu() ) ;
}

sub sudoUserPrefix
{
	my ( $user ) = @_ ;
	return _haveSudo() ? "sudo -u \"$user\" " : "su \"$user\" -c \"" ;
}

sub sudoPrefix
{
	return System::amRoot() ? "" : "sudo -n " ;
}

sub sudoCommand
{
	my ( $cmd ) = @_ ;
	my $head = sudoPrefix() ;
	my $tail = ( $head =~ m/"$/ ) ? "\"" : "" ; # not needed
	return $head . $cmd . $tail ;
}

sub testAccount
{
	# Returns a non-root, non-"daemon" account that can be used with "su -c".
	if( System::mac() )
	{
		my $user = $ENV{LOGNAME} ;
		$user = $ENV{SUDO_USER} if( !$user || $user eq "root" ) ;
		return $user ;
	}
	elsif( System::bsd() )
	{
		my $user = $ENV{LOGNAME} ;
		$user = $ENV{SUDO_USER} if( !$user || $user eq "root" ) ;
		if( !$user || $user eq "root" )
		{
			for my $name ( "operator" , "guest" )
			{
				if( `id -u "$name" 2>/dev/null` )
				{
					$user = $name ;
					last ;
				}
			}
		}
		return $user ;
	}
	else
	{
		my $user = $ENV{LOGNAME} ;
		$user = $ENV{SUDO_USER} if( !$user || $user eq "root" ) ;
		return $user ;
	}
}

sub unlink
{
	my ( $path , $wintries ) = @_ ;
	$wintries = (windows()?20:0) if !defined($wintries) ;
	my $pidfile = ( $path =~ m/pid$/ ) ;
	my $keep_this = $keep && !$pidfile ;
	if( -f $path )
	{
		if( $keep_this )
		{
			log_( "not deleting [$path]" ) ;
		}
		else
		{
			log_( "deleting [$path]" ) ;
			my $rc = CORE::unlink( $path ) ;
			$rc or warn "warning: failed to delete [$path]: $!" ;

			while( !$rc && $wintries && -f $path )
			{
				$wintries-- ;
				sleep_cs( 50 ) ;
				$rc = CORE::unlink( $path )
			}
		}
	}
}

sub _dot_exe
{
	return unix() ? "" : ".exe" ;
}

sub path
{
	return join( "/" , grep { m/./ } @_ ) ;
}

sub sanepath
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
	if(!defined($args{prefix})) {$args{prefix} = ""} # eg. 'sudo -u "nobody" ', 'su nobody -c "'
	if(!defined($args{gtest})) {$args{gtest} = ""}

	my $stderr = $args{stderr} ;
	if( $args{stdout} ne "" && $args{stdout} eq $args{stderr} )
	{
		$stderr = "&1" ;
	}

	if( System::unix() )
	{
		return
			$args{prefix} . $command . " " .
			( $args{stdout} ? ">$args{stdout} " : "" ) .
			( $args{stderr} ? "2>$stderr " : "" ) .
			( ( $args{prefix} =~ m/"$/ ) ? "\" " : "" ) .
			( $args{background} ? "&" : "" ) ;
	}
	else
	{
		return
			"cmd /c \"" .
			( $args{gtest} ? "set G_TEST=$args{gtest} && " : "" ) .
			( $args{background} ? "start /D. " : "" ) .
			$command .
			( $args{stdout} ? " >$args{stdout} " : "" ) .
			( $args{stderr} ? "2>$stderr " : "" ) .
			"\"" ;
	}
}

sub _tempdir
{
	if( unix() )
	{
		# using Cwd::cwd() here can be awkward because permissioning tests
		# typically need some unprivileged access to spool directories etc.
		# (consider filter tests where the filter scripts run as "daemon",
		# or submit tests where the submit tool is run from an unprivileged
		# test account) -- when using absolute paths under the cwd every
		# directory on the path requires "--------x" (see stat(2) and
		# open(2)), but we might be running under a home directory with
		# "rwx------" -- using "/tmp" itself is also awkward because of
		# its 'restricted deletion' flag ("--------t"), so we make a
		# subdirectory
		my $dir = "/tmp/emailrelay-test" ;
		my $old_mask = umask 0 ;
		mkdir $dir , 0777 ;
		umask $old_mask ;
		return $dir ;
	}
	else
	{
		return Cwd::cwd() ;
	}
}

sub tempfile
{
	# Returns the path of a temporary file with a unique name, optionally
	# using the given suffix and directory.
	my ( $suffix , $dir ) = @_ ;
	$suffix ||= "tmp" ;
	$dir ||= _tempdir() ; # was Cwd::cwd(), but awkward if root and /root is rwx------
	my $pid = $$ ;
	my $seq = nextPort() ; # might as well
	return "$dir/e.$pid.$seq.$suffix" ;
}

sub createFile
{
	# Creates a file, optionally containing one or more lines of text.
	my ( $path , $line ) = @_ ;
	my $fh = new FileHandle( $path , "w" ) or die "cannot create [$path]" ;
	if( defined($line) && ref($line) )
	{
		for my $s ( @{$line} )
		{
			print $fh $s , unix() ? "\n" : "\r\n"
		}
	}
	elsif( defined($line) )
	{
		print $fh $line , unix() ? "\n" : "\r\n"
	}
	$fh->close() or die "cannot write to [$path]" ;
}

sub waitFor
{
	my ( $fn , $what , $more , $timeout ) = @_ ;
	$timeout ||= $ages ;
	my $t = time() ;
	my $t_end = $t + $timeout ;
	while( $t <= $t_end )
	{
		return if &{$fn}() ;
		sleep_cs( 5 ) ;
		$t = time() ;
	}
	Check::that( undef , "timed out waiting for $what" , $more ) ;
}

sub waitForFileLine
{
	my ( $file , $re , $more , $timeout ) = @_ ;
	waitFor( sub {
		my $fh = new FileHandle( $file ) ;
		while(<$fh>)
		{
			chomp( my $line = $_ ) ;
			return 1 if( $line =~ m/$re/ )
		}
	} , "file [$file] containing [$re]" , $more , $timeout ) ;
}

sub waitForFileLineCount
{
	my ( $file , $re , $count , $more , $timeout ) = @_ ;
	waitFor( sub {
		my $fh = new FileHandle( $file ) ;
		my $n = 0 ;
		while(<$fh>)
		{
			chomp( my $line = $_ ) ;
			$n++ if( $line =~ m/$re/ )
		}
		return $n == $count ? 1 : 0 ;
	} , "file [$file] containing [$re] exactly [$count] times" , $more , $timeout ) ;
}

sub waitForFile
{
	my ( $file , $more , $timeout ) = @_ ;
	waitFor( sub {
		-f $file
	} , "file [$file]" , $more , $timeout ) ;
}

sub waitForFiles
{
	my ( $glob , $count , $more , $timeout ) = @_ ;
	waitFor( sub {
		$count == scalar(grep{-f $_} glob_($glob))
	} , "$count files matching [$glob]" , $more , $timeout ) ;
}

sub waitForPid
{
	my ( $pidfile ) = @_ ;
	my $pid = undef ;
	waitFor( sub {
		my $fh = new FileHandle( $pidfile , "r" ) ;
		$pid = $fh ? $fh->getline() : undef ;
		$pid =~ s/[\r\n].*//g ;
		int($pid)+0 > 0 ;
	} , "pid from pidfile [$pidfile]" ) ;
	return $pid ;
}

sub waitpid
{
	# Waits for a process to terminate.
	my ( $pid ) = @_ ;
	die if( !defined($pid) || $pid < 0 ) ;
	waitFor( sub {
		!processIsRunning( $pid )
	} , "process [$pid] to terminate" ) ;
}

sub createSmallMessageContentFile
{
	# Creates a small message content file and returns its path.
	return _createMessageContent( tempfile("message") , 10 ) ;
}

sub _createMessageContent
{
	# Creates a message content file containing 'n' lines of text.
	my ( $path , $n ) = @_ ;
	$n = defined($n) ? $n : 10 ;
	my $fh = new FileHandle( $path , "w" ) or die ;
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

sub createPidDir
{
	# Creates a pid directory with open permissions and no sticky group.
	my ( $dir ) = @_ ;
	my $old_mask = umask 0 ;
	my $ok = mkdir $dir , 0777 ;
	umask $old_mask ;
	Check::that( $ok , "failed to create pid-file directory" , $dir , $! ) ;
	my $rc = system( "chmod g-s $dir" ) if unix() ;
	Check::that( $rc == 0 , "failed to remove sticky group from pid-file directory" ) ;
	return $dir ;
}

sub rmdir_
{
	my ( $dir ) = @_ ;
	my $ok = rmdir $dir ;
	Check::that( $ok , "failed to remove directory" , $dir , $! ) ;
}

sub createSpoolDir
{
	# Creates a spool directory with open permissions.
	my ( $key , $sticky_group ) = @_ ;
	$key ||= "spool" ;
	$sticky_group ||= "daemon" ;
	my $mode = 0777 ;
	my $path = tempfile( $key ) ;
	my $old_mask = umask 0 ;
	my $ok = mkdir $path , $mode ;
	if( unix() && `id -u` == 0 )
	{
		my $rc = system( "chgrp \"$sticky_group\" \"$path\"" ) ;
		$rc += system( "chmod g+s \"$path\"" ) ;
		Check::that( $rc == 0 , "cannot set spool dir permissions" ) ;
	}
	umask $old_mask ;
	Check::that( $ok , "failed to create spool directory" , $path ) ;
	return $path ;
}

sub _deleteMatchingFiles
{
	my ( $dir , $tail ) = @_ ;
	for my $path ( glob_( "$dir/*$tail" ) )
	{
		if( -f $path && $path =~ m/${tail}$/ ) # sanity check
		{
			my $ok = CORE::unlink( $path ) ; # not System::unlink
			Check::that( $ok , "cannot delete file" , $path ) ;
		}
	}
}

sub deleteSpoolDir
{
	# Deletes valid-looking message files from a spool
	# directory. Optionally deletes failed ones too.
	my ( $path , $all ) = @_ ;
	$all = defined($all) ? $all : 0 ;
	if( defined($path) && -d $path )
	{
		_deleteMatchingFiles( $path , "content" ) ;
		_deleteMatchingFiles( $path , "envelope" ) ;
		if( $all )
		{
			_deleteMatchingFiles( $path , "envelope.bad" ) ;
			_deleteMatchingFiles( $path , "envelope.busy" ) ;
			_deleteMatchingFiles( $path , "envelope.new" ) ;
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
	Check::that( @files == 1 , "wrong number of matching files" , $filespec ) ;
	return $files[0] ;
}

sub matchOne
{
	# Returns the name of one of the files that match
	# the given filespec. Fails if not the expected count.
	my ( $filespec , $index , $count ) = @_ ;
	my @files = glob_( $filespec ) ;
	Check::that( @files == $count , "wrong number of matching files" , $filespec ) ;
	return $files[$index] ;
}

sub submitSmallMessage
{
	# Submits a small message using the "emailrelay-submit" utility.
	my ( $spool_dir , @to ) = @_ ;
	submitMessage( $spool_dir , 10 , @to ) ;
}

sub submitMessage
{
    # Submits a message of 'n' lines using the "emailrelay-submit" utility.
	my ( $spool_dir , $n , @to ) = @_ ;
	push @to , "me\@there.localnet" if( scalar(@to) == 0 ) ;
	my $content_path = _createMessageContent( tempfile("message") , $n ) ;
	my $cmd = sanepath(exe($bin_dir,"emailrelay-submit")) . " --from me\@here.localnet " .
		"--spool-dir $spool_dir " . join(" ",@to) ;
	log_( "submit: [$cmd]" ) ;
	my $rc = system( "$cmd < $content_path" ) ;
	Check::that( $rc == 0 , "failed to submit" ) ;
	System::unlink( $content_path ) ;
}

{
our $seq = 1 ;
sub submitMessageText
{
    # Submits a message using the "emailrelay-submit" utility.
	my ( $spool_dir , @lines ) = @_ ;
	my $to = "me\@there.localnet" ;
	my $tmp_path = tempfile( "message" ) ;
	my $fh = new FileHandle( $tmp_path , "w" ) or die ;
	print $fh "Subject: test\r\n\r\n" ;
	for my $line ( @lines )
	{
		print $fh $line , "\r\n" ;
	}
	$fh->close() or die ;
	my $cmd = sanepath(exe($bin_dir,"emailrelay-submit")) .
		" --verbose --from me\@here.localnet --spool-dir $spool_dir $to" ;
	my $fh_out = new FileHandle( "$cmd < $tmp_path |" ) ;
	chomp( my $content_path = <$fh_out> ) ;
	$fh_out->close() ;
	( my $envelope_path = $content_path ) =~ s/\.content$/.envelope/ ;
	Check::that( -e $content_path && -e $envelope_path , "failed to submit" , $content_path ) ;
	System::unlink( $tmp_path ) ;

	# impose an ordering
	my $n = $seq++ ;
	( my $new_content_path = $content_path ) =~ s:emailrelay\.(\d+)\.(\d+)\.(\d+)\.content$:emailrelay.$n.content: ;
	( my $new_envelope_path = $envelope_path ) =~ s:emailrelay\.(\d+)\.(\d+)\.(\d+)\.envelope$:emailrelay.$n.envelope: ;
	rename( $content_path , $new_content_path ) or die ;
	rename( $envelope_path , $new_envelope_path ) or die ;
}
}

sub submitMessages
{
	# Submits 'n' messages of 'm' lines using the "emailrelay-submit" utility.
	my ( $spool_dir , $n , $m ) = @_ ;
	for my $i ( 1 .. $n )
	{
		submitMessage( $spool_dir , $m ) ;
	}
}

sub submitMessageSequence
{
	# Submits 'n' messages of 'm' lines having sequential filenames.
	# Uses the "emailrelay-submit" utility to create the template message
	# of 'm' lines and then copies it 'n' times and deletes the original.
	# Filenames are like "emailrelay.001.(content|envelope)". The spool
	# directory must be empty to start with.
	my ( $spool_dir , $n , $m , @to ) = @_ ;
	submitMessage( $spool_dir , $m , @to ) ;
	my ( $content ) = System::glob_( $spool_dir."/*.content" ) ;
	my ( $envelope ) = System::glob_( $spool_dir."/*.envelope" ) ;
	for my $i ( 1 .. $n )
	{
		my $x = sprintf( "%03d" , $i ) ;
		File::Copy::copy( $content , $spool_dir."/emailrelay.$x.content" ) or die ;
		File::Copy::copy( $envelope , $spool_dir."/emailrelay.$x.envelope" ) or die ;
	}
	unlink( $content ) or die ;
	unlink( $envelope ) or die ;
}

sub editEnvelope
{
	# Sets one field of an envelope file.
	my ( $path , $key , $value ) = @_ ;
	my $fh_in = new FileHandle( $path ) or die "cannot edit envelope [$path]" ;
	my $fh_out = new FileHandle( "$path.tmp" , "w" ) or die ;
	while(<$fh_in>)
	{
		( my $line = $_ ) =~ s/\r?\n$// ;
		if( $line =~ m/^X-MailRelay-[^:]*$key:/ )
		{
			$line =~ s/: .*/: $value/ ;
		}
		print $fh_out $line , "\r\n" ;
	}
	$fh_in->close() ;
	$fh_out->close() or die ;
	rename( "$path.tmp" , $path ) or die ;
}

sub _pstatus
{
	my ( $pid , $key , $field ) = @_ ;

	die if !unix() ;
	my $value1 = eval { _psstatus( $pid , $key , $field ) } ;
	my $error1 = $@ =~ s/\n//gr =~ s/\r//gr  =~ s; at /.*;;r ;
	my $value2 = eval { _procstatus( $pid , $key , $field ) } ;
	my $error2 = $@ =~ s/\n//gr =~ s/\r//gr =~ s; at /.*;;r ;
	return $value1 if ( defined($value1) && !$error1 ) ;
	return $value2 if ( defined($value2) && !$error2 ) ;
	die join(" and ",$error1,$error2) if ( $error1 and $error2 ) ;
	return undef ;
}

sub _procstatus
{
	my ( $pid , $key , $field ) = @_ ;
	die "no /proc" if ! -d "/proc" ;
	return undef if ! -e "/proc/$pid" ;
	my $result ;
	my $fh = new FileHandle( "/proc/$pid/status" ) ; # no die
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		my ( $k , $rid , $eid , $sid ) = split( /\s+/ , $line ) ;
		if( $k eq "$key:" )
		{
			$result = $rid if( $key eq "Uid" && $field == 1 ) ;
			$result = $eid if( $key eq "Uid" && $field == 2 ) ;
			$result = $sid if( $key eq "Uid" && $field == 3 ) ;
			$result = $rid if( $key eq "Gid" && $field == 1 ) ;
			$result = $eid if( $key eq "Gid" && $field == 2 ) ;
		}
	}
	return $result ;
}

sub _psstatus
{
	my ( $pid , $key , $field ) = @_ ;
	die "invalid pid for ps -p" if ( !$pid || int($pid) <= 0 || int($pid) ne $pid ) ;
	my $cmd = "ps -p \"$pid\" -o pid,ruid,uid,svuid,gid,rgid,svgid" ;
	my $fh = new FileHandle( "$cmd 2>&1 |" ) or die ;
	my $header = <$fh> ;
	my $line = <$fh> ;
	die "no \"ps -p\"" if ! ( $header =~ m/^\s*PID\s/ ) ; # busybox
	return undef if ( !defined($header) || !defined($line) ) ;
	chomp( $line ) ;
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
	return _pstatus($pid,"Uid",2) ;
}

sub effectiveGroup
{
	# Returns the calling process's effective group id.
	my ( $pid ) = @_ ;
	return _pstatus($pid,"Gid",2) ;
}

sub realUser
{
	# Returns the calling process's real user id.
	my ( $pid ) = @_ ;
	return _pstatus($pid,"Uid",1) ;
}

sub realGroup
{
	# Returns the calling process's group id.
	my ( $pid ) = @_ ;
	return _pstatus($pid,"Gid",1) ;
}

sub savedUser
{
	# Returns the calling process's saved user id.
	my ( $pid ) = @_ ;
	return _pstatus($pid,"Uid",3) ;
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

sub killAll
{
	# Kills a set of processes without any retry shenanigans.
	my ( @pids ) = @_ ;
	for my $pid ( @pids )
	{
		if( defined($pid) && $pid > 1 )
		{
			kill( 15 , $pid ) ;
		}
	}
}

sub _kill1
{
	my ( $pid ) = @_ ;
	kill( 15 , int($pid) ) if( int($pid) > 0 ) ;
}

sub _kill2
{
	my ( $pid ) = @_ ;
	# in case started with sudo
	system( "sudo -n kill \"".int($pid)."\" 2>/dev/null" ) if( unix() && int($pid) > 0 ) ;
}

sub _kill3
{
	my ( $pid ) = @_ ;
	system( "taskkill /T /F /PID $pid >NUL 2>&1" ) if( windows() && int($pid) > 0 ) ;
}

sub kill_
{
	my ( $pid ) = @_ ;
	return if( int($pid) <= 0 ) ;
	$pid = int($pid) ;
	my $try = 1 ;
	waitFor( sub {
		_kill1($pid) ;
		_kill2($pid) if $try > 1 ;
		_kill3($pid) if $try > 1 ;
		$try++ ;
		return !processIsRunning($pid) ;
	} , "pid [$pid] to be killed" ) ;
}

sub killOnce
{
	# Does a simple kill on the process, without waiting for it or
	# checking whether it is killed. This might be preferred if the
	# process becomes a zombie since in that case processIsRunning()
	# will continue to return true.
	my ( $pid ) = @_ ;
	return if( int($pid) <= 0 ) ;
	$pid = int($pid) ;
	kill 15 , $pid ;
}

sub processIsRunning
{
	my ( $pid ) = @_ ;
	return 0 if int($pid) <= 0 ;
	$pid = int($pid) ;
	if( unix() )
	{
		# use ps rather than kill(0) because permissions
		my $uid = _pstatus( $pid , "Uid" , 1 ) ;
		return defined($uid) ? 1 : 0 ;
	}
	else
	{
		my $fh = new FileHandle( "tasklist /FI \"PID eq $pid\" /FO csv /NH 2>NUL |" ) ;
		$fh or die "tasklist error" ;
		while(<$fh>)
		{
			chomp( my $line = $_ ) ;
			my ( $f_name , $f_pid ) = split( "," , $line ) ;
			return ( $f_pid eq "\"$pid\"" ? 1 : 0 ) ;
		}
	}
	return 0 ;
}

sub nextPort
{
	# Returns the next port number in sequence. The implementation
	# uses a state file, which is created if necessary with a random
	# port number. O/s file locking is used to avoid races on the
	# state file contents.
	my $first = 16000 ;
	my $last = 32000 ;
	my $file = ".tmp.port" ;
	my $fh ;
	my $old_mask = umask 0 ;
	$fh = new FileHandle( $file , "a" , 0666 ) ;
	umask $old_mask ;
	$fh->close() if $fh ;
	for( my $i = 0 ; $i < 5 ; $i++ )
	{
		$fh = new FileHandle( $file , "r+" ) ;
		last if $fh ;
		sleep_cs( 1 ) ;
	}
	$fh or die "cannot lock: $!" ;
	flock( $fh , Fcntl::LOCK_EX ) or die "cannot lock: $!" ;
	my $line = $fh->getline() ;
	my $port = int($line) || ( $first + int(rand($last-$first)) ) ;
	$port = $port >= $last ? $first : ($port+1) ;
	seek( $fh , 0 , Fcntl::SEEK_SET ) or die "cannot seek: $!" ;
	$fh->print( "$port\n" ) or die ;
	$fh->close() or die ;
	return $port ;
}

sub edit
{
	# Edits one or more files by applying a text substitution line by line.
	my ( $glob , $re_from , $to ) = @_ ;
	for my $path ( glob_($glob) )
	{
		my $tmp = $path . "." . time() . ".tmp" ;
		my $fh_in = new FileHandle( $path , "r" ) or die "cannot open $path" ;
		my $fh_out = new FileHandle( $tmp , "w" ) or die "cannot open $tmp" ;
		while(<$fh_in>)
		{
			my $line = $_ ;
			my $nl = ( $line =~ m/\n$/ ) ? "\n" : "" ;
			chop( $line ) if $nl ;
			my $cr = ( $line =~ m/\r$/ ) ? "\r" : "" ;
			chop( $line ) if $cr ;
			$line =~ s/$re_from/$to/ ;
			print $fh_out $line , $cr , $nl ;
		}
		$fh_in->close() ;
		$fh_out->close() or die ;
		rename( $tmp , $path ) or die ;
	}
}

1 ;

