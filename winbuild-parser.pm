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
# AutoMakeParser.pm
#
# Parser package (AutoMakeParser) for parsing automake makefiles,
# with full variable expansion and support for conditional sections.
#
# Synopsis:
#
#  my $makefile = new AutoMakeParser( "Makefile.am" , { FOO => 1 , BAR => 0 } , { A => 'aaa' , B => 'bbb' } ) ;
#  $makefile->path() ;
#  $makefile->keys() ;
#  $makefile->value("some_VAR") ;
#  $makefile->subdirs() ;
#  $makefile->programs() ;
#  $makefile->libraries() ;
#  $makefile->includes() ;
#  $makefile->definitions() ;
#  $makefile->sources('foo') ;
#  $makefile->our_libs('foo') ;
#  $makefile->sys_libs('foo') ;
#

use strict ;
use FileHandle ;
use File::Basename ;
use File::Find ;
use File::Path ;

package AutoMakeParser ;

our $verbose = 0 ;

sub new
{
	my ( $classname , $path , $switches , $ro_vars ) = @_ ;
	my %me = (
		m_path => simplepath($path) ,
		m_lines => [] ,
		m_vars => {} ,
		m_switches => $switches ,
		m_stack => [] ,
	) ;
	my $this = bless \%me , $classname ;
	$this->read( $path ) ;
	$this->parse( $path ) ;
	$this->expand_all( $ro_vars ) ;
	return $this ;
}

sub path
{
	return $_[0]->{m_path}
}

sub top
{
	my ( $this ) = @_ ;
	my $depth = scalar(split("/",$this->{m_path})) - 1 ;
	die if $depth < 0 ;
	return simplepath( "../" x $depth ) ;
}

sub value
{
	my ( $this , $key ) = @_ ;
	my $v = $this->{m_vars}->{$key} ;
	return wantarray ? split(' ',$v) : $v ;
}

sub keys
{
	my ( $this ) = @_ ;
	my @k = sort keys %{$this->{m_vars}} ;
	return wantarray ? @k : join(" ",@k) ;
}

sub programs
{
	my ( $this ) = @_ ;
	return map { $this->value($_) } grep { m/_PROGRAMS$/ } $this->keys() ;
}

sub libraries
{
	my ( $this ) = @_ ;
	return map { $this->value($_) } grep { m/_LIBRARIES$/ } $this->keys() ;
}

sub subdirs
{
	my ( $this ) = @_ ;
	return $this->value( "SUBDIRS" ) ;
}

sub our_libs
{
	my ( $this , $program ) = @_ ;
	( my $prefix = $program ) =~ s/[-.]/_/g ;
	return
		map { my $x = File::Basename::basename($_) ; $x =~ s/^lib// ; $x =~ s/\.a$// ; $x }
		grep { my $x = File::Basename::basename($_) ; $x =~ m/^lib.*\.a$/ }
		$this->value( "${prefix}_LDADD" ) ;
}

sub sys_libs
{
	my ( $this , $program ) = @_ ;
	( my $prefix = $program ) =~ s/[-.]/_/g ;
	return
		map { s/-l// ; $_ }
		grep { m/^-l/ }
		$this->value( "${prefix}_LDADD" ) ;
}

sub sources
{
	my ( $this , $target ) = @_ ;
	( my $prefix = $target ) =~ s/[-.]/_/g ;
	return
		grep { m/\.c[p]{0,2}$/ }
		$this->value( "${prefix}_SOURCES" ) ;
}

sub definitions
{
	my ( $this , $var ) = @_ ;
	$var ||= "AM_CPPFLAGS" ;
	my $s = protect_quoted_spaces( simple_spaces( $this->{m_vars}->{$var} ) ) ;
	$s =~ s/-D /-D/g ;
	return
		map { s/\t/ /g ; $_ }
		map { s/-D// ; $_ }
		grep { m/-D\S+/ }
		split( " " , $s ) ;
}

sub includes
{
	my ( $this , $top , $var ) = @_ ;
	$top ||= "" ;
	$var ||= "AM_CPPFLAGS" ;
	my $s = protect_quoted_spaces( simple_spaces( $this->{m_vars}->{$var} ) ) ;
	$s =~ s/-I /-I/g ;
	return
		map { simplepath($_) }
		map { join("/",$top,$_) }
		map { s/\t/ /g ; $_ }
		map { s:-I:: ; $_ } grep { m/-I\S+/ }
		split( " " , $s ) ;
}

# --

sub vars
{
	return $_[0]->{m_vars}
}

sub simplepath
{
	my ( @parts ) = @_ ;
	return join( "/" , grep { $_ ne "" && $_ ne "." } split("/",join("/",@parts)) ) ;
}

sub simple_spaces
{
	my ( $s ) = @_ ;
	$s =~ s/\s/ /g ;
	$s =~ s/^ *// ;
	$s =~ s/ *$// ;
	return $s ;
}

sub protect_quoted_spaces
{
	my ( $s , $tab ) = @_ ;
	if( $s =~ m/"/ )
	{
		my @x = split( /"/ , $s ) ;
		if( $s =~ m/"$/ ) { push @x , "" }
		for( my $i = 0 ; $i < scalar(@x) ; $i++ )
		{
			if( ($i%2) == 1 ) { $x[$i] =~ s/ /$tab/g }
		}
		$s = join( '"' , @x ) ;
	}
	return $s ;
}

sub size
{
	my ( $this ) = @_ ;
	return scalar(@{$this->{m_lines}}) ;
}

sub empty
{
	my ( $this ) = @_ ;
	return $this->size() == 0 ;
}

sub lastline
{
	my ( $this ) = @_ ;
	return ${$this->{m_lines}}[-1] ;
}

sub continued
{
	my ( $this ) = @_ ;
	return !$this->empty() && $this->lastline() =~ m/\\\s*$/ ;
}

sub continue
{
	my ( $this , $line ) = @_ ;
	${$this->{m_lines}}[-1] =~ s/\\\s*$// ;
	${$this->{m_lines}}[-1] .= $line ;
}

sub add
{
	my ( $this , $n , $line ) = @_ ;
	while( $this->size() < ($n-3) ) { push @{$this->{m_lines}} , "" }
	push @{$this->{m_lines}} , $line ;
}

sub read
{
	my ( $this , $path ) = @_ ;
	my $fh = new FileHandle( $path ) or die "error: cannot open automake file: [$path]\n" ;
	my $n = 1 ;
	while(<$fh>)
	{
		chomp( my $line = $_ ) ;
		$this->continued() ? $this->continue($line) : $this->add($n,$line) ;
		$n++ ;
	}
}

sub parse
{
	my ( $this ) = @_ ;
	my @handlers = (
		[ qr/^\s*$/ , \&AutoMakeParser::do_blank ] ,
		[ qr/^\s*#/ , \&AutoMakeParser::do_comment ] ,
		[ qr/^\s*(\S+)\s*\+=\s*(.*)/ , \&AutoMakeParser::do_assign_more ] ,
		[ qr/^\s*(\S+)\s*(=|\?=|:=|::=)\s*(.*)/ , \&AutoMakeParser::do_assign ] ,
		[ qr/^\s*if\s+(\S+)/ , \&AutoMakeParser::do_if ] ,
		[ qr/^\s*else\s*$/ , \&AutoMakeParser::do_else ] ,
		[ qr/^\s*endif\s*$/ , \&AutoMakeParser::do_endif ] ,
	) ;
	my $n = 0 ;
	for my $line ( @{$this->{m_lines}} )
	{
		$n++ ;
		for my $h ( @handlers )
		{
			my ( $hre , $hfn ) = @$h ;
			if( $line =~ $hre )
			{
				&{$hfn}( $this , $n , $line , $1 , $2 , $3 , $4 , $5 , $6 ) ;
				last ; # (new)
			}
		}
		debug_( "$$this{m_path}($n): " , $this->enabled() ? $line : "..." ) ;
	}
	for my $k ( sort keys %{$this->{m_vars}} )
	{
		my $v = $this->{m_vars}->{$k} ;
		debug_( "$$this{m_path}: var: [$k] = [$v]" ) ;
	}
}

sub enabled
{
	my ( $this ) = @_ ;
	my $all = scalar( @{$this->{m_stack}} ) ;
	my $on = scalar( grep { $_ == 1 } @{$this->{m_stack}} ) ;
	return $all == $on ;
}

sub expand_all
{
	my ( $this , $ro_vars ) = @_ ;
	for my $k ( sort keys %{$this->{m_vars}} )
	{
		$this->expand( $k , $ro_vars ) ;
	}
}

sub expand
{
	my ( $this , $k , $ro_vars ) = @_ ;
	my $v = $this->{m_vars}->{$k} ;
	my $vv = $this->expansion( $v , $ro_vars , $k ) ;
	if( $v ne $vv )
	{
		debug_( "$$this{m_path}: expansion: [$k]..." ) ;
		debug_( "$$this{m_path}: expansion:   [$v]" ) ;
		debug_( "$$this{m_path}: expansion:   [$vv]" ) ;
	}
	$this->{m_vars}->{$k} = $vv ;
}

sub expansion
{
	my ( $this , $v , $ro_vars , $context ) = @_ ;
	while(1)
	{
		my ( $kk ) = ( $v =~ m/\$\(([^)]+)\)/ ) ;
		my $pre = $` ;
		my $post = $' ;
		return $v if !defined($kk) ;
		die "error: $$this{m_path}: $context: no value for expansion of [$kk] in [$v]\n" if( !exists( $this->{m_vars}->{$kk} ) && !exists( $ro_vars->{$kk} ) ) ;
		my $vv = exists($this->{m_vars}->{$kk}) ? $this->{m_vars}->{$kk} : $ro_vars->{$kk} ;
		$v = $pre . $vv . $post ;
	}
}

sub do_blank
{
	my ( $this , $n , $line ) = @_ ;
}

sub do_comment
{
	my ( $this , $n , $line ) = @_ ;
}

sub do_if
{
	my ( $this , $n , $line , $switch ) = @_ ;
	my $value = ( exists $this->{m_switches}->{$switch} && $this->{m_switches}->{$switch} ) ? 1 : 0 ;
	push @{$this->{m_stack}} , $value ;
}

sub do_else
{
	my ( $this , $n , $line ) = @_ ;
	die if scalar(@{$this->{m_stack}}) == 0 ;
	${$this->{m_stack}}[-1] = ${$this->{m_stack}}[-1] == 1 ? 0 : 1 ;
}

sub do_endif
{
	my ( $this , $n , $line ) = @_ ;
	pop @{$this->{m_stack}} ;
}

sub do_assign
{
	my ( $this , $n , $line , $lhs , $eq , $rhs ) = @_ ;
	if( $this->enabled() )
	{
		$rhs =~ s/\s+/ /g ;
		$rhs =~ s/^\s*// ;
		$rhs =~ s/\s*$// ;
		# TODO if $eq is "?="
		$this->{m_vars}->{$lhs} = $rhs ;
	}
}

sub do_assign_more
{
	my ( $this , $n , $line , $lhs , $rhs ) = @_ ;
	if( $this->enabled() )
	{
		$rhs =~ s/\s+/ /g ;
		$rhs =~ s/^\s*// ;
		$rhs =~ s/\s*$// ;
		$this->{m_vars}->{$lhs} = join( " " , $this->{m_vars}->{$lhs} , $rhs ) ;
	}
}

sub debug_
{
	print @_ , "\n" if $verbose ;
}

1 ;
