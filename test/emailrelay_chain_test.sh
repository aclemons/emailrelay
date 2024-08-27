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
# emailrelay_chain_test.sh
#
# Runs a bucket-brigade test as follows:
#
#   The script creates messages in store-1. A client sends messages
#   from store-1 to server-1. Server-1 stores messages in store-2.
#   Server-2 is poked to read messages from store-2 and send to
#   server-3 authenticating as 'carol'. Server-3 authenticates carol
#   and runs as a proxy using store-3 to forward messages to
#   server-4 as 'alice'. Server-4 authenticates alice and stores
#   messages in store-4 which server-5 is continuously polling.
#   Server-5 forwards messages that appear in store-4 to
#   server-6 using a client filter. Server-6 delivers messages
#   to a mailbox under store-5 using a delivery filter.
#
# The test succeeds if the message gets into the final spool directory.
#
# If this test takes more a minute (without valgrind) then it
# has failed.
#
# usage: emailrelay_chain_test.sh [-V] [-f] [-v] [-n] [-s] [{-d|-b} <dir>] [-t <tmpdir>]
#            -d  -- buildroot directory
#            -b  -- binary directory
#            -V  -- use valgrind
#            -v  -- use "--debug"
#            -s  -- smaller content
#            -t  -- temp directory
#

# parse the command line
#
opt_use_valgrind="0"
opt_smaller="0"
opt_debug="0"
opt_exe_dir=""
while echo "$1" | grep -q '^-'
do
	case "$1" in
		-V) opt_use_valgrind="1" ; shift ;;
		-s) opt_smaller="1" ; shift ;;
		-v) opt_debug="1" ; shift ;;
		-d) shift ; opt_exe_dir="$1/src/main" ; shift ;;
		-b) shift ; opt_exe_dir="$1" ; shift ;;
		-t) shift ; opt_tmp="$1" ; shift ;;
		*) echo `basename $0`: usage error "($1)" >&2 ; exit 2 ;;
	esac
done

# configuration
#
cfg_sw_debug="" ; test "$opt_debug" -eq 0 || cfg_sw_debug="--debug"
cfg_sw_extra="--domain chaintest.localnet" ; test "$opt_use_valgrind" -eq 0 || cfg_sw_extra="--no-daemon"
cfg_exe_dir="../src/main" ; test -z "$opt_exe_dir" || cfg_exe_dir="$opt_exe_dir"
cfg_main_exe="$cfg_exe_dir/emailrelay"
cfg_null_filter="/bin/touch" ; test -e "$cfg_null_filter" || cfg_null_filter="/usr/bin/touch"
cfg_pp="201" # port-prefix
cfg_admin_port="${cfg_pp}99"
cfg_base_dir="/tmp/`basename $0`.$$.tmp" ; test -z "$opt_tmp" || cfg_base_dir="$opt_tmp"
cfg_sw_valgrind="--trace-children=yes --num-callers=40 --leak-check=yes --track-fds=yes --time-stamp=yes --log-file=$cfg_base_dir/valgrind-"
cfg_run="env IGNOREME=" ; test "$opt_use_valgrind" -eq 0 || cfg_run="valgrind $cfg_sw_valgrind"

Init()
{
	MALLOC_CHECK_="2"
	export MALLOC_CHECK_
	ulimit -c unlimited
	trap "OnTrap 1 ; exit 1" 1 2 3 13 15
	trap "e=\$? ; OnTrap 0 ; exit \$e" 0
}

OnTrap()
{
	trap 0 1 2 3 13 15
	if test "$1" -ne 0
	then
		echo `basename $0`: stopping >&2
	fi
	Cleanup > /dev/null 2>&1
}

RunPoke()
{
	echo ++ poke "$1" "$2" >> "$cfg_base_dir/poke.out"
	Poke "$1" "$2" >> "$cfg_base_dir/poke.out" 2>&1
}

Poke()
{
	local port="$1"
	local command="$2"

	if test "`perl -e 'use IO::Socket ; print 123' 2>/dev/null`0" -eq 1230
	then
		perl -e '
			use IO::Socket ;
			my $p = $ARGV[0] ;
			my $c = $ARGV[1] ;
			my $s = new IO::Socket::INET( PeerHost=>"127.0.0.1" , PeerPort=>$p , Proto=>"tcp" ) or die ;
			$s->send( $c . "\r\n" ) or die ;
			my $buffer = "" ;
			alarm( 10 ) ;
			$s->recv( $buffer , 1024 ) ;
			print $buffer , "\n" ;
		' "$port" "$command"
	else
		( echo "$command" ; sleep 2 ) | nc -w 3 127.0.0.1 "$port"
	fi
}

Cleanup()
{
	RunPoke "${cfg_admin_port}" terminate
	sleep 2
	kill `cat $cfg_base_dir/pid 2>/dev/null`
}

SanityChecks()
{
	if test "`$cfg_main_exe --help 2>/dev/null | grep \"usage:\" | wc -l`" -ne 1
	then
		echo `basename $0`: cannot even run \"$cfg_main_exe --help\" >&2
		trap 0
		exit 1
	fi
}

RunServer()
{
	local cmd="$cfg_main_exe $cfg_sw_extra $cfg_sw_debug $*"
	echo ++ $cmd > "$cfg_base_dir/server.out"
	${cfg_run}-server-${port_} $cmd 2>> "$cfg_base_dir/server.out" &
}

RunClient()
{
	local cmd="$cfg_main_exe $cfg_sw_extra $cfg_sw_debug $*"
	echo ++ $cmd > "$cfg_base_dir/client.out"
	${cfg_run}-client $cmd 2>> "$cfg_base_dir/client.out"
}

Content()
{
	echo "To: recipient-1@f.q.d.n, recipient-2@f.q.d.n"
	echo "Subject: test message 1"
	echo "From: sender"
	echo " " | tr -d ' '
	if test "${opt_smaller}" -eq 1
	then
		cat $0
	else
		cat $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0
		cat $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0
		cat $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0
		cat $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0 $0
	fi
}

Envelope()
{
	echo "X-MailRelay-Format: #2821.3"
	echo "X-MailRelay-Content: 8bit"
	echo "X-MailRelay-From: sender"
	echo "X-MailRelay-ToCount: 2"
	echo "X-MailRelay-To-Remote: recipient-1@f.q.d.n"
	echo "X-MailRelay-To-Remote: recipient-2@f.q.d.n"
	echo "X-MailRelay-Authentication: "
	echo "X-MailRelay-Client: 127.0.0.1"
	echo "X-MailRelay-End: 1"
}

CrLf()
{
	sed 's/$/%/' | tr '%' '\r'
	#sed 's/$/\r/'
	#perl -ne 'chomp($_) ; print $_ , "\r\n"'
}

TestDone()
{
	local mbox_dir="$1"
	test \
		"`ls -1 $mbox_dir/emailrelay.*.content 2>/dev/null | wc -l`" -eq 2 -a \
		"`ls -1 $mbox_dir/emailrelay.*.envelope 2>/dev/null | wc -l`" -eq 2
}

ReportResults()
{
	local ok="$1"
	if test "$ok" -eq 1
	then
		echo `basename $0`: succeeded
	else
		echo `basename $0`: failed >&2
	fi
}

CreateBase()
{
	mkdir -p "$cfg_base_dir"
	chmod 777 "$cfg_base_dir"
}

CreateSpool()
{
	mkdir -p "$cfg_base_dir/$1"
	chgrp daemon "$cfg_base_dir/$1" 2>/dev/null
	chmod 775 "$cfg_base_dir/$1"
}

CreateMessages()
{

	Content | CrLf > "$cfg_base_dir/store-1/emailrelay.0.1.content"
	Envelope | CrLf > "$cfg_base_dir/store-1/emailrelay.0.1.envelope"
	Content | CrLf > "$cfg_base_dir/store-1/emailrelay.0.2.content"
	Envelope | CrLf > "$cfg_base_dir/store-1/emailrelay.0.2.envelope"
}

CreateAuth()
{
	# encrypted version of "carols_password" provided by emailrelay-passwd
	local dotted_key="4001433821.398427562.3259251711.3361837303.2461660504.3615007459.2556666290.2918439953"
	local base64_key="3QiB7qqFvxf/O0TC95BhyFj1uZLjonjXsqFjmBHc860="

	local file="$cfg_base_dir/server.auth"
	echo "# server.auth" > "$file"
	echo "server plain alice alices_password" >> "$file"
	echo "server md5 carol ${base64_key}" >> "$file"
	echo "server md5 bob dfgkljdflkgjdfg" >> "$file"

	file="$cfg_base_dir/client-alice.auth"
	echo "# client-alice.auth" > "$file"
	echo "client plain alice alices_password" >> "$file"

	file="$cfg_base_dir/client-carol.auth"
	echo "# client-carol.auth" > "$file"
	echo "client CRAM-MD5 carol $dotted_key" >> "$file"
}

CreateFilter()
{
	cat <<EOF | sed 's/^ *_//' > "${cfg_base_dir}/filter.sh"
            _#!/bin/sh
            _# filter.sh
            _content="\$1"
            _tmp="\$content.tmp"
            _tr 'a-zA-Z' 'A-Za-z' < "\$content" > "\$tmp"
            _mv "\$tmp" "\$content"
EOF
	chmod +x "$cfg_base_dir/filter.sh"
}

CreateVerifier()
{
	cat <<EOF | sed 's/^ *_//' > "${cfg_base_dir}/verifier.sh"
            _#!/bin/sh
            _# verifier.sh
            _recipient="\$1"
            _mailbox="mbox"
            _echo "full name"
            _echo "\$mailbox"
            _exit 0
EOF
	chmod +x "$cfg_base_dir/verifier.sh"
}

Sleep()
{
	if test "$opt_use_valgrind" -eq 1
	then
		sleep "$1"
		sleep "$1"
		sleep "$1"
		sleep "$1"
		sleep "$1"
	else
		sleep "$1"
	fi
}

Main()
{
	Init
	SanityChecks
	CreateBase
	CreateAuth
	CreateFilter
	CreateVerifier
	CreateSpool store-1
	CreateSpool store-2
	CreateSpool store-3
	CreateSpool store-4
	CreateSpool store-5
	CreateMessages

	RunServer \
		--pid-file=$cfg_base_dir/pid --log --verbose --log-time --no-syslog \
		--1-port=${cfg_pp}01 --1-spool-dir=$cfg_base_dir/store-2 \
		--2-port=${cfg_pp}02 --2-spool-dir=$cfg_base_dir/store-2 \
			--2-admin=${cfg_admin_port} --2-admin-terminate \
			--2-forward-to=localhost:${cfg_pp}03 --2-client-auth=$cfg_base_dir/client-carol.auth \
		--3-port=${cfg_pp}03 --3-spool-dir=$cfg_base_dir/store-3 \
			--3-immediate --3-forward-to=localhost:${cfg_pp}04 --3-filter=$cfg_null_filter \
			--3-client-auth=$cfg_base_dir/client-alice.auth --3-server-auth=$cfg_base_dir/server.auth \
		--4-port=${cfg_pp}04 --4-spool-dir=$cfg_base_dir/store-4 \
			--4-server-auth=$cfg_base_dir/server.auth \
		--5-port=${cfg_pp}05 --5-spool-dir=$cfg_base_dir/store-4 \
			--5-poll=1 --5-forward-to=localhost:${cfg_pp}06 --5-client-filter=$cfg_base_dir/filter.sh \
		--6-port=${cfg_pp}06 --6-spool-dir=$cfg_base_dir/store-5 \
			--6-address-verifier=$cfg_base_dir/verifier.sh \
			--6-filter=deliver:

	Sleep 1

	RunClient \
		--pid-file=$cfg_base_dir/client-pid \
		--forward --no-daemon --dont-serve --log --verbose --log-time --no-syslog \
		--forward-to=localhost:${cfg_pp}01 --spool-dir=$cfg_base_dir/store-1

	RunPoke ${cfg_admin_port} forward

	local success="0"
	for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19
	do
		Sleep 1
		if TestDone "$cfg_base_dir/store-5/mbox"
		then
			success="1"
			break
		fi
	done

	ReportResults "${success}"
	test "${success}" -ne 0
}

if "$cfg_main_exe" --version --verbose | grep -q -i "admin.*disabled"
then
	echo `basename $0`: skipped: no admin interface >&2
	exit 77
fi

Main

