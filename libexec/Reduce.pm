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
# Reduce.pm
#
# Does trial builds in order to identify unused functions in a
# given source file; emits the results to a data file; uses
# the data file to edit the source file and comment-out those
# functions.
#
# Synposis:
#   use Reduce ;
#   my $reduce = new Reduce( ['make foo','make bar'] , {quiet=>1,debug=>1} ) ;
#   $reduce->check( "foo.cpp" ) ;
#   $reduce->check( "bar.cpp" ) ;
#   $reduce->emit( new FileHandle("reduce.dat","w") ) ;
#   #later...
#   $reduce = new Reduce( undef , {quiet=>1} ) ;
#   $reduce->read( "reduce.dat" ) ;
#   $reduce->edit( "foo.cpp" , "foo.cpp.new" ) && rename("foo.cpp.new","foo.cpp") or die ;
#   $reduce->edit( "bar.cpp" , "bar.cpp.new" ) && rename("bar.cpp.new","bar.cpp") or die ;
#
# The results file is tab-separated with these columns:
#   1. <basename> (eg. gstr.cpp)
#   2. <line-number>
#   3. {keep|remove}
#   4. c++ signature
#

use strict ;
use Carp ;
use File::Basename ;
use Functions ;

package Reduce ;

sub new
{
	my ( $classname , $make_commands , $opt ) = @_ ;
	if(!defined($make_commands)) { $make_commands = ["make -C .."] }
	my $this = bless {
		m_data => {} ,
		m_verbose => ( $opt->{quiet} ? 0 : 1 ) ,
		m_comment_out => "" ,
		m_ifndef => "#ifndef G_LIB_SMALL" ,
		m_endif => "#endif" ,
		m_make_commands => $make_commands ,
		m_debug => $opt->{debug} ,
	} , $classname ;
	return $this ;
}

sub basenames
{
	# Returns the basenames from read()/check().
	my ( $this ) = @_ ;
	return keys %{$this->{m_data}} ;
}

sub read
{
	# Reads in the reduce file.
	my ( $this , $reduce_file ) = @_ ;
	return if !$reduce_file ;
	$this->_log( "reduce: reading reduce file [$reduce_file]" ) ;
	my $fh = new FileHandle( $reduce_file ) or Carp::croak( "reduce: error: cannot open the reduce file [$reduce_file]: $!\n" ) ;
	my $current_basename ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		next if $line =~ m/^\s*$/ ;
		next if $line =~ m/^#/ ;
		my ( $r_basename , $r_sigline , $r_action , $r_sig ) = split( "\t" , $line ) ;
		if( $r_basename ne $current_basename )
		{
			$current_basename = $r_basename ;
			$this->{m_data}->{$r_basename} = {} ;
		}
		$this->_log( "reduce: [$r_basename] [$r_sig]" ) ;
		$this->{m_data}->{$r_basename}->{$r_sig} = [$r_action,$r_sigline] ;
	}
	return $this ;
}

sub check
{
	# Checks the source file for unused functions and accumulates the results
	# for emit(). Optionally keeps the edits so that emit() and edit() are
	# not needed.
	my ( $this , $file_in , $keep_edits ) = @_ ;

	my $basename = File::Basename::basename( $file_in ) ;
	$this->{m_data}->{$basename} = {} ;

	# make a working copy
	my $current = "/tmp/reduce-$basename.tmp" ;
	File::Copy::copy( $file_in , $current ) or Carp::croak( "reduce: error: cannot make a working copy of [$file_in]: $!" ) ;

	# find all the functions
	my @sigs = () ;
	{
		my $fh = new FileHandle( $current ) or die ;
		my $fn = new Functions( $fh , sub { if($_[1]==1){push @sigs,[$_[0]->sigline(),$_[0]->sig()]} } ) ;
		$fn->process() ;
		$fh->close() ;
	}

	$this->_log( "reduce: $basename: found " , scalar(@sigs) , " function" , (scalar(@sigs)==1?"":"s") ) ;

	# for each function...
	my $remove_count = 0 ;
	for my $sigpair ( @sigs )
	{
		my ( $sigline , $sig ) = @$sigpair ;
		$this->_log( {start=>1} , "reduce: $basename($sigline): testing without "._namepad($sig,$sigline) ) ;

		# disable the function -- write to $file_in (sic)
		my $sigline_new = $this->_disable_function( $current , $file_in , $sig ) or die ;
		if( !defined($sigline_new) ) { Carp::croak( "reduce: error: failed to comment-out [$sig]" ) }

		# do the trial build
		my $ok = $this->_build( sub{File::Copy::copy($current,$file_in)} ) ;

		# record the result
		if( $ok )
		{
			$this->_log( {end=>1} , ".. good build: remove fn (keep the edit)" ) ;
			$remove_count++ ;
			$this->{m_data}->{$basename}->{$sig} = ["remove",$sigline] ;

		}
		else
		{
			$this->_log( {end=>1} , ".. bad build: keep fn (revert the edit)" ) ;
			$this->{m_data}->{$basename}->{$sig} = ["keep",$sigline] ;
		}

		# restore the file or keep the edits
		if( $ok && $keep_edits )
		{
			File::Copy::copy( $file_in , $current ) or die ;
		}
		else
		{
			File::Copy::copy( $current , $file_in ) or die ;
		}
	}
	unlink( $current ) ;

	my $function_count = scalar(@sigs) ;
	if( $function_count > 0 && $function_count == $remove_count )
	{
		print "reduce: $basename: warning: all functions removed\n" ;
		return 0 ;
	}
	else
	{
		print "reduce: $basename: removed $remove_count/$function_count functions\n" ;
		return 1 ;
	}
}

sub _disable_function
{
	my ( $this , $file_in , $file_out , $sig_to_edit ) = @_ ;
	my $fh_in = new FileHandle( $file_in ) or die ;
	my $fh_out = new FileHandle( $file_out , "w" ) or die ;
	my $fn = new Functions( $fh_in , sub { _disable_function_callback($this,$sig_to_edit,$fh_out,@_) } ) ;
	my $result = $fn->process() ;
	$fh_in->close() or die ;
	$fh_out->close() or die ;
	return $result ;
}

sub _disable_function_callback
{
	my ( $this , $sig_to_edit , $fh_out , $fn , $state , $line , $result_ref ) = @_ ;
	if( $fn->sig() eq $sig_to_edit )
	{
		my $ifndef = $this->{m_ifndef} ;
		my $endif = $this->{m_endif} ;
		my $comment_out = $this->{m_comment_out} ;

		if( $state == 0 )
		{
			print $fh_out $line , "\n" ;
		}
		elsif( $state == 1 )
		{
			$$result_ref = $fn->sigline() ;
			print $fh_out $ifndef , "\n" if $ifndef ;
			print $fh_out $comment_out , $line , "\n" ;
		}
		elsif( $state == 5 || $state == 33 )
		{
			print $fh_out $comment_out , $line , "\n" ;
			print $fh_out $endif , "\n" if $endif ;
		}
		else
		{
			print $fh_out $comment_out , $line , "\n" ;
		}
	}
	else
	{
		print $fh_out $line , "\n" ;
	}
}

sub _by_sigline
{
	my ( $this , $basename , $sig1 , $sig2 ) = @_ ;
	my ( $action1 , $sigline1 ) = @{$this->{m_data}->{$basename}->{$sig1}} ;
	my ( $action2 , $sigline2 ) = @{$this->{m_data}->{$basename}->{$sig2}} ;
	return $sigline1 <=> $sigline2 ;
}

sub emit
{
	my ( $this , $fh_out ) = @_ ;

	for my $basename ( sort keys %{$this->{m_data}} )
	{
		for my $sig ( sort {_by_sigline($this,$basename,$a,$b)} keys %{$this->{m_data}->{$basename}} )
		{
			my ( $action , $sigline ) = @{$this->{m_data}->{$basename}->{$sig}} ;
			print $fh_out join("\t",$basename,$sigline,$action,$sig) , "\n" ;
		}
	}
}

sub edit
{
	# Edits the source file to comment-out unused functions as per the reduce file.
	my ( $this , $file_in , $file_out , $basename ) = @_ ;

	$basename ||= File::Basename::basename( $file_in ) ;

	if( exists($this->{m_data}->{$basename}) )
	{
		$this->_log( "reduce: reducing [$basename]" ) ;
		my $fh_in = new FileHandle( $file_in ) or Carp::confess( "cannot open input" ) ;
		my $fh_out = new FileHandle( $file_out , "w" ) or Carp::confess( "cannot open output" ) ;
		my $fn = new Functions( $fh_in , sub { $this->_edit_callback($basename,$fh_out,@_) } ) ;
		my $result__ignored = $fn->process() ;
		$fh_in->close() or die ;
		$fh_out->close() or Carp::confess( "cannot write output" ) ;
		return 1 ;
	}
	else
	{
		$this->_log( "reduce: no reduce record for [$basename]" ) ;
		return 0 ;
	}
}

sub _edit_callback
{
	my ( $this , $basename , $fh_out , $fn , $state , $line , $result_ref ) = @_ ;

	my $ifndef = $this->{m_ifndef} ;
	my $endif = $this->{m_endif} ;
	my $comment_out = $this->{m_comment_out} ;

	my $sig = $fn->sig() ;
	die if( $state > 0 && !defined($sig) ) ;

	my $data = $this->{m_data}->{$basename} ;
	my ( $action , $sigline ) = ( "" , 0 ) ;
	if( $data && defined($sig) && $data->{$sig} )
	{
		( $action , $sigline ) = @{$data->{$sig}} ;
		die unless ( $action eq "remove" || $action eq "keep" ) ;
	}

	if( $action eq "remove" )
	{
		if( $state == 0 )
		{
			print $fh_out $line , "\n" ;
		}
		elsif( $state == 1 )
		{
			$this->_debug( "reducing [$basename]: commenting-out [$sig]" ) ;
			$$result_ref = $fn->sigline() ;
			print $fh_out $ifndef , "\n" if $ifndef ;
			print $fh_out $comment_out , $line , "\n" ;
		}
		elsif( $state == 5 || $state == 33 )
		{
			print $fh_out $comment_out , $line , "\n" ;
			print $fh_out $endif , "\n" if $endif ;
		}
		else
		{
			print $fh_out $comment_out , $line , "\n" ;
		}
	}
	else
	{
		print $fh_out $line , "\n" ;
	}
}

sub _build
{
	my ( $this , $exit_fn ) = @_ ;
	for my $make ( @{$this->{m_make_commands}} )
	{
		my $rc = $this->{m_debug} ? system( $make ) : system( "$make >/dev/null 2>/dev/null" ) ;
		if( $? & 127 ) { &$exit_fn() if $exit_fn ; die "system() interrupted" } ;
		if( $rc != 0 ) { return 0 }
	}
	return 1 ;
}

sub _namepad
{
	my ( $sig , $sigline ) = @_ ;
	my ( $n ) = ( $sig =~ m;([A-Za-z0-9_:]*)\(; ) ;
	my $max = 30 ;
	if( !$n ) { $n = substr( $sig , 0 , $max ) }
	$n = substr( $n , 0 , $max ) ;
	my $pad = '.' x ($max+5-(length($n)+length($sigline))) ;
	return "[$n]$pad" ;
}

sub _log
{
	my ( $this , @args ) = @_ ;
	my $cfg = {} ;
	if( ref($args[0]) ) { $cfg = shift @args }

	if( $this->{m_verbose} )
	{
		print @args , ( $cfg->{start} ? "" : "\n" ) ;
	}
}

sub _debug
{
}

1 ;
