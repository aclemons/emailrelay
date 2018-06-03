Summary: Simple e-mail message transfer agent and proxy using SMTP
Name: emailrelay
Version: 2.0
Release: 1
License: GPL3
Group: System Environment/Daemons
URL: http://emailrelay.sourceforge.net/
Source: http://sourceforge.net/projects/emailrelay/files/emailrelay/2.0/emailrelay-2.0-src.tar.gz
BuildRoot: /tmp/emailrelay-install

%description
E-MailRelay is an e-mail store-and-forward message transfer agent and proxy
server. It runs on Unix-like operating systems (including Linux and Mac OS X),
and on Windows.

E-MailRelay does three things: it stores any incoming e-mail messages that
it receives, it forwards e-mail messages on to another remote e-mail server,
and it serves up stored e-mail messages to local e-mail reader programs. More
technically, it acts as a SMTP storage daemon, a SMTP forwarding agent, and
a POP3 server.

Whenever an e-mail message is received it can be passed through a user-defined
program, such as a spam filter, which can drop, re-address or edit messages as
they pass through.

E-MailRelay uses the same non-blocking i/o model as Squid and nginx giving
excellent scalability and resource usage.

C++ source code is available and distribution is permitted under the GNU
General Public License V3.

%prep
%setup

%build
./configure --prefix=/usr --localstatedir=/var --libexecdir=/usr/lib --sysconfdir=/etc e_initdir=/etc/init.d --disable-gui --without-man2html --without-doxygen --with-openssl --without-mbedtls --with-pam --disable-install-hook
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

/etc/emailrelay.auth.template
%config /etc/emailrelay.conf
/etc/emailrelay.conf.template
/etc/init.d/emailrelay
%config /etc/pam.d/emailrelay
/usr/lib/emailrelay/emailrelay-filter-copy
/usr/lib/emailrelay/examples/emailrelay
/usr/lib/emailrelay/examples/emailrelay-bcc-check.pl
/usr/lib/emailrelay/examples/emailrelay-deliver.sh
/usr/lib/emailrelay/examples/emailrelay-multicast.sh
/usr/lib/emailrelay/examples/emailrelay-notify.sh
/usr/lib/emailrelay/examples/emailrelay-process.sh
/usr/lib/emailrelay/examples/emailrelay-resubmit.sh
/usr/lib/emailrelay/examples/emailrelay-sendmail.pl
/usr/lib/emailrelay/examples/emailrelay-submit.sh
/usr/sbin/emailrelay
/usr/sbin/emailrelay-passwd
/usr/sbin/emailrelay-submit
%doc /usr/share/doc/emailrelay/AUTHORS
%doc /usr/share/doc/emailrelay/COPYING
%doc /usr/share/doc/emailrelay/ChangeLog
%doc /usr/share/doc/emailrelay/INSTALL
%doc /usr/share/doc/emailrelay/NEWS
%doc /usr/share/doc/emailrelay/README
%doc /usr/share/doc/emailrelay/authentication.png
%doc /usr/share/doc/emailrelay/changelog.html
%doc /usr/share/doc/emailrelay/changelog.md
%doc /usr/share/doc/emailrelay/changelog.rst
%doc /usr/share/doc/emailrelay/changelog.txt
%doc /usr/share/doc/emailrelay/conf.py
%doc /usr/share/doc/emailrelay/developer.html
%doc /usr/share/doc/emailrelay/developer.md
%doc /usr/share/doc/emailrelay/developer.rst
%doc /usr/share/doc/emailrelay/developer.txt
%doc /usr/share/doc/emailrelay/download-button.png
%docdir /usr/share/doc/emailrelay/doxygen
%doc /usr/share/doc/emailrelay/doxygen.cfg.in
%doc /usr/share/doc/emailrelay/doxygen/index.html
%doc /usr/share/doc/emailrelay/emailrelay-doxygen.css
%doc /usr/share/doc/emailrelay/emailrelay-man.html
%doc /usr/share/doc/emailrelay/emailrelay.css
%doc /usr/share/doc/emailrelay/forwardto.png
%doc /usr/share/doc/emailrelay/index.html
%doc /usr/share/doc/emailrelay/index.rst
%doc /usr/share/doc/emailrelay/readme.html
%doc /usr/share/doc/emailrelay/readme.md
%doc /usr/share/doc/emailrelay/readme.rst
%doc /usr/share/doc/emailrelay/readme.txt
%doc /usr/share/doc/emailrelay/reference.html
%doc /usr/share/doc/emailrelay/reference.md
%doc /usr/share/doc/emailrelay/reference.rst
%doc /usr/share/doc/emailrelay/reference.txt
%doc /usr/share/doc/emailrelay/serverclient.png
%doc /usr/share/doc/emailrelay/userguide.html
%doc /usr/share/doc/emailrelay/userguide.md
%doc /usr/share/doc/emailrelay/userguide.rst
%doc /usr/share/doc/emailrelay/userguide.txt
%doc /usr/share/doc/emailrelay/whatisit.png
%doc /usr/share/doc/emailrelay/windows.html
%doc /usr/share/doc/emailrelay/windows.md
%doc /usr/share/doc/emailrelay/windows.rst
%doc /usr/share/doc/emailrelay/windows.txt
/usr/share/emailrelay/emailrelay-icon.png
/usr/share/man/man1/emailrelay-filter-copy.1.gz
/usr/share/man/man1/emailrelay-passwd.1.gz
/usr/share/man/man1/emailrelay-submit.1.gz
/usr/share/man/man1/emailrelay.1.gz

%changelog

* Wed Jul 3 2002 Graeme Walker <graeme_walker@users.sourceforge.net>
- Initial version.

