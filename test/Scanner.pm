#!/usr/bin/perl
#
# Copyright (C) 2001-2010 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# A wrapper for running the "emailrelay-test-scanner"
# program.
#

use strict ;
use Helper ;

package Scanner ;

sub new
{
	my $classname = shift ;
	return bless { h => new Helper( "scanner" , port() ) } , $classname ;
}

sub port { return 11011 }
sub log { return shift->{h}->log(@_) }
sub exe { return shift->{h}->exe(@_) }
sub run { return shift->{h}->run(@_) }
sub pid { return shift->{h}->pid(@_) }
sub kill { return shift->{h}->kill(@_) }
sub cleanup { return shift->{h}->cleanup(@_) }

1 ;
