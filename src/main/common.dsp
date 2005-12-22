# Microsoft Developer Studio Project File - Name="common" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=common - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "common.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "common.mak" CFG="common - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "common - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "common - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "common - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "CommonRelease"
# PROP Intermediate_Dir "CommonRelease"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GR /GX /O2 /I "../../lib/msvc6.0" /I "../gsmtp" /I "../gnet" /I "../glib" /I "../win32" /D "NDEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /D "G_WIN32" /YX"gdef.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "common - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "CommonDebug"
# PROP Intermediate_Dir "CommonDebug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GR /GX /ZI /Od /I "../../lib/msvc6.0" /I "../gsmtp" /I "../gnet" /I "../glib" /I "../win32" /D "_DEBUG" /D "_LIB" /D "WIN32" /D "_MBCS" /D "G_WIN32" /YX"gdef.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "common - Win32 Release"
# Name "common - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\gnet\gaddress_ipv4.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gadminserver.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gappbase.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gappinst.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\garg.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\garg_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gbase64.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gcleanup_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gclient.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gclient_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gclientprotocol.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gconnection.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gcontrol.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gcracker.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gdaemon_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gdate.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gdatetime.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gdatetime_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gdc.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gdescriptor_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gdialog.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gdirectory.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gdirectory_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\geventhandler.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\geventloop.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\geventloop_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\geventserver.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gexception.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gexe.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gfile.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gfile_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gfilestore.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gfs_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\ggetopt.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gidentity_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\glinebuffer.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\glocal.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\glocal_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\glog.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\glogoutput.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\glogoutput_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gmd5_native.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gmessagestore.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gmessagestore_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gmonitor.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gnewfile.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gnewmessage.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gpath.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gpidfile.cpp
# End Source File
# Begin Source File

SOURCE=..\gpop\gpopauth.cpp
# End Source File
# Begin Source File

SOURCE=..\gpop\gpopsecrets.cpp
# End Source File
# Begin Source File

SOURCE=..\gpop\gpopsecrets_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gpop\gpopserver.cpp
# End Source File
# Begin Source File

SOURCE=..\gpop\gpopserverprotocol.cpp
# End Source File
# Begin Source File

SOURCE=..\gpop\gpopstore.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gprocess_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gprocessor.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gprotocolmessage.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gprotocolmessageforward.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gprotocolmessagescanner.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gprotocolmessagestore.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gpump.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gpump_dialog.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gregistry_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\grequest.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gresolve.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gresolve_ipv4.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gresolve_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\groot.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gsasl_native.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gscannerclient.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gscmap.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gsecrets.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gsender.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gserver.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gserverprotocol.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gslot.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gsmtpclient.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gsmtpserver.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gsocket.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gsocket_win32.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gstoredfile.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gstoredmessage.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gstr.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gtime.cpp
# End Source File
# Begin Source File

SOURCE=..\gnet\gtimer.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gtray.cpp
# End Source File
# Begin Source File

SOURCE=..\gsmtp\gverifier.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gwinbase.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gwindow.cpp
# End Source File
# Begin Source File

SOURCE=..\win32\gwinhid.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\gxtext.cpp
# End Source File
# Begin Source File

SOURCE=legal.cpp
# End Source File
# Begin Source File

SOURCE=..\glib\md5.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\glib\gslot.h
# End Source File
# End Group
# End Target
# End Project
