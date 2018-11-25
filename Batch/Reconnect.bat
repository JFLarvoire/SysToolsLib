@echo off
:#----------------------------------------------------------------------------#
:#                                                                            #
:#  File name       Reconnect.bat                                             #
:#                                                                            #
:#  Description     Reconnect disconnected network drives		      #
:#                                                                            #
:#  Notes                                                                     #
:#                                                                            #
:#  History                                                                   #
:#   2014-07-10 JFL Created this script.                                      #
:#   2014-07-28 JFL Check the actual list of mounted drives.                  #
:#   2014-07-29 JFL Bug fix: "net use" output may overflow on two lines.      #
:#   2014-09-15 JFL Display the net use commands.                             #
:#   2015-05-20 JFL Display the net use commands.                             #
:#   2015-07-07 JFL Updated routine :PopArg.                                  #
:#   2015-09-01 JFL Added argument V to reconnect to \\jflzb\root.            #
:#   2015-09-01 JFL Made the list of drives a global extensible table.        #
:#                  Switched drives U: and V: now that jflzb is my main PC.   #
:#                  Allow specifying drives either as u, U, u:, U:            #
:#   2015-11-12 JFL Get the drive list from the registry.                     #
:#                  Removed all JFL-specific drive definitions.               #
:#   2018-11-25 JFL Fixed the help when no drive is currently disconnected.   #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:#----------------------------------------------------------------------------#

setlocal enableextensions enabledelayedexpansion
set "VERSION=2018-11-25"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*

set "POPARG=call :PopArg"
goto :Main

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        PopArg                                                    #
:#                                                                            #
:#  Description     Pop the first arguments from %ARGS% into %ARG%            #
:#                                                                            #
:#  Arguments       %ARGS%	    Command line arguments                    #
:#                                                                            #
:#  Returns         %ARG%           The unquoted argument                     #
:#                  %"ARG"%         The actual argument, possibly quoted      #
:#                                                                            #
:#  Notes 	    Works around the defect of the shift command, which       #
:#                  pops the first argument from the %* list, but does not    #
:#                  remove it from %*.                                        #
:#                                                                            #
:#                  Use an inner call to make sure the argument parsing is    #
:#                  done by the actual cmd.exe parser. This guaranties that   #
:#                  arguments are split exactly as shift would have done.     #
:#                                                                            #
:#                  But call itself has a quirk, which requires a convoluted  #
:#                  workaround to process the /? argument.                    #
:#                                                                            #
:#                  To do: Detect if the last arg has mismatched quotes, and  #
:#                  if it does, append one.                                   #
:#                  Right now such mismatched quotes will cause an error here.#
:#                  It is easily feasible to work around this, but this is    #
:#                  useless as passing back an invalid argument like this     #
:#                  will only cause more errors further down.                 #
:#                                                                            #
:#  History                                                                   #
:#   2015-04-03 JFL Bug fix: Quoted args with an & inside failed to be poped. #
:#   2015-07-06 JFL Bug fix: Call quirk prevented inner call from popping /?. #
:#                                                                            #
:#----------------------------------------------------------------------------#

:PopArg
:# Gotcha: The call parser first scans its command line for an unquoted /?.
:# If it finds one anywhere on the command line, then it ignores the target label and displays call help.
:# To work around that, we initialize %ARG% and %"ARG"% with an impossible combination of values.
set "ARG=Yes"
set ""ARG"=No"
call :PopArg.Helper %ARGS% >NUL 2>NUL &:# Output redirections ensure the call help is not actually output.
:# Finding that impossible combination now is proof that the call was not executed.
:# In this case, try again with the /? quoted, to prevent the call parser from processing it.
:# Note that we can not systematically do this /? enquoting, else it's "/?" that would break the call.
if "%ARG%"=="Yes" if [%"ARG"%]==[No] call :PopArg.Helper %ARGS:/?="/?"% 
goto :eof
:PopArg.Helper
set "ARG=%~1"	&:# Remove quotes from the argument
set ""ARG"=%1"	&:# The same with quotes, if any, should we need them
:# Rebuild the tail of the argument line, as shift does not do it
:# Never quote the set ARGS command, else some complex quoted strings break
set ARGS=%2
:PopArg.GetNext
shift
if [%2]==[] goto :eof
:# Leave quotes in the tail of the argument line
set ARGS=%ARGS% %2
goto :PopArg.GetNext

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

:Reconnect
echo net use %1 %2
net use %1 %2
goto :eof

:# Find the known drives in the registry, and the server path they're connected to
:FindDrives
set "DRIVES="
for /f %%k in ('reg query "HKEY_CURRENT_USER\Network"') do (
  set "KEY=%%k"
  set "DRIVE=!KEY:HKEY_CURRENT_USER\Network\=!:"
  set "DRIVES=!DRIVES! !DRIVE!"
  %IF_DEBUG% echo Found KEY=!KEY! DRIVE=!DRIVE!
  for /f "tokens=3" %%a in ('reg query !KEY! /v RemotePath ^| findstr RemotePath') do (
    set ADDRESS=%%a
    %IF_DEBUG% echo Found !DRIVE! !ADDRESS!
    set "!DRIVE!=!ADDRESS!"
  )
)
if defined DRIVES set "DRIVES=!DRIVES:~1!" &:# Remove the initial space inserted above
%IF_DEBUG% echo Found DRIVES=!DRIVES!
goto :eof

:Help
call :FindDrives
echo.
echo %SCRIPT% version %VERSION% - Reconnect disconnected network drives
echo.
echo Usage: %SCRIPT% [OPTIONS] [DRIVES]
echo.
echo Options:
echo   -?       Display this help
echo   -V       Display the script version and exit
echo.
echo Drives:    The drives to reconnect to. (The : is optional.) Ex: X: Y:
echo            Default on this system: %DRIVES%
echo.
goto :eof

:Main
set "RECONNECT=call :reconnect"
set "DONE=0"
set "IF_DEBUG=rem"
set "DRIVES="

:next_arg
%POPARG%
if "!ARG!"=="" goto :Start
if "!ARG!"=="-?" goto :Help
if "!ARG!"=="/?" goto :Help
if "!ARG!"=="-d" set "IF_DEBUG=" & set "DEBUG=1" & goto :next_arg
if "!ARG!"=="-V" (echo.%VERSION%) & goto :eof
if "!ARG:~0,1!"=="-" (
  >&2 echo Warning: Unexpected option ignored: !ARG!
  goto :next_arg
)
if not defined DRIVES call :FindDrives &:# Do this once, possibly after setting debug mode
for %%d in (%DRIVES%) do (
  if /i "!ARG!:" EQU "%%d" set "ARG=%%d"
  if /i "!ARG!"  EQU "%%d" call :reconnect %%d !%%d! & set "DONE=1" & goto next_arg
)
>&2 echo Warning: Unexpected argument ignored: !"ARG"!
goto :next_arg

:#----------------------------------------------------------------------------#
:# Start the real work

:Start
if "%DONE%"=="1" goto :eof	&:# We've reconnected to specific drives already
:# Else reconnect to all drives that were in use before
if not defined DRIVES call :FindDrives

:# Enumerate network drives, and check their status
for /f "tokens=1,2,3,*" %%i in ('net use ^| findstr :') do (
  :# Normal output:
  :# OK           U:        \\server1\util           Microsoft Windows Network
  :# Line overflow output on two lines:
  :# Unavailable  R:        \\server2.americas.bigcorp.net\results
  :#                                                 Microsoft Windows Network
  set "STATUS=%%i"
  set "DRIVE=%%j"
  set "SHARE=%%k"
  if "!DRIVE:~1,1!"==":" ( :# (Else it's a continuation line)
    echo Drive = !DRIVE! , Share = !SHARE! , Status = !STATUS!
    :# echo Comments = %%l
    if not "!STATUS!"=="OK" (
      echo Reconnecting...
      %RECONNECT% !DRIVE! !SHARE!
    )
  )
)
