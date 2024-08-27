#!/bin/sh
#
# Copyright (C) 2001-2024 Graeme Walker <graeme_walker@users.sourceforge.net>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
# ===
#
# emailrelay-multicast.sh
#
# An example E-MailRelay "--filter" script that copies/links each new message
# into all available sub-directories of the main spool directory. The original
# message files are deleted if they were successfully copied/linked into all
# sub-directories.
#
# This can be used for SMTP multicast by having an emailrelay forwarding
# process polling each sub-directory. (For POP multicasting use the
# "emailrelay-filter-copy" program with the "pop-by-name" feature so that
# there is no need to copy or link content files.)
#
# Hard links are used for the content files in order to conserve disk space.
# Log entries are written into the base envelope file to help with error
# recovery.
#
# By default errors in running this script are fed back to the remote SMTP
# client. Alternatively, edit the code below to ignore these errors and leave
# the submitted e-mail message in the main spool directory.
#

# parse the command-line
#
content="$1"
envelope="$2"
base_dir="`dirname \"${content}\"`"
if test "$1" = "" -o "${base_dir}" = "."
then
	echo usage: `basename $0` '<content-file> <envelope-file>' >&2
	exit 2
fi

# copy/link message files
#
list="`find \"${base_dir}\" -mount -maxdepth 1 -mindepth 1 -type d`"
error_list=""
subdir_exists="0"
for dir in ${list} ""
do
	if test -d "${dir}" -a "${dir}" != ".." -a "${dir}" != "." -a "${dir}" != "${base_dir}"
	then
		subdir_exists="1"
		envelope_tmp="${envelope}.`basename \"${dir}\"`.tmp"
		envelope_dst="${dir}/`basename \"${envelope}\" .new`"
		if ln "${content}" "${dir}" && cp "${envelope}" "${envelope_tmp}" && mv "${envelope_tmp}" "${envelope_dst}"
		then
			echo "X-MailRelay-Multicast: ${dir}>" | tr '>' '\r' >> "${envelope}"
		else
			error_list="${error_list} ${dir}"
			echo "X-MailRelay-Multicast-Error: ${dir}>" | tr '>' '\r' >> "${envelope}"
		fi
	fi
done

# error handling
#
if test "${subdir_exists}" = "0"
then
	# no sub-directories created -- this script has no effect
	exit 0
elif test "${error_list}" = ""
then
	# successfully copied -- delete the original
	rm -f "${content}" "${envelope}"
	exit 100
else
	# something failed -- tell the submitting smtp client, or
	# replace these three lines with "exit 0" if the client should not know...
	echo "<<multicast failed>>"
	echo "<<`basename $0`: `basename "${content}"`: failed to copy message into${error_list}>>"
	exit 1
fi

