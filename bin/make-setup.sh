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
# make-setup.sh
#
# Builds a gui payload tree by running "make install" into it and adding
# a config file. The payload is processed by the gui installer.
#
# For windows this is now done elsewhere.
#
# usage: make-setup.sh [-d] <payload> <icon>
#           -d : debug
#
# Normally run by "make payload" in the "src/gui" directory.
#

# parse the command line
debug="0" ; if test "$1" = "-d" ; then shift ; debug="1" ; fi
payload="$1"
icon="$2"

# check the command-line
if test "$payload" = ""
then
	echo usage: `basename $0` '<payload> [<icon>]' >&2
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

# check the "./configure" was done by "bin/configure.sh" for FHS compliance
if test ! -d "$payload/usr/lib/emailrelay"
then
	echo `basename $0`: cannot see expected directories: configure with \"configure.sh\" >&2
	exit 1
fi

# clean up the "make install" output
rm -f $payload/etc/emailrelay.conf.makeinstall 2>/dev/null
rm -f $payload/usr/sbin/emailrelay-gui
rm -rf $payload/usr/share/doc/emailrelay/doxygen

# add the icon
cp "$icon" $payload/usr/lib/emailrelay/ 2>/dev/null

# create the payload config file
cat <<EOF >$payload/payload.cfg
etc/emailrelay.conf=%dir-config%/emailrelay.conf
etc/emailrelay.conf.template=%dir-config%/emailrelay.conf.template
etc/emailrelay.auth.template=%dir-config%/emailrelay.auth.template
etc/init.d/emailrelay=%dir-config%/init.d/emailrelay
usr/lib/=%dir-install%/lib/
usr/share/=%dir-install%/share/
usr/sbin/=%dir-install%/sbin/
+%dir-spool% group daemon 770 g+s
+%dir-install%/sbin/emailrelay-submit group daemon 775 g+s
EOF

