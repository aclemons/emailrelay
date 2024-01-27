@echo off
rem
rem Copyright (C) 2001-2023 Graeme Walker <graeme_walker@users.sourceforge.net>
rem 
rem This program is free software: you can redistribute it and/or modify
rem it under the terms of the GNU General Public License as published by
rem the Free Software Foundation, either version 3 of the License, or
rem (at your option) any later version.
rem 
rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.
rem 
rem You should have received a copy of the GNU General Public License
rem along with this program.  If not, see <http://www.gnu.org/licenses/>.
rem ===
rem
rem winbuildall.bat
rem
rem Builds perl, libressl, mbedtls, qt and emailrelay from source when
rem run from a msvc 64-bit build environment ("developer command prompt"),
rem without using "cmake". Typically used from a Microsoft "WinDev" VM or
rem a continuous-integration build server. The output build artifacts are
rem statically-linked executables in "emailrelay-bin-x64/src/main/release"
rem and "emailrelay-bin-x64/src/gui/release".
rem
rem There is no reliance on cmake because it is not available on a
rem clean "WinDev" VM and it is impossible to build from source.
rem
rem This batch file should be in a base directory containing read-only
rem source trees called "perl-src", "libressl-src", "mbedtls-src",
rem "qt-src" and "emailrelay-src":
rem
rem Download perl:
rem $ wget https://www.cpan.org/src/5.0/perl-5.38.2.tar.gz
rem $ tar -xzf perl-5.38.2.tar.gz
rem $ ren perl-5.38.2 perl-src
rem
rem Download libressl:
rem $ wget https://ftp.openbsd.org/pub/OpenBSD/LibreSSL/libressl-3.8.2.tar.gz
rem $ tar -xzf libressl-3.8.2.tar.gz
rem $ ren libressl-3.8.2 libressl-src
rem
rem Download mbedtls:
rem $ git clone https://github.com/Mbed-TLS/mbedtls.git mbedtls-src
rem $ git -C mbedtls-src checkout -q "mbedtls-2.28"
rem
rem Download qt5:
rem $ git clone https://code.qt.io/qt/qt5.git qt-src
rem $ git -C qt-src checkout 5.15.2
rem $ cd qt-src && perl init-repository --module-subset=qtbase,qttools,qttranslations
rem
rem Download emailrelay:
rem $ git clone https://git.code.sf.net/p/emailrelay/git emailrelay-src
rem $ git -C emailrelay-src checkout 2.5.2
rem $ copy emailrelay-src\libexec\winbuildall.bat .
rem

setlocal
set thisdir=%~dp0
set thisdrive=%~d0
set arch=%Platform%
set config=release
if "%1"=="debug" set config=debug

set emailrelaysrc=%thisdir%emailrelay-src
set perlsrc=%thisdir%perl-src
set mbedtlssrc=%thisdir%mbedtls-src
set libresslsrc=%thisdir%libressl-src
set qtsrc=%thisdir%qt-src

if not exist %emailrelaysrc%\src\glib\gdef.h (
	echo winbuildall: no emailrelay source at %emailrelaysrc%
	goto error
)
if not exist %libresslsrc%\ssl\ssl_lib.c (
	echo winbuildall: no libressl source at %libresslsrc%
	goto error
)
if not exist %mbedtlssrc%\include\mbedtls\ssl.h (
	echo winbuildall: no mbedtls source at %mbedtlssrc%
	goto error
)
if not exist %qtsrc%\qtbase\src\corelib (
	echo winbuildall: no qt source at %qtsrc%
	goto error
)
if not "%Platform%" == "x64" (
	if not "%Platform%" == "x86" (
		echo winbuildall: please run from a visual studio 64-bit developer command prompt
		goto error
	)
)

rem perl
rem
set cctype=MSVC143
if "%VisualStudioVersion%" == "16.0" set cctype=MSVC142
perl.exe -e "exit 99" 2>NUL
if %errorlevel% == 99 (
	set perl=perl.exe
) else (
	if not exist %perlsrc%\win32 (
		echo winbuildall: no perl source at %perlsrc%
		goto error
	)
	mkdir %thisdir%perl-bin 2>NUL
	if not exist %thisdir%perl-bin\bin\perl.exe (
		echo winbuildall: building perl: CCTYPE=%cctype% INST_DRV=%thisdrive% INST_TOP=%thisdir%perl-bin
		cd %perlsrc%\win32 && nmake CCTYPE=%cctype% INST_DRV=%thisdrive% INST_TOP=%thisdir%perl-bin clean
		cd %perlsrc%\win32 && nmake CCTYPE=%cctype% INST_DRV=%thisdrive% INST_TOP=%thisdir%perl-bin install
	)
	set perl=%thisdir%perl-bin\bin\perl.exe
)
%perl% -e "exit 99" 2>NUL
if not %errorlevel% == 99 (
	echo winbuildall: perl %perl% not built
	goto error
)

rem libressl
rem
if exist %thisdir%libressl-bin-%arch%\library\%config%\ssl.lib (
	echo winbuildall: libressl already built
) else (
	cd %thisdir% && %perl% %emailrelaysrc%/libexec/libresslbuild.pl --config=%config% --arch=%arch% %libresslsrc% libressl-bin-%arch%
	if not exist %thisdir%libressl-bin-%arch%\library\%config%\ssl.lib (
		echo winbuildall: libressl not built
		goto error
	)
)

rem mbedtls
rem
if exist %thisdir%mbedtls-bin-%arch%\library\%config%\mbedtls.lib (
	echo winbuildall: mbedtls already built
) else (
	cd %thisdir% && %perl% %emailrelaysrc%/libexec/mbedtlsbuild.pl --config=%config% --arch=%arch% %mbedtlssrc% mbedtls-bin-%arch%
	if not exist %thisdir%mbedtls-bin-%arch%\library\%config%\mbedtls.lib (
		echo winbuildall: mbedtls not built
		goto error
	)
)

rem qt
rem
set corelib=Qt5Core.lib
if "%config%"=="debug" set corelib=Qt5Cored.lib
if exist %thisdir%qt-bin-%arch%\lib\%corelib% (
	echo winbuildall: qt already built
) else (
	cd %thisdir% && %perl% %emailrelaysrc%/libexec/qtbuild.pl --config=%config% --arch=%arch% %qtsrc% qt-build-%arch%-%config% qt-bin-%arch%
	if not exist %thisdir%qt-bin-%arch%\lib\%corelib% (
		echo winbuildall: qt not built
		goto error
	)
)

rem emailrelay
rem
mkdir %thisdir%emailrelay-bin-%arch% 2>NUL
cd %thisdir%emailrelay-bin-%arch% && %perl% %emailrelaysrc%/libexec/make2nmake --openssl
set OPENSSL_INC=%thisdir%libressl-bin-%arch%\include
set OPENSSL_RLIB=%thisdir%libressl-bin-%arch%\library\release
set OPENSSL_DLIB=%thisdir%libressl-bin-%arch%\library\debug
set MBEDTLS_INC=%thisdir%mbedtls-bin-%arch%\include
set MBEDTLS_RLIB=%thisdir%mbedtls-bin-%arch%\library\release
set MBEDTLS_DLIB=%thisdir%mbedtls-bin-%arch%\library\debug
set QT_INC=%thisdir%qt-bin-%arch%\include
set QT_LIB=%thisdir%qt-bin-%arch%\lib
set QT_MOC=%thisdir%qt-bin-%arch%\bin\moc.exe
cd %thisdir%emailrelay-bin-%arch% && nmake /nologo /e CONFIG=%config% ARCH=%arch%
if not exist %thisdir%emailrelay-bin-%arch%\src\main\%config%\emailrelay.exe (
	echo winbuildall: emailrelay executable not built
	goto error
)
if not exist %thisdir%emailrelay-bin-%arch%\src\gui\%config%\emailrelay-gui.exe (
	echo winbuildall: emailrelay gui executable not built
	goto error
)

echo winbuildall: done
endlocal
goto end

:error
echo winbuildall: failed
endlocal

:end
