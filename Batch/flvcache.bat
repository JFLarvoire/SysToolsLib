@echo off
:#----------------------------------------------------------------------------#
:#                                                                            #
:#  File name       flvcache.bat                                              #
:#                                                                            #
:#  Description     Find Flash Video files in local cache                     #
:#                                                                            #
:#  Notes           Actually the latest version uses a simple dir command,    #
:#                  not the Unix find command. So most options actually do    #
:#                  not work.                                                 #
:#                                                                            #
:#                  Uses a WIN32 port of Unix' find command.                  #
:#                  See http://unxutils.sourceforge.net                       #
:#                  As Unix' find.exe program has the same name as Windows'   #
:#                  built-in find.exe command, but NOT the same features,     #
:#                  we use xfind.bat to invoke the Unix version.              #
:#                                                                            #
:#  History                                                                   #
:#   2011-11-03 JFL Created this script, based on the earlier gccache.bat.    #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Make sure environment variables used here are local
setlocal

set ARG0=%0
set EXEC=call :Exec
set RETURN=goto :EOF

:# Define global booleans, possibly inheriting existing values
if not .%VERBOSE%.==.1. set VERBOSE=0
if not .%DEBUG%.==.1.   set DEBUG=0
if not .%NOEXEC%.==.1.  set NOEXEC=0

goto Main

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Exec                                                      #
:#                                                                            #
:#  Description     Conditionally Display and Execute a command.              #
:#                                                                            #
:#  Arguments       %* = The command and its arguments                        #
:#                                                                            #
:#  Notes                                                                     #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-19 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Exec
if .%VERBOSE%.==.1. echo %*
if .%NOEXEC%.==.1. if .%VERBOSE%.==.0. echo %*
if not .%NOEXEC%.==.1. %*
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Main                                                      #
:#                                                                            #
:#  Description     Main routine                                              #
:#                                                                            #
:#  Note                                                                      #
:#                                                                            #
:#  History                                                                   #
:#   2010-09-04 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Help
echo.
echo Usage: gccache [options]
echo.
echo Options:
echo   -?        Display this help
echo   -cmin -N  Find files changed ^< N minutes ago. Default: %CMIN% minutes.
echo   -size +N  Minimum file size. Default: %SIZE%
echo   -v        Verbose mode. Display the commands executed.
echo   -X        Display commands to execute, but don't execute them.
%RETURN%

:Main

:# Set application-specific variables
set NAME=fl*.tmp
set CMIN=-10
set SIZE=+100k
set PRINTF="%%%%CY-%%%%Cm-%%%%Cd %%%%CH:%%%%CM:%%%%CS %%%%10s %%%%p\n"
set ARGS=

goto getarg

:# Process the command line options
:nextarg
shift
:getarg
if .%1.==.. goto Start
if .%1.==.-?. goto Help
if .%1.==./?. goto Help
if %1==--call goto Call
if %1==-d goto Debug
if %1==-cmin goto cmin
if %1==-name goto name
if %1==-size goto size
if %1==-v goto Verbose
if %1==-X goto NoExec
:# Anything else is considered as a Unix find option
set ARGS=%ARGS% %1
goto nextarg

:# Mechanism for calling a subroutine from the command line
:Call
shift
set PROC=%1
shift
call :%PROC% %1 %2 %3 %4 %5 %6 %7 %8 %9
%RETURN%

:cmin
shift
set cmin=%1
goto nextarg

:name
shift
set name=%1
goto nextarg

:Debug
set DEBUG=1
set D=-d
goto nextarg

:NoExec
set NOEXEC=1
goto nextarg

:size
shift
set size=%1
goto nextarg

:Verbose
set VERBOSE=1
set V=-v
goto nextarg

:# Find the files in the Microsoft Internet Explorer cache
:Start
set HOME=%HOMEDRIVE%%HOMEPATH%
:# Just in case, make sure APPDATA is defined. (afaik it is defined in all Windows >= XP)
if "%APPDATA%"=="" set APPDATA=%HOME%\Application Data
:# XP does not have a LOCALAPPDATA variable defined. Actually in XP it's the same as APPDATA.
if "%LOCALAPPDATA%"=="" set LOCALAPPDATA=%APPDATA%
:# AppData Local Temp cache
if exist   "%LOCALAPPDATA%\Temp" (
  set CACHE=%LOCALAPPDATA%\Temp
  goto cache_ok
)
echo >&2 Error: Failed to find the AppData Local Temp cache.
goto :EOF
:cache_ok
:* dir "%CACHE%"
:# %EXEC% xfind "%CACHE%" -name %NAME% -cmin %CMIN% -size %SIZE% -printf %PRINTF% %ARGS%
dir "%CACHE%\%NAME%" /od
