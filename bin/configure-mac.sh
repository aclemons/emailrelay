#!/bin/sh
#
# Copyright (C) 2001-2013 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# configure-mac.sh
#
# A convenience script that runs the emailrelay "../configure" script for 
# Mac OS X. 
#
#   $ cd src/emailrelay-1.99
#   $ mkdir build
#   $ cd build
#   $ sh ../bin/configure-mac.sh
#   $ make
#
# After configuring with this script the installation can be done
# with "make install" for a system-wide installation, or with 
# "make install DESTDIR=~" for a private installation.
#
# The default definition of "e_initdir" ensures that the server starts 
# up automatically at boot time.
#

app="/Applications/E-MailRelay"
lib="/Library"

configure="./configure"
if test ! -f "${configure}" ; then configure="../configure" ; fi
if test ! -f "${configure}" ; then configure="../../configure" ; fi
chmod +x "${configure}" 2> /dev/null

${configure} \
	--enable-mac \
	--sbindir="${app}" \
	e_qtmoc=/usr/bin/moc \
	e_libexecdir="${app}" \
	e_examplesdir="${app}/Documentation/examples" \
	e_sysconfdir="${app}" \
	--mandir="${app}/Documentation/man" \
	e_icondir="${app}" \
	e_docdir="${app}/Documentation" \
	e_spooldir="${lib}/Mail/Spool" \
	e_pamdir="/etc/pam.d" \
	e_initdir="${lib}/StartupItems/E-MailRelay" "$@"

