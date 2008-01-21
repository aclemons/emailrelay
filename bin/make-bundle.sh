#!/bin/sh
#
# Copyright (C) 2001-2008 Graeme Walker <graeme_walker@users.sourceforge.net>
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
# make-bundle.sh
#
# Makes a Mac OS X application bundle.
#
# usage: make-bundle.sh [-f] <name> <exe> <icon> [<version>]
#
# Silently does nothing on non-Mac systems.
#

force="0" ; if test "$1" = "-f" ; then force="1" ; shift ; fi
name="$1"
exe="$2"
icon="$3"
version="$4" ; if test "${version}" = "" ; then version="1.8.0" ; fi

if test "${name}" = ""
then
	echo usage: `basename $0` '[-f] <name> <exe> <icon> [<version>]' >&2
	exit 1
fi

if test "`uname`" != "Darwin" -a "$force" -eq 0
then
	exit 0
fi

if test ! -x "${exe}"
then
	echo `basename $0`: invalid exe >&2
	exit 1
fi

yyyy="`date -u +'%Y'`"
copyright="Copyright (c) 2001-${yyyy} Graeme Walker &lt;graeme_walker@users.sourceforge.net&gt;>"
bundle_version="`date -u +'%Y.%m.%d.%H.%M.%S'`"
key="`basename \"${exe}\" | sed 's/\..*//'`"

if test -d "${name}.app"
then
	rm -rf "${name}.app"
fi

dir="${name}.app/Contents"
mkdir -p "${dir}/MacOS" 2>/dev/null
mkdir -p "${dir}/Resources" 2>/dev/null

ln -f "${exe}" "${dir}/MacOS/${name}"
cp "${icon}" "${dir}/Resources/${name}.icns"

cat > "${dir}/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleDisplayName</key> <string>${name}</string>
	<key>CFBundleIconFile</key> <string>${name}.icns</string>
	<key>CFBundleIdentifier</key> <string>net.sourceforge.emailrelay.${key}</string>
	<key>CFBundleName</key> <string>${name}</string>
	<key>CFBundlePackageType</key> <string>APPL</string>
	<key>CFBundleShortVersionString</key> <string>${version}</string>
	<key>CFBundleSignature</key> <string>gwgw</string>
	<key>CFBundleVersion</key> <string>${bundle_version}</string>
	<key>LSHasLocalizedDisplayName</key> <false/>
	<key>NSHumanReadableCopyright</key> <string>${copyright}</string>
	<key>NSAppleScriptEnabled</key> <false/>
</dict>
</plist>
EOF

