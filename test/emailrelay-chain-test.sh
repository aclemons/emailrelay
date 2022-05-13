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
# emailrelay-chain-test.sh
#
# Tests the E-MailRelay system.
#
# Creates several temporary spool directories under /tmp and runs
# emailrelay servers to bucket-brigade a test message from one to
# the next...
#
#   The script creates messages in store-1. A client sends messages
#   from store-1 to server-1. Server-1 stores messages in store-2.
#   Server-2 is poked to read messages from store-2 and send to
#   server-3 authenticating as 'carol'. Server-3 authenticates carol
#   and runs as a proxy using store-3 to forward messages to
#   server-4 as 'alice'. Server-4 authenticates alice and stores
#   messages in store-4 which server-5 is continuously polling.
#   Server-5 forwards messages that appear in store-4 to
#   server-6 using a client filter. Server-6 stores messages
#   in store-5 using a server filter. If using fetchmail
#   then fetchmail extracts messages from server-6 and sends them
#   on to server-7 which stores them in store-6.
#
# The test succeeds if the message gets into the final spool directory.
#
# Once all the servers have been killed the separate log files are
# concatenated into the summary log file "/tmp/emailrelay-test.sh.out".
#
# If this test takes more a minute (without valgrind) then it
# has failed.
#
# usage: emailrelay-chain-test.sh [-v] [-f] [-d] [-n] [-s] [<exe-dir> [<content-file>]]
#	-v    use valgrind
#	-f    use fetchmail
#	-d    use --debug
#	-n    no cleanup
#	-s    smaller content
#

# parse the command line
#
opt_use_valgrind="0"
opt_use_fetchmail="0"
opt_smaller="0"
opt_debug="0"
opt_no_cleanup="0"
while getopts 'vfdnsa:' opt ; do
	case "$opt" in
		v) opt_use_valgrind="1" ;;
		f) opt_use_fetchmail="1" ;;
		s) opt_smaller="1" ;;
		d) opt_debug="1" ;;
		n) opt_no_cleanup="1" ;;
	esac
done
shift `expr $OPTIND - 1`
opt_exe_dir="$1"

# configuration
#
cfg_sw_debug="" ; test "$opt_debug" -eq 0 || cfg_sw_debug="--debug"
cfg_sw_extra="--domain chaintest.localnet" ; test "$opt_use_valgrind" -eq 0 || cfg_sw_extra="--no-daemon"
cfg_exe_dir="../src/main" ; test -z "$opt_exe_dir" || cfg_exe_dir="$opt_exe_dir"
cfg_main_exe="$cfg_exe_dir/emailrelay"
cfg_null_filter="/bin/touch" ; test -f "$cfg_null_filter" || cfg_null_filter="/usr/bin/touch"
cfg_pp="201" # port-prefix
cfg_base_dir="/tmp/`basename $0`.$$.tmp"
cfg_summary_log="/tmp/`basename $0`.out"
cfg_sw_valgrind="--trace-children=yes --num-callers=40 --leak-check=yes --track-fds=yes --time-stamp=yes --log-file=$cfg_base_dir/valgrind-"
cfg_run="env IGNOREME=" ; test "$opt_use_valgrind" -eq 0 || cfg_run="valgrind $cfg_sw_valgrind"

Init()
{
	MALLOC_CHECK_="2"
	export MALLOC_CHECK_
	ulimit -c unlimited
	trap "Trap 1 ; exit 1" 1 2 3 13 15
	trap "e=\$? ; Trap 0 ; exit \$e" 0
}

Trap()
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
	local port="$1"
	local command="$2"
	local log="$3"

	echo ++ poke ${cfg_pp}${port} flush > "$cfg_base_dir/$log"
	Poke "${cfg_pp}${port}" "$command" >> "$cfg_base_dir/$log"
}

Poke()
{
	local port="$1"
	local command="$2"

	if test "$command" = "" ; then command="flush" ; fi
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
	Poke "${cfg_pp}11" terminate
	Poke "${cfg_pp}12" terminate
	Poke "${cfg_pp}13" terminate
	Poke "${cfg_pp}14" terminate
	Poke "${cfg_pp}15" terminate
	Poke "${cfg_pp}16" terminate
	if test "$opt_use_fetchmail" -eq 1
	then
		Poke "${cfg_pp}17" terminate
	fi
	sleep 2

	kill `cat $cfg_base_dir/pid-* 2>/dev/null`

	if test -d "$cfg_base_dir"
	then
		grep "MailRelay-Reason" $cfg_base_dir/*/*envelope*bad > "$cfg_summary_log"
		grep "." $cfg_base_dir/log-? >> "$cfg_summary_log"
		if test "$opt_use_valgrind" -eq 1
		then
			grep "." $cfg_base_dir/valgrind* >> "$cfg_summary_log"
		fi
		ls -lR "$cfg_base_dir" >> "$cfg_summary_log"
		if test "$opt_no_cleanup" -eq 0
		then
			echo `basename $0`: cleaning up >&2
			rm -rf "$cfg_base_dir"
		fi
	fi
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
	local port="$1"
	local admin_port="$2"
	local spool="$3"
	local log="$4"
	local pidfile="$5"
	local extra="$6"

	local pop_port="`expr ${port} + 80`"

	mkdir -p "$cfg_base_dir/$spool"
	chgrp daemon "$cfg_base_dir/$spool" 2>/dev/null
	chmod 775 "$cfg_base_dir/$spool"

	local cmd="$cfg_main_exe $cfg_sw_extra $cfg_sw_debug"
	cmd="$cmd --log --log-time --verbose --no-syslog"
	cmd="$cmd --port ${cfg_pp}${port}"
	cmd="$cmd --spool-dir $cfg_base_dir/$spool"
	cmd="$cmd --admin ${cfg_pp}${admin_port}"
	cmd="$cmd --admin-terminate"
	cmd="$cmd --pop --pop-port ${cfg_pp}${pop_port}"
	cmd="$cmd --pop-auth $cfg_base_dir/pop.auth"
	cmd="$cmd --pid-file $cfg_base_dir/$pidfile"
	cmd="$cmd $extra"

	echo ++ $cmd > "$cfg_base_dir/$log"
	${cfg_run}-server-${port_} $cmd 2>> "$cfg_base_dir/$log" &
}

RunClient()
{
	local to="$1"
	local spool="$2"
	local log="$3"
	local pidfile="$4"

	local cmd="$cfg_main_exe $cfg_sw_extra $cfg_sw_debug"
	cmd="$cmd --forward --no-daemon --dont-serve --log --verbose --log-time --no-syslog"
	cmd="$cmd --pid-file $cfg_base_dir/$pidfile"
	cmd="$cmd --forward-to $to"
	cmd="$cmd --spool-dir $cfg_base_dir/$spool"

	echo ++ $cmd > "$cfg_base_dir/$log"
	${cfg_run}-client $cmd 2>> "$cfg_base_dir/$log"
}

RunFetchmail()
{
	local pop_port="$1"
	local smtp_port="$2"

	local cfg="$cfg_base_dir/fetchmailrc"
	echo poll localhost username fetchmailer password fetchmail_secret > "$cfg"
	chmod 700 "$cfg"

	local cmd="fetchmail"
	cmd="$cmd -f $cfg --verbose --protocol APOP"
	cmd="$cmd --port ${cfg_pp}${pop_port}"
	cmd="$cmd --smtphost localhost/${cfg_pp}${smtp_port} localhost"

	echo ++ $cmd > ${cfg_base_dir}/log-fetchmail
	$cmd >> "$cfg_base_dir/log-fetchmail" 2>&1
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
	local store="$1"
	test \
		"`ls -1 $cfg_base_dir/$store/emailrelay.*.content 2>/dev/null | wc -l`" -eq 2 -a \
		"`ls -1 $cfg_base_dir/$store/emailrelay.*.envelope 2>/dev/null | wc -l`" -eq 2
}

ReportResults()
{
	local ok="$1"
	if test "$ok" -eq 1
	then
		echo `basename $0`: succeeded
	else
		echo `basename $0`: failed: see $cfg_summary_log >&2
	fi
}

CreateMessages()
{
	mkdir -p "$cfg_base_dir/store-1"
	chgrp daemon "$cfg_base_dir/store-1" 2>/dev/null
	chmod 775 "$cfg_base_dir/store-1"

	Content | CrLf > "$cfg_base_dir/store-1/emailrelay.0.1.content"
	Envelope | CrLf > "$cfg_base_dir/store-1/emailrelay.0.1.envelope"
	Content | CrLf > "$cfg_base_dir/store-1/emailrelay.0.2.content"
	Envelope | CrLf > "$cfg_base_dir/store-1/emailrelay.0.2.envelope"
}

CreateAuth()
{
	mkdir -p "$cfg_base_dir"
	chmod 777 "$cfg_base_dir"

	# encrypted version "carols_password" provided by emailrelay-passwd
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

	file="$cfg_base_dir/pop.auth"
	echo "server plain fetchmailer fetchmail_secret" >> "$file"
}

CreateFilter()
{
	cat <<EOF | sed 's/^ *_//' > ${cfg_base_dir}/filter.sh
            _#!/bin/sh
            _# filter.sh
            _tmp="${cfg_base_dir}/\`basename \$0\`.\$\$.tmp"
            _content="\$1"
            _tr 'a-zA-Z' 'A-Za-z' < "\$content" > \$tmp
            _cp \$tmp "\$content"
EOF
	chmod +x "$cfg_base_dir/filter.sh"
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
	CreateAuth
	CreateFilter
	CreateMessages

	RunServer 01 11 store-2 log-1 pid-1
	RunServer 02 12 store-2 log-2 pid-2 "--forward-to localhost:${cfg_pp}03 --client-auth $cfg_base_dir/client-carol.auth"
	RunServer 03 13 store-3 log-3 pid-3 "--immediate --forward-to localhost:${cfg_pp}04 --filter $cfg_null_filter --client-auth $cfg_base_dir/client-alice.auth --server-auth $cfg_base_dir/server.auth"
	RunServer 04 14 store-4 log-4 pid-4 "--server-auth $cfg_base_dir/server.auth"
	RunServer 05 15 store-4 log-5 pid-5 "--poll 1 --forward-to localhost:${cfg_pp}06 --client-filter $cfg_base_dir/filter.sh"
	RunServer 06 16 store-5 log-6 pid-6 "--filter $cfg_base_dir/filter.sh"

	Sleep 1
	RunClient "localhost:${cfg_pp}01" store-1 log-c pid-c
	RunPoke 12 flush log-p

	local success="0"
	for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20
	do
		Sleep 5
		if TestDone store-5
		then
			success="1"
			break
		fi
	done

	if test "$success" -eq 1 -a "$opt_use_fetchmail" -eq 1
	then
		success="0"
		RunServer 07 17 store-6 log-7 pid-7
		Sleep 1
		RunFetchmail 86 07
		if TestDone store-6
		then
			success="1"
		fi
	fi

	ReportResults "${success}"
	test "${success}" -ne 0
}

Main

