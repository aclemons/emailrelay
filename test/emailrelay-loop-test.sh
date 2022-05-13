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
# emailrelay-loop-test.sh
#
# Starts a polling E-MailRelay server that loops back to itself and runs forever.
#
# usage: emailrelay-loop-test.sh [-v] [<exe>]
#        -v  - use valgrind
#
# To get a good valgrind report make use of the admin 'terminate' command
# to quit the server cleanly. The admin port is recorded in the ".info" file.
#

# configure
thisdir="`cd \`dirname $0\` ; pwd`"
if test "$1" = "-v" ; then v="valgrind --leak-check=full --show-reachable=yes --num-callers=50 " ; shift ; fi
if test "$1" != "" ; then exe="$1" ; fi
if test "$1" = ""
then
	exe="$thisdir/../src/main/emailrelay"
	test -x "$exe" || exe="$thisdir/../emailrelay"
fi
submit="${exe}-submit"
tmp="/tmp"
port_1=`perl -e 'print int(rand()*1000) + 1025'`
port_2=`expr $port_1 + 1`
files="100"
lines="100" # per file
characters="10000" # per line
options="--user daemon"
id="`basename $0`.$$.tmp"
base="$tmp/$id"
base_="$base"
if test "`uname -o`" = "Msys"
then
	realtmp="`mount | grep 'on /tmp' | sed 's/ .*//' | sed 's:\\\\:/:g'`"
	base_="$realtmp/$id"
	options="--hidden --no-daemon --log-file=$base_/log.txt"
	files="10" # otherwise too slow to create them
fi

# check
test -x "$exe" -a -x "$submit" || echo `basename $0`: error: cannot execute "[$exe] or [$submit]" >&2
test -x "$exe" -a -x "$submit" || exit 1

# initialise
trap "e=\$? ; rm -rf $base ; exit $e" 0
trap "rm -rf $base ; exit 127" 1 2 3 13 15
mkdir "${base}"
perl -e 'print "To: me\n\n";for($i=0;$i<'"$lines"';$i++){print "x" x '"$characters"',"\n"}' > $base/.tmp
for i in `perl -e 'print join(" ",1..'"$files"'),"\n"'` ; do $submit -f me -s $base me < $base/.tmp ; done
echo $port_1 $port_2 > $base/.info
echo $base > $tmp/`basename $0 .sh`.txt

if test "`uname -o`" = "Msys"
then
	verifier="$base_/verifier.js"
	echo 'WScript.Stdout.WriteLine("");' >> $verifier
	echo 'WScript.Stdout.WriteLine(WScript.Arguments(0));' >> $verifier
	echo 'WScript.Quit(1);' >> $verifier
else
	verifier="$base/verifier"
	echo '#!/bin/sh' > $verifier
	echo 'echo ""' >> $verifier
	echo 'echo $1' >> $verifier
	echo 'exit 1' >> $verifier
	chmod +x $verifier
fi

# run
set -x
${v}${exe} --spool-dir=$base_ --log --no-daemon --port $port_1 --admin $port_2 --admin-terminate --forward-to=localhost:$port_1 --poll 1 --address-verifier $verifier --log-time $options --size 9999999 --pid-file $base_/pidfile
