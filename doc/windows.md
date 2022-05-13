E-MailRelay Windows
===================

Setup program
-------------
Installing E-MailRelay on Windows should be straightforward if you have the
setup program `emailrelay-setup.exe` and its associated `payload` files.

Run `emailrelay-setup.exe` as an administrator if you are going to be installing
into protected directories like `Program Files`.

You may need to run `vc_redist.x64.exe` first to install the Microsoft C++
run-time files.

The setup GUI will take you through the installation options and then install
the run-time files into your chosen locations.

If anything goes wrong with the installation process then you can still just
run the main E-MailRelay executable straight out of the distribution zip file.
Follow the `Manual installation` section below for more help.

Running the program
-------------------
After a successful installation you should see E-MailRelay listed in the Windows
Start Menu, or an E-MailRelay link under `Program Files/E-MailRelay`. This will
run the `emailrelay-start.bat` batch file to start the E-MailRelay server, and
you should then see an icon appear in the Windows system tray under the "Show
hidden icons" button.

Note that the `emailrelay-start.bat` file lives under `ProgramData`, and although
this might be a hidden directory you can still navigate there by right-clicking
on the E-MailRelay link under `Program Files` and selecting `Open file location`.

Configuration
-------------
E-MailRelay is configured though command-line options like `--verbose` or
`--spool-dir=c:/temp` in the `emailrelay-startup.bat` batch file.

You can edit the batch file directly using Notepad, or run `emailrelay-gui.exe`.

All command-line options are documented in the E-MailRelay reference document.

Account information can be configured by editing the E-MailRelay `secrets` file.
Look for the `--client-auth` or `--server-auth` options in the startup batch
file to find out where this is.

Manual installation
-------------------
The manual installation process for when you do not have the self-extracting
setup program, goes something like this:

* Create a new program directory `c:\Program Files\E-MailRelay`
* Copy the packaged files into `c:\Program Files\E-MailRelay`
* Create a new spool directory `c:\ProgramData\E-MailRelay\spool`
* Create a new text file, eg. `c:\ProgramData\E-MailRelay\auth.txt`, to contain account details
* Add your account details to `auth.txt` with a line like `client plain myaccount mypassword`
* Right-drag `emailrelay.exe` onto the desktop to create a shortcut for the server.
* Add `--as-server --verbose` to the server shortcut properties in the `target` box.
* Right-drag again to create a shortcut to do the forwarding.
* Add `--as-client example.com:smtp --client-auth c:\ProgramData\E-MailRelay\auth.txt` to the client shortcut.

Copy the shortcuts to `Start Menu` and `Startup` folders as necessary.

Running as a service
--------------------
E-MailRelay can be set up as a service so that it starts up automatically at
boot-time. Do do this manually you must first have a one-line batch file
called `emailrelay-start.bat` that contains all the command-line options for
running the E-MailRelay server, and you must have a simple service-wrapper
configuration file called `emailrelay-service.cfg` that points to it, and this
must be in the same directory as the service wrapper executable
(`emailrelay-service.exe`).

The startup batch file should contain a single line, something like this:

        start "emailrelay" "C:\Program Files\E-MailRelay\emailrelay.exe" --forward-to smtp.example.com:25 ...

There is no need to use `--no-daemon` and `--hidden`; these are added
automatically.

The contents of the service-wrapper configuration file can be a single
line that points to the directory containing the startup batch file,
like this:

        dir-config="C:\ProgramData\E-MailRelay"

Then just run `emailrelay-service --install` from an Administrator command
prompt to install the service.

Every time the service starts it reads the service-wrapper configuration file
and the startup batch file in order to run the E-MailRelay program.

If you need to run multiple E-MailRelay services then put a unique service
name and display name on the `emailrelay-service --install <name> <display-name>`
command-line. The service name you give is used to derive the name of the
`<name>-start.bat` batch file that contains the E-MailRelay server's
command-line options, so you will need to create that first.

Uninstall
---------
To uninstall:

* Stop the program and/or the service.
* Uninstall the service, if installed (`emailrelay-service --remove`).
* Delete the files from the E-MailRelay `program files` folder (eg. `C:\Program Files\E-MailRelay`).
* Delete the files from the E-MailRelay `program data` folder (eg. `C:\ProgramData\E-MailRelay`).
* Delete any desktop shortcuts (eg. `%USERPROFILE%\Desktop\E-MailRelay.lnk`).
* Delete any start menu shortcuts (eg. `%APPDATA%\Microsoft\Windows\Start Menu\Programs\E-MailRelay.lnk`).
* Delete any auto-start shortcuts (eg. `%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\E-MailRelay.lnk`).
* Clean up the registry under `HKLM\System\CurrentControlSet\Services\EventLog\Application\emailrelay`.

Diagnostics
-----------
E-MailRelay logging is sent to the Windows Event Log and/or written to a log
file, and individual failed mail messages will have the failure reason recorded
inside the `.bad` envelope file.

The Windows Event Log can be accessed by running `eventvwr.exe` or searching for
`Event Viewer`; from there look under `Windows Logs` and `Application`.

You can increase the verbosity of the logging by adding the `--verbose` option
to the E-MailRelay command-line, typically by editing the `emailrelay-start.bat`
batch script in `C:\ProgramData\E-MailRelay`.

Testing with telnet
-------------------
The `telnet` program can be used for testing an E-MailRelay server.

To install the program search for `Windows Features` and enable the "Telnet
client" checkbox.

Then run telnet from a command prompt, using `localhost` and the E-MailRelay
port number as command-line parameters:

        telnet localhost 25

This should show a greeting from the E-MailRelay server and then you can
start typing [SMTP][] commands like `EHLO`, `MAIL FROM:<..>`, `RCPT TO:<...>`
and `DATA`.





[SMTP]: https://en.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol

_____________________________________
Copyright (C) 2001-2021 Graeme Walker
