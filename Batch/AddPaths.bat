@echo off
:##############################################################################
:#                                                                            #
:#  Filename        AddPaths.bat                                              #
:#                                                                            #
:#  Description     Add all JFL tools collection paths                        #
:#                                                                            #
:#  Notes 	    Designed to be executable in a VM, pointing at tools in   #
:#                  the host or in a network share.                           #
:#                                                                            #
:#                  Uses AddPath.bat internally.			      #
:#                                                                            #
:#  History                                                                   #
:#   2013-07-24 JFL Created this script.			              #
:#   2018-12-21 JFL Only add WIN64 paths when running on an AMD64 processor.  #
:#                  Conversely add DOS path when running on an x86 processor. #
:#                  Display the result path list just once in the end.        #
:#                  Added option -r to remove all my personal paths.          #
:#   2020-02-24 JFL Use the new paths.bat, instead of the old addpath.bat.    #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2020-02-24"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "SDRV=%~d0"
set "ARG0=%~f0"
goto Main

:Help
echo.
echo %SCRIPT% version %VERSION% - Add all JFL tools collection paths
echo.
echo Usage: %SCRIPT% [OPTIONS]
echo.
echo Options:
echo   -?         Display this help
echo   -r         Remove all paths added by this script
echo   -V         Display the script version and exit
echo   -X         Display the commands generated and exit
echo.
goto :eof

:#----------------------------------------------------------------------------#
:# Main routine

:Main
set "CALL=call"
set "EXEC="
set "VERBOSE=0"
goto :get_args

:next_arg
shift
:get_args
if .%1.==.. goto :go
if "%~1"=="/?" goto :Help
if "%~1"=="-?" goto :Help
if "%~1"=="-r" call :Remove & goto :addpath.exit
if "%~1"=="-v" set "VERBOSE=1" & goto :next_arg
if "%~1"=="-V" (echo.%VERSION%) & goto :eof
if "%~1"=="-X" set "CALL=echo" & set "EXEC=rem" & goto :next_arg
goto :next_arg

:Add1Path.Init
:# Look for addpath.bat in the same directory as this script
set "ADDPATH=%SPATH%\paths.bat"
:# Else look for paths.bat in the PATH
if not exist "%ADDPATH%" for %%I in (paths.bat) do set "ADDPATH=%%~$PATH:I"
:# If it's not there either, report the error
if not defined ADDPATH >&2 echo Error: Can't find paths.bat & exit /b 1
:# Note: Even if paths.bat is in the PATH, do not use its unqualified name,
:# because this would break during paths removal.
goto :eof

:Add1Path
set "RETCODE=0"
if exist "%~1" (
  %CALL% %ADDPATH% -q    -a "%~1"
  if errorlevel 1 set "RETCODE=1"
  %CALL% %ADDPATH% -q -s -a "%~1"
  if errorlevel 1 set "RETCODE=1"
)
exit /b %RETCODE%

:Remove
call :Add1Path.Init
if errorlevel 1 exit /b
set "RETCODE=0"
for %%p in (
  "%SPATH%\Win64"
  "%SPATH%\Win32"
  "%SPATH%"
  "%SPATH%\DOS"
  "%SPATH%\ezWinPorts\Win64\bin"
  "%SPATH%\ezWinPorts\Win32\bin"
  "%SPATH%\UnxUtils\usr\local\wbin"
  "%SPATH%\GnuWin32\bin"
  "%SDRV%\Windows\SUA\common"
  "%SDRV%\MinGW\msys\1.0\bin"
  "%SPATH%\SysInternals"
) do (
  %CALL% %ADDPATH% -q    -r "%%~p"
  if errorlevel 1 set "RETCODE=1"
  %CALL% %ADDPATH% -q -s -r "%%~p"
  if errorlevel 1 set "RETCODE=1"
)
:# Display the result path list in the end
%CALL% %ADDPATH%
exit /b %RETCODE%

:go
:# Check that the addpaths.bat directory contains the other basic tools we need.
if not exist "%SPATH%\paths.bat" (
  >&2 echo Error: %SCRIPT% must be located in the Tools directory itself.
  exit /b 1
)

set "IF64=if [%PROCESSOR_ARCHITECTURE%]==[AMD64]"
set "IF32=if [%PROCESSOR_ARCHITECTURE%]==[x86]"

:# Add all JFL tools collection paths, with the hightest priority paths first
call :Add1Path.Init
if errorlevel 1 exit /b
%IF64% call :Add1Path "%SPATH%\Win64"			&:# Windows 64-bits tools
       call :Add1Path "%SPATH%\Win32"			&:# Windows 32-bits tools
       call :Add1Path "%SPATH%"				&:# Other types of Windows programs and scripts
%IF32% call :Add1Path "%SPATH%\DOS"			&:# DOS 16-bits tools. Must be AFTER "%SPATH%", as there's another paths.bat there for DOS only.
%IF64% call :Add1Path "%SPATH%\ezWinPorts\Win64\bin"	&:# Unix tools 64-bits ports from ezwinports.sourceforge.net
       call :Add1Path "%SPATH%\ezWinPorts\Win32\bin"	&:# Unix tools 32-bits ports from ezwinports.sourceforge.net
       call :Add1Path "%SPATH%\UnxUtils\usr\local\wbin"	&:# Unix tools ports from unxutils.sourceforge.net
       call :Add1Path "%SPATH%\GnuWin32\bin"		&:# Unix tools ports from gnuwin32.sourceforge.net
       call :Add1Path "%SDRV%\Windows\SUA\common"	&:# Unix tools ports from Microsoft SUA (SubSystem for Unix Applications)
       call :Add1Path "%SDRV%\MinGW\msys\1.0\bin"	&:# Unix tools ports from mingw.sourceforge.net
       call :Add1Path "%SPATH%\SysInternals"		&:# Microsoft SysInternals tools
:# Display the result path list in the end
%CALL% %ADDPATH%

:# endlocal is necessary for returning the modified value back to the caller
:addpath.exit
endlocal & %EXEC% set "Path=%Path%"
