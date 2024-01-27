**********
Change Log
**********

2.5.1 -> 2.5.2
==============

* Windows build scripts reworked.
* Filters can be Windows PowerShell *.ps1* scripts.
* Fix for client protocol [bug-id #57].

2.5 -> 2.5.1
============

* Filters can control the SMTP_ server's error response code.
* Proxying with *-\ -immediate* passes back SMTP reponses more faithfully.
* The *-\ -disable-admin* build-time configure option is restored.
* New *smtp disable* admin command to disable new SMTP sessions.
* Fewer Windows handles inherited by filter programs [bug-id #55].
* Fix for *-\ -dnsbl* option parsing.

2.4 -> 2.5
==========

* Multiple configurations in one process.
* SMTP PIPELINING (RFC-2920_).
* SMTP CHUNKING/8BITMIME 'BDAT' extension (RFC-3030_), disabled by default.
* SMTP SMTPUTF8 extension (RFC-6531_), disabled by default.
* No 7-bit/8-bit check on received message content (see NEWS file).
* New built-in filters: *deliver:*, *split:*, *copy:*, *mx:*, *msgid:*.
* New built-in address verifier: *account:*
* No *.local* files (see NEWS file).
* PAM_ authentication is now enabled with *-\ -server-auth=pam:* not */pam*.
* Client authentication details can be given directly from the command-line.
* Multiple *client* authentication secrets, selected by a new envelope field.
* Main binary can act as a simple submission tool (\ *configure -\ -enable-submission*\ ).

2.3 -> 2.4
==========

* Multiple *-\ -filter* and *-\ -client-filter* options allowed.
* TLS_ key and certificate files can be specified separately.
* Finer control when using *-\ -anonymous* (eg. *-\ -anonymous=server*).
* The *auth-config* options can distinguish between TLS and non-TLS authentication.
* Hourly log file rotation using *%h* (eg. *-\ -log-file=log.%d.%h*).
* Listening file descriptors can be inherited from the parent process (see *-\ -interface*).
* Listening ports on Windows use exclusive binding.
* The Linux event loop uses *epoll* by default rather than *select*.
* Some support for message routing (see NEWS).
* Fix of error handling in network client filters (\ *-\ -client-filter=net:...*\ ) [bug-id #50].

2.2 -> 2.3
==========

* Unix domain sockets supported (eg. *-\ -interface=/tmp/smtp.s*).
* Windows event log not used for verbose logging (prefer *-\ -log-file*).
* New admin *forward* command to trigger forwarding without waiting.
* Optional base64 encoding of passwords in secrets files (\ *plain:b*\ ).
* Support for MbedTLS version 3.

2.1 -> 2.2
==========

* Connections from IPv4 'private use' addresses are allowed by default (see *-\ -remote-clients*).
* Interface names can be used with *-\ -interface* (eg. *-\ -interface=eth0*).
* New *-\ -server-tls-connection* option for server-side implicit TLS.
* New *-\ -forward-to-some* option to permit some message recipients to be rejected.
* New *-\ -log-address* option to aid adaptive firewalling.
* Dynamic log file rolling when using *-\ -log-file=%d*.
* Choice of syslog 'facility' on Linux with *-\ -syslog=local0* etc.
* Pipelined SMTP QUIT commands sent by broken clients are tolerated.
* Better handling of overly-verbose or unkillable *-\ -filter* scripts.
* Optional epoll event loop on Linux (\ *configure -\ -enable-epoll*\ ).
* Some internationalisation support (see NEWS file).
* Support for Windows XP restored when built with mingw-w64.
* C++ 2011 compiler is required.

2.0.1 -> 2.1
============

* Backwards compatibility features for 1.9-to-2.0 transition removed.
* Better handling of too-many-connections on Windows.
* New *-\ -idle-timeout* option for server-side connections.
* Support for RFC-5782_ DNSBL_ blocking (\ *-\ -dnsbl*\ ).
* Filter scripts are given the path of the envelope file in argv2.
* Message files can be edited by *-\ -client-filter* scripts.
* Better support for CRAM-SHAx authentication.
* New *-\ -client-auth-config* and *-\ -server-auth-config* options.
* New *-\ -show* option on windows to better control the user interface style.
* The *-\ -pop* option always requires *-\ -pop-auth*.
* No message is spooled if all its envelope recipients are local-mailboxes.
* TLS cipher name added to *Received* line as per RFC-8314_ 4.3.
* Certificate contents are not logged.
* Timestamp parts of spool filenames no longer limited to six digits.

2.0 -> 2.0.1
============

* Make PLAIN client authentication work against servers with broken 334 responses.

1.9.2 -> 2.0
============

* Improved IPv6 support, with IPv4 and IPv6 used independently at run-time (see *-\ -interface*).
* Server process is not blocked during *-\ -filter* or *-\ -address-verifier* execution, if multi-threaded.
* Support for the *mbedTLS* TLS library as an alternative to OpenSSL (\ *configure -\ -with-mbedtls*\ ).
* TLS server certificates specified with new *-\ -server-tls-certificate* option, not *-\ -server-tls*.
* TLS servers enable client certificate verification with *-\ -server-tls-verify*, not *-\ -tls-config*.
* TLS clients can verify server certificates with *-\ -client-tls-verify* and *-\ -client-tls-verify-name*.
* The *-\ -tls-config* option works differently (see NEWS file).
* New *-\ -client-tls-server-name* option for server name identification (SNI).
* New *-\ -client-tls-required* option to force client connections to use TLS.
* New *-\ -server-tls-required* option to force remote SMTP clients to use STARTTLS.
* New *-\ -forward-on-disconnect* option replaces *-\ -poll=0*.
* The *-\ -anonymous* option now suppresses the *Received* line, whatever the *-\ -domain*.
* The second field in the secrets file indicates the password encoding, not AUTH mechanism.
* The *-\ -verifier* option is now *-\ -address-verifier*, with simplified command-line parameters.
* Command-line file paths can use *@app* as a prefix to be relative to the executable directory.
* Command-line file paths can be relative to the startup cwd even when daemonised.
* Filter exit codes between 104 and 107 are interpreted differently (see NEWS file).
* Message rejection reasons passed back to the submitting SMTP client are much less verbose.
* Forwarding events are queued up if the forwarding client is still busy from last time.
* The bind address for outgoing connections is no longer taken from first unqualified *-\ -interface* address [bug-id #27].
* The SMTP client protocol tries more than one authentication mechanism.
* Some support for XOAUTH2 client-side authentication.
* Client protocol sends QUIT with a socket shutdown().
* The Windows commdlg list-view widget is used for the server status pages.
* The Windows connection-lookup feature is withdrawn (\ *-\ -peer-lookup*\ ).
* Several build-time configure options like *-\ -disable-pop* are withdrawn.
* C++ 2011 is preferred, and required for multi-threading.
* Support for very old versions of Windows is dropped.

1.9.1 -> 1.9.2
==============

* Fixed a leak in the event-loop garbage collection.
* A local hostname that fails to resolve is not an error.
* A warning is emitted if there is more than one client authentication secret.
* Multiple *-\ -interface* options are allowed separately on the command-line.
* Added a new *-\ -client-interface* option.
* The *Received* line is formatted as per RFC-3848_ (\ *with ESMTPSA*\ ).
* The LOGIN and PLAIN mechanisms in the secrets file are now equivalent.
* The Windows service wrapper can use a configuration file to locate the startup batch file.
* Simplified the implementation of the GUI installation program.
* Reworded the *read error: disconnected* log message.
* Less verbose logging of *no more messages to send*.
* Qt4 or Qt5 selected by the *configure* script.
* Improved the RPM spec file.

1.9 -> 1.9.1
============

* Updated OpenSSL from 1.0.1e to 1.0.1g in the Windows build.

1.8.2 -> 1.9
============

* Added negotiated TLS/SSL for POP_ (ie. *STLS*).
* The first two fields in the secrets files are reordered (with backwards compatibility).
* Added Linux PAM authentication (*configure -\ -with-pam* and then *-\ -server-auth=/pam*).
* Optional protocol-specific *-\ -interface* qualifiers, eg. *-\ -interface smtp=127.0.0.1,pop=192.168.1.1*.
* Outgoing client connection bound with the first *-\ -interface* or *-\ -interface client=...* address.
* Support for SMTP-over-TLS on outgoing client connection (\ *-\ -client-tls-connection*\ ) (cf. *STARTTLS*)
* Support for SOCKS_ 4a on outgoing client connection, eg. *-\ -forward-to example.com:25@127.0.0.1:9050*.
* TLS configuration options (\ *-\ -tls-config=...*\ ) for SSLv2/3 fallback etc.
* No *Received* line added if *-\ -anonymous* and an empty *-\ -domain* name.
* Error text for *all recipients rejected* is now more accurately *one or more recipients rejected*.
* New behaviour for *-\ -client-filter* exit values of 100 and over.
* New commands on the admin interface, *failures* and *unfail-all*.
* Shorter descriptions in the usage help unless *-\ -verbose*.
* New default spool directory location on windows, now under *system32*.
* Windows project files for MSVC 2012 included.
* Removed support for Windows NT and Windows 9x.
* Better support for Windows Vista and Windows 7.
* Removed Windows *-\ -icon* option.
* Removed *-\ -enable-fhs* option for *configure* (see INSTALL document for equivalent usage).
* Added *-\ -log-file* option to redirect stderr.
* Added Windows *-\ -peer-lookup* option.
* Fix for MD5 code in 64-bit builds.

1.8.1 -> 1.8.2
==============

* Fix namespaces for gcc 3.4.

1.8 -> 1.8.1
============

* Changed the definition of *-\ -as-proxy* to use *-\ -poll 0* rather than *-\ -immediate* [bug-id 1961652].
* Fixed stalling bug when using server-side TLS/SSL (\ *-\ -server-tls*\ ) [bug-id 1961655].
* Improved Debian packaging for Linux (\ *make deb*\ ).

1.7 -> 1.8
==========

* Speed optimisations (as identified by KCachegrind/valgrind in KDevelop).
* Build-time size optimisations (eg. *./configure -\ -disable-exec -\ -enable-small-exceptions ...*).
* Build-time options to reduce runtime library dependencies (eg. *./configure -\ -disable-dns -\ -disable-identity*).
* New switch to limit the size of submitted messages (\ *-\ -size*\ ).
* New semantics for *-\ -poll 0*, providing a good alternative to *-\ -immediate* when proxying.
* SMTP client protocol emits a RSET after a rejected recipient as a workround for broken server protocols.
* SMTP client protocol continues if the server advertises AUTH but the client has no authentication secrets.
* When a message cannot be forwarded the offending SMTP protocol response number, if any, is put in the envelope file.
* A warning is printed if logging is requested but both stderr and syslog are disabled.
* A cross-compiling toolchain builder script added for running on mips-based routers (\ *extra/mips*\ ).
* New example scripts for SMTP multicasting and editing envelope files.
* Improved native support for Mac OS X (10.5) with graphical installation from disk image.
* Compatibility with gcc 2.95 restored.

1.6 -> 1.7
==========

* TLS/SSL support for SMTP using OpenSSL (*./configure -\ -with-openssl* with *-\ -client-tls* and *-\ -server-tls*).
* Authentication mechanism *PLAIN* added.
* Some tightening up of the SMTP server protocol.
* Windows service wrapper has an *-\ -uninstall* option.
* Windows installation GUI uninstalls the service before reinstalling it.

1.5 -> 1.6
==========

* GPLv3 licence (see *http://gplv3.fsf.org*).
* New *-\ -prompt-timeout* switch for the timeout when waiting for the initial 220 prompt from the SMTP server.
* Fix for flow-control assertion error when the POP server sends a very long list of spooled messages.
* Wildcard matching for trusted IP addresses in the authentication secrets file can now use CIDR notation.
* More fine-grained switching of effective user-id to read files and directories when running as root.
* Fewer new client connections when proxying.
* The server drops the connection if a remote SMTP client causes too many protocol errors.
* More complete implementation of *-\ -hidden* on Windows.
* Scanner switch (\ *-\ -scanner*\ ) replaced by a more general *-\ -filter* and *-\ -client-filter* switch syntax.
* Support for address verification (\ *-\ -verifier*\ ) over the network.
* Better support for running as a Windows service (\ *emailrelay-service -\ -install*\ ).
* Utility filter program *emailrelay-filter-copy* exits with 100 if it deletes the envelope file.
* Windows *cscript.exe* wrapper is added automatically to non-bat/exe *-\ -filter* command-lines.
* Installation GUI makes backups of the files it edits and preserves authentication secrets.
* Installation GUI can install *init.d* links.
* Experimental SpamAssassin spamc/spamd protocol support.
* Acceptance tests added to the distribution.

1.4 -> 1.5
==========

* New installation and configuration GUI using TrollTech Qt 4.x (\ *./configure -\ -enable-gui*\ )
* Default address verifier accepts all addresses as valid and never treats them as local mailboxes.
* Fix for server exit bug when failing to send data down a newly accepted connection.
* Spooled content files can be left in the parent directory to save diskspace when using *-\ -pop-by-name*.
* Client protocol improved for the case where there are no valid recipients.
* New *-\ -syslog* switch to override *-\ -no-syslog*.
* New *-\ -filter-timeout* switch added.
* Support for *-\ -foo=bar* switch syntax (ie. with *=*).
* Multiple listening interfaces allowed with a comma-separated *-\ -interface* list.
* New *-\ -filter* utility called *emailrelay-filter-copy* to support *-\ -pop-by-name*.
* Documentation also created in docbook format (requires xmlto).
* Windows installation document revised.

1.3.3 -> 1.4
============

* POP3 server (enable with *-\ -pop*, disable at build-time with *./configure -\ -disable-pop*).
* Fix for logging reentrancy bug (affects *./configure -\ -enable-debug* with *-\ -debug*).
* Fix to ensure sockets are always non-blocking (affects *-\ -scanner*).
* Allow *-\ -verifier* scripts to reject addresses with a temporary *4xx* error code.
* Automatic re-reading of secrets files.
* Write to the Windows event log even if no write access to the registry.
* Modification of set-group-id policy if not started as root.
* Better checking of spool directory access on startup.
* New *emailrelay-submit.sh* example script for submitting messages for *-\ -pop-by-name*.
* The *-\ -dont-listen* switch is now *-\ -no-smtp*.
* Better IPv6 support (Linux only).

1.3.2 -> 1.3.3
==============

* No bind() for outgoing connections [bug-id 1051689].
* Updated rpm spec file [bug-id 1224850].
* Fix for gcc3.4 compilation error in *md5.cpp*.
* Fix for glob()/size_t compilation warning.
* Documentation of *auth* switches corrected.
* State-machine template type declaration modernised, possibly breaking older compilers.

1.3.1 -> 1.3.2
==============

* Fix for core dump when *-\ -client-filter* pre-processing fails.
* Revised code structure to prepare for asynchronous pre-processing.
* Better diagnostics when pre-processor exec() fails.
* Better cleanup of empty and orphaned files.

1.3 -> 1.3.1
============

* Windows resource leak from CreateProcess() fixed.
* Windows dialog box double-close fix.
* Some documentation for the *-\ -scanner* switch.
* New usage patterns section in the user guide.

1.2 -> 1.3
==========

* Client protocol waits for a greeting from the server on startup [bug-id 842156].
* Fix for incorrect backslash normalisation on *-\ -verifier* command-lines containing spaces [bug-id 890646].
* Verifier programs can now summarily abort a connection using an exit value of 100.
* New *-\ -anonymous* switch that reduces information leakage to the SMTP client and disables *VRFY*.
* Better validation of *MAIL-FROM* and *RCPT-TO* formatting.
* Rewrite of low-level MD5 code.
* Performance tuning.
* Template *emailrelay.conf* gets installed in */etc*.
* New switches for the *configure* script.
* More JavaScript example scripts.

1.1.2 -> 1.2
============

* The *-\ -filter* and *-\ -verifier* arguments interpreted as command-lines; spaces in executable paths now need escaping.
* The *-\ -interface* switch applies to outgoing connections too.
* New *-\ -client-filter* switch to do synchronous message processing before sending.
* Keeps authentication after a *rset* command.
* Fix for dangling reference bug, seen after *quit* command on Windows.
* JavaScript examples in the documentation.

1.1.1 -> 1.1.2
==============

* Earlier check for un-bindable ports on startup, and later fork()ing [bug-id 776972].
* Resolved the file-descriptor kludge for *-\ -verifier* on Windows.
* Less strict about failing eight bit messages sent to servers with no *8BITMIME* extension.
* Supplementary group memberships revoked at startup if root or suid.
* Pre-processor (\ *-\ -filter*\ ) program's standard output searched for a failure reason string.
* Undocumented *-\ -scanner* switch added for asynchronous processing by a separate network server.

1.1.0 -> 1.1.1
==============

* Restored the fix for building with gcc2.96.
* Support for MinGW builds on Windows.
* More reasonable size of the *-\ -help -\ -verbose* message box on Windows.
* Windows *-\ -icon* switch changed from *-i* to *-c* to avoid conflicting with *-\ -interface*.
* Shows *next server address* correctly in the configuration report when using *-\ -forward-to*.
* Fix for *make install* when *man2html* is not available.
* Updated init script.

1.0.2 -> 1.1.0
==============

* In proxy mode unexpected client-side disconnects and timeouts do not leave *.bad* files [see also bug-id 659039].
* By default proxy mode does not interpret addresses for local delivery (\ *-\ -postmaster*\ ).
* Polling option added (\ *-\ -poll*\ ) to rescan the spool directory periodically.
* New special exit code (103) for the pre-processor to trigger immediate polling; 100 to 107 now reserved.
* Orphaned zero-length content files are deleted properly if the server-side dialogue is cut short.
* The *-\ -interface* switch applies to the *-\ -admin* interface too.
* Improved internal event architecture using slot/signal design pattern, and fewer singleton classes.
* Event notification available through the administration interface.
* New *-\ -hidden* switch for Windows.
* Syslog output includes process-id.
* Support for Sun WorkShop 5.0 added.
* Documentation overhaul.

1.0.0 -> 1.0.2
==============

* Support for trusted IP addresses, allowing certain clients to avoid authentication.
* Address verifier interface extended to include authentication information.
* New public mail relay section added to the user guide.
* Example verifier scripts etc. added to the reference guide.

1.0.0 -> 1.0.1
==============

* In proxy mode unexpected client-side disconnects and timeouts result in *.bad* files [bug-id 659039].
* Require successful *AUTH* before *MAIL FROM* when using *-\ -server-auth*.
* Better word-wrap on *-\ -help* output.
* Use of RedHat's *functions* code, and support for *chkconfig*, added to the *init.d* script.
* Builds with gcc3.2 (1.0.0-pl5).
* Fix for files left as *busy* after a connection failure in proxy mode [bug-id 631032] (1.0.0-pl3/4/5).
* Trivial documentation fixes (1.0.0-pl3).
* Fix for the double-dot escape bug in the client protocol [bug-id 611624] (1.0.0-pl2).
* Fix build when using gcc2.96 rather than gcc2.95 (1.0.0-pl1).
* Fix default spool directory in example scripts (1.0.0-pl1).

0.9.9 -> 1.0.0
==============

* Briefer *-\ -help* output; works with *-\ -verbose*.
* Option to listen on a specific network interface (\ *-\ -interface*\ ).
* Option for an external address verifier program (\ *-\ -verifier*\ ).
* Some Linux Standard Base stuff added to the *init.d* script.
* Pid files world-readable and deleted on abnormal termination.
* Compiles with gcc 3.0 and intel 6.0.
* Autoconf tweak for MacOS X.
* Corrected the *Received:* typo [bug-id 572236].
* EHLO response parsing is now case-insensitive [bug-id 561522].
* Fewer missing-secrets warnings [bug-id 564987].

0.9.8 -> 0.9.9
==============

* More flexible logging options (*-\ -verbose* and *-\ -debug* work better).
* File Hierarchy Standard (FHS_) option for *configure* (\ *-\ -enable-fhs*\ ).
* FHS-compatible RPMs.
* Spool files writeable by pre-processor when server started as root.
* Default directories in executables and scripts come from *configure*.
* The *init.d* script is renamed *emailrelay* (was *emailrelay.sh*).
* Man pages are gzipped when installed.
* Fix for access violation under Windows NT when client disconnects.
* Use of event log when compiled on Windows NT.
* Fix for info-after-flush bug when using the administration interface. [rc2]
* New *resubmit* script. [rc2]
* Submit utility works under Windows. [rc2]
* Improved Windows project files. [rc2]

0.9.7 -> 0.9.8
==============

* Fix for running pre-processor (\ *-\ -filter*\ ) as root.
* Ignore bogus *AUTH=LOGIN* lines in EHLO response.
* Submit utility improved to work with mutt.
* Installation of submit man page.

0.9.6 -> 0.9.7
==============

* CRAM-MD5 authentication mechanism added.
* Revoke root permissions at start up, and reclaim them when needed.
* Allow mail pre-processing (\ *-\ -filter*\ ) when started as root.
* Domain-override switch (\ *-\ -domain*\ ) added.
* Non-privileged user switch (\ *-\ -user*\ ) added.
* Better handling of NarrowPipe exception (ie. 8-bit message to 7-bit server).
* Allow null return path in MAIL-FROM.
* Reject recipients which look like *<user>@localhost* (as used by fetchmail for local delivery).
* Treat recipients which look like *postmaster@localhost* or *postmaster@<fqdn>* as local postmaster.
* Optional timestamps on log output (\ *-\ -log-time*\ ).
* Fix EHLO to HELO fallback for 501/502 responses in client protocol.
* Submission utility *emailrelay-submit* added.
* HTML4.0 compliant HTML documentation, using CSS.

0.9.5 -> 0.9.6
==============

* SMTP AUTHentication extension -\ - LOGIN mechanism only.
* Client-side protocol timeout.
* Client-side connection timeout.
* Preprocessor can cancel further message processing.
* Client's IP address recorded in envelope files.
* Multiple hard-coded listening addresses supported at compile-time.
* Fix for automatic reopening of stderr stream.

0.9.4 -> 0.9.5
==============
Windows fixes and improvements...

* system-tray + dialog-box user interface
* fix for dropped connections
* fix for content file deletion
* fix for directory iterator

0.9.3 -> 0.9.4
==============

* Fixed memory leak when no *-\ -log* switch.
* Windows build is more *gui* and less *command-line*.
* *Info* command added to the administration interface.
* Doxygen files removed from binary RPM.

0.9.2 -> 0.9.3
==============

* Proxy mode (*-\ -immediate* and *-\ -as-proxy*).
* Message pre-processing (\ *-\ -filter*\ ).
* Message store classes better separated using abstract interfaces.
* Improved notification script, with MIME encoding.
* Builds with old 2.91 version of gcc.

0.9.1 -> 0.9.2
==============

* Better autoconf detection.
* Workround for FreeBSD uname() feature.
* Added missing *.sh_* files to the distribution.
* Fixed a benign directory iterator bug.
* Use of gcc's *exception* header.

0.9 -> 0.9.1
============

* Improved documentation from doxygen.
* More complete use of namespaces.
* Experimental compile-time support for IPv6.


.. _DNSBL: https://en.wikipedia.org/wiki/DNSBL
.. _FHS: https://wiki.linuxfoundation.org/lsb/fhs
.. _PAM: https://en.wikipedia.org/wiki/Linux_PAM
.. _POP: https://en.wikipedia.org/wiki/Post_Office_Protocol
.. _RFC-2920: https://tools.ietf.org/html/rfc2920
.. _RFC-3030: https://tools.ietf.org/html/rfc3030
.. _RFC-3848: https://tools.ietf.org/html/rfc3848
.. _RFC-5782: https://tools.ietf.org/html/rfc5782
.. _RFC-6531: https://tools.ietf.org/html/rfc6531
.. _RFC-8314: https://tools.ietf.org/html/rfc8314
.. _SMTP: https://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol
.. _SOCKS: https://en.wikipedia.org/wiki/SOCKS
.. _TLS: https://en.wikipedia.org/wiki/Transport_Layer_Security

