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
# SmtpClient.pm
#
# A network client for driving the smtp interface.
#

use strict ;
use FileHandle ;
use Net::Telnet ;

package SmtpClient ;

sub new
{
	my ( $classname , $port , $server ) = @_ ;

	$server = defined($server) ? $server : "localhost" ;
	$port = defined($port) ? $port : 10025 ;

	my $t = new Net::Telnet( Timeout=>15 , Prompt=>'/250 [^\r\n]+/' ) ;
	$t->binmode(0) ; # convert to '\r\n' on output

	my %me = (
		m_port => $port ,
		m_server => $server ,
		m_t => $t ,
	) ;
	return bless \%me , $classname ;
}

sub open
{
	# Opens the connection.
	my ( $this , $wait ) = @_ ;
	$wait = defined($wait) ? $wait : 1 ;
	my $t = $this->t() ;
	my $ok = $t->open( Host=>$this->server() , Port=>$this->port() ) ;
	my ($s1,$s2) = $t->waitfor( '/220 [^\r\n]+/' ) if $wait ;
	return $ok ;
}

sub close
{
	# Drops the connection.
	my ( $this ) = @_ ;
	$this->t()->close() ;
}

sub port { return shift->{'m_port'} }
sub server { return shift->{'m_server'} }
sub t { return shift->{'m_t'} }

sub ehlo
{
	# Says ehlo.
	my ( $this ) = @_ ;
	my $t = $this->t() ;
	$t->buffer_empty() ; # sync
	$t->cmd( "ehlo here" ) ;
}

sub mail
{
	# Says mail-from. Can optionally be expected to fail
	# with an authentication-require error message.
	my ( $this , $expect_mail_from_to_fail ) = @_ ;
	my $t = $this->t() ;
	if( $expect_mail_from_to_fail )
	{
		$t->cmd( String => "mail from:<me\@here>" , Prompt => '/530 authentication required/' ) ;
	}
	else
	{
		$t->cmd( "mail from:<me\@here>" ) ;
	}
}

sub submit_start
{
	# Starts message submission. See also
	# submit_line() and submit_end().
	my ( $this , $to , $expect_rcpt_to_to_fail ) = @_ ;
	$to ||= "you\@there" ;
	$expect_rcpt_to_to_fail ||= 0 ;
	my $t = $this->t() ;
	$t->buffer_empty() ; # sync
	$t->cmd( "ehlo here" ) ;
	$t->cmd( "mail from:<me\@here>" ) ;
	if( $expect_rcpt_to_to_fail )
	{
		$t->cmd( String => "rcpt to:<$to>" , Prompt => '/550 [^\r\n]+/' ) ;
	}
	else
	{
		$t->cmd( "rcpt to:<$to>" ) ;
		$t->cmd( String => "data" , Prompt => '/354 [^\r\n]+/' ) ;
		$t->print( "From: me\@here" ) ;
		$t->print( "To: you\@there" ) ;
		$t->print( "Subject: test message" ) ;
		$t->print( "" ) ;
	}
}

sub submit_end
{
	# Ends message submission by sending a dot.
	my ( $this , $expect_dot_to_fail ) = @_ ;
	$expect_dot_to_fail ||= 0 ;
	my $t = $this->t() ;
	if( $expect_dot_to_fail )
	{
		$t->cmd( String => "." , Prompt => '/452 [^\r\n]+/' ) ;
	}
	else
	{
		$t->cmd( "." ) ;
	}
}

sub submit_line
{
	# Sends a line of a submitted message.
	my ( $this , $line ) = @_ ;
	my $t = $this->t() ;
	$t->print( $line ) ;
}

sub submit
{
	# Submits a whole test message.
	my ( $this , $expect_dot_to_fail ) = @_ ;
	$this->submit_start() ;
	$this->submit_line( "This is a test." ) ;
	$this->submit_end( $expect_dot_to_fail ) ;
}

sub doBadHelo
{
	# Sends an invalid helo.
	my ( $this ) = @_ ;
	my $t = $this->t() ;
	$t->cmd( String => "HELO" , Prompt => '/501 parameter [^\r\n]+/' , Errmode => 'return' ) ;
	return $t->errmsg("") ;
}

sub doBadCommand
{
	# Sends an invalid 'foo' command.
	my ( $this ) = @_ ;
	my $t = $this->t() ;
	$t->cmd( String => "FOO" , Prompt => '/500 command [^\r\n]+/' , Errmode => 'return' ) ;
	return $t->errmsg("") ;
}

1 ;

