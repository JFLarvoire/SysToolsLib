@echo off
:##############################################################################
:#                                                                            #
:#  Filename        subcmd.bat                                                #
:#                                                                            #
:#  Description     Start a sub-shell, changing the prompt to show the level  #
:#                                                                            #
:#  Notes 	    Useful to test complex batch files, that might fail and   #
:#                  pollute the environment with lots of internal variables.  #
:#                                                                            #
:#  History                                                                   #
:#   2015-11-24 JFL Created this script.                                      #
:#   2015-12-01 JFL Display the extension and expansion modes in the prompt.  #
:#   2015-12-07 JFL Restore the initial environment before running the shell. #
:#   2016-11-11 JFL Fixed the sub shell numbering counter.                    #
:#   2017-09-04 JFL Fixed the help, and the /E:off invokation.                #
:#                                                                            #
:#         © Copyright 2017 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

:# Important: Do not use any command extensions, as this needs to run even if extensions are off
setlocal &:# Do not change extensions and expansion now, as we want to record the initial state first.
set VERSION=2017-09-04
set SCRIPT=%0
set ARGS=%*
goto :main

:help
echo %SCRIPT% - Start a sub-shell, with a prompt showing the shell level and modes.
echo.
echo Usage: %SCRIPT% [CMD_OPTIONS]
echo.
echo cmd.exe options:
echo   /e:[on^|off]   Enable or disable command extensions
echo   /v:[on^|off]   Enable or disable delayed argument expansion
goto :eof

:main
:# Get the initial extension and expansion states
:# Note: Don't use quotes around set commands in the next two lines, as this will not work if extensions are disabled
set E0=off
set V0=off
set "E0=on" 2>NUL &:# Fails if extensions are disabled
if "!!"=="" set V0=on

:# Enable extensions to process the command line
setlocal EnableExtensions EnableDelayedExpansion

set "E="	&:# New extension mode specified
set "V="	&:# New expansion mode specified

:next_arg
set "ARG=%~1"
shift
if "!ARG!"=="-?" goto :help
if "!ARG!"=="/?" goto :help
if /i "!ARG:~0,2!"=="/e" set "E=!ARG:~3!" & goto :next_arg
if /i "!ARG:~0,2!"=="/v" set "V=!ARG:~3!" & goto :next_arg
:# Anything else is part of the command line. Fall though

set "CMD=cmd"
if not defined NSUBCMD set "NSUBCMD=1"
set /a NSUBCMD+=1
set "PROMPT=#%NSUBCMD%"
if not defined E set "E=%E0%" & set "CMD=%CMD% /E:%E0%"
set "PROMPT=%PROMPT% E:%E%"
if not defined V set "V=%V0%" & set "CMD=%CMD% /V:%V0%"
set "PROMPT=%PROMPT% V:%V%"

:# Restore the initial environment, then start the subshell with an updated prompt
:# Do not quote the NSUBCMD setting, as this fails with /E:off.
endlocal & endlocal & %CMD% %* /k "set NSUBCMD=%NSUBCMD%& prompt %PROMPT% $P^>"
