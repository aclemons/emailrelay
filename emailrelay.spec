Summary: Simple e-mail message transfer agent and proxy using SMTP
Name: emailrelay
Version: 1.9
Release: 1
License: GPL3
Group: System Environment/Daemons
URL: http://emailrelay.sourceforge.net/
Source: http://sourceforge.net/sourceforge/emailrelay/emailrelay-1.9-src.tar.gz
BuildRoot: /tmp/emailrelay-install

%description
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

%prep
%setup

%build
./configure --prefix=/usr --localstatedir=/var --libexecdir=/usr/lib --sysconfdir=/etc e_initdir=/etc/init.d --disable-gui --without-man2html --without-doxygen --with-openssl --with-zlib --enable-static-linking --disable-install-hook
make

%install
make install-strip destdir=$RPM_BUILD_ROOT DESTDIR=$RPM_BUILD_ROOT

%post
test -f /usr/lib/lsb/install_initd && cd $RPM_BUILD_ROOT/etc/init.d && /usr/lib/lsb/install_initd emailrelay || true
test -f $RPM_BUILD_ROOT/etc/emailrelay.conf || cp $RPM_BUILD_ROOT/etc/emailrelay.conf.template $RPM_BUILD_ROOT/etc/emailrelay.conf || true

%preun
test $1 -eq 0 && test -f /usr/lib/lsb/remove_initd && cd /etc/init.d && /usr/lib/lsb/remove_initd emailrelay || true

%clean
test "$RPM_BUILD_ROOT" = "/" || rm -rf "$RPM_BUILD_ROOT"

%files

%config /etc/emailrelay.conf
%config /etc/pam.d/emailrelay
/etc/emailrelay.conf.template
/etc/emailrelay.auth.template
/etc/init.d/emailrelay
/usr/share/man/man1/emailrelay.1.gz
/usr/share/man/man1/emailrelay-filter-copy.1.gz
/usr/share/man/man1/emailrelay-poke.1.gz
/usr/share/man/man1/emailrelay-passwd.1.gz
/usr/share/man/man1/emailrelay-submit.1.gz
%doc /usr/share/doc/emailrelay/AUTHORS
%doc /usr/share/doc/emailrelay/COPYING
%doc /usr/share/doc/emailrelay/INSTALL
/usr/share/doc/emailrelay/README.mac
/usr/share/doc/emailrelay/README.windows
/usr/share/doc/emailrelay/README.embedded
/usr/share/doc/emailrelay/diagram-1.png
/usr/share/doc/emailrelay/diagram-2.png
/usr/share/doc/emailrelay/emailrelay-doxygen.css
/usr/share/doc/emailrelay/valid-html401.png
/usr/share/doc/emailrelay/index.html
/usr/share/doc/emailrelay/windows.html
/usr/share/doc/emailrelay/gsmtp-classes.png
/usr/share/doc/emailrelay/emailrelay.docbook
/usr/share/doc/emailrelay/developer.txt
/usr/share/doc/emailrelay/reference.html
/usr/share/doc/emailrelay/NEWS
%doc /usr/share/doc/emailrelay/reference.txt
/usr/share/doc/emailrelay/developer.html
/usr/share/doc/emailrelay/auth.png
/usr/share/doc/emailrelay/emailrelay.css
/usr/share/doc/emailrelay/sequence-3.png
/usr/share/doc/emailrelay/gsmtp-serverprotocol.png
/usr/share/doc/emailrelay/gnet-client.png
/usr/share/doc/emailrelay/gnet-classes.png
%doc /usr/share/doc/emailrelay/README
/usr/share/doc/emailrelay/changelog.html
%doc /usr/share/doc/emailrelay/userguide.txt
/usr/share/doc/emailrelay/readme.html
/usr/share/doc/emailrelay/userguide.html
%doc /usr/share/doc/emailrelay/ChangeLog
%doc /usr/share/doc/emailrelay/windows.txt
/usr/share/doc/emailrelay/emailrelay-man.html
%docdir /usr/share/doc/emailrelay/doxygen
/usr/share/doc/emailrelay/doxygen/index.html
/usr/share/doc/emailrelay/doxygen/emailrelay-doxygen.css
/usr/lib/emailrelay/examples/emailrelay-process.sh
/usr/lib/emailrelay/examples/emailrelay-deliver.sh
/usr/lib/emailrelay/examples/emailrelay-resubmit.sh
/usr/lib/emailrelay/examples/emailrelay-notify.sh
/usr/lib/emailrelay/examples/emailrelay-submit.sh
/usr/lib/emailrelay/examples/emailrelay-multicast.sh
/usr/lib/emailrelay/emailrelay-poke
/usr/lib/emailrelay/emailrelay-filter-copy
/usr/sbin/emailrelay-submit
/usr/sbin/emailrelay
/usr/sbin/emailrelay-passwd
/usr/share/emailrelay/emailrelay-icon.png

%changelog

* Wed Jul 3 2002 Graeme Walker <graeme_walker@users.sourceforge.net>
- Initial version.

