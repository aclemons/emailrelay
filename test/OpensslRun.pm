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
# OpensslRun.pm
#
# Runs "openssl s_client" or "openssl s_server".
#
# Synopsis:
#
#	use OpensslRun ;
#	$OpensslRun::openssl = $Openssl::openssl ;
#	$OpensslRun::client_options = "..." ;
#	$OpensslRun::server_options = "..." ;
#	$OpensslRun::log_line_fn = sub { print "[$_[0]]\n" } ;
#	$OpensslRun::log_run_fn = sub { print "running [$_[0]]\n" } ;
#	OpensslRun::runClient(
#		"127.0.0.1:9999" ,
#		"client.out" ,
#		"/etc/ssl/certs/cert.pem" ,
#		"/etc/ssl/certs/ca.pem" ,
#		sub { print "client done\n" ; } ) ;
#	my $info = $OpensslRun::parseLog( "client.out" ) ;
#	OpensslRun::runServer(
#		9999 ,
#		"server.out" ,
#		"/etc/ssl/certs/cert.pem" ,
#		"/etc/ssl/certs/ca.pem" ,
#		sub { print "server done\n" } ) ;
#

use strict ;
use FileHandle ;

package OpensslRun ;
our $openssl = "openssl" ;
our $client_options = "" ;
our $server_options = "" ;
our $log_line_fn = sub {} ;
our $log_run_fn = sub {} ;

sub runClient
{
	# Runs "openssl s_client -cert <cert> -CAfile <ca_file> -connect <peer>".
	# Requires a completion callback that terminates the server, otherwise
	# the client will never exit. Logs each line of output using the
	# supplied log function, and also logs to file.

	my ( $peer , $logfile , $cert , $ca_file , $on_completion ) = @_ ;

	my $end_match_1 = qr{^New.*Cipher is } ;
	my $end_match_2 = qr{^---} ;
	my $error_match = qr{^ERROR} ;

	my $cmd = "$openssl s_client -4 -state -msg -debug -starttls smtp -crlf -connect $peer -showcerts $client_options" ;
	$cmd .= " -cert $cert" if defined($cert) ;
	$cmd .= " -CAfile $ca_file" if defined($ca_file) ;
	$cmd .= " -verify 10" ; # failure is not fatal -- look for "verify error:" near top of s_client log

	_log_run( "runClient" , $cmd , $logfile ) ;
	return _run( $cmd , $logfile , $end_match_1 , $end_match_2 , $error_match , $on_completion ) ;
}

sub runClientSecureMatch
{
	return qr{^New.*Cipher is } ;
}

sub runServer
{
	# Runs "openssl s_server -cert <cert> -CAfile <ca_file> -port <port>".
	# Requires a completion callback that will kill the server; the server
	# pid is passed as a parameter. Logs each line of output using the
	# global log function and also logs to the specified file.

	my ( $port , $logfile , $cert , $ca_file , $on_completion , $windows ) = @_ ;

	my $end_match_1 = qr{Handshake.*, Finished} ;
	my $end_match_2 = qr{CIPHER is |^SSL_accept:\S+ read finished A} ;
	my $error_match = runServerErrorMatch() ;

	my $cmd = "$openssl s_server -4 -state -msg -debug -crlf -accept $port $server_options" ;
	$cmd .= " -cert $cert" if defined($cert) ;
	$cmd .= " -CAfile $ca_file" if defined($ca_file) ;
	$cmd .= " -Verify 99" ;
	###$cmd .= " </dev/tty" unless $windows ; # otherwise with "make -j" the s_server terminates immediately because stdin is eof

	_log_run( "runServer" , $cmd , $logfile ) ;
	return _run( $cmd , $logfile , $end_match_1 , $end_match_2 , $error_match , $on_completion ) ;
}

sub runServerErrorMatch
{
	return qr{^ERROR|unable.to.bind|Alert.*fatal|error:.* alert } ;
}

sub parseLog
{
	# Parses a runClient() log file. Can also work on a runServer() log but yielding
	# less information.

	my ( $logfile ) = @_ ;

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

sub _run
{
	# Runs the given openssl command (in practice s_server or s_client) with its output
	# piped back to this perl script. The output is logged via the logging function
	# and also logged to file. The output is also searched for an end marker,
	# or a pair of end markers; the second end marker has to occur after the first.
	# Once the second end marker is seen the command is considered to have done its
	# work and the on-completion callback is called. The callback will typically
	# try to terminate the peer or kill the openssl process directly.

	my ( $cmd , $log_file , $end_match_1 , $end_match_2 , $error_match , $on_completion ) = @_ ;

	my $fh_log = new FileHandle( $log_file , "w" ) or die ;
	$fh_log->autoflush() ;

	# run the command -- the cmd must not have any special shell operators
	# otherwise it will trigger perl's use of "sh -c" and we will get the
	# shell's pid and not the s_server's
	my $pid = open( my $fh , "$cmd 2>&1 |" ) ;
	my $ended ;
	my $matched_first ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		print $fh_log $line , "\n" ;
		_log_line( $line ) ;
		my $e = ( $line =~ m/$error_match/ ) ;
		my $m1 = ( $line =~ m/$end_match_1/ ) ;
		my $m2 = defined($end_match_2) && ( $line =~ m/$end_match_2/ ) ;
		if( $e )
		{
			last ;
		}
		elsif( $m1 && !defined($end_match_2) )
		{
			$ended = 1 ;
			last ;
		}
		elsif( $matched_first && $m2 )
		{
			$ended = 1 ;
			last ;
		}
		$matched_first = 1 if( $m1 ) ;
	}
	$fh_log->close() ;
	&$on_completion($pid) if defined($on_completion) ;
	close($fh) ; # may block until the peer goes away
	return $ended ;
}

sub _log_line
{
	my ( $line ) = @_ ;
	&$log_line_fn( $line ) ;
}

sub _log_run
{
	my ( $run_what , $cmd , $logfile ) = @_ ;
	&$log_run_fn( $cmd , $logfile ) ;
}

1 ;
