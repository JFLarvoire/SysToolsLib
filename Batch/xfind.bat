@echo off
:##############################################################################
:#                                                                            #
:#  Filename:       xfind.bat                                                 #
:#                                                                            #
:#  Description:    Wrapper for the Unix find command                         #
:#                                                                            #
:#  Notes:	    Necessary because Windows has a homonym find command,     #
:#		    which will usually be found first in Windows' PATH.	      #
:#                                                                            #
:#                  Uses a Windows port of the Unix find command.             #
:#                  Searches for a few known ports in the directories:        #
:#                  ezwinports.sourceforge.net  %~dp0%\ezWinPorts\Win64\bin   #
:#                  ezwinports.sourceforge.net  %~dp0%\ezWinPorts\Win32\bin   #
:#                  unxutils.sourceforge.net    %~dp0%\UnxUtils\usr\local\wbin#
:#                  gnuwin32.sourceforge.net    %~dp0%\GnuWin32\bin           #
:#                  mingw.sourceforge.net       %~d0%\MinGW\msys\1.0\bin      #
:#                                                                            #
:#  History:                                                                  #
:#   2010-11-29 JFL Added support for Windows SUA port of the find command.   #
:#                  Added a help screen.                                      #
:#   2010-12-08 JFL Use %~f0 instead of "which %0".			      #
:#                  Added options -m and -s to allow testing both Unix finds. #
:#                  Restructured code to make it more understandable.         #
:#   2011-01-04 JFL Use routine condquote to quote find.exe if needed.        #
:#   2013-04-04 JFL First try ezWinPorts versions of find & grep, which are   #
:#                  much faster than the others. (Specially the 64-bits one)  #
:#                  Renamed the mingw port with its real name: unxutils       #
:#   2013-04-05 JFL Added support for the gnuwin32 and mingw ports.           #
:#   2015-02-14 JFL Added option -d.                                          #
:#   2016-12-20 JFL Added routine :FindInPath, to allow finding targets       #
:#                  even when running a copy of this script not in the PATH.  #
:#                  Changed the output filtering, to correct only file names. #
:#   2017-08-30 JFL Bug fix: Output the same # of : as find did. (0,1,2,...)  #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2017-08-30"
set "SCRIPT=%~nx0"
set "SCRIPT_DRIVE=%~d0"
set "SCRIPT_PATH=%~dp0" & set "SCRIPT_PATH=!SCRIPT_PATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*

set FUNCTION=rem
set RETURN=goto :eof
set FOREACHLINE=for /f "delims="
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

:strlen stringVar lenVar                -- returns the length of a string
:strlen.q stringVar lenVar                -- returns the length of a string
setlocal EnableDelayedExpansion
set "len=0"
if defined %~1 for /l %%b in (12,-1,0) do (
  set /a "i=(len|(1<<%%b))-1"
  for %%i in (!i!) do if not "!%~1:~%%i,1!"=="" set /a "len=%%i+1"
)
endlocal & if "%~2" neq "" set "%~2=%len%"
exit /b

:# Search for a Windows port of Unix tools.
:# Search in the standard location, or underneath this script directory.
:# This is useful, because some of my VMs do dot have a copy of the tools,
:# but instead have the host's tools directory in their PATH.

:FindInPath %1=SubDir %2=RetVar
%FUNCTION% FindInPath %1 %2
setlocal EnableExtensions DisableDelayedExpansion
for %%s in (%1) do for %%r in ("%%~$PATH:s") do set "RESULT=%%~dpr"	&:# Keep only the parent path
if defined RESULT set "RESULT=%RESULT:~0,-1%"	&:# Remove the trailing \
endlocal & set "%~2="%RESULT%""
%RETURN%

:# Search for the ezwinport.sourceforge.net port of a Unix program.
:ezWinPorts %1=program. Returns variable %1 set to the exe full pathname.
:# Search in the standard location, or underneath this script directory.
call :FindInPath ezWinPorts IN_PATH
set "SUBDIRS=Win32"
if "%PROCESSOR_ARCHITECTURE%"=="AMD64" set "SUBDIRS=Win64 Win32"
for %%p in ("%SCRIPT_PATH%" %IN_PATH% %SCRIPT_DRIVE% C:) do (
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
call :FindInPath UnxUtils IN_PATH
for %%p in ("%SCRIPT_PATH%" %IN_PATH% %SCRIPT_DRIVE% C:) do (
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
call :FindInPath GnuWin32 IN_PATH
for %%p in ("%SCRIPT_PATH%" %IN_PATH% %SCRIPT_DRIVE% C:) do (
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
echo.
echo %SCRIPT% - Invoke the Windows port of Unix find available on this system.
echo.
echo Usage: xfind [XFIND_WRAPPER_OPTIONS] [UNIX_FIND_OPTIONS]
echo.
echo xfind wrapper options:
echo   -?    This help
echo   --    End of wrapper options
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
set "DEBUG=0"
set "NOEXEC=0"
goto get_args

:# Parse wrapper arguments
:next_arg
shift
:get_args
if .%1.==.. goto help
if "%~1"=="-?" goto help
if "%~1"=="/?" goto help
if "%~1"=="--" shift & goto go
if "%~1"=="-d" set "DEBUG=1" & goto next_arg
if "%~1"=="-e" set "PORTS=%PORTS% ezWinPorts" & goto next_arg
if "%~1"=="-g" set "PORTS=%PORTS% GnuWin32" & goto next_arg
if "%~1"=="-m" set "PORTS=%PORTS% MinGW" & goto next_arg
if "%~1"=="-s" set "PORTS=%PORTS% SUA" & goto next_arg
if "%~1"=="-u" set "PORTS=%PORTS% UnxUtils" & goto next_arg
if "%~1"=="-v" set "VERBOSE=1" & goto next_arg
if "%~1"=="-V" (echo.%VERSION%) & goto :eof
if "%~1"=="-X" set "NOEXEC=1" & goto next_arg
:# Anything else is a native find option. Go for it.

:go
:# Search all known ports if no specific one specified 
if "%PORTS%"=="" set "PORTS=ezWinPorts UnxUtils GnuWin32 SUA MinGW"

:# Search in the list of ports
set "CMD="
for %%u in (%PORTS%) do (
  call :%%u find
  if not "!find!"=="" goto :found_exe
)

:# Admit failure
>&2 echo Error: Cannot find a Windows port of the Unix find program
goto :eof

:# OK, we've identified a Windows EXE. Generate the full command line now.
:found_exe
if .%DEBUG%.==.1. echo set "find=%find%"
call :condquote find

set CMDLINE=%find%
:loop
if not .%1.==.. (
  set CMDLINE=%CMDLINE% %1
  shift
  goto loop
)

:# Execute it
if %VERBOSE%==1 echo %CMDLINE%
if %VERBOSE%==0 if %NOEXEC%==1 echo %CMDLINE%
:# Filter the output, to change Unix / in the files paths to Windows \
:# The output is structured like: PATHNAME:LINE
:# Problem: The PATHNAME itself may contain a : after the drive name.
:# This does not always occur. For example the default path is .
setlocal DisableDelayedExpansion
if %NOEXEC%==0 %FOREACHLINE% %%l in ('%CMDLINE%') do (
  set "LINE=%%l"
  setlocal EnableDelayedExpansion
  if "!LINE:~1,1!"==":" (	:# The file pathname begins with a drive name
    set "DRIVE=!LINE:~0,2!"
    set "LINE=!LINE:~2!"
  ) else (			:# The file pathname is a relative name
    set "DRIVE="
  )
  for /f "tokens=1 delims=:" %%n in ("!LINE!") do (
    setlocal DisableDelayedExpansion
    set "PATHNAME=%%n"
    :# We can't safely read a second token here, as tails starting with another : will be truncated
    :# Instead, build the tail by removing the pathname from the whole line.
    setlocal EnableDelayedExpansion
    call :strlen PATHNAME LNAME
    for /f %%l in ("!LNAME!") do set "TAIL=!LINE:~%%~l!"
  )
  :# Change slashes in the pathname, but not in the tail
  echo !DRIVE!!PATHNAME:/=\!!TAIL!
  endlocal
  endlocal
  endlocal
)
