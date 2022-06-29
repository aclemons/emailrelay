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
		access_win32.cpp \
		boot_win32.cpp \
		dir_win32.cpp \
		glink_win32.cpp
}

unix {
	TARGET = emailrelay-gui.real
	#LIBS += ...
	SOURCES += \
		access_unix.cpp \
		boot_unix.cpp \
		dir_unix.cpp \
		glink_unix.cpp
}

RC_FILE = emailrelay-gui.rc

build_pass:CONFIG(debug,debug|release) {
	DEFINES += _DEBUG=1
}

build_pass:CONFIG(release,debug|release) {
}

SOURCES += \
	guimain.cpp \
	gdialog.cpp \
	gpage.cpp \
	pages.cpp \
	installer.cpp \
	dir.cpp \
	glibsources.cpp \
	legal.cpp \
	serverconfiguration.cpp

HEADERS += \
	gdialog.h \
	gpage.h \
	pages.h

INCLUDEPATH += \
	.. \
	../main \
	../gssl \
	../glib

