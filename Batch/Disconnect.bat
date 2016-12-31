@echo OFF
:##############################################################################
:#                                                                            #
:#  Filename:       disconnect.bat                                            #
:#                                                                            #
:#  Description:    Force disconnecting a zombie network drive                #
:#                                                                            #
:#  Notes:	    Sometimes Windows Explorer keeps listing network drives   #
:#		    even though they've been disconnected for long.           #
:#		    This causes many problems, including long timeouts.       #
:#									      #
:#                  See the following forum thread for details:               #
:#                  https://social.technet.microsoft.com/Forums/en-US/w7itpronetworking/thread/b611f1e0-2364-455c-b2ae-e6bd0c73bb86/
:#									      #
:#  History:                                                                  #
:#   2015-01-05 JFL Created this script based on the proposed workaround.     #
:#   2016-12-20 JFL Added -? and -V options.                                  #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2016-12-20"
set "SCRIPT=%~nx0"				&:# Script name
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"	&:# Script path, without the trailing \
set  "ARG0=%~f0"				&:# Script full pathname
set ^"ARGS=%*^"					&:# Argument line

:# Go process command-line arguments
goto Main

:Help
echo %SCRIPT% version %VERSION%
echo Force disconnection from a hung network drive. Warning: Kills Windows Explorer.
goto :eof

:Main
if "%~1"=="-?" goto :Help
if "%~1"=="/?" goto :Help
if "%~1"=="-V" (echo %VERSION%) & goto :eof

echo Warning: This will disconnect ALL network drives, and kill Windows Explorer!
echo Press Ctrl-C to escape, or
pause

:# Delete all network drive letters
net use * /delete /y
:# Kill all instances of Windows Explorer
taskkill /f /IM explorer.exe
:# Restart one Explorer instance to get a Start Menu
explorer.exe 

