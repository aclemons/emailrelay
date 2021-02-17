#!/usr/bin/perl
#
# Copyright (C) 2001-2021 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# CompilationDatabase.pm
#
# Parses automake files throughout a source tree using AutoMakeParser
# and generates a "compilation database" file, "compile_commands.json".
#
# See also: https://clang.llvm.org/docs/JSONCompilationDatabase.html
#
# Synopsis:
#
#    use CompilationDatabase ;
#    my $cdb = new CompilationDatabase( $src_dir ,
#        ["-DFOO=BAR",...] , ["-I_TOP_"] , ["-Wall"] , {WINDOWS=>0,...} , {top_srcdir=>'..'} ) ;
#    my @files = $cdb->list() ;
#    my @stanzas = $cdb->stanzas() ;
#    $cdb->print() ;
#

use strict ;
use File::Basename ;
use AutoMakeParser ;

package CompilationDatabase ;
our $debug = 0 ;

sub new
{
	my ( $classname , $base_makefile_dir , $dflags , $iflags , $cxxflags , $switches , $ro_vars ) = @_ ;
	$AutoMakeParser::debug = 1 if $debug > 1 ;
	my %me = (
		m_base_dir => $base_makefile_dir ,
		m_dflags => $dflags ,
		m_iflags => $iflags ,
		m_cxxflags => $cxxflags ,
		m_switches => $switches ,
		m_ro_vars => $ro_vars ,
		m_full_paths => 0 ,
	) ;
	return bless \%me , $classname ;
}

sub list
{
	my ( $this ) = @_ ;

	my @list = () ;
	my $verbose = $debug ;
	my @makefiles = AutoMakeParser::readall( $this->{m_base_dir} , $this->{m_switches} , $this->{m_ro_vars} , $verbose ) ;
	for my $m ( @makefiles )
	{
		my $sub_dir = File::Basename::dirname( $m->path() ) ;
		for my $library ( $m->libraries() )
		{
			push @list , map { "$sub_dir/$_" } $m->sources($library) ;
		}
		for my $program ( $m->programs() )
		{
			push @list , map { "$sub_dir/$_" } $m->sources($program) ;
		}
	}
	return @list ;
}

sub print
{
	my ( $this ) = @_ ;
	print "[\n" ;
	print join( ",\n" , $this->stanzas() ) ;
	print "]\n" ;
}

sub stanzas
{
	my ( $this ) = @_ ;

	my @makefiles = AutoMakeParser::readall( $this->{m_base_dir} , $this->{m_switches} , $this->{m_ro_vars} ) ;
	my @output = () ;
	for my $m ( @makefiles )
	{
		my $dir = File::Basename::dirname( $m->path() ) ;
		my @includes = map { "-I$_" } $m->includes( $m->top() , undef , join(" ",@{$this->{m_iflags}}) , $this->{m_full_paths} ) ;
		my @definitions = map { "-D$_" } $m->definitions() ;
		push @definitions , @{$this->{m_dflags}} ;

		if( $debug )
		{
			print "cdb: makefile=" , $m->path() , "\n" ;
			print "cdb:  AM_CPPFLAGS=[",join("|",$m->value("AM_CPPFLAGS")),"]\n" ;
			print "cdb:  top=",$m->top(),"\n" ;
			print "cdb:  \@includes=" , join("|",@includes) , "\n" ;
			print "cdb:  \@definitions=" , join("|",@definitions) , "\n" ;
		}

		for my $library ( $m->libraries() )
		{
			map { push @output , $this->_stanza($dir,$_,\@includes,\@definitions) } $m->sources($library) ;
		}
		for my $program ( $m->programs() )
		{
			map { push @output , $this->_stanza($dir,$_,\@includes,\@definitions) } $m->sources($program) ;
		}
	}
	return @output ;
}

sub _stanza
{
	my ( $this , $dir , $source , $includes , $definitions ) = @_ ;

	my @all_flags = ( @{$this->{m_cxxflags}} , @$definitions , @$includes ) ;

	my $directory = Cwd::realpath( $dir ) ;
	$source = join("/",$directory,$source) if $this->{m_full_paths} ;

	my $command = "/usr/bin/c++ @all_flags -c $source" ;
	$command =~ s/"/\\"/g ;

	return
		"{\n" .
		" \"directory\" : \"$directory\" ,\n" .
		" \"command\" : \"$command\",\n" .
		" \"file\" : \"$source\" ,\n" .
		"}" ;
}

1 ;
