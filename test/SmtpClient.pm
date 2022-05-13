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
# SmtpClient.pm
#
# A network client for driving the smtp interface.
#
# Synopsis:
#
#	my $sc = new SmtpClient( 10025 , "localhost" ) ;
#	$sc->open() or die ;
#	$sc->ehlo() ;
#	$sc->doBadHelo() ;
#	$sc->doBadCommand() ;
#	$sc->mail() ;
#	$sc->submit_start() ; # ehlo, mail-from, rcpt-to, headers
#	$sc->submit_line("testing 123") ;
#	$sc->submit_end() ; # or submit_end_nowait()
#	$sc->close() ;
#

use strict ;
use FileHandle ;
use NetClient ;

package SmtpClient ;

sub new
{
	my ( $classname , $port , $server ) = @_ ;

	$port ||= 10025 ;
	$server ||= $System::localhost ;

	my %me = (
		m_port => $port ,
		m_server => $server ,
		m_s => undef ,
		m_connect_timeout => 3 ,
		m_timeout => 3 ,
		m_prompt => qr/250 [^\n]*\n/ ,
	) ;
	return bless \%me , $classname ;
}

sub port { return shift->{'m_port'} }
sub server { return shift->{'m_server'} }

sub open
{
	# Opens the connection.
	my ( $this , $wait ) = @_ ;
	$wait = defined($wait) ? $wait : 1 ;

	$this->{m_s} = NetClient::newSocket( $this ) ;
	return undef if !$this->{m_s} ;
	NetClient::read( $this , qr/220 [^\n]+\n/ , -1 ) if $wait ;
	return 1 ;
}

sub close
{
	# Drops the connection.
	my ( $this ) = @_ ;
	$this->{m_s}->close() ;
	$this->{m_s} = undef ;
}

sub ehlo
{
	# Says ehlo.
	my ( $this ) = @_ ;
	NetClient::cmd( $this , "EHLO here" ) ;
}

sub mail
{
	# Says mail-from. Can optionally be expected to fail
	# with an authentication-require error message.
	my ( $this , $expect_mail_from_to_fail ) = @_ ;
	if( $expect_mail_from_to_fail )
	{
		NetClient::send( $this , "mail from:<me\@here>\r\n" , qr/530 authentication required[^\n]*\n/ ) ;
	}
	else
	{
		NetClient::send( $this , "mail from:<me\@here>\r\n" ) ;
	}
}

sub submit_start
{
	# Starts message submission. See also
	# submit_line() and submit_end().
	my ( $this , $to , $expect_rcpt_to_to_fail ) = @_ ;
	$to ||= "you\@there" ;
	$expect_rcpt_to_to_fail ||= 0 ;
	NetClient::cmd( $this , "ehlo here" ) ;
	NetClient::cmd( $this , "mail from:<me\@here>" ) ;
	if( $expect_rcpt_to_to_fail )
	{
		NetClient::cmd( $this , "rcpt to:<$to>" , qr/550 [^\n]+\n/ ) ;
	}
	else
	{
		NetClient::cmd( $this , "rcpt to:<$to>" ) ;
		NetClient::cmd( $this , "data" , qr/354 [^\n]+\n/ ) ;
		NetClient::send( $this , "From: me\@here\r\n" ) ;
		NetClient::send( $this , "To: you\@there\r\n" ) ;
		NetClient::send( $this , "Subject: test message\r\n" ) ;
		NetClient::send( $this , "\r\n" ) ;
	}
}

sub submit_end
{
	# Ends message submission by sending a dot.
	my ( $this , $expect_452 , $expect_552 ) = @_ ;
	my $prompt = undef ;
	if( $expect_452 ) { $prompt = qr/452 [^\n]+\n/ }
	if( $expect_552 ) { $prompt = qr/552 [^\n]+\n/ }
	return NetClient::cmd( $this , "." , $prompt ) ;
}

sub submit_end_nowait
{
	my ( $this , $with_quit ) = @_ ;
	NetClient::send( $this , $with_quit ? ".\r\nQUIT\r\n" : ".\r\n" ) ;
}

sub submit_line
{
	# Sends a body line for a submit_start()ed message.
	my ( $this , $line ) = @_ ;
	$line = "this is a test" if !defined $line ;
	NetClient::send( $this , "$line\r\n" ) ;
}

sub submit
{
	# Submits a whole test message.
	my ( $this , $expect_dot_to_fail ) = @_ ;
	$this->submit_start() ;
	$this->submit_line( "This is a test." ) ;
	$this->submit_end( $expect_dot_to_fail )
}

sub doBadHelo
{
	# Sends an invalid helo, expecing 501.
	my ( $this ) = @_ ;
	NetClient::cmd( $this , "HELO" , qr/501 parameter [^\n]+\n/ ) ;
}

sub doBadCommand
{
	# Sends an invalid 'foo' command, expecting 500.
	# Returns any 'peer disconnected' error message.
	my ( $this ) = @_ ;
	eval { NetClient::cmd( $this , "FOO" , qr/500 command [^\n]+\n/ ) } ;
	my $error = $@ =~ s/\n//gr =~ s/\r//gr ;
	return $error ;
}

1 ;

