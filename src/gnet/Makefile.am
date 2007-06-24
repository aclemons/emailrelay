
EXTRA_DIST=\
	gaddress_ipv6.cpp \
	gdescriptor_win32.cpp \
	geventloop_win32.cpp \
	glocal_win32.cpp \
	grequest.cpp \
	gresolver_ipv6.cpp \
	gresolver_win32.cpp \
	gsimpleclient_win32.cpp \
	gsocket_win32.cpp \
	mingw.mak

INCLUDES = -I$(top_srcdir)/lib/$(COMPILER_VERSION) -I$(top_srcdir)/src/glib 
noinst_LIBRARIES = libgnet.a libipv4.a libipv6.a

libgnet_a_SOURCES = \
	gaddress.h \
	gbufferedclient.cpp \
	gbufferedclient.h \
	gbufferedserverpeer.cpp \
	gbufferedserverpeer.h \
	gclientptr.h \
	gclient.cpp \
	gclient.h \
	gconnection.cpp \
	gconnection.h \
	gdescriptor.h \
	gdescriptor_unix.cpp \
	gevent.h \
	geventhandler.cpp \
	geventhandler.h \
	geventloop.cpp \
	geventloop.h \
	geventloop_unix.cpp \
	gheapclient.cpp \
	gheapclient.h \
	glinebuffer.cpp \
	glinebuffer.h \
	glocal.cpp \
	glocal.h \
	glocal_unix.cpp \
	gmonitor.cpp \
	gmonitor.h \
	gmultiserver.cpp \
	gmultiserver.h \
	gnet.h \
	grequest.h \
	gresolver.cpp \
	gresolver.h \
	gresolverinfo.cpp \
	gresolverinfo.h \
	gresolver_unix.cpp \
	gsender.cpp \
	gsender.h \
	gserver.cpp \
	gserver.h \
	gsimpleclient.cpp \
	gsimpleclient.h \
	gsimpleclient_unix.cpp \
	gsocket.cpp \
	gsocket.h \
	gsocket_unix.cpp \
	gtimer.cpp \
	gtimer.h

libipv4_a_SOURCES=gaddress_ipv4.cpp gresolver_ipv4.cpp
libipv6_a_SOURCES=gaddress_ip.cpp gresolver_ip.cpp

