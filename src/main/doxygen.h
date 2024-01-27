/*
   Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* \htmlonly */

/*! \mainpage E-MailRelay Source code

This documentation has been generated by doxygen from the E-MailRelay's
source code. The top-level namespaces in the <a href="namespaces.html">Namespace List</a>
page are a good starting point for browsing -- the detailed description section
towards the end of each namespace page gives a list of the namespace's
key classes.

The E-MailRelay <a href="../developer.html">developer guide</a> gives an overview
of the code structure.

*/

/*! \namespace Main
\short
Application-level classes.

The Main namespace contains application-level classes for
the E-MailRelay process.

Key classes are:
- Run
- Unit
- CommandLine
- Configuration

 */

/*! \namespace GPop
\short
POP3 classes.

The GPop namespace contains classes relating to the POP3
protocol.

Key classes are:
- Server
- ServerProtocol
- Store

 */

/*! \namespace GSsl
\short
TLS/SSL transport layer security classes.

The GSsl namespace contains classes that implement the TLS/SSL
security layer.

Key classes are:
- Protocol
- Library

 */

/*! \namespace GSmtp
\short
SMTP classes.

The GSmtp namespace contains classes relating to the SMTP
protocol.

Key classes are:
- Client
- ClientProtocol
- ProtocolMessage
- Server
- ServerProtocol

 */

/*! \namespace GFilters
\short
Message filter classes.

The GFilters namespace contains classes relating to message filtering.

Key classes are:
- FilterFactory
- ExecutableFilter

 */

/*! \namespace GVerifiers
\short
Address verifier classes.

The GVerifiers namespace contains classes for address verification.

Key classes are:
- VerifierFactory
- ExecutableVerifier

 */

/*! \namespace GStore
\short
Message store classes.

The GStore namespace contains classes relating to e-mail storage.

Key classes are:
- MessageStore
- FileStore
- NewMessage
- NewFile
- StoredMessage
- StoredFile

 */

/*! \namespace GAuth
\short
SASL authentication classes.

The GAuth namespace contains classes relating to SASL
and PAM authentication.

Key classes are:
- SaslClient
- SaslServer
- SaslServerFactory
- Secrets

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
- MultiServer
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
- OptionParser
- Log
- LogOutput
- Path
- Process
- Str

*/

/* \endhtmlonly */
