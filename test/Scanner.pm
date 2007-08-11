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
# Scanner.pm
#

use strict ;
use FileHandle ;

package Scanner ;

our @pid_list = () ;
our $bin_dir = "." ;

sub new
{
	my ( $classname ) = @_ ;
	my %me = (
		m_logfile => ".tmp.scanner.out.$$" ,
		m_pidfile => ".tmp.scanner.pid.$$" ,
		m_pid => undef ,
	) ;
	my $this = bless \%me , $classname ;
	$this->_check() ;
	return $this ;
}

sub port
{
	return 10010 ;
}

sub log
{
	my ( $this ) = @_ ;
	return $this->{'m_logfile'} ;
}

sub _check
{
	my ( $this ) = @_ ;
	if( ! -x $this->exe() )
	{
		die "no scanner executable [".$this->exe()."]" ;
	}
}

sub exe
{
	my ( $this ) = @_ ;
	return "$bin_dir/emailrelay-test-scanner" ;
}

sub run
{
	my ( $this ) = @_ ;
	my $pidfile = $this->{'m_pidfile'} ;
	my $logfile = $this->{'m_logfile'} ;
	my $exe = $this->exe() ;
	system( "$exe --log --debug --pid-file $pidfile > $logfile 2>&1 &" ) ;
	for( my $i = 0 ; $i < 200 ; $i++ )
	{
		select( undef , undef , undef , 0.02 ) ;
		if( -f $pidfile ) { next }
	}
	my $f = new FileHandle("< $pidfile") ;
	my $line = <$f> ; chomp $line ;
	$this->{'m_pid'} = $line ;
	push @pid_list , $this->{'m_pid'} ;
}

sub pid
{
	my ( $this ) = @_ ;
	return $this->{'m_pid'} ;
}

sub _sleep_cs
{
	my ( $cs ) = @_ ;
	$cs = defined($cs) ? $cs : 1 ;
	select( undef , undef , undef , 0.01 * $cs ) ;
}

sub _wait
{
	# wait to die
	my ( $this , $timeout_cs ) = @_ ;
	for( my $i = 0 ; $i < $timeout_cs ; $i++ )
	{
		_sleep_cs() ;
		if( kill(0,$this->pid()) == 0 )
		{
			next
		}
	}
}

sub kill
{
	# kill and wait to die
	my ( $this , $signal , $timeout_cs ) = @_ ;
	$signal = defined($signal) ? $signal : 15 ;
	$timeout_cs = defined($timeout_cs) ? $timeout_cs : 100 ;
	kill( $signal , $this->pid() ) ;
	$this->_wait( $timeout_cs ) ;
}

sub cleanup
{
	my ( $this ) = @_ ;
	unlink( $this->{'m_pidfile'} ) ;
	unlink( $this->{'m_logfile'} ) ;
}

1 ;
