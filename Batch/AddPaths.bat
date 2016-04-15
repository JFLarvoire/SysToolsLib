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
:#                                                                            #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2013-07-24"
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
if "%~1"=="-v" set "VERBOSE=1" & goto :next_arg
if "%~1"=="-V" (echo.%VERSION%) & goto :eof
if "%~1"=="-X" set "CALL=echo" & set "EXEC=rem" & goto :next_arg
goto :next_arg

:Add1Path.Init
for %%I in (addpath.bat) do set "ADDPATH=%%~$PATH:I"
if not "!ADDPATH!"=="" (	:# addpath is in the PATH. No need to specify its path.
  set ADDPATH=addpath
) else (			:# Else use the one in the same directory as AddPaths.bat
  set "ADDPATH=%SPATH%\addpath"		&:# Assume there's no space in %SPATH%, so no need for quotes.
  if not "!ADDPATH!"=="!ADDPATH: =!" set ADDPATH="%SPATH%\addpath"
)
goto :eof

:Add1Path
if exist "%~1" (
  %EXEC% echo.
  %CALL% %ADDPATH%    "%~1"
  %EXEC% echo.
  %CALL% %ADDPATH% -s "%~1"
)
goto :eof

:go
:# Check that the addpaths.bat directory contains the other basic tools we need.
if not exist "%SPATH%\addpath.bat" (
  >&2 echo Error: %SCRIPT% must be located in the Tools directory itself.
  exit /b 1
)

:# Add all JFL tools collection paths, with the hightest priority paths first
call :Add1Path.Init
call :Add1Path "%SPATH%\Win64"			&:# Windows 64-bits tools
call :Add1Path "%SPATH%\Win32"			&:# Windows 32-bits tools
call :Add1Path "%SPATH%"			&:# Other types of Windows programs and scripts
call :Add1Path "%SPATH%\ezWinPorts\Win64\bin"	&:# Unix tools 64-bits ports from ezwinports.sourceforge.net
call :Add1Path "%SPATH%\ezWinPorts\Win32\bin"	&:# Unix tools 32-bits ports from ezwinports.sourceforge.net
call :Add1Path "%SPATH%\UnxUtils\usr\local\wbin"&:# Unix tools ports from unxutils.sourceforge.net
call :Add1Path "%SPATH%\GnuWin32\bin"		&:# Unix tools ports from gnuwin32.sourceforge.net
call :Add1Path "%SDRV%\Windows\SUA\common"	&:# Unix tools ports from Microsoft SUA (SubSystem for Unix Applications)
call :Add1Path "%SDRV%\MinGW\msys\1.0\bin"	&:# Unix tools ports from mingw.sourceforge.net
call :Add1Path "%SPATH%\SysInternals"		&:# Microsoft SysInternals tools

:# endlocal is necessary for returning the modified value back to the caller
endlocal & %EXEC% set "Path=%Path%"
