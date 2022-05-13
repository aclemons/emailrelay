#!/bin/sh
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
# emailrelay-test.sh
#
# Tests the E-MailRelay system using "emailrelay-test.pl".
#
# usage: emailrelay-test.sh [-q] [<perl-script-options>] [<test-name> ...]
#

# test for perl
perl -e "use Carp; use FileHandle; use Getopt::Std; use IO::Socket; use IO::Select; exit(99);" 2>/dev/null
rc=$?
if test $rc -ne 99
then
	echo `basename $0`: no perl, or missing perl modules >&2
	exit 77 # for automake
fi

# parse the command-line
quiet=0
if test "$1" = "-q"
then
	# no shift - pass it on
	quiet=1
fi

# run the tests
if test -z "$srcdir" ; then srcdir="`cd \`dirname \"$0\"\` && pwd`" ; fi
chmod +x "$0" "$srcdir/emailrelay-test.pl" 2>/dev/null
if sh -c "true </dev/tty" 2>/dev/null
then
	# redirect stdin because "make -j" and "openssl s_server"
	perl -I"${srcdir}" "$srcdir/emailrelay-test.pl" "$@" </dev/tty
else
	perl -I"${srcdir}" "$srcdir/emailrelay-test.pl" "$@"
fi
rc=$?

if test "$quiet" -eq 1 -o "$rc" -eq 77 ; then : ; else
	echo `basename $0`: done: $rc
fi
exit $rc
