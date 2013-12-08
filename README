E-MailRelay Readme
==================

Abstract
--------
E-MailRelay is a simple SMTP proxy and store-and-forward message transfer agent
(MTA). When running as a proxy all e-mail messages can be passed through a
user-defined program, such as a spam filter, which can drop, re-address or edit
messages as they pass through. When running as a store-and-forward MTA incoming
messages are stored in a local spool directory, and then forwarded to the next
SMTP server on request.

E-MailRelay can also run as a POP3 server. Messages received over SMTP can be
automatically dropped into several independent POP3 mailboxes.

E-MailRelay uses the same non-blocking i/o model as Squid and Nginx giving
excellent scalability and resource usage.

C++ source code is available for Linux, FreeBSD, MacOS X etc, and Windows.
Distribution is under the GNU General Public License V3.

Quick start
-----------
To use E-MailRelay in store-and-forward mode use the "--as-server" option to
start the storage daemon in the background, and then trigger delivery of spooled
messages by running with the "--as-client" option and the address of the target
host.

For example, to start a storage daemon listening on port 10025 use a command
like this:

	emailrelay --as-server --port 10025 --spool-dir /tmp

And then to forward the spooled mail to "smarthost" run something like this:

	emailrelay --as-client smarthost:25 --spool-dir /tmp

To get behaviour more like a proxy you can add the "--poll" option so that
messages are forwarded continuously rather than on-demand. This example starts a
store-and-forward server that forwards spooled-up e-mail every hour:

	emailrelay --as-server --poll 3600 --forward-to smarthost:25

For a proxy server that forwards each message as it is being received, without
any delay, you can use the "--as-proxy" mode:

	emailrelay --as-proxy smarthost:25

If you want to edit or filter e-mail as it passes through the proxy then specify
your pre-processor program with the "--filter" option, something like this:

	emailrelay --as-proxy smarthost:25 --filter /usr/local/bin/addsig

To run E-MailRelay as a POP server without SMTP use "--pop" and "--no-smtp":

	emailrelay --pop --no-smtp --log --close-stderr

The "emailrelay-submit" utility can be used to put messages straight into the
spool directory so that the POP clients can fetch them.

By default E-MailRelay will always reject connections from remote machines. To
allow connections from anywhere use the "--remote-clients" option, but please
check your firewall settings to make sure this cannot be exploited by spammers.

On Windows add "--hidden" to suppress message boxes and also add "--no-daemon"
if running as a service.

For more information on the command-line options refer to the reference guide
or run:

	emailrelay --help --verbose

Documentation
-------------
The following documentation is provided:
* README             -- this document
* COPYING            -- the GNU General Public License
* INSTALL            -- build & install instructions (including the GNU text)
* AUTHORS            -- authors, credits and additional copyrights
* userguide.txt      -- user guide
* reference.txt      -- reference document
* ChangeLog          -- change log for releases

Source code documentation will be generated when building from source if
"doxygen" is available.

Configurations
--------------
Recent releases were developed on Ubuntu Linux 12.04 using:
* linux 3.2.0
* gcc 4.6.3
* autoconf 2.68

and on Windows 7 using:
* MSVC 2012
* MinGW 20120426
* OpenSSL 1.0.1c

The code was originally developed on SuSE Linux 7.1 using:
* linux 2.4.10
* gcc 2.95.3
* glibc 2.2.4 (libc.so.6)
* autoconf 2.52

and on Windows 98 using:
* MSVC 6.0

Versions of the code have also been built successfully on:
* MacOS X 10.3.9
* MacOS X 10.5.1 using gcc 4.0.1 on G4 PPC hardware
* FreeBSD 9.2 on Intel hardware
* Linux on Alpha hardware (Debian 2.2)
* Linux on Sparc hardware
* Linux on ARM 11 (Raspberry Pi) hardware
* Linux on RS6000 PPC hardware
* Linux on MIPS embedded hardware using gcc 3.4.6
* Linux using clang++ 3.0
* Linux using intel c++ 6.0
* Linux using intel c++ 10.1
* Solaris 8 using gcc on Sparc hardware
* Solaris 8 using WorkShop 5.0
* Solaris 10
* Windows NT 4.0 using MSVC 6.0
* Windows NT 4.0 using Cygwin (DLL 1.3.22) and gcc 3.2
* Windows NT 4.0 using MinGW 2.0.0 and gcc 3.2
* Windows XP using MinGW 3.1.0 gcc 3.4.2
* Windows Vista

Feedback
--------
Please feel free to e-mail the author at
"mailto:graeme_walker@users.sourceforge.net" or the SourceForge mailing list
"mailto:emailrelay-help@lists.sourceforge.net".

