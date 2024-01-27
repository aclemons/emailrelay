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
rem runperl.bat
rem
rem Runs "perl %1" and checks for an output file %2.
rem
rem The perl program is located using "ftype" or the PATH or c:/perl/bin.
rem Using ftype is more likely to find ActiveState perl rather than
rem MSYS or Cygwin. For ActiveState the perl script sees a $^O value
rem of "MSWin32".
rem

@rem find perl using ftype
SET RUNPERL_PERL=
for /f "tokens=2 delims== " %%I in ('cmd /c "ftype perl 2>NUL:"') do set RUNPERL_PERL=%%~I
IF "%RUNPERL_PERL%"=="" goto no_ftype
IF NOT EXIST "%RUNPERL_PERL%" goto no_ftype

@rem run the ftype perl
IF EXIST %2 del /f %2
for /f "tokens=2 delims== " %%I in ('cmd /c "ftype perl 2>NUL:"') do %%I %1 %3 %4 %5
goto done

@rem find perl on the path
:no_ftype
where /q perl
if errorlevel 1 goto no_path

@rem run perl on the path
IF EXIST %2 del /f %2
cmd /c perl %1 %3 %4 %5
goto done

@rem try c:\perl\bin
:no_path
c:\perl\bin\perl.exe -e "exit 99" 2>NUL:
if not errorlevel 99 goto fail_no_perl
c:\perl\bin\perl.exe %1 %3 %4 %5
goto done

@rem after running perl check for the touchfile
:done
IF NOT EXIST "%2" goto fail_no_touchfile
echo done
goto end

@rem error if no perl
:fail_no_perl
echo error: failed to find perl: please install ActiveState perl
goto end

@rem error if no touchfile
:fail_no_touchfile
echo error: perl command failed: no output file created
goto end

:end
if "%RUNPERL_NOPAUSE%"=="" pause
