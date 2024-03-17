*******
Windows
*******

Setup program
=============
To install E-MailRelay on Windows run *emailrelay-setup.exe*.

The installation GUI will take you through the installation options and then
install the run-time files from the *payload* directory into your chosen
locations and also create a startup batch file (\ *emailrelay-start.bat*\ ).

If you plan to install into *Program Files* and *ProgramData*, or if you want
to run E-MailRelay as a Windows service then you will need to allow the
installation program to run as Administrator.

You can also run the main E-MailRelay program *emailrelay.exe* directly without
going through the installation process but you will need to refer to the
documentation to set the appropriate configuration options.

Running the program
===================
After a successful installation you should see E-MailRelay listed in the
Windows Start Menu and/or as an entry in the Windows *Services* tool.

The Start Menu item will run the startup batch file *emailrelay-start.bat*
which contains all the necessary configuration options, and once the
E-MailRelay server is running you should see an icon appear in the Windows
system notification area under the *Show hidden icons* button.

Or if E-MailRelay runs as a service you should see an entry in the Windows
*Services* tool with a status of Running. Check the E-MailRelay log file to see
what it is doing.

Configuration
=============
E-MailRelay is configured with options like *--verbose* or *--spool-dir* in the
*emailrelay-startup.bat* batch file.

Note that *emailrelay-start.bat* lives under *ProgramData*, and although this
might be a hidden directory you can still navigate there by right-clicking on
the E-MailRelay link under *Program Files* and selecting *Open file location*.

You can edit the batch file using Notepad: right-click on the E-MailRelay link
or on the *emailrelay-start.bat* file, then *show more options* and *edit*.

Alternatively, use the *emailrelay-gui* program to make the changes.

All configuration options are documented in the E-MailRelay reference document.

Account user-ids and passwords can be configured by editing the E-MailRelay
*secrets* file. Check the *--client-auth* or *--server-auth* options in the
startup batch file to find out where this is.

Manual installation
===================
The manual installation process for when you cannot run the setup program can be
as simple as this:

* Create a new program directory *c:\\Program Files\\E-MailRelay*.
* Create a new spool directory *c:\\Program Files\\E-MailRelay\\spool*.
* Copy the EXE files from *programs* in the zip file into *c:\\Program Files\\E-MailRelay*.
* Create a new text file, eg. *c:\\Program Files\\E-MailRelay\\auth.txt*, to contain account details.
* Add account details to *auth.txt* with a line like *client plain myaccount mypassword*.
* Right-drag *emailrelay.exe* onto the desktop to create a shortcut for the server.
* Add configuration options to the server shortcut properties in the *target* box.

The configuration options should normally include:

* \ *--log*\
* \ *--verbose*\
* \ *--log-file=@app\\log-%d.txt*\
* \ *--spool-dir=@app\\spool*\
* \ *--client-auth=@app\\auth.txt*\
* \ *--client-tls*\
* \ *--forward-to=smtp.example.com:25*\
* \ *--forward-on-disconnect*\
* \ *--poll=60*\

Copy the shortcut to *Start Menu* and *Startup* folders as necessary.

Running as a service
====================
E-MailRelay can be set up as a Windows service so that it starts up
automatically at boot-time. This can be enabled on the *Server startup* page
in the installation program or later using the *emailrelay-gui* configuration
program.

Alternatively, to set up the service manually you must first have a one-line
batch file called *emailrelay-start.bat* that contains all the configuration
options for running the E-MailRelay server, and you must have a simple
service-wrapper configuration file called *emailrelay-service.cfg* that points
to it, and this must be in the same directory as the service wrapper executable
(\ *emailrelay-service.exe*\ ).

The startup batch file should contain a single line, something like this:

::

    start "emailrelay" "C:\Program Files\E-MailRelay\emailrelay.exe" --forward-to smtp.example.com:25 ...

There is no need to use *--no-daemon* and *--hidden*; these are added
automatically.

The contents of the service-wrapper configuration file can be a single
line that points to the directory containing the startup batch file,
like this:

::

    dir-config="C:\ProgramData\E-MailRelay"

Then just run *emailrelay-service --install* from an Administrator command
prompt to install the service.

Every time the service starts it reads the service-wrapper configuration file
and the startup batch file in order to run the E-MailRelay program.

If you need to run multiple E-MailRelay services then put a unique service
name and display name on the *emailrelay-service --install <name> <display-name>*
command-line. The service name you give is used to derive the name of the
*<name>-start.bat* batch file that contains the E-MailRelay server's
configuration options, so you will need to create that first.

Uninstall
=========
To uninstall:

* Stop the program and/or the service.
* Uninstall the service, if installed (\ *emailrelay-service --remove*\ ).
* Delete the files from the E-MailRelay *program files* folder (eg. *C:\\Program Files\\E-MailRelay*).
* Delete the files from the E-MailRelay *program data* folder (eg. *C:\\ProgramData\\E-MailRelay*).
* Delete any desktop shortcuts (eg. *%USERPROFILE%\\Desktop\\E-MailRelay.lnk*).
* Delete any start menu shortcuts (eg. *%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\E-MailRelay.lnk*).
* Delete any auto-start shortcuts (eg. *%APPDATA%\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\E-MailRelay.lnk*).
* Clean up the registry under *HKLM\\System\\CurrentControlSet\\Services\\EventLog\\Application\\emailrelay*.

Diagnostics
===========
E-MailRelay logging is sent to the Windows Event Log and/or written to a log
file, and individual failed mail messages will have the failure reason recorded
inside the *.bad* envelope file.

The Windows Event Log can be accessed by running *eventvwr.exe* or searching for
\ *Event Viewer*\ ; from there look under *Windows Logs* and *Application*.

You can increase the verbosity of the logging by adding the *--verbose* option
to the E-MailRelay options, typically by editing the *emailrelay-start.bat*
batch script in *C:\\ProgramData\\E-MailRelay*.

Testing with telnet
===================
The *telnet* program can be used for testing an E-MailRelay server.

To install the program search for *Windows Features* and enable the "Telnet
client" checkbox.

Then run telnet from a command prompt, using *localhost* and the E-MailRelay
port number as command-line parameters:

::

    telnet localhost 25

This should show a greeting from the E-MailRelay server and then you can
start typing SMTP_ commands like *EHLO*, *MAIL FROM:<..>*, *RCPT TO:<...>*
and *DATA*. Refer to RFC-821_ Appendix F for some examples.






.. _RFC-821: https://tools.ietf.org/html/rfc821
.. _SMTP: https://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol

.. footer:: Copyright (C) 2001-2023 Graeme Walker
