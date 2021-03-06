#!/bin/sh
#
# Copyright (C) 2001-2020 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# Tests the E-MailRelay system using a sequence of perl tests.
#
# usage: emailrelay-test.sh [<perl-script-options>] [<test-name> ...]
#

# test for perl
perl -e "use Carp; use FileHandle; use Getopt::Std; use Net::Telnet; exit(99);" 2>/dev/null
rc=$?
if test $rc -ne 99
then
	echo `basename $0`: no perl or missing perl modules: for debian try installing libnet-telnet-perl or use cpan to install Net::Telnet >&2
	echo `basename $0`: succeeding trivially >&2
	exit 0
fi

# run the tests
if test -z "$srcdir" ; then srcdir="`cd \`dirname \"$0\"\` && pwd`" ; fi
perl -I"${srcdir}" "$srcdir/emailrelay-test.pl" "$@"
rc=$?

echo `basename $0`: done: $rc

if test $rc -eq 0
then
	rm -rf .tmp.*
fi

rm -f .emailrelay-test-server.pid
exit $rc
