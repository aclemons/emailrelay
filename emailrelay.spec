Summary: Simple e-mail message transfer agent using SMTP
Name: emailrelay
Version: 1.1.2
Release: 1
Copyright: GPL
Group: System Environment/Daemons
Source: http://emailrelay.sourceforge.net/.../emailrelay-src-1.1.2.tar.gz
BuildRoot: /tmp/emailrelay-install

%define prefix /usr

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
./configure --enable-fhs
make HAVE_DOXYGEN=no HAVE_MAN2HTML=no

%install
make install-strip destdir=$RPM_BUILD_ROOT DESTDIR=$RPM_BUILD_ROOT HAVE_DOXYGEN=no HAVE_MAN2HTML=no

%post
test -f /usr/lib/lsb/install_initd && cd /etc/init.d && /usr/lib/lsb/install_initd emailrelay || true

%preun
test $1 -eq 0 && test -f /usr/lib/lsb/remove_initd && cd /etc/init.d && /usr/lib/lsb/remove_initd emailrelay || true

%clean
rm -rf $RPM_BUILD_ROOT

%files

/etc/init.d/emailrelay
%{prefix}/lib/emailrelay/emailrelay-poke
%{prefix}/sbin/emailrelay
%{prefix}/sbin/emailrelay-passwd
%{prefix}/sbin/emailrelay-submit
%{prefix}/share/doc/emailrelay/NEWS
%{prefix}/share/doc/emailrelay/README
%{prefix}/share/doc/emailrelay/changelog.gz
%{prefix}/share/doc/emailrelay/changelog.html
%{prefix}/share/doc/emailrelay/developer.html
%{prefix}/share/doc/emailrelay/developer.txt
%{prefix}/share/doc/emailrelay/emailrelay.css
%{prefix}/share/doc/emailrelay/examples/emailrelay-deliver.sh
%{prefix}/share/doc/emailrelay/examples/emailrelay-notify.sh
%{prefix}/share/doc/emailrelay/examples/emailrelay-process.sh
%{prefix}/share/doc/emailrelay/examples/emailrelay-resubmit.sh
%{prefix}/share/doc/emailrelay/index.html
%{prefix}/share/doc/emailrelay/readme.html
%{prefix}/share/doc/emailrelay/reference.html
%{prefix}/share/doc/emailrelay/reference.txt
%{prefix}/share/doc/emailrelay/userguide.html
%{prefix}/share/doc/emailrelay/userguide.txt
%{prefix}/share/doc/emailrelay/windows.html
%{prefix}/share/doc/emailrelay/windows.txt
%{prefix}/share/man/man1/emailrelay-passwd.1.gz
%{prefix}/share/man/man1/emailrelay-poke.1.gz
%{prefix}/share/man/man1/emailrelay-submit.1.gz
%{prefix}/share/man/man1/emailrelay.1.gz
/var/spool/emailrelay/

%changelog

* Wed Jul 3 2002 Graeme Walker <graeme_walker@users.sourceforge.net>
- Initial version.

