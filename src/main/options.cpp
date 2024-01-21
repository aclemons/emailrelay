//
// Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
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
///
/// \file options.cpp
///

#include "gdef.h"
#include "options.h"
#include "ggettext.h"

namespace Main
{
	namespace OptionsImp
	{
		constexpr unsigned int t_logging = 1U<<0 ;
		constexpr unsigned int t_process = 1U<<1 ;
		constexpr unsigned int t_tls = 1U<<2 ;
		//constexpr unsigned int t_xxx = 1U<<3 ;
		constexpr unsigned int t_smtpclient = 1U<<4 ;
		constexpr unsigned int t_smtpserver = 1U<<5 ;
		constexpr unsigned int t_pop = 1U<<6 ;
		constexpr unsigned int t_info = 1U<<7 ;
		constexpr unsigned int t_auth = 1U<<8 ;
		constexpr unsigned int t_admin = 1U<<9 ;
		constexpr unsigned int t_filter = 1U<<10 ;
		constexpr unsigned int t_basic = 1U<<11 ;
	}
}

std::vector<Main::Options::Tag> Main::Options::tags()
{
	using namespace Main::OptionsImp ;
	using G::tx ;
	return {
		{ t_basic , tx("Basic options") } ,
		{ t_smtpclient , tx("SMTP client options") } ,
		{ t_smtpserver , tx("SMTP server options") } ,
		{ t_pop , tx("POP server options") } ,
		{ t_admin , tx("Admin server options") } ,
		{ t_auth , tx("Authentication options") } ,
		{ t_tls , tx("TLS options") } ,
		{ t_process , tx("Process options") } ,
		{ t_logging , tx("Logging options") } ,
	} ;
}

// clang-format off

G::Options Main::Options::spec( bool is_windows )
{
	using G::tx ;
	using M = G::Option::Multiplicity ;
	using namespace Main::OptionsImp ;

	G::Options opt ;

	if( is_windows )
	{
		#ifdef G_WINDOWS

		G::Options::add( opt , 'l' , "log" ,
			tx("log information on stderr and to the event log! "
				"(but see --close-stderr and --no-syslog)") , "" ,
			M::zero , "" , 20 ,
			t_logging ) ;

		G::Options::add( opt , 't' , "no-daemon" ,
			tx("uses an ordinary window, not the system tray!, equivalent to --show=window") , "" ,
			M::zero , "" , 30 ,
			t_process ) ;

		G::Options::add( opt , 'k' , "syslog" ,
			tx("forces system event log output if logging is enabled (overrides --no-syslog)") , "" ,
			M::zero , "" , 30 ,
			t_logging ) ;

		G::Options::add( opt , 'n' , "no-syslog" ,
			tx("disables use of the system event log") , "" ,
			M::zero , "" , 30 ,
			t_logging ) ;

		G::Options::add( opt , 'H' , "hidden" ,
			tx("hides the application window and suppresses message boxes (requires --no-daemon)") , "" ,
			M::zero , "" , 30 ,
			t_process ) ;
				// Windows only. Hides the application window and disables all message
				// boxes, overriding any --show option. This is useful when running
				// as a windows service.

		G::Options::add( opt , '\0' , "show" ,
			tx("start the application window in the given style") , "" ,
			M::one , "style" , 30 ,
			t_process ) ;
				// Windows only. Starts the application window in the given style: "hidden",
				// "popup", "window", "window,tray", or "tray". Ignored if also using
				// --no-daemon or --hidden. If none of --window, --no-daemon and
				// --hidden are used then the default style is "tray".

		#endif
	}
	else
	{
		G::Options::add( opt , 'l' , "log" ,
			tx("writes log information on standard error and syslog! "
				"(but see --close-stderr and --no-syslog)") , "" ,
			M::zero , "" , 20 ,
			t_logging ) ;
				// Enables logging to the standard error stream and to the syslog. The
				// --close-stderr and --no-syslog options can be used to disable output to
				// standard error stream and the syslog separately. Note that --as-server,
				// --as-client and --as-proxy imply --log, and --as-server and --as-proxy
				// also imply --close-stderr.

		G::Options::add( opt , 't' , "no-daemon" ,
			tx("does not detach from the terminal") , "" ,
			M::zero , "" , 30 ,
			t_process ) ;
				// Disables the normal backgrounding at startup so that the program
				// runs in the foreground, without forking or detaching from the
				// terminal.
				//
				// On Windows this disables the system tray icon so the program
				// uses a normal window; when the window is closed the program
				// terminates.

		G::Options::add( opt , 'u' , "user" ,
			tx("names the effective user to switch to if started as root (default is \"daemon\")") , "" ,
			M::one , "username" , 30 ,
			t_process ) ;
				//default: daemon
				//example: nobody
				// When started as root the program switches to a non-privileged effective
				// user-id when idle or when running external filter scripts and address
				// verifiers. This option can be used to define the non-privileged user-id.
				// It also determines the group ownership of new files and sockets if the
				// directory owner is not 'sticky'. Specify "root" to disable all user-id
				// switching. Ignored on Windows.

		G::Options::add( opt , 'k' , "syslog" ,
			tx("forces syslog output if logging is enabled (overrides --no-syslog)") , "" ,
			M::zero_or_one , "facility" , 30 ,
			t_logging ) ;
				// When used with --log this option enables logging to the syslog even
				// if the --no-syslog option is also used. This is typically used as
				// a convenient override when using --as-client.

		G::Options::add( opt , 'n' , "no-syslog" ,
			tx("disables syslog output (always overridden by --syslog)") , "" ,
			M::zero , "" , 30 ,
			t_logging ) ;
				// Disables logging to the syslog. Note that
				// --as-client implies --no-syslog.

		G::Options::add( opt , '\0' , "localedir" ,
			tx("enables text localisation using the given locale base directory") , "" ,
				M::one , "dir" , 30 ,
				t_process ) ;
					//example: /opt/share/locale
					// Enables localisation and specifies the locale base directory where
					// message catalogues can be found. An empty directory can be used
					// for the built-in default.
	}

	{
		G::Options::add( opt , 'q' , "as-client" ,
			tx("runs as a client, forwarding all spooled mail to <host>!: "
				"equivalent to \"--log --no-syslog --no-daemon --dont-serve --forward --forward-to\"") , "" ,
			M::one , "host:port" , 10 ,
			t_basic , t_smtpclient ) ;
				//example: smtp.example.com:25
				// This is equivalent to --log, --no-syslog, --no-daemon, --dont-serve,
				// --forward and --forward-to. It is a convenient way of running a
				// forwarding agent that forwards spooled mail messages and then
				// terminates.

		G::Options::add( opt , 'd' , "as-server" ,
			tx("runs as a server, storing mail in the spool directory!: "
				"equivalent to \"--log --close-stderr\"") , "" ,
			M::zero , "" , 10 ,
			t_basic , t_smtpserver ) ;
				// This is equivalent to --log and --close-stderr. It is a convenient way
				// of running a background storage daemon that accepts mail messages and
				// spools them. Use --log instead of --as-server to keep standard error
				// stream open.

		G::Options::add( opt , 'y' , "as-proxy" ,
			tx("runs as a proxy server, forwarding each mail immediately to <host>!: "
				"equivalent to \"--log --close-stderr --forward-on-disconnect --forward-to\"") , "" ,
			M::one , "host:port" , 10 ,
			t_basic ) ;
				//example: smtp.example.com:25
				// This is equivalent to --log, --close-stderr, --forward-on-disconnect and
				// --forward-to. It is a convenient way of running a store-and-forward
				// daemon. Use --log, --forward-on-disconnect and --forward-to instead
				// of --as-proxy to keep the standard error stream open.

		G::Options::add( opt , 'v' , "verbose" ,
			tx("generates more verbose output! "
				"(works with --help and --log)") , "" ,
			M::zero , "" , 10 ,
			t_logging , t_basic ) ;
				// Enables more verbose logging when used with --log, and more verbose
				// help when used with --help.

		G::Options::add( opt , 'h' , "help" ,
			tx("displays help text and exits") , "" ,
			M::zero , "" , 11 ,
			t_basic , t_info ) ;
				// Displays help text and then exits. Use with --verbose for more complete
				// output.

		G::Options::add( opt , 'p' , "port" ,
			tx("specifies the SMTP listening port number (default is 25)") , "" ,
			M::one , "port" , 20 ,
			t_smtpserver ) ;
				//default: 25
				//example: 587
				// Sets the port number used for listening for incoming SMTP connections.

		G::Options::add( opt , 'r' , "remote-clients" ,
			tx("allows remote clients to connect") , "" ,
			M::zero , "" , 20 ,
			t_smtpserver ) ;
				// Allows incoming connections from addresses that are not local. The
				// default behaviour is to reject connections that are not local in
				// order to prevent accidental exposure to the public internet,
				// although a firewall should also be used. Local address ranges are
				// defined in RFC-1918, RFC-6890 etc.

		G::Options::add( opt , 's' , "spool-dir" ,
			tx("specifies the spool directory") , "" ,
			M::one , "dir" , 10 ,
			t_basic ) ;
				//example: /var/spool/emailrelay
				//example: C:/ProgramData/E-MailRelay/spool
				// Specifies the directory used for holding mail messages that have been
				// received but not yet forwarded.

		G::Options::add( opt , 's' , "delivery-dir" ,
			tx("specifies a base directory for local mailbox delivery") , "" ,
			M::one , "dir" , 30 ,
			t_smtpserver ) ;
				//example: /var/spool/emailrelay/in
				//example: C:/ProgramData/E-MailRelay/spool/in
				// Specifies the base directory for mailboxes when delivering
				// messages that have local recipients. This defaults to the main
				// spool directory.

		G::Options::add( opt , 'V' , "version" ,
			tx("displays version information and exits") , "" ,
			M::zero , "" , 20 ,
			t_basic , t_info|t_logging ) ;
				// Displays version information and then exits.

		G::Options::add( opt , 'K' , "server-tls" ,
			tx("enables negotiated TLS when acting as an SMTP server! "
				"(ie. STARTTLS) (requires --server-tls-certificate)") , "" ,
			M::zero , "" , 30 ,
			t_tls , t_smtpserver ) ;
				// Enables TLS for incoming SMTP and POP connections. SMTP clients can
				// then request TLS encryption by issuing the STARTTLS command. The
				// --server-tls-certificate option must be used to define the server
				// certificate.

		G::Options::add( opt , '\0' , "server-tls-connection" ,
			tx("enables implicit TLS when acting as an SMTP server! "
				"(ie. SMTPS) (requires --server-tls-certificate)") , "" ,
			M::zero , "" , 30 ,
			t_tls , t_smtpserver ) ;
				// Enables SMTP over TLS when acting as an SMTP server. This is for SMTP
				// over TLS (SMTPS), not TLS negotiated within SMTP using STARTTLS.

		G::Options::add( opt , '\0' , "server-tls-required" ,
			tx("mandatory use of TLS before SMTP server authentication or mail-to") , "" ,
			M::zero , "" , 30 ,
			t_tls , t_smtpserver ) ;
				// Makes the use of TLS mandatory for any incoming SMTP and POP connections.
				// SMTP clients must use the STARTTLS command to establish a TLS session
				// before they can issue SMTP AUTH or SMTP MAIL-TO commands.

		G::Options::add( opt , '\0' , "server-tls-certificate" ,
			tx("specifies a private TLS key+certificate file for --server-tls! "
				"or --server-tls-connection") , "" ,
			M::many , "pem-file" , 30 ,
			t_tls , t_smtpserver ) ;
				//example: /etc/ssl/certs/emailrelay.pem
				//example: C:/ProgramData/E-MailRelay/emailrelay.pem
				// Defines the TLS certificate file when acting as a SMTP or POP server.
				// This file must contain the server's private key and certificate chain
				// using the PEM file format. Alternatively, use this option twice
				// with the first specifying the key file and the second the certificate
				// file. Keep the file permissions tight to avoid accidental exposure
				// of the private key.

		G::Options::add( opt , '\0' , "server-tls-verify" ,
			tx("enables verification of remote client's certificate! "
				"against CA certificates in the given file or directory") , "" ,
			M::one , "ca-list" , 30 ,
			t_tls , t_smtpserver ) ;
				//example: /etc/ssl/certs/ca-certificates.crt
				//example: C:/ProgramData/E-MailRelay/ca-certificates.crt
				// Enables verification of remote SMTP and POP clients' certificates
				// against any of the trusted CA certificates in the specified file
				// or directory. In many use cases this should be a file containing
				// just your self-signed root certificate. Specify "<default>"
				// (including the angle brackets) for the TLS library's default set
				// of trusted CAs.

		G::Options::add( opt , 'j' , "client-tls" ,
			tx("enables negotiated TLS when acting as an SMTP client! "
				"(ie. STARTTLS)") , "" ,
			M::zero , "" , 30 ,
			t_tls , t_smtpclient ) ;
				// Enables negotiated TLS for outgoing SMTP connections; the SMTP
				// STARTTLS command will be issued if the remote server supports it.

		G::Options::add( opt , 'b' , "client-tls-connection" ,
			tx("enables SMTP over TLS for SMTP client connections") , "" ,
			M::zero , "" , 30 ,
			t_tls , t_smtpclient ) ;
				// Enables the use of a TLS tunnel for outgoing SMTP connections.
				// This is for SMTP over TLS (SMTPS), not TLS negotiated within SMTP
				// using STARTTLS.

		G::Options::add( opt , '\0' , "client-tls-certificate" ,
			tx("specifies a private TLS key+certificate file for --client-tls") , "" ,
			M::many , "pem-file" , 30 ,
			t_tls , t_smtpclient ) ;
				//example: /etc/ssl/certs/emailrelay.pem
				//example: C:/ProgramData/E-MailRelay/emailrelay.pem
				// Defines the TLS certificate file when acting as a SMTP client. This file
				// must contain the client's private key and certificate chain using the
				// PEM file format. Alternatively, use this option twice with the first
				// one specifying the key file and the second the certificate file.
				// Keep the file permissions tight to avoid accidental exposure of
				// the private key.

		G::Options::add( opt , '\0' , "client-tls-verify" ,
			tx("enables verification of remote server's certificate! "
				"against CA certificates in the given file or directory") , "" ,
			M::one , "ca-list" , 30 ,
			t_tls , t_smtpclient ) ;
				//example: /etc/ssl/certs/ca-certificates.crt
				//example: C:/ProgramData/E-MailRelay/ca-certificates.crt
				// Enables verification of the remote SMTP server's certificate against
				// any of the trusted CA certificates in the specified file or directory.
				// In many use cases this should be a file containing just your self-signed
				// root certificate. Specify "<default>" (including the angle brackets)
				// for the TLS library's default set of trusted CAs.

		G::Options::add( opt , '\0' , "client-tls-verify-name" ,
			tx("enables verification of the cname in the remote server's certificate! "
				"(requires --client-tls-verify)") , "" ,
			M::one , "cname" , 30 ,
			t_tls , t_smtpclient ) ;
				//example: smtp.example.com
				// Enables verification of the CNAME within the remote SMTP server's certificate.

		G::Options::add( opt , '\0' , "client-tls-server-name" ,
			tx("includes the server hostname in the tls handshake! "
				"(ie. server name identification)") , "" ,
			M::one , "hostname" , 30 ,
			t_tls , t_smtpclient ) ;
				//example: smtp.example.com
				// Defines the target server hostname in the TLS handshake. With
				// --client-tls-connection this can be used for SNI, allowing the remote
				// server to adopt an appropriate identity.

		G::Options::add( opt , '\0' , "client-tls-required" ,
			tx("mandatory use of TLS for SMTP client connections! "
				"(requires --client-tls)") , "" ,
			M::zero , "" , 30 ,
			t_tls , t_smtpclient ) ;
				// Makes the use of TLS mandatory for outgoing SMTP connections. The SMTP
				// STARTTLS command will be used before mail messages are sent out.
				// If the remote server does not allow STARTTLS then the SMTP connection
				// will fail.

		G::Options::add( opt , '9' , "tls-config" ,
			tx("sets low-level TLS configuration options! "
				"(eg. tlsv1.2)") , "" ,
			M::many , "options" , 30 ,
			t_tls ) ;
				//example: mbedtls,tlsv1.2
				// Selects and configures the low-level TLS library, using a comma-separated
				// list of keywords. If OpenSSL and mbedTLS are both built in then keywords
				// of "openssl" and "mbedtls" will select one or the other. Keywords like
				// "tlsv1.0" can be used to set a minimum TLS protocol version, or
				// "-tlsv1.2" to set a maximum version.

		G::Options::add( opt , 'g' , "debug" ,
			tx("generates debug-level logging if built in") , "" ,
			M::zero , "" , 30 ,
			t_logging ) ;
				// Enables debug level logging, if built in. Debug messages are usually
				// only useful when cross-referenced with the source code and they may
				// expose plain-text passwords and mail message content.

		G::Options::add( opt , 'C' , "client-auth" ,
			tx("enables SMTP authentication with the remote server, using the given client secrets file") , "" ,
			M::one , "file" , 30 ,
			t_auth , t_smtpclient ) ;
				//example: /etc/private/emailrelay.auth
				//example: C:/ProgramData/E-MailRelay/emailrelay.auth
				//example: plain:bWU:c2VjcmV0
				// Enables SMTP client authentication with the remote server, using the
				// client account details taken from the specified secrets file.
				// The secrets file should normally contain one line having between four
				// and five space-separated fields. The first field must be "client",
				// the second field is the password type ("plain" or "md5"), the
				// third is the xtext-encoded user-id and the fourth is the xtext-encoded
				// password. Alternatively, the user-id and password fields can be
				// Base64 encoded if the second field is "plain:b". It is also possible
				// to do without a secrets file and give the Base64 encoded user-id and
				// password directly on the command-line or in the configuration file
				// formatted as "plain:<base64-user-id>:<base64-password>". Note that
				// putting these account details on the command-line is not recommended
				// because it will make the password easily visible to all users on the
				// local machine.

		G::Options::add( opt , '\0' , "client-auth-config" ,
			tx("configures the client authentication module") , "" ,
			M::one , "config" , 30 ,
			t_auth , t_smtpclient ) ;
				//example: m:cram-sha1,cram-md5
				//example: x:plain,login
				//example: m:;a:plain
				// Configures the SMTP client authentication module using a
				// semicolon-separated list of configuration items. Each item is a
				// single-character key, followed by a colon and then a comma-separated
				// list. A 'm' character introduces an ordered list of preferred
				// authentication mechanisms and an 'x' introduces a list of mechanisms
				// to avoid. An 'a' list and a 'd' list can be used similarly to prefer
				// and avoid certain mechanisms once the session is encrypted with TLS.

		G::Options::add( opt , 'L' , "log-time" ,
			tx("adds a timestamp to the logging output") , "" ,
			M::zero , "" , 30 ,
			t_logging ) ;
				// Adds a timestamp to the logging output using the local timezone.

		G::Options::add( opt , '\0' , "log-address" ,
			tx("adds the network address of remote clients to the logging output") , "" ,
			M::zero , "" , 30 ,
			t_logging ) ;
				// Adds the network address of remote clients to the logging output.

		G::Options::add( opt , 'N' , "log-file" ,
			tx("log to file instead of stderr! "
				"(with '%d' replaced by the current date)") , "" ,
			M::one , "file" , 30 ,
			t_logging ) ;
				//example: /var/log/emailrelay-%d
				//example: C:/ProgramData/E-MailRelay/log-%d.txt
				// Redirects standard-error logging to the specified file. Logging to
				// the log file is not affected by --close-stderr. The filename can
				// include "%d" to get daily log files; the "%d" is replaced by the
				// current date in the local timezone using a "YYYYMMDD" format.

		G::Options::add( opt , 'S' , "server-auth" ,
			tx("enables authentication of remote SMTP clients, using the given server secrets file") , "" ,
			M::one , "file" , 30 ,
			t_auth , t_smtpserver ) ;
				//example: /etc/private/emailrelay.auth
				//example: C:/ProgramData/E-MailRelay/emailrelay.auth
				//example: pam:
				// Enables SMTP server authentication of remote SMTP clients. Account
				// names and passwords are taken from the specified secrets file. The
				// secrets file should contain lines that have four space-separated
				// fields, starting with "server" in the first field; the second field
				// is the password encoding ("plain" or "md5"), the third is the client
				// user-id and the fourth is the password. The user-id is RFC-1891 xtext
				// encoded, and the password is either xtext encoded or generated by
				// "emailrelay-passwd". Alternatively, the username and password can be
				// Base64 encoded if the second field is "plain:b". A special value of
				// "pam:" can be used for authentication using linux PAM.

		G::Options::add( opt , '\0' , "server-auth-config" ,
			tx("configures the server authentication module") , "" ,
			M::one , "config" , 30 ,
			t_auth , t_smtpserver ) ;
				//example: m:cram-sha256,cram-sha1
				//example: x:plain,login
				//example: m:;a:plain
				// Configures the SMTP server authentication module using a
				// semicolon-separated list of configuration items. Each item is a
				// single-character key, followed by a colon and then a comma-separated
				// list. A 'm' character introduces an ordered list of allowed
				// authentication mechanisms and an 'x' introduces a list of
				// mechanisms to deny. An 'a' list and a 'd' list can be used similarly
				// to allow and deny mechanisms once the session is encrypted with
				// TLS. In typical usage you might have an empty allow list for an
				// unencrypted session and a single preferred mechanism once
				// encrypted, "m:;a:plain".

		G::Options::add( opt , 'Z' , "server-smtp-config" ,
			tx("configures the smtp server protocol") , "" ,
			M::many , "config" , 30 ,
			t_smtpserver ) ;
				//example: +chunking,+smtputf8
				// Configures the SMTP server protocol using a comma-separated
				// list of optional features, including 'pipelining', 'chunking',
				// 'smtputf8', and 'smtputf8strict'.

		G::Options::add( opt , 'c' , "client-smtp-config" ,
			tx("configures the smtp client protocol") , "" ,
			M::many , "config" , 30 ,
			t_smtpclient ) ;
				//example: +eightbitstrict,-pipelining
				// Configures the SMTP client protocol using a comma-separated
				// list of optional features, including 'pipelining',
				// 'smtputf8strict', 'eightbitstrict' and 'binarymimestrict'.

		G::Options::add( opt , 'e' , "close-stderr" ,
			tx("closes the standard error stream soon after start-up") , "" ,
			M::zero , "" , 31 ,
			t_logging , t_process ) ;
				// Causes the standard error stream to be closed soon after start-up.
				// This is useful when operating as a background daemon and it is
				// therefore implied by --as-server and --as-proxy.

		G::Options::add( opt , 'a' , "admin" ,
			tx("enables the administration interface and specifies its listening port number") , "" ,
			M::one , "port" , 30 ,
			t_admin ) ;
				//example: 587
				// Enables an administration interface on the specified listening port
				// number. Use telnet or something similar to connect. The administration
				// interface can be used to trigger forwarding of spooled mail messages
				// if the --forward-to option is used.

		G::Options::add( opt , 'x' , "dont-serve" ,
			tx("disables acting as a server on any port! "
				"(part of --as-client and usually used with --forward)") , "" ,
			M::zero , "" , 30 ,
			t_process ) ;
				// Disables all network serving, including SMTP, POP and administration
				// interfaces. The program will terminate as soon as any initial
				// forwarding is complete.

		G::Options::add( opt , 'X' , "no-smtp" ,
			tx("disables listening for SMTP connections! "
				"(usually used with --admin or --pop)") , "" ,
			M::zero , "" , 30 ,
			t_process ) ;
				// Disables listening for incoming SMTP connections.

		G::Options::add( opt , 'z' , "filter" ,
			tx("specifies an external program to process messages as they are stored") , "" ,
			M::many , "program" , 30 ,
			t_smtpserver , t_filter ) ;
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
				// 1 and 99. Use "net:<tcp-address>" to communicate with a filter
				// daemon over the network, or "spam:<tcp-address>" for a
				// spamassassin spamd daemon to accept or reject mail messages, or
				// "spam-edit:<tcp-address>" to have spamassassin edit the message
				// content without rejecting it, or "exit:<number>" to emulate a filter
				// program that just exits.

		G::Options::add( opt , 'W' , "filter-timeout" ,
			tx("sets the timeout (in seconds) for running the --filter (default is 60)") , "" ,
			M::one , "time" , 30 ,
			t_smtpserver , t_filter ) ;
				//default: 60
				//example: 10
				// Specifies a timeout (in seconds) for running a --filter program. The
				// default is 60 seconds.

		G::Options::add( opt , 'w' , "prompt-timeout" ,
			tx("sets the timeout (in seconds) for getting an initial prompt from the server (default is 20)") , "" ,
			M::one , "time" , 30 ,
			t_smtpserver ) ;
				//default: 20
				//example: 3
				// Specifies a timeout (in seconds) for getting the initial prompt from
				// a remote SMTP server. If no prompt is received after this time then
				// the SMTP dialog goes ahead without it.

		G::Options::add( opt , 'D' , "domain" ,
			tx("sets an override for the host's fully qualified network name") , "" ,
			M::one , "fqdn" , 30 ,
			t_smtpserver ) ;
				//example: smtp.example.com
				// Specifies the network name that is used in SMTP EHLO commands,
				// "Received" lines, and for generating authentication challenges.
				// The default is derived from a DNS lookup of the local hostname.

		G::Options::add( opt , 'f' , "forward" ,
			tx("forwards stored mail on startup! "
				"(requires --forward-to)") , "" ,
			M::zero , "" , 30 ,
			t_smtpclient ) ;
				// Causes spooled mail messages to be forwarded when the program first
				// starts.

		G::Options::add( opt , '1' , "forward-on-disconnect" ,
			tx("forwards stored mail once the SMTP client disconnects! "
				"(requires --forward-to)") , "" ,
			M::zero , "" , 30 ,
			t_smtpclient ) ;
				// Causes spooled mail messages to be forwarded whenever a SMTP client
				// connection disconnects.

		G::Options::add( opt , 'o' , "forward-to" ,
			tx("specifies the address of the remote SMTP server! "
				"(required by --forward, --forward-on-disconnect and --immediate)") , "" ,
			M::one , "host:port" , 30 ,
			t_smtpclient ) ;
				//example: smtp.example.com:25
				// Specifies the transport address of the remote SMTP server that
				// spooled mail messages are forwarded to.

		G::Options::add( opt , '\0' , "forward-to-some" ,
			tx("allows forwarding to some addressees even if others are rejected") , "" ,
			M::zero , "" , 32 ,
			t_smtpclient ) ;
				// Allow forwarding to continue even if some recipient addresses on an
				// e-mail envelope are rejected by the remote server.

		G::Options::add( opt , '\0' , "forward-to-all" ,
			tx("requires all addressees to be accepted when forwarding") , "" ,
			M::zero , "" , 32 ,
			t_smtpclient ) ;
				// Requires all recipient addresses to be accepted by the remote
				// server before forwarding. This is currently the default behaviour
				// so this option is for forwards compatibility only.

		G::Options::add( opt , 'T' , "response-timeout" ,
			tx("sets the response timeout (in seconds) when talking to a remote server (default is 60)") , "" ,
			M::one , "time" , 31 ,
			t_smtpclient ) ;
				//default: 60
				//example: 2
				// Specifies a timeout (in seconds) for getting responses from remote
				// SMTP servers. The default is 60 seconds.

		G::Options::add( opt , '\0' , "idle-timeout" ,
			tx("sets the connection idle timeout (in seconds) (default is 60)") , "" ,
			M::one , "time" , 31 ,
			t_smtpclient ) ;
				//default: 60
				//example: 2
				// Specifies a timeout (in seconds) for receiving network traffic from
				// remote SMTP and POP clients. The default is 60 seconds.

		G::Options::add( opt , 'U' , "connection-timeout" ,
			tx("sets the timeout (in seconds) when connecting to a remote server (default is 40)") , "" ,
			M::one , "time" , 31 ,
			t_smtpclient ) ;
				//default: 40
				//example: 10
				// Specifies a timeout (in seconds) for establishing a TCP connection
				// to remote SMTP servers. The default is 40 seconds.

		G::Options::add( opt , 'm' , "immediate" ,
			tx("enables immediate forwarding of messages as they are received! "
				"from the submitting client and before their receipt is acknowledged (requires --forward-to)") , "" ,
			M::zero , "" , 32 ,
			t_smtpclient , t_smtpserver ) ;
				// Causes mail messages to be forwarded as they are received, even before
				// they have been accepted. This can be used to do proxying without
				// store-and-forward, but in practice clients tend to to time out
				// while waiting for their mail message to be accepted.

		G::Options::add( opt , 'I' , "interface" ,
			tx("defines the listening network addresses used for incoming connections! "
				"(comma-separated list with optional smtp=,pop=,admin= qualifiers)") , "" ,
			M::many , "ip-address-list" , 30 ,
			t_smtpserver ) ;
				//example: 127.0.0.1,smtp=eth0
				//example: fe80::1%1,smtp=::,admin=lo-ipv4,pop=10.0.0.1
				//example: smtp=fd#3,smtp=fd#4,pop=fd#5
				//example: lo
				//example: 10.0.0.1
				// Specifies the IP network addresses or interface names used to bind
				// listening ports. By default listening ports for incoming SMTP, POP
				// and administration connections will bind the 'any' address for IPv4
				// and for IPv6, ie. "0.0.0.0" and "::". Multiple addresses can be
				// specified by using the option more than once or by using a
				// comma-separated list. Use a prefix of "smtp=", "pop=" or "admin=" on
				// addresses that should apply only to those types of listening port.
				// Any link-local IPv6 addresses must include a zone name or scope id.
				//
				// Interface names can be used instead of addresses, in which case all
				// the addresses associated with that interface at startup will used
				// for listening. When an interface name is decorated with a "-ipv4"
				// or "-ipv6" suffix only their IPv4 or IPv6 addresses will be used
				// (eg. "ppp0-ipv4").
				//
				// To inherit listening file descriptors from the parent process on
				// unix use a syntax like this: --interface smtp=fd#3,smtp=fd#4,pop=fd#5.

		G::Options::add( opt , '6' , "client-interface" ,
			tx("defines the local network address used for outgoing connections") , "" ,
			M::one , "ip-address" , 31 ,
			t_smtpclient ) ;
				//example: 10.0.0.2
				// Specifies the IP network address to be used to bind the local end of
				// outgoing SMTP connections. By default the address will depend on the
				// routing tables in the normal way. Use "0.0.0.0" to use only IPv4
				// addresses returned from DNS lookups of the --forward-to address,
				// or "::" for IPv6.

		G::Options::add( opt , 'i' , "pid-file" ,
			tx("defines a file for storing the daemon process-id") , "" ,
			M::many , "path" , 30 ,
			t_process ) ;
				//example: /run/emailrelay/emailrelay.pid
				//example: C:/ProgramData/E-MailRelay/pid.txt
				// Causes the process-id to be written into the specified file when the
				// program starts up, typically after it has become a background daemon.
				// The immediate parent directory is created if necessary.

		G::Options::add( opt , 'O' , "poll" ,
			tx("enables polling of the spool directory for messages to be forwarded with the specified period! "
				"(requires --forward-to)") , "" ,
			M::one , "period" , 30 ,
			t_smtpclient ) ;
				//example: 60
				// Causes forwarding of spooled mail messages to happen at regular intervals
				// (with the time given in seconds).

		G::Options::add( opt , '\0' , "address-verifier" ,
			tx("specifies an external program for address verification") , "" ,
			M::one , "program" , 30 ,
			t_smtpserver ) ;
				//example: /usr/local/sbin/emailrelay-verifier.sh
				//example: C:/ProgramData/E-MailRelay/verifier.js
				// Runs the specified external program to verify a message recipient's e-mail
				// address. A network verifier can be specified as "net:<tcp-address>". The
				// "account:" built-in address verifier can be used to check recipient
				// addresses against the list of local system account names.

		G::Options::add( opt , 'Y' , "client-filter" ,
			tx("specifies an external program to process messages when they are forwarded") , "" ,
			M::many , "program" , 31 ,
			t_smtpclient , t_filter ) ;
				//example: /usr/local/sbin/emailrelay-client-filter
				//example: C:/ProgramData/E-MailRelay/client-filter.js
				// Runs the specified external filter program whenever a mail message is
				// forwarded. The filter is passed the name of the message file in the spool
				// directory so that it can edit it as required. A network filter can be
				// specified as "net:<tcp-address>" and prefixes of "spam:", "spam-edit:"
				// and "exit:" are also allowed. The "spam:" and "spam-edit:" prefixes
				// require a SpamAssassin daemon to be running. For store-and-forward
				// applications the --filter option is normally more useful than
				// --client-filter.

		G::Options::add( opt , 'Q' , "admin-terminate" ,
			tx("enables the terminate command on the admin interface") , "" ,
			M::zero , "" , 30 ,
			t_admin , t_process ) ;
				// Enables the "terminate" command in the administration interface.

		G::Options::add( opt , 'A' , "anonymous" ,
			tx("disables the SMTP VRFY command and sends less verbose SMTP responses") , "" ,
			M::zero_or_one , "scope" , 30 ,
			t_smtpserver ) ;
				// Disables the server's SMTP VRFY command, sends less verbose
				// SMTP greeting and responses, stops "Received" lines being
				// added to mail message content files, and stops the SMTP
				// client protocol adding "AUTH=" to the "MAIL" command.
				// For finer control use a comma-separated list of things
				// to anonymise: "vrfy", "server", "content" and/or "client".

		G::Options::add( opt , 'B' , "pop" ,
			tx("enables the pop server") , "" ,
			M::zero , "" , 30 ,
			t_pop , t_smtpserver ) ;
				// Enables the POP server, listening by default on port 110, providing
				// access to spooled mail messages. Negotiated TLS using the POP "STLS"
				// command will be enabled if the --server-tls option is also given.

		G::Options::add( opt , 'E' , "pop-port" ,
			tx("specifies the pop listening port number (default is 110)! "
				"(requires --pop)") , "" ,
			M::one , "port" , 30 ,
			t_pop , t_smtpserver ) ;
				//default: 110
				//example: 995
				// Sets the POP server's listening port number.

		G::Options::add( opt , 'F' , "pop-auth" ,
			tx("defines the pop server secrets file") , "" ,
			M::one , "file" , 31 ,
			t_auth , t_pop|t_smtpserver ) ;
				//example: /etc/private/emailrelay-pop.auth
				//example: C:/ProgramData/E-MailRelay/pop.auth
				//example: pam:
				// Specifies a file containing valid POP account details. The file
				// format is the same as for the SMTP server secrets file, ie. lines
				// starting with "server", with user-id and password in the third
				// and fourth fields. A special value of "pam:" can be used for
				// authentication using linux PAM.

		G::Options::add( opt , 'G' , "pop-no-delete" ,
			tx("disables message deletion via pop! "
				"(requires --pop)") , "" ,
			M::zero , "" , 30 ,
			t_pop , t_smtpserver ) ;
				// Disables the POP DELE command so that the command appears to succeed
				// but mail messages are not deleted from the spool directory.

		G::Options::add( opt , 'J' , "pop-by-name" ,
			tx("modifies the pop spool directory according to the pop user name! "
				"(requires --pop)") , "" ,
			M::zero , "" , 30 ,
			t_pop , t_smtpserver ) ;
				// Modifies the POP server's spool directory to be the sub-directory
				// with the same name as the user-id used for POP authentication. This
				// allows POP clients to see only their own messages after they have
				// been moved into separate sub-directories typically by the built-in
				// "deliver:" or "copy:" filters. Content files can remain in the
				// main spool directory to save disk space; they will be deleted by
				// the POP server when it deletes the last matching envelope file.

		G::Options::add( opt , 'M' , "size" ,
			tx("limits the size of submitted messages") , "" ,
			M::one , "bytes" , 30 ,
			t_smtpserver ) ;
				//example: 10000000
				// Limits the size of mail messages that can be submitted over SMTP.

		G::Options::add( opt , '\0' , "dnsbl" ,
			tx("configuration for DNSBL blocking of SMTP client addresses") , "" ,
			M::many , "config" , 30 ,
			t_smtpserver ) ;
				//example: 1.1.1.1:53,1000,1,spam.dnsbl.example.com,block.dnsbl.example.com
				// Specifies a list of DNSBL servers that are used to reject SMTP
				// connections from blocked addresses. The configuration string
				// is made up of comma-separated fields: the DNS server's
				// transport address, a timeout in milliseconds, a rejection
				// threshold, and then the list of DNSBL servers.

		G::Options::add( opt , '\0' , "test" , "testing" , "" , M::one , "x" , 0 , 0 ) ;
	}
	return opt ;
}

