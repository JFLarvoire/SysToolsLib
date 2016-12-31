@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:       x.bat                                                     *
:#                                                                            *
:#  Description:    Execute the Windows port of a Unix program                *
:#                                                                            *
:#  Notes:	    Example: x find \windows -name notepad.exe		      *
:#                                                                            *
:#                  Searches for a few known ports in the directories:        *
:#                  ezwinports.sourceforge.net  %~dp0%\ezWinPorts\Win64\bin   *
:#                  ezwinports.sourceforge.net  %~dp0%\ezWinPorts\Win32\bin   *
:#                  unxutils.sourceforge.net    %~dp0%\UnxUtils\usr\local\wbin*
:#                  gnuwin32.sourceforge.net    %~dp0%\GnuWin32\bin           *
:#                  Microsoft SUA               %~d0%\Windows\SUA\common      *
:#                  mingw.sourceforge.net       %~d0%\MinGW\msys\1.0\bin      *
:#                                                                            *
:#  History:                                                                  *
:#   2004/10/12 JFL Created this batch.					      *
:#   2010-12-17 JFL Added the -X option.				      *
:#   2013-04-05 JFL Generalized the script to use one of the 5 known ports:   *
:#                  ezWinPorts UnxUtils GnuWin32 SUA MinGW                    *
:#                  Added options -e, -u, -g, -s, -m to use specific ones.    *
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2013-04-05"
set "SCRIPT=%~nx0"
set "SCRIPT_DRIVE=%~d0"
set "SCRIPT_PATH=%~dp0" & set "SCRIPT_PATH=!SCRIPT_PATH:~0,-1!"
set "ARG0=%~f0"

set FUNCTION=rem
set RETURN=goto :eof
goto main

:# Quote file pathnames that require it. %1=Input variable. %2=Opt. output variable.
:condquote
%FUNCTION% condquote %1 %2
setlocal enableextensions
call set "P=%%%~1%%"
set "P=%P:"=%"
set RETVAR=%~2
if not defined RETVAR set RETVAR=%~1
for %%c in (" " "&" "(" ")" "@" "," ";" "[" "]" "{" "}" "=" "'" "+" "`" "~") do (
  :# Note: Cannot directly nest for loops, due to incorrect handling of /f in the inner loop.
  cmd /c "for /f "tokens=1,* delims=%%~c" %%a in (".%%P%%.") do @if not "%%b"=="" exit 1"
  if errorlevel 1 (
    set P="%P%"
    goto :condquote_ret
  )
)
:condquote_ret
endlocal & set "%RETVAR%=%P%"
%RETURN%

:# Search for a Windows port of Unix tools.
:# Search in the standard location, or underneath this script directory.
:# This is useful, because some of my VMs do dot have a copy of the tools,
:# but instead have the host's tools directory in their PATH.

:# Search for the ezwinport.sourceforge.net port of a Unix program.
:ezWinPorts %1=program. Returns variable %1 set to the exe full pathname.
:# Search in the standard location, or underneath this script directory.
set "SUBDIRS=Win32"
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "SUBDIRS=Win64 Win32"
for %%p in ("%SCRIPT_PATH%" "%SCRIPT_DRIVE%" "C:") do (
  for %%s in (%SUBDIRS%) do (
    if .%DEBUG%.==.1. echo Checking "%%~p\ezWinPorts\%%~s\bin\%~1.exe"
    if exist "%%~p\ezWinPorts\%%~s\bin\%~1.exe" (
      set "%~1=%%~p\ezWinPorts\%%~s\bin\%~1.exe"
      set "NUL=NUL"
      set "FILTER=| remplace / \\"
      goto :eof
    )
  )
)
goto :eof

:# Search for the unxutils.sourceforge.net port of a Unix program.
:UnxUtils %1=program. Returns variable %1 set to the exe full pathname.
:# Search in the standard location, or underneath this script directory.
for %%p in ("%SCRIPT_PATH%" "%SCRIPT_DRIVE%" "C:") do (
  if .%DEBUG%.==.1. echo Checking "%%~p\UnxUtils\usr\local\wbin\%~1.exe"
  if exist "%%~p\UnxUtils\usr\local\wbin\%~1.exe" (
    set "%~1=%%~p\UnxUtils\usr\local\wbin\%~1.exe"
    set "NUL=NUL"
    set "FILTER=| remplace -q \n \r\n | remplace -q \r\r \r"
    goto :eof
  )
)
goto :eof

:# Search for the gnuwin32.sourceforge.net port of a Unix program.
:GnuWin32 %1=program. Returns variable %1 set to the exe full pathname.
:# Search in the standard location, or underneath this script directory.
for %%p in ("%SCRIPT_PATH%" "%SCRIPT_DRIVE%" "C:") do (
  if .%DEBUG%.==.1. echo Checking "%%~p\GnuWin32\bin\%~1.exe"
  if exist "%%~p\GnuWin32\bin\%~1.exe" (
    set "%~1=%%~p\GnuWin32\bin\%~1.exe"
    set "NUL=NUL"
    set "FILTER="
    goto :eof
  )
)
goto :eof

:# Search for the Microsoft SUA port of a Unix program.
:SUA %1=program. Returns variable %1 set to the exe full pathname.
:# Search in its standard Windows subdirectory, or on the same drive as this script.
for %%p in ("%windir%" "%SCRIPT_DRIVE%\Windows") do (
  if .%DEBUG%.==.1. echo Checking "%%~p\SUA\common\%~1.exe"
  if exist "%%~p\SUA\common\%~1.exe" (
    set "%~1=%%~p\SUA\common\%~1.exe"
    set "NUL=/dev/null"
    set "FILTER=| remplace -q \n \r\n | remplace -q \r\r \r"
    goto :eof
  )
)
goto :eof

:# Search for the mingw.sourceforge.net port of a Unix program.
:MinGW %1=program. Returns variable %1 set to the exe full pathname.
:# Search in the standard location, or on the same drive as this script.
for %%d in ("%SCRIPT_DRIVE%" "C:") do (
  for %%p in ("\MinGW\msys\1.0\bin" "\MinGW\bin") do (
    if .%DEBUG%.==.1. echo Checking "%%d%%p\%~1.exe"
    if exist "%%d%%p\%~1.exe" (
      set "%~1=%%d%%p\%~1.exe"
      set "NUL=NUL"
      set "FILTER="
      goto :eof
    )
  )
)
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Main                                                      #
:#                                                                            #
:#  Description     Process command line arguments                            #
:#                                                                            #
:#  Arguments       %*	    Command line arguments                            #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:help
echo %SCRIPT% - Execute the Windows port of a Unix program
echo.
echo Usage: x [OPTIONS] program [arguments]
echo.
echo Options:
echo   -?    Display this help message
echo   -e    Use the ezwinports.sourceforge.net port (default)
echo   -g    Use the gnuwin32.sourceforge.net port (3rd choice)
echo   -m    Use the mingw.sourceforge.net port (5th choice)
echo   -s    Use the Microsoft SUA port (4th choice)
echo   -u    Use the unxutils.sourceforge.net port (2nd choice)
echo   -v    Verbose mode: Display the command and run it
echo   -V    Display the script version and exit
echo   -X    Display the command to run, but don't run it
goto :eof

:main
if .%DEBUG%.==.1. echo %ARG0% %*

set "PORTS="
set "VERBOSE=0"
set "NOEXEC=0"
goto get_args

:# Parse wrapper arguments
:next_arg
shift
:get_args
if .%1.==.. goto help
if "%~1"=="-?" goto help
if "%~1"=="/?" goto help
if "%~1"=="-e" set "PORTS=%PORTS% ezWinPorts" & goto next_arg
if "%~1"=="-g" set "PORTS=%PORTS% GnuWin32" & goto next_arg
if "%~1"=="-m" set "PORTS=%PORTS% MinGW" & goto next_arg
if "%~1"=="-s" set "PORTS=%PORTS% SUA" & goto next_arg
if "%~1"=="-u" set "PORTS=%PORTS% UnxUtils" & goto next_arg
if "%~1"=="-v" set "VERBOSE=1" & goto next_arg
if "%~1"=="-V" (echo.%VERSION%) & goto :eof
if "%~1"=="-X" set "NOEXEC=1" & goto next_arg
set "PGM=%~1"
if "%PGM:~0,1%"=="-" (>&2 echo Error: Invalid switch %1) & goto help

:# Search all known ports if no specific one specified 
if "%PORTS%"=="" set "PORTS=ezWinPorts UnxUtils GnuWin32 SUA MinGW"

:# Search in the list of ports
set "CMD="
for %%u in (%PORTS%) do (
  call :%%u %PGM%
  if not "!%PGM%!"=="" goto :found_exe
)

:# Admit failure
>&2 echo Error: Cannot find a Windows port of the Unix %PGM% program
goto :eof

:# OK, we've identified a Windows EXE. Generate the full command line now.
:found_exe
if .%DEBUG%.==.1. echo set "%PGM%=!%PGM%!"
call :condquote %PGM%

:# Rebuild the full command line
set "CMDLINE=!%PGM%!"
:loop
shift
if .%1.==.. goto loop_end
set CMDLINE=%CMDLINE% %1
goto loop
:loop_end

:# Execute it
if %VERBOSE%==1 echo %CMDLINE%
if %VERBOSE%==0 if %NOEXEC%==1 echo %CMDLINE%
if %NOEXEC%==0 %CMDLINE% %FILTER%

