******************
E-MailRelay Readme
******************

Abstract
========
E-MailRelay is a lightweight SMTP_ store-and-forward mail server with POP_ access
to spooled messages. It can be used as a personal internet mail server with
SpamAssassin spam filtering and DNSBL_ connection blocking. Forwarding can be
to a fixed smarthost or using DNS MX routing. External scripts can be used for
address validation and e-mail message processing.

.. image:: whatisit.png
   :alt: whatisit.png


E-MailRelay runs as a single process using the same non-blocking i/o model as
Squid and nginx giving excellent scalability and resource usage.

Quick start
===========
E-MailRelay can be run straight from the command-line.

To use E-MailRelay in store-and-forward mode use the *--as-server* option to
start the storage daemon in the background, and then do delivery of spooled
messages by running with *--as-client*.

.. image:: serverclient.png
   :alt: serverclient.png


For example, to start a storage daemon in the background listening on port 587
use a command like this:

::

    emailrelay --as-server --syslog --port 587 --spool-dir /tmp

Or to run it in the foreground:

::

    emailrelay --log --verbose --no-daemon --port 587 --spool-dir /tmp

And then to forward the spooled mail to *smtp.example.com* run something
like this:

::

    emailrelay --as-client smtp.example.com:25 --spool-dir /tmp

To get behaviour more like a proxy you can add the *--poll* and *--forward-to*
options so that messages are forwarded continuously rather than on-demand.

.. image:: forwardto.png
   :alt: forwardto.png


This example starts a store-and-forward server that forwards spooled-up e-mail
every minute:

::

    emailrelay --as-server --poll 60 --forward-to smtp.example.com:25

Or for a proxy server that forwards each message soon after it has been
received, you can use *--as-proxy* or add *--forward-on-disconnect*:

::

    emailrelay --as-server --forward-on-disconnect --forward-to smtp.example.com:25

To edit or filter e-mail as it passes through the proxy specify your filter
program with the *--filter* option, something like this:

::

    emailrelay --as-proxy smtp.example.com:25 --filter=addsig.js

E-MailRelay can be used as a personal internet mail server:

.. image:: mailserver.png
   :alt: mailserver.png


Use *--remote-clients* (\ *-r*\ ) to allow connections from outside the local
network, define your domain name with *--domain* and use an address verifier as
a first line of defense against spammers:

::

    emailrelay --as-server -v -r --domain=example.com --address-verifier=allow:

Then enable POP access to the incoming e-mails with *--pop*, *--pop-port* and
\ *--pop-auth*\ :

::

    emailrelay ... --pop --pop-port 10110 --pop-auth /etc/emailrelay.auth

Set up the POP account with a user-id and password in the *--pop-auth* secrets
file. The secrets file should contain a single line of text like this:

::

    server plain <userid> <password>

For more information on the command-line options refer to the reference guide
or run:

::

    emailrelay --help --verbose


Autostart
=========
To install E-MailRelay on Windows run the *emailrelay-setup* program and choose
the automatic startup option so that E-MailRelay runs as a Windows service.

To install E-MailRelay on Linux from a RPM package:

::

    sudo rpm -i emailrelay*.rpm

Or from a DEB package:

::

    sudo dpkg -i emailrelay*.deb

To get the E-MailRelay server to start automatically you should check the
configuration file */etc/emailrelay.conf* is as you want it and then run the
following commands to activate the *systemd* service:

::

    systemctl enable emailrelay
    systemctl start emailrelay
    systemctl status emailrelay

On other systems try some combination of these commands to set up and activate
the E-MailRelay service:

::

    cp /usr/lib/emailrelay/init/emailrelay /etc/init.d/
    update-rc.d emailrelay enable
    rc-update add emailrelay
    invoke-rc.d emailrelay start
    service emailrelay start
    tail /var/log/messages
    tail /var/log/syslog


Documentation
=============
The following documentation is provided:

* README -- this document
* COPYING -- the GNU General Public License
* INSTALL -- generic build & install instructions
* AUTHORS -- authors, credits and additional copyrights
* userguide.txt -- user guide
* reference.txt -- reference document
* ChangeLog -- change log for releases

Source code documentation will be generated when building from source if
*doxygen* is available.


.. _DNSBL: https://en.wikipedia.org/wiki/DNSBL
.. _POP: https://en.wikipedia.org/wiki/Post_Office_Protocol
.. _SMTP: https://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol

