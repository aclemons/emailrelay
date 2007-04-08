#!/bin/sh
#
# Copyright (C) 2001-2007 Graeme Walker <graeme_walker@users.sourceforge.net>
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later
# version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
# ===
#
# fastbuild.sh
#
# A complier stub for doing fastbuilds, where each executable
# is compiled from a single source-file containing a long
# list of source-file includes.
#
# This script will probably not work on weird o/s's. It
# was developed on SUSE 9.0.
#

Ar()
{
	echo fastbuild: creating empty library "${2}"
	touch "${2}"
}

ReallyCompile()
{
	new_="${1}"
	source_="${2}"
	shift
	shift
	echo fastbuild: compiling "${new_}" instead of "${source_}"
	g++ `echo $@ | sed "s/${source_}/${new_}/g"`
}

OutputFilter()
{
	tr '\t' ' ' | sed 's/.* -o //' | sed 's/ .*//'
}

Skip()
{
	source_="${1}"
	shift
	output_="`echo $@ | OutputFilter`"
	echo fastbuild: skipping "${source}"
	touch "${output_}"
}

CompileOrSkip()
{
	source="`echo $@ | sed 's/.* //'`"
	case "${source}" in
	main.cpp) ReallyCompile emailrelay-fastbuild.cpp "${source}" "$@" ;;
	passwd.cpp) ReallyCompile passwd-fastbuild.cpp "${source}" "$@" ;;
	submit.cpp) ReallyCompile submit-fastbuild.cpp "${source}" "$@" ;;
	guimain.cpp) ReallyCompile gui-fastbuild.cpp "${source}" "$@" ;;
	filter_copy.cpp) ReallyCompile filtercopy-fastbuild.cpp "${source}" "$@" ;;
	all.cpp) g++ "$@" ;;
	moc_*.cpp) g++ "$@" ;;
	*) Skip "${source}" "$@" ;;
	esac
}

ReallyLink()
{
	echo fastbuild: linking $2 into $1
	g++ -o "${1}" "${2}"
}

Link()
{
	output_="`echo $@ | OutputFilter`"
	output_name_="`basename \"${output_}\"`"
	echo fastbuild: linking "${output_name_}"
	case "${output_name_}" in
	emailrelay) ReallyLink ${output_} main.o ;;
	emailrelay-submit) ReallyLink ${output_} submit.o ;;
	emailrelay-passwd) ReallyLink ${output_} passwd.o ;;
	emailrelay-gui) ReallyLink ${output_} guimain.o ;;
	emailrelay-filter-copy) ReallyLink ${output_} filter_copy.o ;;
	*) echo fastbuild: error: unrecognised target binary >&2 ;;
	esac
}

if test "${1}" = "cru"
then
	Ar "$@"
:
elif test "`echo $@ | grep '\.cpp *$' | wc -l`" -ne 0
then
	CompileOrSkip "$@"
:
elif test "`echo $@ | grep -- '-o *[-a-z][-a-z]* ' | wc -l`" -ne 0
then
	Link "$@"
:
else
	echo fastbuild: unrecognised command line
	g++ "$@"
fi

