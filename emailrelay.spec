Summary: Simple e-mail message transfer agent using SMTP
Name: emailrelay
Version: 1.3
Release: 1
Copyright: GPL
Group: System Environment/Daemons
Source: http://emailrelay.sourceforge.net/.../emailrelay-src-1.3.tar.gz
BuildRoot: /tmp/emailrelay-install

%description
E-MailRelay is a simple SMTP proxy and store-and-forward message transfer agent 
(MTA). When running as a proxy all e-mail messages can be passed through a 
user-defined program, such as a spam filter, which can drop, re-address or edit 
messages as they pass through. When running as a store-and-forward MTA incoming 
messages are stored in a local spool directory, and then forwarded to the next 
SMTP server on request. 

Because of its functional simplicity E-MailRelay is easy to configure, typically
only requiring the address of the target SMTP server to be put on the command 
line.

C++ source code is available for Linux, FreeBSD, MacOS X etc, and Windows.
Distribution is under the GNU General Public License.

%prep
%setup

%build
./configure --enable-fhs --without-man2html --without-doxygen
make

%install
make install-strip destdir=$RPM_BUILD_ROOT DESTDIR=$RPM_BUILD_ROOT

%post
test -f /usr/lib/lsb/install_initd && cd /etc/init.d && /usr/lib/lsb/install_initd emailrelay || true

%preun
test $1 -eq 0 && test -f /usr/lib/lsb/remove_initd && cd /etc/init.d && /usr/lib/lsb/remove_initd emailrelay || true

%clean
test "$RPM_BUILD_ROOT" = "/" || rm -rf "$RPM_BUILD_ROOT"

%files

%config /etc/emailrelay.conf
/etc/init.d/emailrelay
/usr/lib/emailrelay/emailrelay-poke
/usr/sbin/emailrelay
/usr/sbin/emailrelay-passwd
/usr/sbin/emailrelay-submit
/usr/share/doc/emailrelay/changelog.gz
/usr/share/doc/emailrelay/changelog.html
/usr/share/doc/emailrelay/developer.html
/usr/share/doc/emailrelay/developer.txt
/usr/share/doc/emailrelay/emailrelay.css
/usr/share/doc/emailrelay/emailrelay-man.html
/usr/share/doc/emailrelay/examples/emailrelay-deliver.sh
/usr/share/doc/emailrelay/examples/emailrelay-notify.sh
/usr/share/doc/emailrelay/examples/emailrelay-process.sh
/usr/share/doc/emailrelay/examples/emailrelay-resubmit.sh
/usr/share/doc/emailrelay/index.html
/usr/share/doc/emailrelay/NEWS
/usr/share/doc/emailrelay/*.png
/usr/share/doc/emailrelay/README
/usr/share/doc/emailrelay/readme.html
/usr/share/doc/emailrelay/reference.html
/usr/share/doc/emailrelay/reference.txt
/usr/share/doc/emailrelay/userguide.html
/usr/share/doc/emailrelay/userguide.txt
/usr/share/doc/emailrelay/windows.html
/usr/share/doc/emailrelay/windows.txt
/usr/share/man/man1/emailrelay.1.gz
/usr/share/man/man1/emailrelay-passwd.1.gz
/usr/share/man/man1/emailrelay-poke.1.gz
/usr/share/man/man1/emailrelay-submit.1.gz
/var/spool/emailrelay/

%changelog

* Wed Jul 3 2002 Graeme Walker <graeme_walker@users.sourceforge.net>
- Initial version.

