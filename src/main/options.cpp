//
// Copyright (C) 2001-2019 Graeme Walker <graeme_walker@users.sourceforge.net>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ===
//
// options.cpp
//

#include "gdef.h"
#include "options.h"

std::string Main::Options::spec( bool is_windows )
{
	std::string windows_(
		"l!log!writes log information on stderr and to the event log! (but see --close-stderr and --no-syslog)!0!!2|"

		"t!no-daemon!uses an ordinary window, not the system tray!, equivalent to --show=window!0!!3|"

		"k!syslog!forces system event log output if logging is enabled (overrides --no-syslog)!!0!!3|"

		"n!no-syslog!disables use of the system event log!!0!!3|"

		"H!hidden!hides the application window and suppresses message boxes (requires --no-daemon)!!0!!3|"
			// Windows only. Hides the application window and disables all message
			// boxes. This is useful when running as a windows service.

		"!show!start the application window in the given style!!1!style!3|"
			// Windows only. Starts the application window in the given style: "hidden",
			// "popup", "window", "window,tray", or "tray". Ignored if also using
			// "--no-daemon" or "--hidden". If none of "--window", "--no-daemon" and
			// "--hidden" are used then the default style is "tray".
	) ;

	std::string unix_(
		"l!log!writes log information on standard error and syslog! (but see --close-stderr and --no-syslog)!0!!2|"
			// Enables logging to the standard error stream and to the syslog. The
			// --close-stderr and --no-syslog options can be used to disable output to
			// standard error stream and the syslog separately. Note that --as-server,
			// --as-client and --as-proxy imply --log, and --as-server and --as-proxy
			// also imply --close-stderr.

		"t!no-daemon!does not detach from the terminal!!0!!3|"
			// Disables the normal backgrounding at startup so that the program
			// runs in the foreground, without forking or detaching from the
			// terminal.
			//
			// On Windows this disables the system tray icon so the program
			// uses a normal window; when the window is closed the program
			// terminates.

		"u!user!names the effective user to switch to if started as root (default is \"daemon\")!!1!username!3|"
			//default: daemon
			//example: nobody
			// When started as root the program switches to an non-privileged effective
			// user-id when idle. This option can be used to define which user-id is
			// used. Specify "root" to disable all user-id switching. Ignored on Windows.

		"k!syslog!forces syslog output if logging is enabled (overrides --no-syslog)!!0!!3|"
			// When used with --log this option enables logging to the syslog even
			// if the --no-syslog option is also used. This is typically used as
			// a convenient override when using --as-client.

		"n!no-syslog!disables syslog output (always overridden by --syslog)!!0!!3|"
			// Disables logging to the syslog. Note that
			// --as-client implies --no-syslog.
	) ;

	std::string common(
		"q!as-client!runs as a client, forwarding all spooled mail to <host>!: equivalent to \"--log --no-syslog --no-daemon --dont-serve --forward --forward-to\"!1!host:port!1|"
			//example: smtp.example.com:25
			// This is equivalent to --log, --no-syslog, --no-daemon, --dont-serve,
			// --forward and --forward-to. It is a convenient way of running a
			// forwarding agent that forwards spooled mail messages and then
			// terminates.

		"d!as-server!runs as a server, storing mail in the spool directory!: equivalent to \"--log --close-stderr\"!0!!1|"
			// This is equivalent to --log and --close-stderr. It is a convenient way
			// of running a background storage daemon that accepts mail messages and
			// spools them. Use --log instead of --as-server to keep standard error
			// stream open.

		"y!as-proxy!runs as a proxy server, forwarding each mail immediately to <host>!: equivalent to \"--log --close-stderr --forward-on-disconnect --forward-to\"!1!host:port!1|"
			//example: smtp.example.com:25
			// This is equivalent to --log, --close-stderr, --forward-on-disconnect and
			// --forward-to. It is a convenient way of running a store-and-forward
			// daemon. Use --log, --forward-on-disconnect and --forward-to instead
			// of --as-proxy to keep the standard error stream open.

		"v!verbose!generates more verbose output! (works with --help and --log)!0!!1|"
			// Enables more verbose logging when used with --log, and more verbose
			// help when used with --help.

		"h!help!displays help text and exits!!0!!1|"
			// Displays help text and then exits. Use with --verbose for more complete
			// output.

		"p!port!specifies the SMTP listening port number (default is 25)!!1!port!2|"
			//default: 25
			//example: 587
			// Sets the port number used for listening for incoming SMTP connections.

		"r!remote-clients!allows remote clients to connect!!0!!2|"
			// Allows incoming connections from addresses that are not local. The
			// default behaviour is to ignore connections that are not local in
			// order to prevent accidental exposure to the public internet, but
			// a firewall should also be used. The definition of 'local' is
			// different for IPv4 and IPv6.

		"s!spool-dir!specifies the spool directory!!1!dir!2|"
			//example: /var/spool/emailrelay
			//example: C:/ProgramData/E-MailRelay/spool
			// Specifies the directory used for holding mail messages that have been
			// received but not yet forwarded.

		"V!version!displays version information and exits!!0!!2|"
			// Displays version information and then exits.

		"K!server-tls!enables negotiated TLS when acting as an SMTP server! (ie. STARTTLS) (requires --server-tls-certificate)!0!!3|"
			// Enables TLS for incoming SMTP and POP connections. SMTP clients can
			// then request TLS encryption by issuing the STARTTLS command. The
			// --server-tls-certificate option must be used to define the server
			// certificate.

		"!server-tls-required!mandatory use of TLS before SMTP server authentication or mail-to! (requires --server-tls)!0!!3|"
			// Makes the use of TLS mandatory for any incoming SMTP and POP connections.
			// SMTP clients must use the STARTTLS command to establish a TLS session
			// before they can issue SMTP AUTH or SMTP MAIL-TO commands.

		"!server-tls-certificate!specifies a private TLS key+certificate file for --server-tls!!1!pem-file!3|"
			//example: /etc/ssl/certs/emailrelay.pem
			//example: C:/ProgramData/E-MailRelay/emailrelay.pem
			// Defines the TLS certificate file when acting as a SMTP or POP server.
			// This file must contain the server's private key and certificate chain
			// using the PEM file format. Keep the file permissions tight to avoid
			// accidental exposure of the private key.

		"!server-tls-verify!enables verification of remote client's certificate! against CA certificates in the given file or directory!1!ca-list!3|"
			//example: /etc/ssl/certs/ca-certificates.crt
			//example: C:/ProgramData/E-MailRelay/ca-certificates.crt
			// Enables verification of remote SMTP and POP clients' certificates
			// against any of the trusted CA certificates in the specified file
			// or directory. In many use cases this should be a file containing
			// just your self-signed root certificate.

		"j!client-tls!enables negotiated TLS when acting as an SMTP client! (ie. STARTTLS)!0!!3|"
			// Enables negotiated TLS for outgoing SMTP connections; the SMTP
			// STARTTLS command will be issued if the remote server supports it.

		"b!client-tls-connection!enables SMTP over TLS for SMTP client connections!!0!!3|"
			// Enables the use of a TLS tunnel for outgoing SMTP connections.
			// This is for SMTP over TLS (SMTPS), not TLS negotiated within SMTP
			// using STARTTLS.

		"!client-tls-certificate!specifies a private TLS key+certificate file for --client-tls!!1!pem-file!3|"
			//example: /etc/ssl/certs/emailrelay.pem
			//example: C:/ProgramData/E-MailRelay/emailrelay.pem
			// Defines the TLS certificate file when acting as a SMTP client. This file
			// must contain the client's private key and certificate chain using the
			// PEM file format. Keep the file permissions tight to avoid accidental
			// exposure of the private key.

		"!client-tls-verify!enables verification of remote server's certificate! against CA certificates in the given file or directory!1!ca-list!3|"
			//example: /etc/ssl/certs/ca-certificates.crt
			//example: C:/ProgramData/E-MailRelay/ca-certificates.crt
			// Enables verification of the remote SMTP server's certificate against
			// any of the trusted CA certificates in the specified file or directory.
			// In many use cases this should be a file containing just your self-signed
			// root certificate.

		"!client-tls-verify-name!enables verification of the cname in the remote server's certificate! (requires --client-tls-verify)!1!cname!3|"
			//example: smtp.example.com
			// Enables verification of the CNAME within the remote SMTP server's certificate.

		"!client-tls-server-name!includes the server hostname in the tls handshake! (ie. server name identification)!1!hostname!3|"
			//example: smtp.example.com
			// Defines the target server hostname in the TLS handshake. With
			// --client-tls-connection this can be used for SNI, allowing the remote
			// server to adopt an appropriate identity.

		"!client-tls-required!mandatory use of TLS for SMTP client connections! (requires --client-tls)!0!!3|"
			// Makes the use of TLS mandatory for outgoing SMTP connections. The SMTP
			// STARTTLS command will be used before mail messages are sent out.
			// If the remote server does not allow STARTTLS then the SMTP connection
			// will fail.

		"9!tls-config!sets low-level TLS configuration options! (eg. tlsv1.2)!2!options!3|"
			//example: mbedtls,tlsv1.2
			// Selects and configures the low-level TLS library, using a comma-separated
			// list of keywords. If OpenSSL and mbedTLS are both built in then keywords
			// of "openssl" and "mbedtls" will select one or the other. Keywords like
			// "tlsv1.0" can be used to set a minimum TLS protocol version, or
			// "-tlsv1.2" to set a maximum version.

		"g!debug!generates debug-level logging if built in!!0!!3|"
			// Enables debug level logging, if built in. Debug messages are usually
			// only useful when cross-referenced with the source code and they may
			// expose plaintext passwords and mail message content.

		"C!client-auth!enables SMTP authentication with the remote server, using the given client secrets file!!1!file!3|"
			//example: /etc/emailrelay.auth
			//example: C:/ProgramData/E-MailRelay/emailrelay.auth
			// Enables SMTP client authentication with the remote server, using the
			// client account details taken from the specified secrets file. The
			// secrets file should normally contain one line that starts with "client"
			// and that line should have between four and five space-separated
			// fields; the second field is the password encoding ("plain" or "md5"),
			// the third is the user-id and the fourth is the password. The user-id
			// is RFC-1891 xtext encoded, and the password is either xtext encoded
			// or generated by "emailrelay-passwd". If the remote server does not
			// support SMTP authentication then the SMTP connection will fail.

		"!client-auth-config!configures the client authentication module!!1!config!3|"
			//example: m:cram-sha1,cram-md5
			//example: x:plain,login
			// Configures the SMTP client authentication module using a
			// semicolon-separated list of configuration items. Each item is a
			// single-character key, followed by a colon and then a comma-separated
			// list. A 'm' character introduces an ordered list of authentication
			// mechanisms, and an 'x' is used for blocklisted mechanisms.

		"L!log-time!adds a timestamp to the logging output!!0!!3|"
			// Adds a timestamp to the logging output using the local timezone.

		"N!log-file!log to file instead of stderr! (with '%d' replaced by the current date)!1!file!3|"
			//example: /var/log/emailrelay-%d
			//example: C:/ProgramData/E-MailRelay/log-%d.txt
			// Redirects standard-error logging to the specified file. Logging to
			// the log file is not affected by --close-stderr. The filename can
			// include "%d" to get daily log files; the "%d" is replaced by the
			// current date in the local timezone using a "YYYYMMDD" format.

		"S!server-auth!enables authentication of remote SMTP clients, using the given server secrets file!!1!file!3|"
			//example: /etc/private/emailrelay.auth
			//example: C:/ProgramData/E-MailRelay/emailrelay.auth
			//example: /pam
			// Enables SMTP server authentication of remote SMTP clients. Account
			// names and passwords are taken from the specified secrets file. The
			// secrets file should contain lines that have four space-separated
			// fields, starting with "server" in the first field; the second field
			// is the password encoding ("plain" or "md5"), the third is the client
			// user-id and the fourth is the password. The user-id is RFC-1891 xtext
			// encoded, and the password is either xtext encoded or generated by
			// "emailrelay-passwd". A special value of "/pam" can be used for
			// authentication using linux PAM.

		"!server-auth-config!configures the server authentication module!!1!config!3|"
			//example: m:cram-sha256,cram-sha1
			//example: x:plain,login
			// Configures the SMTP server authentication module using a
			// semicolon-separated list of configuration items. Each item is a
			// single-character key, followed by a colon and then a comma-separated
			// list. A 'm' character introduces a preferred sub-set of the built-in
			// authentication mechanisms, and an 'x' is used for blocklisted
			// mechanisms.

		"e!close-stderr!closes the standard error stream soon after start-up!!0!!3|"
			// Causes the standard error stream to be closed soon after start-up.
			// This is useful when operating as a backgroud daemon and it is
			// therefore implied by --as-server and --as-proxy.

		"a!admin!enables the administration interface and specifies its listening port number!!1!admin-port!3|"
			//example: 587
			// Enables an administration interface on the specified listening port
			// number. Use telnet or something similar to connect. The administration
			// interface can be used to trigger forwarding of spooled mail messages
			// if the --forward-to option is used.

		"x!dont-serve!disables acting as a server on any port! (part of --as-client and usually used with --forward)!0!!3|"
			// Disables all network serving, including SMTP, POP and administration
			// interfaces. The program will terminate as soon as any initial
			// forwarding is complete.

		"X!no-smtp!disables listening for SMTP connections! (usually used with --admin or --pop)!0!!3|"
			// Disables listening for incoming SMTP connections.

		"z!filter!specifies an external program to process messages as they are stored!!1!program!3|"
			//example: /usr/local/sbin/emailrelay-filter
			//example: C:/Program\ Files/E-MailRelay/filter.bat
			//example: net:127.0.0.1:1111
			//example: spam:[::1].783
			//example: spam-edit:127.0.0.1:783
			//example: exit:103
			// Runs the specified external filter program whenever a mail message is
			// stored. The filter is passed the name of the message file in the
			// spool directory so that it can edit it as required. The mail message
			// is rejected if the filter program terminates with an exit code between
			// 1 and 99. Use "net:<transport-address>" to communicate with a filter
			// daemon over the network, or "spam:<transport-address>" for a
			// spamassassin spamd daemon to accept or reject mail messages, or
			// "spam-edit:<transport-address>" to have spamassassin edit the message
			// content without rejecting it, or "exit:<number>" to emulate a filter
			// program that just exits.

		"W!filter-timeout!sets the timeout (in seconds) for running the --filter (default is 300)!!1!time!3|"
			//default: 300
			//example: 10
			// Specifies a timeout (in seconds) for running a --filter program. The
			// default is 300 seconds.

		"w!prompt-timeout!sets the timeout (in seconds) for getting an initial prompt from the server (default is 20)!!1!time!3|"
			//default: 20
			//example: 3
			// Specifies a timeout (in seconds) for getting the initial prompt from
			// a remote SMTP server. If no prompt is received after this time then
			// the SMTP dialog goes ahead without it.

		"D!domain!sets an override for the host's fully qualified network name!!1!fqdn!3|"
			//example: smtp.example.com
			// Specifies the network name that is used in SMTP EHLO commands,
			// "Received" lines, and for generating authentication challenges.
			// The default is derived from a DNS lookup of the local hostname.

		"f!forward!forwards stored mail on startup! (requires --forward-to)!0!!3|"
			// Causes spooled mail messages to be forwarded when the program first
			// starts.

		"1!forward-on-disconnect!forwards stored mail once the SMTP client disconnects! (requires --forward-to)!0!!3|"
			// Causes spooled mail messages to be forwarded whenever a SMTP client
			// connection disconnects.

		"o!forward-to!specifies the address of the remote SMTP server! (required by --forward, --forward-on-disconnect and --immediate)!1!host:port!3|"
			//example: smtp.example.com:25
			// Specifies the transport address of the remote SMTP server that is
			// use for mail message forwarding.

		"T!response-timeout!sets the response timeout (in seconds) when talking to a remote server (default is 1800)!!1!time!3|"
			//default: 1800
			//example: 2
			// Specifies a timeout (in seconds) for getting responses from remote
			// SMTP servers. The default is 1800 seconds.

		"!idle-timeout!sets the idle timeout (in seconds) when talking to a remote client (default is 1800)!!1!time!3|"
			//default: 1800
			//example: 2
			// Specifies a timeout (in seconds) for receiving network traffic from
			// remote SMTP and POP clients. The default is 1800 seconds.

		"U!connection-timeout!sets the timeout (in seconds) when connecting to a remote server (default is 40)!!1!time!3|"
			//default: 40
			//example: 10
			// Specifies a timeout (in seconds) for establishing a TCP connection
			// to remote SMTP servers. The default is 40 seconds.

		"m!immediate!enables immediate forwarding of messages as they are received! from the submitting client and before their receipt is acknowledged (requires --forward-to)!0!!3|"
			// Causes mail messages to be forwarded as they are received, even before
			// they have been accepted. This can be used to do proxying without
			// store-and-forward, but in practice clients tend to to time out
			// while waiting for their mail message to be accepted.

		"I!interface!defines the listening network addresses used for incoming connections! (comma-separated list with optional smtp=,pop=,admin= qualifiers)!2!ip-address-list!3|"
			//example: 192.168.0.1,127.0.0.1,pop=::1,smtp=::
			// Specifies the IP network addresses used to bind listening ports. By
			// default listening ports for incoming SMTP, POP and administration
			// connections will bind the 'any' address for IPv4 and for IPv6,
			// ie. "0.0.0.0" and "::". Use this option to limit listening to particular
			// addresses (and by implication to particular network interfaces).
			// Multiple addresses can be specified by using the option more than
			// once or by using a comma-separated list. Use a prefix of "smtp=", "pop="
			// or "admin=" on addresses that should apply only to those types of
			// listening port.

		"6!client-interface!defines the local network address used for outgoing connections!!1!ip-address!3|"
			//example: 10.0.0.2
			// Specifies the IP network address to be used to bind the local end of
			// outgoing SMTP connections. By default the address will depend on the
			// routing tables in the normal way. Use "0.0.0.0" to use only IPv4
			// addresses returned from DNS lookups of the --forward-to address,
			// or "::" for IPv6.

		"i!pid-file!defines a file for storing the daemon process-id!!2!pid-file!3|"
			//example: /run/emailrelay/emailrelay.pid
			//example: C:/ProgramData/E-MailRelay/pid.txt
			// Causes the process-id to be written into the specified file when the
			// program starts up, typically after it has become a backgroud daemon.

		"O!poll!enables polling of the spool directory for messages to be forwarded with the specified period! (requires --forward-to)!1!period!3|"
			//example: 60
			// Causes forwarding of spooled mail messages to happen at regular intervals
			// (with the time given in seconds).

		"!address-verifier!specifies an external program for address verification!!1!program!3|"
			//example: /usr/local/sbin/emailrelay-verifier.sh
			//example: C:/ProgramData/E-MailRelay/verifier.js
			// Runs the specified external program to verify a message recipent's e-mail
			// address. A network verifier can be specified as "net:<transport-address>".

		"Y!client-filter!specifies an external program to process messages when they are forwarded!!1!program!3|"
			//example: /usr/local/sbin/emailrelay-client-filter
			//example: C:/ProgramData/E-MailRelay/client-filter.js
			// Runs the specified external filter program whenever a mail message is
			// forwarded. The filter is passed the name of the message file in the spool
			// directory so that it can edit it as required. A network filter can be
			// specified as "net:<transport-address>" and prefixes of "spam:", "spam-edit:"
			// and "exit:" are also allowed. The --filter option is normally more useful
			// than --client-filter.

		"Q!admin-terminate!enables the terminate command on the admin interface!!0!!3|"
			// Enables the "terminate" command in the administration interface.

		"A!anonymous!disables the SMTP VRFY command and sends less verbose SMTP responses!!0!!3|"
			// Disables the server's SMTP VRFY command, sends less verbose SMTP
			// responses and SMTP greeting, and stops "Received" lines being
			// added to mail message content files.

		"B!pop!enables the pop server!!0!!3|"
			// Enables the POP server listening, by default on port 110, providing
			// access to spooled mail messages. Negotiated TLS using the POP "STLS"
			// command will be enabled if the --server-tls option is also given.

		"E!pop-port!specifies the pop listening port number (default is 110)! (requires --pop)!1!port!3|"
			//default: 110
			//example: 995
			// Sets the POP server's listening port number.

		"F!pop-auth!defines the pop server secrets file!!1!file!3|"
			//example: /etc/private/emailrelay-pop.auth
			//example: C:/ProgramData/E-MailRelay/pop.auth
			//example: /pam
			// Specifies a file containing valid POP account details. The file
			// format is the same as for the SMTP server secrets file, ie. lines
			// starting with "server", with user-id and password in the third
			// and fourth fields. A special value of "/pam" can be used for
			// authentication using linux PAM.

		"G!pop-no-delete!disables message deletion via pop! (requires --pop)!0!!3|"
			// Disables the POP DELE command so that the command appears to succeed
			// but mail messages are not deleted from the spool directory.

		"J!pop-by-name!modifies the pop spool directory according to the pop user name! (requires --pop)!0!!3|"
			// Modifies the spool directory used by the POP server to be a
			// sub-directory with the same name as the POP authentication user-id.
			// This allows multiple POP clients to read the spooled messages
			// without interfering with each other, particularly when also
			// using --pop-no-delete. Content files can stay in the main spool
			// directory with only the envelope files copied into user-specific
			// sub-directories. The "emailrelay-filter-copy" program is a
			// convenient way of doing this when run via --filter.

		"M!size!limits the size of submitted messages!!1!bytes!3|"
			//example: 10000000
			// Limits the size of mail messages that can be submitted over SMTP.

		"!dnsbl!configuration for DNSBL blocking of smtp client addresses!!2!config!3|"
			//example: 1.1.1.1:53,1000,1,spam.dnsbl.example.com,block.dnsbl.example.com
			// Specifies a list of DNSBL servers that are used to reject SMTP
			// connections from blocked addresses. The configuration string
			// is made up of comma-separated fields: the DNS server's
			// transport address, a timeout in milliseconds, a rejection
			// threshold, and then the list of DNSBL servers.

		"!test!!!1!x!99|"
	) ;

	return ( is_windows ? windows_ : unix_ ) + common ;
}

/// \file options.cpp
