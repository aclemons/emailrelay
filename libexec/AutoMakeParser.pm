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
# AutoMakeParser.pm
#
# Parser package for parsing automake makefiles, with full variable
# expansion and support for conditional sections.
#
# Synopsis:
#
#  use AutoMakeParser ;
#  $AutoMakeParser::debug = 0 ;
#  my @makefiles = AutoMakeParser::readall( "." , { FOO => 1 , BAR => 0 } , { A => 'aaa' , B => 'bbb' } ) ;
#  my $makefile = new AutoMakeParser( "Makefile.am" , { FOO => 1 , BAR => 0 } , { A => 'aaa' , B => 'bbb' } ) ;
#  $makefile->path() ;
#  $makefile->keys_() ;
#  $makefile->value("some_VAR") ;
#  $makefile->subdirs() ;
#  $makefile->programs() ;
#  $makefile->libraries() ;
#  $makefile->includes() ;
#  $makefile->definitions() ;
#  $makefile->compile_options() ;
#  $makefile->sources('foo') ;
#  $makefile->our_libnames('foo') ;
#  $makefile->our_libdirs('foo') ;
#  $makefile->sys_libs('foo') ;
#
# Typical directories in a autoconf vpath build (see includes()):
#
#    project <-- $(top_srcdir)        <-------+-+ "base_to_top"
#       |                                     | |
#       +----src  <-- readall() base        --+ | <---+ base()
#       |     |                                 |     |
#       |     +-- sub1  <-- path()              | ----+
#       |                                       |
#       +----bin                                |
#       |                                       |
#       +--build <-- $(top_builddir)            |  <-- $(top_builddir)
#            |                                  |
#            +-- src                          --+  <--+ base()
#                 |                                   |
#                 +-- sub1  <-- c++ cwd         ------+
#
# See also ConfigStatus.pm.
#

use strict ;
use FileHandle ;
use Cwd ;
use Carp ;
use File::Basename ;

package AutoMakeParser ;
our $debug = 0 ;

sub new
{
	my ( $classname , $path , $switches , $vars_in ) = @_ ;
	my %me = (
		m_path => simplepath($path) ,
		m_switches => $switches ,
		m_vars => {} ,
		m_depth => undef , # from readall()
		m_lines => [] ,
		m_stack => [] ,
	) ;
	my $this = bless \%me , $classname ;
	$this->read( $path ) ;
	$this->parse( $path ) ;
	$this->expand_all( $vars_in ) ;
	$this->copy( $vars_in ) ;
	return $this ;
}

sub path
{
	return $_[0]->{m_path} ;
}

sub readall
{
	my ( $base_dir , $switches , $vars , $verbose , $verbose_prefix ) = @_ ;
	my @makefiles = () ;
	_readall_imp( \@makefiles , 0 , $base_dir , $switches , $vars , $verbose , $verbose_prefix ) ; # recursive
	return @makefiles ;
}

sub _readall_imp
{
	my ( $makefiles , $depth , $dir , $switches , $vars , $verbose , $verbose_prefix ) = @_ ;
	$verbose_prefix ||= "" ;
	my $m = new AutoMakeParser( "$dir/Makefile.am" , $switches , $vars ) ;
	$m->{m_depth} = $depth ;
	print "${verbose_prefix}makefile=[" , $m->path() , "] ($depth)\n" if $verbose ;
	push @$makefiles , $m ;
	for my $subdir ( $m->value("SUBDIRS") )
	{
		_readall_imp( $makefiles , $depth+1 , "$dir/$subdir" , $switches , $vars , $verbose , $verbose_prefix ) ;
	}
}

sub depth
{
	# Returns the readall() recursion depth.
	#
	my ( $this ) = @_ ;
	return $this->{m_depth} ;
}

sub base
{
	# Returns the relative path up to the first readall()
	# makefile. The returned value will be something like
	# "../../../". See also includes().
	#
	my ( $this ) = @_ ;
	my $depth = $this->{m_depth} ;
	Carp::confess("bad depth") if ( !defined($depth) || $depth < 0 ) ;
	return $depth == 0 ? "." : ( "../" x $depth ) ;
}

sub value
{
	my ( $this , $key ) = @_ ;
	my $v = $this->{m_vars}->{$key} ;
	return wantarray ? split(' ',$v) : $v ;
}

sub keys_
{
	my ( $this ) = @_ ;
	my @k = sort keys %{$this->{m_vars}} ;
	return wantarray ? @k : join(" ",@k) ;
}

sub programs
{
	my ( $this ) = @_ ;
	return map { $this->value($_) } grep { m/_PROGRAMS$/ } $this->keys_() ;
}

sub libraries
{
	my ( $this ) = @_ ;
	return map { $this->value($_) } grep { m/_LIBRARIES$/ } $this->keys_() ;
}

sub subdirs
{
	my ( $this ) = @_ ;
	return $this->value( "SUBDIRS" ) ;
}

sub our_libs_raw
{
	# eg. ("../foo/libbar.a",...)
	my ( $this , $program ) = @_ ;
	( my $prefix = $program ) =~ s/[-.]/_/g ;
	return
		grep { my $x = File::Basename::basename($_) ; $x =~ m/^lib.*\.a$/ }
		$this->value( "${prefix}_LDADD" ) ;
}

sub our_libnames
{
	# eg. ("bar",...)
	my ( $this , $program ) = @_ ;
	( my $prefix = $program ) =~ s/[-.]/_/g ;
	return
		map { my $x = File::Basename::basename($_) ; $x =~ s/^lib// ; $x =~ s/\.a$// ; $x }
		grep { my $x = File::Basename::basename($_) ; $x =~ m/^lib.*\.a$/ }
		$this->value( "${prefix}_LDADD" ) ;
}

sub our_libdirs
{
	# eg. libhere.a -> base/thisdir
	# eg. top-builddir/foo/libthere.a -> base/foo
	#
	my ( $this , $program , $base , $thisdir ) = @_ ;
	$base = "" if !defined($base) ;
	( my $prefix = $program ) =~ s/[-.]/_/g ;
	return
		map { simplepath(File::Basename::dirname(join("/",$base,$_))) }
		map { my $x = $_ ; if( $thisdir && ( $x !~ m:/: ) ) { $x = "$thisdir/$x" } ; $x }
		grep { my $x = File::Basename::basename($_) ; $x =~ m/^lib.*\.a$/ }
		$this->value( "${prefix}_LDADD" ) ;
}

sub sys_libs
{
	my ( $this , $program ) = @_ ;
	( my $prefix = $program ) =~ s/[-.]/_/g ;
	my @a =
		map { s/-l// ; $_ }
		grep { m/^-l/ }
		$this->value( "LIBS" ) ;
	my @b =
		map { s/-l// ; $_ }
		grep { m/^-l/ }
		$this->value( "${prefix}_LDADD" ) ;
	return ( @a , @b ) ;
}

sub sources
{
	my ( $this , $target ) = @_ ;
	( my $prefix = $target ) =~ s/[-.]/_/g ;
	return
		grep { m/\.c[p]{0,2}$/ }
		$this->value( "${prefix}_SOURCES" ) ;
}

sub link_options
{
	my ( $this ) = @_ ;
	my @a = $this->value( "AM_LDFLAGS" ) ;
	my @b = $this->value( "LDFLAGS" ) ;
	my @options = ( @a , @b ) ;
	return wantarray ? @options : join(" ",@options) ;
}

sub compile_options
{
	my ( $this ) = @_ ;
	my @a = $this->_compile_options_imp( "AM_CPPFLAGS" , $this->{m_vars} ) ;
	my @b = $this->_compile_options_imp( "CXXFLAGS" , $this->{m_vars} ) ;
	my @options = ( @a , @b ) ;
	return wantarray ? @options : join(" ",@options) ;
}

sub _compile_options_imp
{
	my ( $this , $var , $vars ) = @_ ;
	$vars ||= $this->{m_vars} ;
	my $s = protect_quoted_spaces( simple_spaces( $vars->{$var} ) ) ;
	$s =~ s/-D /-D/g ;
	$s =~ s/-I /-I/g ;
	return
		map { s/\t/ /g ; $_ }
		grep { !m/-I/ }
		grep { !m/-D/ }
		split( " " , $s ) ;
}

sub definitions
{
	my ( $this ) = @_ ;
	my @a = $this->_definitions_imp( "AM_CPPFLAGS" , $this->{m_vars} ) ;
	my @b = $this->_definitions_imp( "CXXFLAGS" , $this->{m_vars} ) ;
	my @defs = ( @a , @b ) ;
	return wantarray ? @defs : join(" ",@defs) ;
}

sub _definitions_imp
{
	my ( $this , $var , $vars ) = @_ ;
	my $s = protect_quoted_spaces( simple_spaces( $vars->{$var} ) ) ;
	$s =~ s/-D /-D/g ;
	return
		map { s/\t/ /g ; $_ }
		map { s/-D// ; $_ }
		grep { m/-D\S+/ }
		split( " " , $s ) ;
}

sub includes
{
	# Returns a list of include directories derived from the
	# AM_CPPFLAGS and CXXFLAGS macros. The returned list also
	# optionally starts with the autoconf header directory,
	# obtained by expanding "$(top_srcdir)/src".
	#
	# Include paths need to vary through the source tree,
	# so a 'base' parameter is provided here which is used
	# as a prefix for all relative paths from the AM_CPPFLAGS
	# and CXXFLAGS expansions and as a suffix for the
	# autoconf header directory.
	#
	# For example, if CXXFLAGS is "-I$(top_srcdir)/src/sub"
	# and top_srcdir is "." then includes(base()) will yield
	# ".././src/sub" for one makefile and "../.././src/sub"
	# for another.
	#
	# In practice the value for top_srcdir should be carefully
	# chosen as some "base-to-top" relative path that makes things
	# work correctly if readall() was not based at top_srcdir
	# or when targeting vpath builds. See above.
	#
	my ( $this , $base , $full_paths , $add_autoconf_dir ) = @_ ;
	$base ||= "" ;
	my $autoconf_dir = simplepath( join( "/" , $this->value("top_srcdir") , $base , "src" ) ) ;
	$autoconf_dir = $this->fullpath( $autoconf_dir ) if $full_paths ;
	my @a = $this->_includes_imp( $base , "AM_CPPFLAGS" , $this->{m_vars} , $full_paths ) ;
	my @b = $this->_includes_imp( $base , "CXXFLAGS" , $this->{m_vars} , $full_paths ) ;
	my @c = ( $autoconf_dir && $add_autoconf_dir ) ? ( $autoconf_dir ) : () ;
	my @incs = ( @c , @a , @b ) ;
	return wantarray ? @incs : join(" ",@incs) ;
}

sub _includes_imp
{
	my ( $this , $base , $var , $vars , $full_paths ) = @_ ;
	my $s = protect_quoted_spaces( simple_spaces( $vars->{$var} ) ) ;
	$s =~ s/-I /-I/g ;
	return
		map { $full_paths?$this->fullpath($_):$_ }
		map { simplepath($_) }
		map { my $p=$_ ; ($base&&($p!~m;^/;))?join("/",$base,$p):$p }
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
	my ( $path ) = @_ ;
	my $first = ( $path =~ m;^/; ) ? "/" : "" ;
	my @out = () ;
	my @split = grep {m/./} split( "/" , $path ) ;
	for my $x ( @split )
	{
		next if( $x eq "" || $x eq "." ) ;
		if( $x eq ".." && scalar(@out) && $out[-1] ne ".." )
		{
			pop @out ;
		}
		else
		{
			push @out , $x ;
		}
	}
	return $first . join( "/" , @out ) ;
}

sub fullpath
{
	my ( $this , $relpath ) = @_ ;
	return simplepath( join("/",File::Basename::dirname(Cwd::realpath($this->path())),$relpath) ) ;
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

sub copy
{
	my ( $this , $vars_in ) = @_ ;
	return if !defined($vars_in) ;
	for my $k ( keys %$vars_in )
	{
		if( !exists($this->{m_vars}->{$k}) )
		{
			$this->{m_vars}->{$k} = $vars_in->{$k} ;
		}
	}
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
		debug_( "$$this{m_path}($n): " , $line ) if ( $this->enabled() && ( $line =~ m/\S/ ) && ( $line !~ m/^#/ ) ) ;
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
	my $line = join( " " , @_ ) ;
	$line =~ s/ *\t */ /g ;
	print $line , "\n" if $debug ;
}

1 ;
