Summary: Simple e-mail message transfer agent and proxy using SMTP
Name: emailrelay
Version: 2.5rc1
Release: 1
License: GPL3
Group: System Environment/Daemons
URL: http://emailrelay.sourceforge.net
Source: http://sourceforge.net/projects/emailrelay/files/emailrelay/2.5rc1/emailrelay-2.5rc1-src.tar.gz
BuildRoot: /tmp/emailrelay-install
##BuildRequires: systemd-rpm-macros

%description
E-MailRelay is a lightweight SMTP store-and-forward mail server with POP access
to spooled messages. It can be used as a personal internet mail server with
SpamAssassin spam filtering and DNSBL connection blocking. Forwarding can be
to a fixed smarthost or using DNS MX routing. External scripts can be used for
address validation and e-mail message processing.

E-MailRelay runs as a single process using the same non-blocking i/o model as
Squid and nginx giving excellent scalability and resource usage.

%global debug_package %{nil}
%prep
%setup

%build
./configure --prefix=/usr --localstatedir=/var --libexecdir=/usr/lib --sysconfdir=/etc e_initdir=/usr/lib/emailrelay/init e_systemddir=/usr/lib/systemd/system --without-doxygen --without-man2html --with-openssl --without-mbedtls --with-pam --disable-gui --disable-install-hook --disable-testing
make

%install
make install-strip destdir=$RPM_BUILD_ROOT DESTDIR=$RPM_BUILD_ROOT
test -f $RPM_BUILD_ROOT/etc/emailrelay.conf || cp $RPM_BUILD_ROOT/etc/emailrelay.conf.template $RPM_BUILD_ROOT/etc/emailrelay.conf || true

%post
test -x /usr/lib/emailrelay/init/emailrelay && /usr/lib/emailrelay/init/emailrelay setup || true
##%systemd_post emailrelay.service
if test "$1" -eq 1 ; then systemctl --no-reload preset emailrelay.service &>/dev/null || true ; fi

%preun
##%systemd_preun emailrelay.service
if test "$1" -eq 0 ; then systemctl --no-reload disable --now emailrelay.service &>/dev/null || true ; fi

%postun
##%systemd_postun_with_restart emailrelay.service
if test "$1" -ge 1 ; then systemctl try-restart emailrelay.service &>/dev/null || true ; fi

%clean
test "$RPM_BUILD_ROOT" = "/" || rm -rf "$RPM_BUILD_ROOT"

%files

/etc/emailrelay.auth.template
%config /etc/emailrelay.conf
/etc/emailrelay.conf.template
%config /etc/pam.d/emailrelay
%dir /usr/lib/emailrelay
%attr(2755, root, daemon) /usr/lib/emailrelay/emailrelay-filter-copy
%dir /usr/lib/emailrelay/examples
/usr/lib/emailrelay/examples/emailrelay
/usr/lib/emailrelay/examples/emailrelay-bcc-check.pl
/usr/lib/emailrelay/examples/emailrelay-check-ipaddress.js
/usr/lib/emailrelay/examples/emailrelay-check-ipaddress.pl
/usr/lib/emailrelay/examples/emailrelay-deliver.sh
/usr/lib/emailrelay/examples/emailrelay-dkim-signer.pl
/usr/lib/emailrelay/examples/emailrelay-fail2ban-filter
/usr/lib/emailrelay/examples/emailrelay-fail2ban-jail
/usr/lib/emailrelay/examples/emailrelay-ldap-verify.py
/usr/lib/emailrelay/examples/emailrelay-multicast.sh
/usr/lib/emailrelay/examples/emailrelay-notify.sh
/usr/lib/emailrelay/examples/emailrelay-resubmit.sh
/usr/lib/emailrelay/examples/emailrelay-rot13.pl
/usr/lib/emailrelay/examples/emailrelay-sendmail.pl
/usr/lib/emailrelay/examples/emailrelay-set-from.js
/usr/lib/emailrelay/examples/emailrelay-set-from.pl
/usr/lib/emailrelay/examples/emailrelay-set-message-id.js
/usr/lib/emailrelay/examples/emailrelay-submit.sh
/usr/lib/emailrelay/init/emailrelay
/usr/lib/systemd/system/emailrelay.service
/usr/sbin/emailrelay
/usr/sbin/emailrelay-passwd
%attr(2755, root, daemon) /usr/sbin/emailrelay-submit
%docdir /usr/share/doc/emailrelay
%dir /usr/share/doc/emailrelay
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
%doc /usr/share/doc/emailrelay/conf.py.sphinx
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
%doc /usr/share/doc/emailrelay/mailserver.png
%doc /usr/share/doc/emailrelay/popbyname.png
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
%dir /usr/share/emailrelay
/usr/share/emailrelay/emailrelay-icon.png
/usr/share/man/man1/emailrelay-filter-copy.1.gz
/usr/share/man/man1/emailrelay-passwd.1.gz
/usr/share/man/man1/emailrelay-submit.1.gz
/usr/share/man/man1/emailrelay.1.gz
%dir %attr(2775, root, daemon) /var/spool/emailrelay

%changelog

* Wed Jul 3 2002 Graeme Walker <graeme_walker@users.sourceforge.net>
- Initial version.

