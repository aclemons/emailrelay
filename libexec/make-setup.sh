#!/bin/sh
#
# Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# make-setup.sh
#
# Builds a gui payload tree by running "make install" into it and adding
# a config file. The payload is processed by the gui installer.
#
# For windows this is now done elsewhere.
#
# usage: make-setup.sh [-d] <payload>
#           -d : debug
#
# Normally run by "make payload" in the "src/gui" directory.
#

# parse the command line
debug="0" ; if test "$1" = "-d" ; then shift ; debug="1" ; fi
payload="$1"

# check the command-line
if test "$payload" = ""
then
	echo usage: `basename $0` '<payload>' >&2
	exit 2
fi
if test ! -d "$payload" -o -f "$payload/*"
then
	echo error: payload "[$payload]" is not an empty directory >&2
	exit 1
fi

# run "make install" into the payload directory
echo `basename $0`: running make install into $payload
payload_path="`cd $payload && pwd`"
( cd ../.. && make install GCONFIG_HAVE_DOXYGEN=no DESTDIR=$payload_path ) > /dev/null 2>&1

# check the "./configure" was done by "bin/configure.sh --enable-gui --without-doxygen"
if test \
	! -d "$payload/usr/lib/emailrelay" -o \
	! -f "$payload/usr/share/emailrelay/emailrelay.no.qm" -o \
	! -f "$payload/usr/lib/emailrelay/emailrelay.auth.in" -o \
	! -f "$payload/usr/lib/emailrelay/emailrelay.conf.in" -o \
	! -f "$payload/usr/sbin/emailrelay-gui.real" -o \
	-f "$payload/usr/share/doc/emailrelay/doxygen/classes.html"
then
	echo `basename $0`: cannot see expected files: configure with \"configure.sh --enable-gui --without-doxygen\" >&2
	exit 1
fi

# clean up the "make install" output
rm -f $payload/usr/sbin/emailrelay-gui

# create the payload config file
cat <<EOF >$payload/payload.cfg
# all of /etc excluding .conf and .auth files created by gui
etc/pam.d/emailrelay=%dir-install%/etc/pam.d/emailrelay
etc/init.d/emailrelay=%dir-install%/etc/init.d/emailrelay
# all sub-dirs of /usr
usr/lib/=%dir-install%/lib/
usr/share/=%dir-install%/share/
usr/sbin/=%dir-install%/sbin/
# permission fix-ups
+%dir-spool% group daemon 770 g+s
+%dir-install%/sbin/emailrelay-submit group daemon 775 g+s
EOF

