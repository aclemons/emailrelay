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
# On windows a static build of emailrelay-gui needs a
# static build of the Qt library code.
#
# Eg:
#	msvc> cd <qt-source>\qtbase
#   msvc> edit mkspecs\common\msvc-desktop.conf (/MT)
#   msvc> PATH=<qt-source>\qtbase\bin;%PATH%
#   msvc> configure.bat -static -release -prefix "<qt-install>" ... etc
#   msvc> nmake -f Makefile release
#   msvc> nmake -f Makefile install
#
# Then:
#   msvc> PATH=<qt-install>\bin;%PATH%
#   msvc> qmake CONFIG+="win static" emailrelay-gui.pro
#   msvc> copy ..\main\icon\*.ico .
#   msvc> mc messages.mc
#   msvc> nmake -f Makefile release
#

CONFIG += \
	warn_on \
	strict_c++ \
	debug_and_release \
	build_all

TEMPLATE = app

QT += widgets

win {
	QMAKE_LFLAGS += "/MANIFESTUAC:level='highestAvailable'"
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

build_pass:CONFIG(debug,debug|release) {
	DEFINES += _DEBUG=1
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

