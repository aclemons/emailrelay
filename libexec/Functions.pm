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
# Functions.pm
#
# Identifies functions in source code.
#
# Calls back with each line and a state variable: 0 for outside,
# 1 for the first line of the signature, 2 for any additional lines
# of the signature, 3 for the opening brace line or 33 for "=default;",
# 4 for the body, and 5 for the closing brace line.
#
# Synposis:
#  use Function ;
#  my $fh = new FileHandle("foo.cpp") ;
#  my $fn = new Functions( $fh , \&callback ) ;
#  $fn->process() ;
#  sub callback
#  {
#     my( $fn , $state , $line ) = @_ ;
#     my $sig = $fn->sig() ;
#     my $sigline = $fn->sigline() ;
#     ...
#  }
#

use strict ;

package Functions ;

sub new
{
	my ( $classname , $fh , $callback ) = @_ ;
	return bless {
		m_fh => $fh ,
		m_callback => $callback ,
		m_sig_lines => [] ,
		m_sig_line_number => 0 ,
	} , $classname ;
}

sub process
{
	my ( $this ) = @_ ;
	my $fh = $this->{m_fh} ;
	my $callback = $this->{m_callback} ;
	$this->{m_sig_line_number} = 0 ;
	my $line_number = 0 ;
	my $result ;
	my $in_body ;
	my $in_sig ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		$line_number++ ;

		if( $in_body && $line =~ m/^}\s*$/ )
		{
			&$callback( $this , 5 , $line , \$result ) ;
			$in_body = 0 ;
		}
		elsif( $in_body )
		{
			&$callback( $this , 4 , $line , \$result ) ;
		}
		elsif( $in_sig && ( $line =~ m/^{/ || $line =~ m/^=\s*default\s*;\s*$/ || $line =~ m/^\s*$/ ) )
		{
			my $is_false_sig = ( $line =~ m/^\s*$/ ) ; # eg. a static variable
			my $is_brace = ( $line =~ m/^{/ ) ;
			if( $is_false_sig )
			{
				for my $sl ( @{$this->{m_sig_lines}} )
				{
					my $rc = &$callback( $this , 0 , $sl , \$result ) ;
				}
				my $rc = &$callback( $this , 0 , "" , \$result ) ;
				$in_sig = 0 ;
				$in_body = 0 ;
			}
			else
			{
				my $_1or2 = 1 ;
				for my $sl ( @{$this->{m_sig_lines}} )
				{
					my $rc = &$callback( $this , $_1or2 , $sl , \$result ) ;
					$_1or2 = 2 ;
				}
				if( $is_brace )
				{
					&$callback( $this , 3 , $line , \$result ) ;
					$in_sig = 0 ;
					$in_body = 1 ;
				}
				else
				{
					&$callback( $this , 33 , $line , \$result ) ;
					$in_sig = 0 ;
					$in_body = 0 ;
				}
			}
		}
		elsif( $in_sig )
		{
			$line =~ s;//.*;; ;
			$line =~ s/\s*$// ;
			push @{$this->{m_sig_lines}} , $line ;
		}
		elsif( $line =~ m/^[A-Za-z]/ && !(
			$line =~ m/^namespace/ ||
			$line =~ m/^public:/ ||
			$line =~ m/^private:/ ||
			$line =~ m/^struct/ ||
			$line =~ m/^class/ ||
			$line =~ m/^extern/ ) )
		{
			$line =~ s/\s*$// ;
			@{$this->{m_sig_lines}} = ( $line ) ;
			$this->{m_sig_line_number} = $line_number ;
			$in_sig = 1 ;
		}
		else
		{
			&$callback( $this , 0 , $line , \$result ) ;
		}
	}
	return $result ;
}

sub sig
{
	my ( $this ) = @_ ;
	my $sig ;
	for my $s ( @{$this->{m_sig_lines}} )
	{
		my $sig_line = $s ; # deep copy!
		$sig_line =~ s/^\s*// ;
		$sig = $sig ? "$sig $sig_line" : $sig_line ;
	}
	return $sig ;
}

sub sigline
{
	my ( $this ) = @_ ;
	return $this->{m_sig_line_number} ;
}

1 ;
