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
# emailrelay-soak-test.sh
#
# Soak tests the E-MailRelay system to check for large memory leaks.
#
# It starts two servers, with a proxy fronting onto one of them,
# then in a loop it creates a large message file and runs a client
# to forward it to both the proxy and the naked server. The main
# loop can be stopped by creating a ".stop" file.
#
# Run 'ps' manually to check for large leaks.
#
# Files are created under a sub-directory of /tmp.
#
# usage: emailrelay-soak-test.sh [-v] [<exe>]
#        -v  - use valgrind
#
# (As a side-effect it does "killall emailrelay".)
#

# configuration
#
exe="`dirname $0`/../src/main/emailrelay"
content="/etc/services"
pp="1001" # port prefix
valgrind=""
valgrind_sw=""
sleep=1
if test "${1}" = "-v"
then
	shift
	valgrind="valgrind"
	valgrind_sw="--leak-check=full --show-reachable=no --num-callers=50"
	sleep=4
fi
test -z "${1}" || exe="${1}"

# configuration fallback
#
bin_dir="`dirname $0`"
if test \! -f "${exe}" -a -x "${bin_dir}/../emailrelay"
then
	exe="${bin_dir}/../emailrelay"
	echo `basename $0`: using executable \"${exe}\" >&2
fi

# initialisation
#
as_client="--no-syslog --no-daemon --dont-serve --forward --forward-to" # no --log
as_server="--log" # no --close-stderr
as_proxy="--log --immediate --forward-to" # no --close-stderr
base_dir="/tmp/`basename $0`.$$.tmp"
auth_file="${base_dir}/`basename $0 .sh`.auth"
mkdir "${base_dir}"
trap "Cleanup 0" 0
trap "Cleanup 1" 1 2 3 13 15

Cleanup()
{
	echo cleaning up...
	trap "" 0 1 2 3 13 15
	pids="`cat server-1.pid server-2.pid proxy.pid 2>/dev/null`"
	if test "$pids" != ""
	then
		kill $pids
		sleep 1
		kill -9 $pids 2>/dev/null
	fi
	rm -rf ${base_dir} 2>/dev/null
	echo ... done
	exit
}

Auth()
{
	echo "server plain joe joe+00s+3Dpassword"
	echo "client plain joe joe+00s+3Dpassword"
}

Content()
{
	echo "To:" ${USER}@`uname -n`
	echo "Subject: test message from process" $$
	echo "From: tester"
	echo ""
	for i in 1 1 1 1 1 1 1 1 1 1
	do
		cat "${content}"
	done
}

Envelope()
{
	echo "X-MailRelay-Format: #2821.5"
	echo "X-MailRelay-Content: 8bit"
	echo "X-MailRelay-From: me"
	echo "X-MailRelay-ToCount: 1"
	echo "X-MailRelay-To-Remote:" ${USER}@`uname -n`
	echo "X-MailRelay-Authentication: anon"
	echo "X-MailRelay-Client: 127.0.0.1"
	echo "X-MailRelay-ClientCertificate: "
	echo "X-MailRelay-MailFromAuthIn: "
	echo "X-MailRelay-MailFromAuthOut: "
	echo "X-MailRelay-End: 1"
}

CrLf()
{
	sed 's/$/£/' | tr '£' '\r'
}

CreateMessage()
{
	spool_dir="${1}"

	Content | CrLf > ${spool_dir}/emailrelay.$$.1.content
	Envelope | CrLf > ${spool_dir}/emailrelay.$$.1.envelope
}

RunClient()
{
	to_address="${1}"
	spool_dir="${2}"

	${exe} ${as_client} ${to_address} --spool-dir ${spool_dir} --client-auth ${auth_file}
}

Send()
{
	to_address="${1}"

	dir="${base_dir}/spool-send"
	mkdir -p "${dir}"

	CreateMessage "${dir}"
	RunClient "${to_address}" "${dir}"
}

RunServer()
{
	port="${1}"
	spool_dir="${2}"
	log="${3}"
	pid_file="${4}"

	if test "`echo ${pid_file} | grep '^/'`" = ""
	then
		pid_file="`pwd`/${pid_file}"
	fi

	mkdir -p "${spool_dir}"
	set -x
	${valgrind} ${valgrind_sw} ${exe} ${as_server} --port ${port} --spool-dir ${spool_dir} \
		--pid-file ${pid_file} \
		--server-auth ${auth_file} \
		--admin-terminate \
		--admin `expr ${port} + 100` --forward-to localhost:smtp --no-syslog \
		--no-daemon > "${log}" 2>&1 &
	set +x
}

RunProxy()
{
	port="${1}"
	to_address="${2}"
	spool_dir="${3}"
	log="${4}"
	pid_file="${5}"

	if test "`echo ${pid_file} | grep '^/'`" = ""
	then
		pid_file="`pwd`/${pid_file}"
	fi

	mkdir -p "${spool_dir}"
	set -x
	${valgrind} ${valgrind_sw} ${exe} ${as_proxy} ${to_address} --port ${port} --spool-dir ${spool_dir} \
		--pid-file ${pid_file} \
		--server-auth ${auth_file} \
		--client-auth ${auth_file} \
		--admin-terminate \
		--admin `expr ${port} + 100` --no-syslog \
		--no-daemon > "${log}" 2>&1 &
	set +x
}

Init()
{
	killall emailrelay 2>/dev/null
	Auth > ${auth_file}
}

RunServers()
{
	RunServer ${pp}3 ${base_dir}/spool-3 server-2.out server-2.pid
	RunProxy ${pp}1 localhost:${pp}3 ${base_dir}/spool-1 proxy.out proxy.pid
	RunServer ${pp}2 ${base_dir}/spool-2 server-1.out server-1.pid
	sleep $sleep # to allow pid files time to be written
}

CheckServers()
{
	if test \! -f server-1.pid -o \! -f server-2.pid -o \! -f proxy.pid
	then
		echo `basename $0`: error starting 'server(s)' >&2
		cat server-?.out proxy.out 2>/dev/null | grep -v "^==" | sed 's/^/    /' >&2
		exit 1
	fi
}

StopServers()
{
	for port in ${pp}1 ${pp}2 ${pp}3
	do
    	perl -e '
        	use IO::Socket ;
        	my $p = $ARGV[0] + 100 ;
        	my $c = "terminate" ;
        	my $s = new IO::Socket::INET( PeerHost=>"127.0.0.1" , PeerPort=>$p , Proto=>"tcp" ) or die ;
        	$s->send( $c . "\r\n" ) or die ;
        	my $buffer = "" ;
        	alarm( 2 ) ;
        	$s->recv( $buffer , 1024 ) ;
    	' $port
	done
}

Ps()
{
	echo `basename $0`: output from \"ps -l -p `cat server-2.pid` -p `cat server-1.pid` -p `cat proxy.pid`\"...
	ps -l -p `cat server-2.pid` -p `cat server-1.pid` -p `cat proxy.pid` | sed 's/^/     /'
}

Main()
{
	rm -f .stop 2>/dev/null
	while test ! -f .stop
	do
		Send localhost:${pp}1
		Send localhost:${pp}2
		echo -n .
		rm -f ${base_dir}/spool-?/*content
	done
	rm -f .stop
	StopServers
	sleep 2
}

Init
RunServers
CheckServers
Ps
Main

