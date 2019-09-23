#!/usr/bin/perl
#
# Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Openssl.pm
#
# A package for creating a set of inter-linked certificate files using the "openssl"
# utility. The package also provides methods for running "openssl s_client" and
# "openssl s_server", designed for use in automated test scripts.
#
# The constructor creates cryptographic files for a cast of actors. The actual
# filenames are modified by the caller's function, so that all files can be
# put into one directory and/or given unique names.
#
# Cast of actors:
#
#      Alice    Bob    Malory("Alice")
#        ^       ^       ^
#        |       |       |
#      Carol    Dave     |
#        ^       ^       |
#         \     /        |
#          Trent       Trudy
#           ^  |        ^  |
#           +--+        +--+
#
# Each actor has at least a ".key" file for their private key and a ".csr"
# file for their certificate, so for example, the filename for Alice's key
# is file("alice.key"). The file() method is overloaded to create concatenated
# files, so a new file is created by calling file("alice.key","alice.csr").
#
# Every filename generated is remembered in the object so that the cleanup()
# method can be used to delete them all in one go.
#
# Recall that a certificate is the public key being certified, plus the CNAME
# (ie. domain) for which it is certified, plus the CA's signature; the CA's
# signature being the hash of the key+CNAME, encrypted with the CA's private
# key. The certificate allows the message recipient to verify that the public
# key used to validate the signature on the message is itself valid, according
# to the CA. The message recipient just has to hash the certificate public key
# and associated CNAME, and also decrypt the certificate's CA signature with
# the CA's public key, and then compare the two. Of course, the CA's public
# key can be verified in the same way, resulting in a chain of trust.
#
# Note that certificate chains must be ordered from leaf to root.
#
# Synopsis:
#
#	Openssl::available() or die ;
#	$Openssl::keep = 0 ;
#	$Openssl::openssl = Openssl::search("/usr/local/bin") ;
#	my $openssl = new Openssl( sub{"/tmp/$$.".$_[0]} ) ;
#	$openssl->createActors() ;
#	my $alice_pem = $openssl->file("alice.key","alice.crt") ;
#	my $ca_file = $openssl->file({"ca.crt"=>["carol.crt","trent.crt"]}) ;
#	$openssl->selfcert( "trudy" , "Trudy" ) ;
#	$openssl->genkey( "malory" ) ;
#	$openssl->sign( "carol" , "Carol" , "trent" ) ;
#	$openssl->runClient( "localhost:10025" , "sclient.out" , $pem , $ca_file , sub{exit(0)} ) ;
#	$openssl->runServer( 10025 , "sserver.out" , $cert , $ca_file , sub {my $pid=$_[0];kill($pid)} ) ;
#	$openssl->readActors() or $openssl->createActors() ;
#	$openssl->cleanup() ;
#

use strict ;
use FileHandle ;
use File::Basename ;
use Carp ;
use System ;

package Openssl ;
our $keep = 0 ;
our $openssl = "openssl" ;

sub available
{
	# Returns true if the "openssl" tool is available.
	return _available( $openssl ) ;
}

sub _available
{
	my ( $tool ) = @_ ;
	my $fh = new FileHandle( "$tool errstr 0 2>&1 |" ) ;
	my $result = 0 ;
	if( defined($fh) )
	{
		my $line = <$fh> ;
		$result = 1 if( $line && ( $line =~ m/^error:0/ ) ) ;
	}
	return $result ;
}

sub search
{
	# Searches for the "openssl" tool in the given directory or
	# on the PATH or in other likely places and returns a value
	# that can be assigned to "$openssl".

	my ( $dir0 ) = @_ ;
	my @dirs = System::windows() ? ( "" , "c:/program files/git/mingw64/bin" ) : ( "" ) ;
	unshift @dirs , $dir0 if defined($dir0) ;
	for my $dir ( @dirs )
	{
		my $tool = System::mangledpath( System::exe($dir,"openssl") ) ;
		$tool = "\"$tool\"" if( $tool =~ m/ / ) ;
		if( _available($tool) )
		{
			return $tool ;
		}
	}
	return System::exe("openssl") ;
}

sub new
{
	# Constructor. Use createActors() or readActors() as required.
	# The filename function can be used to modify the filenames
	# of any created file. The logging function is used for
	# runServer() and runClient() logging.

	my ( $classname , $fn_filename , $fn_log ) = @_ ;

	defined($fn_filename) or die ;
	my $this = bless {
		m_fn_filename => $fn_filename ,
		m_fn_log => $fn_log ,
		m_serial => {} ,
		m_paths => {} ,
		m_cleanup => [] ,
	} , $classname ;
	return $this ;
}

sub createActors
{
	# Creates the standard cast of actors:
	# Alice -> Carol -> Trent, Bob -> Dave -> Trent,
	# and Malory -> Trudy.

	my ( $this ) = @_ ;

	$this->selfcert( "trent" , "Trent" ) ;
	$this->selfcert( "trudy" , "Trudy" ) ;
	$this->genkey( "alice" ) ;
	$this->genkey( "bob" ) ;
	$this->genkey( "carol" ) ;
	$this->genkey( "dave" ) ;
	$this->genkey( "malory" ) ;
	$this->sign( "carol" , "Carol" , "trent" , 1 ) ;
	$this->sign( "alice" , "Alice" , "carol" ) ;
	$this->sign( "dave" , "Dave" , "trent" , 1 ) ;
	$this->sign( "bob" , "Bob" , "dave" ) ;
	$this->sign( "malory" , "Alice" , "trudy" ) ;
	return $this ;
}

sub readActors
{
	# Reads the pre-prepared cast of actors, without requiring
	# the openssl tool.

	my ( $this ) = @_ ;
	for my $name ( qw( alice bob carol dave malory trent trudy ) )
	{
		$this->_add_path( "$name.key" ) ;
		$this->_add_path( "$name.crt" ) ;
	}
	return $this ;
}

sub createActorsIn
{
	# Prepares the cast of actors in files that can be read by
	# readActors().

	my ( $dir ) = @_ ;
	my $openssl = new Openssl( sub{"$dir/".$_[0]} ) ;
	$openssl->createActors() ;
	my %keep = () ;
	map { $keep{"$_.key"}=$keep{"$_.crt"}=1 } qw( alice bob carol dave malory trent trudy ) ;
	@{$openssl->{m_cleanup}} = grep { ! exists($keep{File::Basename::basename($_)}) } @{$openssl->{m_cleanup}} ;
	$openssl->cleanup() ;
}

sub file
{
	# Finds-or-creates a file that is a simple concatenation of the
	# given files, and returns the full filesystem path.
	#
	# my $path = $x->file( "one" , "two" , "three" ) ;
	# my $path = $x->file( {one_two_three=>["one","two","three"]} ) ;
	#
	# If the list override is used, ie. with no name for the created
	# file, then a filename will be constructed from mangling
	# the names of the component files.
	#
	# The "find-or-create" functionality refers to the previous
	# use of file() for the given filename, without reference
	# to the filesystem.

	my ( $this , $fname_or_href , @fnames ) = @_ ;

	# interpret usage as (@fnames) or ({targetname=>[@fnames]})
	my $targetname ;
	if( ref($fname_or_href) )
	{
		my $href = $fname_or_href ;
		die if( scalar(@fnames) != 0 or scalar(keys %$href) != 1 ) ;
		( $targetname ) = %$href ;
		@fnames = @{$href->{$targetname}} ;
	}
	else
	{
		unshift @fnames , $fname_or_href ;
		$targetname = join( "+" , map {my $x=$_;$x =~ s/\./-/g;$x} @fnames ) . ".pem" ;
	}

	# find-or-create the target file
	if( exists($this->{m_paths}->{$targetname}) )
	{
		return $this->{m_paths}->{$targetname} ;
	}
	else
	{
		my $path_out = $this->_new_path( $targetname ) ;
		for my $fname ( @fnames )
		{
			my $path_in = $this->_old_path( $fname ) ;
			$this->_concatenate( $path_in , $path_out ) ;
		}
		return $path_out ;
	}
}

sub sign
{
	# Creates a certificate file ("<user>.crt") that is signed by the CA's
	# secret key ("<ca>.pem") to bind the user's (public) key ("<user>.key")
	# with their cname. The statefulness of the CA is held in a "<ca>.serial"
	# file.

	my ( $this , $user_basename , $user_cname , $ca_basename , $user_is_ca ) = @_ ;

	my $ca_pem = $this->file( "$ca_basename.key" , "$ca_basename.crt" ) ;
	my $user_key = $this->_old_path( "$user_basename.key" ) ;

	# generate the user's csr file
	my $user_csr = $this->_new_path( "$user_basename.csr" ) ;
	my $req_cmd = "$openssl req -new -key $user_key -batch -subj /CN=$user_cname -out $user_csr" ;
	$this->_run( $req_cmd ) ;
	$this->_check( $user_csr ) ;

	# generate the ca's serial-number file
	my $ca_serial ;
	if( exists($this->{m_serial}->{$ca_pem}) )
	{
		$ca_serial = $this->{m_serial}->{$ca_pem} ;
	}
	else
	{
		$ca_serial = $this->_new_path( "$ca_basename.serial" ) ;
		$this->{m_serial}->{$ca_pem} = $ca_serial ;
	}

	# create an extensions file -- see "man x509v3_config"
	my $extfile ;
	if( $user_is_ca )
	{
		$extfile = $this->_new_path( "$user_basename.ext" ) ;
		my $fh = new FileHandle( $extfile , "w" ) or die ;
		print $fh "basicConstraints=CA:TRUE\n" ;
	}

	# generate the user's certificate file
	my $user_cert = $this->_new_path( "$user_basename.crt" ) ;
	my $days = 30000 ; # expiry time in days
	my $x509_cmd = "$openssl x509 -req -days $days -in $user_csr -CA $ca_pem -CAserial $ca_serial -CAcreateserial -out $user_cert" ;
	$x509_cmd .= " -extfile $extfile" if defined($extfile) ;
	$this->_run( $x509_cmd ) ;
	$this->_check( $user_cert ) ;
}

sub selfcert
{
	# Creates a key and self-signed certificate file ("<basename>.key" and "<basename>.crt").
	# Typically used for root CAs.

	my ( $this , $basename , $cname ) = @_ ;
	my $crt_file = $this->_new_path( "$basename.crt" ) ;
	my $key_file = $this->_new_path( "$basename.key" ) ;
	my $days = 30000 ; # expiry time in days
	$this->_run( "$openssl req -x509 -newkey rsa:2048 -days $days -subj /CN=$cname -nodes -out $crt_file -keyout $key_file" ) ;
	$this->_check( $crt_file , $key_file ) ;
}

sub genkey
{
	# Creates a private key file ("<basename>.key").

	my ( $this , $basename ) = @_ ;

	my $key_file = $this->_new_path( "$basename.key" ) ;
	$this->_run( "$openssl genrsa -out $key_file 2048" ) ;
	$this->_check( $key_file ) ;
	$this->_add_cleanable( $key_file ) ;
}

sub runClient
{
	# Runs "openssl s_client -cert <cert> -CAfile <ca_file> -connect <peer>".
	# Requires a completion callback that terminates the server, otherwise
	# the client will never exit. Logs each line of output using the
	# log function supplied to the ctor, and also logs to file. The
	# path of the log file is returned.

	my ( $this , $peer , $logfile , $cert , $ca_file , $on_completion ) = @_ ;

	my $line ;
	my $cmd = "$openssl s_client -tls1 -msg -starttls smtp -crlf -connect $peer -showcerts" ;
	$cmd .= " -cert $cert" if defined($cert) ;
	$cmd .= " -CAfile $ca_file" if defined($ca_file) ;
	$cmd .= " -verify 10" ; # failure is not fatal -- look for "verify error:" near top of s_client log

	return $this->_run_logged( $cmd , $logfile , "s_client" , qr{^SSL-Session:} , qr{^---} , $on_completion ) ;
}

sub runServer
{
	# Runs "openssl s_server -cert <cert> -CAfile <ca_file> -port <port>".
	# Requires a completion callback that will kill the server; the server
	# pid is passed as a parameter. Logs each line of output using the
	# log function supplied to the ctor, and also logs to file. The
	# path of the log file is returned.

	my ( $this , $port , $logfile , $cert , $ca_file , $on_completion ) = @_ ;

	my $cmd = "$openssl s_server -tls1 -msg -crlf -accept $port" ;
	$cmd .= " -cert $cert" if defined($cert) ;
	$cmd .= " -CAfile $ca_file" if defined($ca_file) ;
	$cmd .= " -Verify 99" ;

	return $this->_run_logged( $cmd , $logfile , "s_server" , qr{^ERROR|^>>> TLS.*Handshake.*Finished} , undef , $on_completion ) ;
}

sub parseLog
{
	# Parses a runClient() log file. Can also work on a runServer() log but yielding
	# less information.

	my ( $this , $logfile ) = @_ ;

	my $result = {} ;
	my $fh = new FileHandle( $logfile ) or die ;
	my $state = 0 ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		my $connected = ( $line =~ m/^CONNECTED/ ) ;
		my $verify_error = ( $line =~ m/^verify error:/ ) ;
		my ( $verify_return_code ) = ( $line =~ m/Verify return code: (.*)/ ) ;
		my $ca_names_none = ( $line =~ m/^No client certificate CA names sent/ ) ;
		my $ca_names_begin = ( $line =~ m/^Acceptable client certificate CA names/ ) ;
		my $ca_names_end = ( $line =~ m/^---/ ) ;
		my $server_certificate_begin = ( $line =~ m/^Server certificate/ ) ;
		my $server_certificate = ( $line =~ m/^subject|^issuer/ ) ;
		my $server_certificate_end = ( $line =~ m/^---/ ) ;

		if( $state == 0 )
		{
			if( $connected ) { $result->{connected} = 1 }
			if( $verify_error ) { $result->{verify_error} = 1 }
			if( $ca_names_none ) { $result->{ca_names} = undef }
			if( $ca_names_begin ) { $state = 1 ; $result->{ca_names} = [] }
			if( $server_certificate_begin ) { $state = 2 ; $result->{server_certificate} = [] }
			if( defined($verify_return_code) ) { $result->{verify_return_code} = $verify_return_code }
		}
		elsif( $state == 1 && $ca_names_end )
		{
			$state = 0 ;
		}
		elsif( $state == 1 )
		{
			push @{$result->{ca_names}} , $line ;
		}
		elsif( $state == 2 && $server_certificate_end )
		{
			$state = 0 ;
		}
		elsif( $state == 2 )
		{
			push @{$result->{server_certificate}} , $line ;
		}
	}
	return $result ;
}

sub cleanup
{
	# Deletes all files created by this object.

	my ( $this ) = @_ ;
	if( !$keep )
	{
		for my $file ( @{$this->{m_cleanup}} )
		{
			$this->_log( "deleting [$file]" ) ;
			unlink( $file ) ;
		}
	}
}

# ==

sub _new_path
{
	my ( $this , $filename ) = @_ ;
	my $fn = $this->{m_fn_filename} ;
	my $path = &{$fn}( $filename ) ;
	if( -f $path ) { die "error: file already exists: [$path]" }
	$this->{m_paths}->{$filename} = $path ;
	$this->_add_cleanable( $path ) ;
	return $path ;
}

sub _add_path
{
	my ( $this , $filename ) = @_ ;
	my $fn = $this->{m_fn_filename} ;
	my $path = &{$fn}( $filename ) ;
	if( ! -f $path ) { die "error: no such file: [$path]" }
	$this->{m_paths}->{$filename} = $path ;
	return $path ;
}

sub _old_path
{
	my ( $this , $filename ) = @_ ;
	if( !exists($this->{m_paths}->{$filename}) ) { Carp::croak "error: no path for filename [$filename]" }
	my $path = $this->{m_paths}->{$filename} ;
	-f $path or die "error: no such file [$path] ($filename)" ;
	return $path ;
}

sub _add_cleanable
{
	my ( $this , @files ) = @_ ;
	for my $file ( @files )
	{
		push @{$this->{m_cleanup}} , $file ;
	}
}

sub _concatenate
{
	my ( $this , $file_in , $file_target ) = @_ ;
	my $fh_in = new FileHandle( $file_in , "r" ) or Carp::croak "failed to open [$file_in]" ;
	my $fh_out = new FileHandle( $file_target , "a" ) or die "error: failed to create [$file_target]" ;
	while(<$fh_in>)
	{
		print $fh_out $_ ;
	}
	$fh_in->close() or die ;
	$fh_out->close() or die ;
}

my $_generator = 0 ;
sub _run
{
	my ( $this , $cmd ) = @_ ;
	my $i = $_generator++ ;
	my $tmp = $this->_new_path( sprintf("openssl%02d.out",$i) ) ;
	system( "$cmd >$tmp 2>&1" ) ;
}

sub _check
{
	my ( $this , @files ) = @_ ;
	for my $file ( @files )
	{
		-f $file or die "failed to create [$file]" ;
		my $fh = new FileHandle( $file , "r" ) or die "failed to open [$file]" ;
		my $line = <$fh> ;
		$line or die "failed to read line from [$file]" ;
		$fh->close() ;
	}
}

sub _run_logged
{
	# Runs the given openssl command (in practice s_server or s_client) with its output
	# piped back to this perl script. The output is logged via the logging function
	# and also logged to file. The output is also searched for an end marker,
	# or a pair of end markers; the second end marker has to occur after the first.
	# Once the second end marker is seen the command is considered to have done its
	# work and the on-completion callback is called. The callback will typically
	# try to terminate the peer or kill the openssl process directly.

	my ( $this , $cmd , $log_file , $log_prefix , $end_match_1 , $end_match_2 , $on_completion ) = @_ ;

	$this->_log( "[$cmd]" ) ;
	my $fh_log = new FileHandle( $log_file , "w" ) or die ;
	push @{$this->{m_cleanup}} , $log_file ;

	my $ending ;
	my $pid = open( FH , "$cmd 2>&1 |" ) ;
	while(<FH>)
	{
		chomp( my $line = $_ ) ;
		print $fh_log $line , "\n" ;
		$this->_log( "$log_prefix: [$line]" ) ;
		my $match_1 = ( $line =~ m/$end_match_1/ ) ;
		my $match_2 = defined($end_match_2) && ( $line =~ m/$end_match_2/ ) ;
		last if ( $match_1 && !defined($end_match_2) ) ;
		last if ( $ending && $match_2 ) ;
		$ending = 1 if( $match_1 ) ;
	}
	$fh_log->close() ;
	&{$on_completion}($pid) if defined($on_completion) ;
	close(FH) ; # may block until the peer goes away
}

sub _log
{
	my ( $this , $line ) = @_ ;
	if( defined($this->{m_fn_log}) )
	{
		my $fn = $this->{m_fn_log} ;
		&{$fn}( $line ) ;
	}
}

1 ;
