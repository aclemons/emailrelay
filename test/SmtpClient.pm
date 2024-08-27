#!/usr/bin/perl
#
# Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# A perl-only network client for driving the smtp interface.
#
# Synopsis:
#
#	my $sc = new SmtpClient( 10025 , "localhost" ) ;
#	$sc->open() or die ;
#	$sc->ehlo() ;
#	$sc->doBadHelo() ;
#	$sc->doBadCommand() ;
#	$sc->mail({expect_...=>0}) ; # mail-from
#	$sc->submit_start(["me","you"],{expect_...=>0}) ; # ehlo, mail-from, rcpt-to, content-headers
#	$sc->submit_line("testing 123") ;
#	$sc->submit_end({nowait=>0,with_quit=>0}) ; # or...
#   $sc->submit( ["me","you"] , {nowait=>0,with_quit=>0} ) ;
#	$sc->close() ;
#

use strict ;
use FileHandle ;
use NetClient ;
use System ;

package SmtpClient ;

sub new
{
	my ( $classname , $port , $server ) = @_ ;

	$port ||= 10025 ;
	$server ||= $System::localhost ;

	my %me = (
		m_port => $port ,
		m_server => $server ,
		m_nc => undef ,
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
	my ( $this , $wait220 ) = @_ ;
	$wait220 = $wait220->{wait220} if ref($wait220) eq "HASH" ;
	$wait220 = 1 if !defined($wait220) ;

	$this->{m_nc} = new NetClient( $this->{m_port} , $this->{m_server} , $this->{m_timeout} , $this->{m_prompt} ) ;
	return undef if !$this->{m_nc} ;
	$this->{m_nc}->read( qr/220 [^\n]+\n/ , -1 ) if $wait220 ;
	return 1 ;
}

sub close
{
	# Drops the connection.
	my ( $this ) = @_ ;
	$this->{m_nc}->close() ;
}

sub ehlo
{
	# Says ehlo.
	my ( $this ) = @_ ;
	$this->{m_nc}->cmd( "EHLO here" ) ;
}

sub mail
{
	# Says mail-from. Can optionally be expected to fail
	# with an authentication-require error message.
	my ( $this , $opt ) = @_ ;

	my $expect_mailfrom_failure = $opt->{expect_mailfrom_failure} ;

	if( $expect_mailfrom_failure )
	{
		$this->{m_nc}->send( "mail from:<me\@here>\r\n" , qr/530 authentication required[^\n]*\n/ ) ;
	}
	else
	{
		$this->{m_nc}->send( "mail from:<me\@here>\r\n" ) ;
	}
}

sub submit_start
{
	# Starts message submission. See also submit_line()
	# and submit_end().
	my ( $this , $to , $opt ) = @_ ;

	if( !defined($to) ) { $to = 'you@there' }
	my $expect_rcpt_to_failure = $opt->{expect_rcpt_to_failure} ;

	my @to_list = ref($to) ? @$to : ($to) ;
	$this->{m_nc}->cmd( "ehlo here" ) ;
	$this->{m_nc}->cmd( 'mail from:<me@here>' ) ;
	if( $expect_rcpt_to_failure )
	{
		my $rcpt_to = $to_list[0] ;
		$this->{m_nc}->cmd( "rcpt to:<$rcpt_to>" , qr/550 [^\n]+\n/ ) ;
	}
	else
	{
		for my $rcpt_to ( @to_list )
		{
			$this->{m_nc}->cmd( "rcpt to:<$rcpt_to>" ) ;
		}
		$this->{m_nc}->cmd( "data" , qr/354 [^\n]+\n/ ) ;
		$this->{m_nc}->send( "From: me\@here\r\n" ) ;
		$this->{m_nc}->send( "To: you\@there\r\n" ) ;
		$this->{m_nc}->send( "Subject: test message\r\n" ) ;
		$this->{m_nc}->send( "\r\n" ) ;
	}
	return $this ;
}

sub submit_end
{
	# Ends message submission by sending a dot.
	# Returns the SMTP response line or undef on error.
	my ( $this , $opt ) = @_ ;

	my $nowait = $opt->{nowait} ;
	my $with_quit = $opt->{with_quit} ; # if nowait

	if( $nowait )
	{
		if( $with_quit )
		{
			$this->{m_nc}->send( ".\r\nQUIT\r\n" ) ;
		}
		else
		{
			$this->{m_nc}->send( ".\r\n" ) ;
		}
	}
	else
	{
		return $this->{m_nc}->cmd( "." , qr/[^\n]*\n/ ) ;
	}
}

sub submit_line
{
	# Sends a body line for a submit_start()ed message.
	my ( $this , $line ) = @_ ;
	$line = "this is a test" if !defined $line ;
	$this->{m_nc}->send( "$line\r\n" ) ;
	return $this ;
}

sub submit
{
	# Submits a whole test message.
	my ( $this , $to_list , $opt ) = @_ ;
	$this->submit_start( $to_list ) ;
	$this->submit_line( "This is a test." ) ;
	return $this->submit_end( $opt ) ;
}

sub doBadHelo
{
	# Sends an invalid helo, expecing 501.
	my ( $this ) = @_ ;
	$this->{m_nc}->cmd( "HELO" , qr/501 parameter [^\n]+\n/ ) ;
}

sub doBadCommand
{
	# Sends an invalid 'foo' command, expecting 500.
	# Returns any 'peer disconnected' error message.
	my ( $this ) = @_ ;
	eval { $this->{m_nc}->cmd( "FOO" , qr/500 command [^\n]+\n/ ) } ;
	my $error = $@ =~ s/\n//gr =~ s/\r//gr ;
	return $error ;
}

1 ;

