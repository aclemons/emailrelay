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
# Openssl.pm
#
# Provides methods for running the "openssl" tool to create private keys,
# self-signed certificates etc.
#
# Synopsis:
#	use Openssl ;
#	use OpensslFileStore ;
#	$Openssl::openssl = Openssl::search() ;
#	Openssl::available() or die ;
#	$openssl = new Openssl( new OpensslFileStore(".") ) ;
#	$openssl->selfcert( "trent" , "Trent" ) ;
#	$openssl->genkey( "alice" ) ;
#	$openssl->sign( "alice" , "Alice" , "trent" ) ;
#	$openssl->cleanup() ;
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
# See also: OpensslRun, OpensslCast, OpensslFileStore
#

use strict ;
use FileHandle ;
use File::Basename ;
use Carp ;

package Openssl ;
our $openssl = "openssl" ;
our $log_fn = sub {} ;

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
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		$result = 1 if( $line && ( $line =~ m/^error:0000/ ) ) ;
	}
	return $result ;
}

sub search
{
	# Searches for the "openssl" tool in the given directory or
	# on the PATH or in other likely places and returns a value
	# that can be assigned to "$openssl".
	my ( $dir0 , $windows ) = @_ ;

	my @dirs = () ;
	if( defined($dir0) )
	{
		if( ! -d $dir0 ) { die "invalid directory for the openssl tool [$dir0]\n" }
		push @dirs , $dir0 ;
	}
	else
	{
		push @dirs , "" ; # PATH
		push @dirs , "c:/program files/git/mingw64/bin" if $windows ;
	}
	for my $dir ( @dirs )
	{
		my $tool = $dir ? "$dir/openssl" : "openssl" ;
		$tool .= ".exe" if $windows ;
		$tool =~ s;/;\\;g if $windows ;
		$tool = "\"$tool\"" if( $tool =~ m/ / ) ;
		if( _available($tool) )
		{
			return $tool ;
		}
	}
	die "no openssl tool in [$dir0]" if defined($dir0) ;
	return $windows ? "openssl.exe" : "openssl" ;
}

sub new
{
	# Constructor taking a FileStore instance.
	my ( $classname , $fs ) = @_ ;
	my $this = bless {
		m_fs => $fs ,
	} , $classname ;
	return $this ;
}

sub sign
{
	# Creates a certificate file ("<user>.crt") that is signed by the CA's
	# secret key ("<ca>.pem") to bind the user's (public) key ("<user>.key")
	# with their cname. The statefulness of the CA is held in a "<ca>.serial"
	# file.
	my ( $this , $user_basename , $user_cname , $ca_basename , $user_is_ca ) = @_ ;

	my $ca_pem = $this->{m_fs}->catfile( "$ca_basename.key" , "$ca_basename.crt" ) ;
	my $user_key = $this->{m_fs}->infile( "$user_basename.key" ) ;

	# generate the user's csr file
	my $user_csr = $this->{m_fs}->tmpfile( "$user_basename.csr" ) ;
	my $req_cmd = "$openssl req -new -key $user_key -batch -subj /CN=$user_cname -out $user_csr" ;
	$this->_run( $req_cmd ) ;
	$this->_check( $user_csr ) ;

	# create an extensions file -- see "man x509v3_config"
	my $extfile ;
	if( $user_is_ca )
	{
		$extfile = $this->{m_fs}->tmpfile( "$user_basename.ext" ) ;
		my $fh = new FileHandle( $extfile , "w" ) or die ;
		print $fh "basicConstraints=CA:TRUE\n" ;
		$fh->close() or die ;
	}

	# generate the user's certificate file
	my $ca_serial = $this->{m_fs}->tmpfile( "$ca_basename.serial" ) ;
	my $user_cert = $this->{m_fs}->outfile( "$user_basename.crt" ) ;
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
	my $crt_file = $this->{m_fs}->outfile( "$basename.crt" ) ;
	my $key_file = $this->{m_fs}->outfile( "$basename.key" ) ;
	my $days = 30000 ; # expiry time in days
	$this->_run( "$openssl req -x509 -newkey rsa:2048 -days $days -subj /CN=$cname -nodes -out $crt_file -keyout $key_file" ) ;
	$this->_check( $crt_file , $key_file ) ;
}

sub genkey
{
	# Creates a private key file ("<basename>.key").
	my ( $this , $basename ) = @_ ;
	my $key_file = $this->{m_fs}->outfile( "$basename.key" ) ;
	$this->_run( "$openssl genrsa -out $key_file 2048" ) ;
	$this->_check( $key_file ) ;
}

sub concatenate
{
	# Concatenates key and certificate files into a new pem file.
	my ( $this , @fnames ) = @_ ;
	return undef if !defined($fnames[0]) ;
	return $this->{m_fs}->catfile( @fnames ) ;
}

sub cleanup
{
	my ( $this ) = @_ ;
	return $this->{m_fs}->cleanup() ;
}

sub _run
{
	my ( $this , $cmd ) = @_ ;
	my $out = $this->{m_fs}->logfile() ;
	_log( $cmd , $out ) ;
	system( "$cmd >$out 2>&1" ) ;
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

sub _log
{
	my ( $cmd , $out ) = @_ ;
	&$log_fn( $cmd , $out ) ;
}

1 ;
