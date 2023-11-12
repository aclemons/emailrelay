#
# Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
# 
# Copying and distribution of this file, with or without modification,
# are permitted in any medium without royalty provided the copyright
# notice and this notice are preserved.  This file is offered as-is,
# without any warranty.
# ===
#
# emailrelay-gui.pro
#
# QMake project file for the emailrelay gui.
#
# Eg:
#   $ qmake --qt=5 CONFIG+=unix emailrelay-gui.pro
#   $ make release
#
# Eg:
#   $ qmake CONFIG+=win32 emailrelay-gui.pro
#   $ nmake /u VERBOSE=1 release
#
# See also: qtbuild.pl, winbuild.pl
#

CONFIG += warn_on
CONFIG += strict_c++
CONFIG += debug_and_release
CONFIG += static

TEMPLATE = app

QT += widgets

win32 {
	QMAKE_LFLAGS += "/MANIFESTUAC:level='highestAvailable'"
	QMAKE_LFLAGS += "/MANIFESTINPUT:emailrelay-gui.exe.manifest"
	LIBS += advapi32.lib ole32.lib oleaut32.lib
	SOURCES += \
		guiaccess_win32.cpp \
		guiboot_win32.cpp \
		guidir_win32.cpp \
		guilink_win32.cpp
}

unix {
	TARGET = emailrelay-gui.real
	#LIBS += ...
	SOURCES += \
		guiaccess_unix.cpp \
		guiboot_unix.cpp \
		guidir_unix.cpp \
		guilink_unix.cpp
}

RC_FILE = emailrelay-gui.rc
RC_INCLUDEPATH = ..\src\main\icon ..\..\src\main\icon ..\..\..\src\main\icon ..\..\..\..\src\main\icon

build_pass:CONFIG(debug,debug|release) {
	DEFINES += G_WITH_DEBUG
}

build_pass:CONFIG(release,debug|release) {
}

SOURCES += \
	guimain.cpp \
	guidialog.cpp \
	guipage.cpp \
	guipages.cpp \
	installer.cpp \
	guidir.cpp \
	glibsources.cpp \
	guilegal.cpp \
	serverconfiguration.cpp

HEADERS += \
	guidialog.h \
	guipage.h \
	guipages.h

INCLUDEPATH += \
	.. \
	../main \
	../gssl \
	../glib

