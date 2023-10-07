***************
Developer Guide
***************

Principles
==========
The main principles in the design of E-MailRelay can be summarised as:

* Functionality without imposing policy
* Minimal third-party dependencies
* Windows/Unix portability without #ifdefs
* Event-driven, non-blocking, single-threaded networking code
* Multi-threading optional

Portability
===========
The E-MailRelay code is written in C++11. Earlier versions of E-MailRelay used
C++03.

The header files *gdef.h* in *src/glib* is used to fix up some compiler
portability issues such as missing standard types, non-standard system headers
etc. Conditional compilation directives (*#ifdef* etc.) are largely confined
this file in order to improve readability.

Windows/Unix portability is generally addressed by providing a common class
declaration with two implementations. The implementations are put into separate
source files with a *_unix* or *_win32* suffix, and if necessary a 'pimple' (or
'Bridge') pattern is used to keep the o/s-specific details out of the header.
If only small parts of the implementation are o/s-specific then there can be
three source files per header. For example, *gsocket.cpp*, *gsocket_win32.cpp*
and *gsocket_unix.cpp* in the *src/gnet* directory.

Underscores in source file names are used exclusively to indicate build-time
alternatives.

Event model
===========
The E-MailRelay server uses non-blocking socket i/o, with a select() or epoll()
event loop. This event model means that the server can handle multiple network
connections simultaneously from a single thread, and even if multi-threading is
disabled at build-time the only blocking occurs when external programs are
executed (see *--filter* and *--address-verifier*).

The advantages of a non-blocking event model are discussed in the well-known
`C10K Problem <http://www.kegel.com/c10k.html>`_ document.

This event model can make the code more complicated than the equivalent
multi-threaded approach since (for example) it is not possible to wait for a
complete line of input to be received from a remote SMTP_ client because there
might be other connections that need servicing half way through.

At higher levels the C++ slot/signal design pattern is used to propagate events
between objects (not to be confused with operating system signals). The
slot/signal implementation has been simplified compared to Qt or boost by not
supporting signal multicasting, so each signal connects to no more than one
slot. The implementation now uses std::function.

The synchronous slot/signal pattern needs some care when when the signalling
object gets destructed as a side-effect of raising a signal, and that situation
can be non-obvious precisely because of the slot/signal code decoupling. In
most cases signals are emitted at the end of a function and the stack unwinds
back to the event loop immediately afterwards, but in other situations,
particularly when emitting more than one signal, defensive measures are
required.

Module structure
================
The main C++ libraries in the E-MailRelay code base are as follows:

"glib"
------
    Low-level classes for file-system abstraction, date and time representation,
    string utility functions, logging, command line parsing etc.


"gssl"
------
    A thin layer over the third-party TLS_ libraries.


"gnet"
------
    Network and event-loop classes.


"gauth"
-------
    Implements various authentication mechanisms.


"gsmtp"
-------
    SMTP protocol classes.


"gpop"
------
    POP3 protocol classes.


"gstore"
--------
    Message store classes.


"gfilters"
----------
    Built-in filters.


"gverifiers"
------------
    Built-in address verifiers.

All of these libraries are portable between Unix-like systems and Windows.

Under Windows there is an additional library under *src/win32* for the user
interface implemented using the Microsoft Win32 API.

SMTP class structure
====================
The message-store functionality uses three abstract interfaces: *MessageStore*,
*NewMessage* and *StoredMessage*. The *NewMessage* interface is used to create
messages within the store, and the *StoredMessage* interface is used for
reading and extracting messages from the store. The concrete implementation
classes based on these interfaces are respectively *FileStore*, *NewFile* and
\ *StoredFile*\ .

Protocol classes such as *GSmtp::ServerProtocol* receive network and timer
events from their container and use an abstract *Sender* interface to send
network data. This means that the protocols can be independent of the network
and event loop framework.

The interaction between the SMTP server protocol class and the message store is
mediated by the *ProtocolMessage* interface. Two main implementations of this
interface are available: one for normal spooling (\ *ProtocolMessageStore*\ ), and
another for immediate forwarding (\ *ProtocolMessageForward*\ ). The *Decorator*
pattern is used whereby the forwarding class uses an instance of the storage
class to do the message storing and filtering, while adding in an instance
of the *GSmtp::Client* class to do the forwarding.

Message filtering (\ *--filter*\ ) is implemented via an abstract *GSmtp::Filter*
interface. Concrete implementations in the *GFilters* namespace are provided for
doing nothing, running an external executable program, talking to an external
network server, etc.

Address verifiers (\ *--address-verifier*\ ) are implemented via an abstract
*GSmtp::Verifier* interface, with concrete implementations in the *GVerifiers*
namespace.

The protocol, processor and message-store interfaces are brought together by
the high-level *GSmtp::Server* and *GSmtp::Client* classes. Dependency
injection is used to create the concrete instances of the *MessageStore*,
*Filter* and *Verifier* interfaces.

Event handling and exceptions
=============================
The use of non-blocking i/o in the network library means that most processing
operates within the context of an i/o event or timeout callback, so the top
level of the call stack is nearly always the event loop code. This can make
catching C++ exceptions a bit awkward compared to a multi-threaded approach
because it is not possible to put a single catch block around a particular
high-level feature.

The event loop delivers asynchronous socket events to the *EventHandler*
interface, timer events to the *TimerBase* interface, and 'future' events to
the *FutureEventCallback* interface. If any of the these event handlers throws
an exception then the event loop catches it and delivers it back to an
exception handler through the *onException()* method of an associated
*ExceptionHandler* interface. If an exception is thrown out of _this_ callback
then the event loop code lets it propagate back to *main()*, typically
terminating the program.

However, sometimes there are objects that need to be more resilient to
exceptions. In particular, a network server should not terminate just because
one of its connections fails unexpectedly. In these cases the owning parent
object receives the exception notification together with an *ExceptionSource*
pointer that identifies the child object that threw the exception. This allows
the parent object to absorb the exception and delete the child, without the
exception killing the whole server.

Event sources in the event loop are typically held as a file descriptor and a
windows event handle, together known as a *Descriptor*. Event loop
implementations typically watch a set of Descriptors for events and call the
relevant EventHandler/ExceptionHandler code via the *EventEmitter* class.

Multi-threading
===============
Multi-threading is used to make DNS lookup and external program asynchronous so
unless disabled at build-time std::thread is used in a future/promise pattern to
wrap up *getaddrinfo()* and *waitpid()* system calls. The shared state comprises
only the parameters and return results from these system calls, and
synchronisation back to the main thread uses the main event loop (see
\ *GNet::FutureEvent*\ ). Threading is not used elsewhere so the C/C++ run-time
library does not need to be thread-safe.

E-MailRelay GUI
===============
The optional GUI program *emailrelay-gui* uses the Qt toolkit for its user
interface components. The GUI can run as an installer or as a configuration
helper, depending on whether it can find an installation *payload*. Refer to
the comments in *src/gui/guimain.cpp* for more details.

The user interface runs as a stack of dialog-box pages with forward and back
buttons at the bottom. Once the stack has been completed by the user then each
page is asked to dump out its state as a set of key-value pairs (see
\ *src/gui/pages.cpp*\ ). These key-value pairs are processed by an installer class
into a list of action objects (in the *Command* design pattern) and then the
action objects are run in turn. In order to display the progress of the
installation each action object is run within a timer callback so that the Qt
framework gets a chance to update the display between each one.

During development the user interface pages and the installer can be tested
separately since the interface between them is a simple text stream containing
key-value pairs.

When run in configure mode the GUI normally ends up simply editing the
*emailrelay.conf* file (or *emailrelay-start.bat* on Windows) and/or the
*emailrelay.auth* secrets file.

When run in install mode the GUI expects to unpack all the E-MailRelay files
from the payload into target directories. The payload is a simple directory
tree that lives alongside the GUI executable or inside the Mac application
bundle, and it contains a configuration file to tell the installer where
to copy its files.

When building the GUI program the library code shared with the main server
executable is compiled separately so that different GUI-specific compiler
options can be used. This is done as a 'unity build', concatenating the shared
code into one source file and compiling that for the GUI. (This technique
requires that private 'detail' namespaces are named rather than anonymous so
that there cannot be any name clashes within the combined anonymous namespace.)

Windows build
=============
E-MailRelay can be compiled on Windows using Microsoft Visual Studio C++ (MSVC)
or mingw-w64.

For MSVC builds there is a perl script (\ *winbuild.pl*\ ) that creates *cmake*
files from the autotools makefiles, runs *cmake* to create the MSVC project
files and then runs *msbuild* to compile E-MailRelay. If perl, cmake, MSVC, Qt
and mbedTLS source are installed in the right way then the *winbuild.bat* batch
file should be able to do a complete MSVC release build in one go. After that
the *winbuild-install.bat* batch file can be used to create a distribution.

When building for a public release the E-MailRelay setup program should be
statically linked and copied into the distribution created by *winbuild.pl*.
This requires a static build of Qt: edit *msvc-desktop.conf* to use */MT*;
run *configure.bat* with *-static -release*; run *nmake -f Makefile* for
*release* then *install*. Then build the E-MailRelay GUI using
\ *emailrelay-gui.pro*\ : *qmake CONFIG+='win static' emailrelay-gui.pro*;
then *mc messages.mc*; then copy *emailrelay-icon.ico*; and finally
\ *nmake -f Makefile.Release*\ .

For MinGW cross-builds use *./configure.sh -w64* and *make* on a Linux box and
copy the built executables and the MinGW run-time to the target. Any extra
run-time files can be identified by running *dumpbin /dependents* in the normal
way.

Windows packaging
=================
On Windows E-MailRelay is packaged as a zip file containing the executables
(including the emailrelay GUI as *emailrelay-setup.exe*), documentation, and a
*payload* directory tree. The payload contains many of the same files all over
again, and while this duplication is not ideal it is at least straightforward.

The Qt tool *windeployqt* is used to add run-time dependencies, such as the
platform DLL. (Public releases of Windows builds are normally statically linked,
so many of the DLLs added by *windeployqt* are not needed.)

To target ancient versions of Windows start with a cross-build using MinGW
for 32-bit (\ *./configure.sh -w32*\ ); then *winbuild.pl mingw* can be used to
assemble a slimmed-down package for distribution.

Unix packaging
==============
On Unix-like operating systems it is more natural to use some sort of package
derived from the *make install* process rather than an installer program, so
the emailrelay GUI is not normally used.

Top-level makefile targets *dist*, *deb* and *rpm* can be used to create a
binary tarball, a debian package, and an RPM package respectively.

Internationalisation
====================
The GUI code has i18n support using the Qt framework, with the tr() function
used throughout the GUI source code. The GUI main() function loads translations
from the *translations* sub-directory (relative to the executable), although
that can be overridden with the *--qm* command-line option. Qt's *-reverse*
option can also be used to reverse the widgets when using RTL languages.

The non-GUI code has some i18n support by using gettext() via the inline txt()
and tx() functions defined in *src/glib/ggettext.h*. The configure script
detects gettext support in the C run-time library, but without trying different
compile and link options. See also *po/Makefile.am*.

On Windows the main server executable has a tabbed dialog-box as its user
interface, but that does not have any support for i18n.

Source control
==============
The source code is stored in the SourceForge *svn* and/or *git* repository.

For example:

::

    $ svn co https://svn.code.sf.net/p/emailrelay/code emailrelay
    $ cd emailrelay/tags/V_2_5

or

::

    $ git clone https://git.code.sf.net/p/emailrelay/git emailrelay
    $ cd emailrelay
    $ git checkout V_2_5

Code that has been formally released will be tagged with a tag like *V_2_5* and
any minor post-release fixes will be on a *fixes* branch like *V_2_5_fixes*.

Compile-time features
=====================
Compile-time features can be selected with options passed to the *configure*
script. These include the following:

* Configuration GUI (\ *--enable-gui*\ )
* Multi-threading (\ *--enable-std-thread*\ )
* TLS library (\ *--with-openssl*\ , *--with-mbedtls*)
* Debug-level logging (\ *--enable-debug*\ )
* Event loop using epoll (\ *--enable-epoll*\ )
* PAM_ support (\ *--with-pam*\ )

Use *./configure --help* to see a complete list of options.





.. _PAM: https://en.wikipedia.org/wiki/Linux_PAM
.. _SMTP: https://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol
.. _TLS: https://en.wikipedia.org/wiki/Transport_Layer_Security

.. footer:: Copyright (C) 2001-2023 Graeme Walker
