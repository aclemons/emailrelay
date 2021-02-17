******************
E-MailRelay Readme
******************

Abstract
========
E-MailRelay is an e-mail store-and-forward message transfer agent and proxy
server. It runs on Unix-like operating systems (including Linux and Mac OS X),
and on Windows.

E-MailRelay does three things: it stores any incoming e-mail messages that
it receives, it forwards e-mail messages on to another remote e-mail server,
and it serves up stored e-mail messages to local e-mail reader programs. More
technically, it acts as a SMTP_ storage daemon, a SMTP forwarding agent, and
a POP3 server.

Whenever an e-mail message is received it can be passed through a user-defined
program, such as a spam filter, which can drop, re-address or edit messages as
they pass through.

.. image:: whatisit.png
   :alt: whatisit.png


E-MailRelay uses the same non-blocking i/o model as Squid and nginx giving
excellent scalability and resource usage.

C++ source code is available and distribution is permitted under the GNU
General Public License V3.

Quick start
===========
To use E-MailRelay in store-and-forward mode use the *--as-server* option to
start the storage daemon in the background, and then do delivery of spooled
messages by running with the *--as-client* option.

.. image:: serverclient.png
   :alt: serverclient.png


For example, to start a storage daemon listening on port 587 use a command
like this:

::

    emailrelay --as-server --port 587 --spool-dir /tmp

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

    emailrelay --as-proxy smtp.example.com:25 --filter addsig.js

To run E-MailRelay as a POP_ server without SMTP use *--pop* and *--no-smtp*:

::

    emailrelay --pop --no-smtp --log --close-stderr

The *emailrelay-submit* utility can be used to put messages straight into the
spool directory so that the POP clients can fetch them.

By default E-MailRelay will always reject connections from remote networks. To
allow connections from anywhere use the *--remote-clients* option, but please
check your firewall settings to make sure this cannot be exploited by spammers.

For more information on the command-line options refer to the reference guide
or run:

::

    emailrelay --help --verbose

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

Feedback
========
Please feel free to e-mail the author at *mailto:graeme_walker@users.sourceforge.net*.


.. _POP: https://en.wikipedia.org/wiki/Post_Office_Protocol
.. _SMTP: https://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol

