/*
   Copyright (C) 2001-2003 Graeme Walker <graeme_walker@users.sourceforge.net>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later
   version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
*/

/* \htmlonly */

/*! \mainpage E-MailRelay Source code

This documentation has been generated by doxygen from the E-MailRelay's 
source code. The <a href="namespaces.html">Namespace List</a> is a good starting point
for browsing -- the detailed description section towards the end of each namespace
page gives a list of the namespace's key classes.

The E-MailRelay <a href="../developer.html">design and implementation guide</a> gives an overview 
of the code structure, and there are a number of supporting diagrams:
<ul>
<li><a href="../gnet-classes.png">GNet namespace class diagram</a></li>
<li><a href="../gsmtp-classes.png">GSmtp namespace class diagram</a></li>
<li><a href="../sequence-1.png">ProtocolMessage sequence diagram 1</a></li>
<li><a href="../sequence-2.png">ProtocolMessage sequence diagram 2</a></li>
<li><a href="../sequence-3.png">Proxy-mode forwarding sequence diagram</a></li>
<li><a href="../sequence-4.png">Scanning sequence diagram</a></li>
<li><a href="../gnet-client.png">GNet::Client state transition diagram</a></li>
<li><a href="../gsmtp-scannerclient.png">GNet::ScannerClient state transition diagram</a></li>
<li><a href="../gsmtp-serverprotocol.png">GSmtp::ServerProtocol state transition diagram</a></li>
</ul>

*/

/*! \namespace Main
\short 
Application-level classes.

The Main namespace contains application-level classes for
the E-MailRelay process.

Key classes are:
- Run
- CommandLine
- Configuration

 */

/*! \namespace GSmtp
\short 
SMTP and message-store classes.

The GSmtp namespace contains classes relating to the SMTP
protocol and to e-mail storage.

Key classes are:
- Client
- ClientProtocol
- ProtocolMessage
- MessageStore
- Server
- ServerProtocol

 */

/*! \namespace GNet
\short 
Network classes.

The GNet namespace contains network interface classes
based on the Berkley socket and WinSock system APIs.

Key classes are:
- Address
- EventHandler
- EventLoop
- Resolver
- Server
- Socket
- Timer

*/

/*! \namespace G
\short 
Low-level classes.

The G namespace contains low-level classes for file-system abstraction, 
date and time representation, string utility functions, logging, 
command line parsing etc.

Key classes are:
- Directory
- File
- GetOpt
- Log
- LogOutput
- Path
- Process
- Str

*/

/* \endhtmlonly */