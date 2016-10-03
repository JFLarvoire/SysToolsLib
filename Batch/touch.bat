@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    touch.bat						      *
:#                                                                            *
:#  Description:    Same as Unix touch.exe, using pure batch		      *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2016-09-30 JFL Created this script, based on tricks used in nmake files. *
:#                                                                            *
:#        © Copyright 2016 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal enableextensions enabledelayedexpansion
set "VERSION=2016-09-30"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*

set "POPARG=call :PopArg"
set "PEEKARG=call :PeekArg"
goto Main

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

:PeekArg
set "ARG=Yes"
set ""ARG"=No"
call :PeekArg.Helper %ARGS% >NUL 2>NUL &:# Output redirections ensure the call help is not actually output.
if "%ARG%"=="Yes" if [%"ARG"%]==[No] call :PeekArg.Helper %ARGS:/?="/?"% 
goto :eof
:PeekArg.Helper
set "ARG=%~1"	&:# Remove quotes from the argument
set ""ARG"=%1"	&:# The same with quotes, if any, should we need them
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Main                                                      #
:#                                                                            #
:#  Description     Process command line arguments, and main routine body     #
:#                                                                            #
:#  Arguments       %*	    Command line arguments                            #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:main
set "DO="
set "touch.exe=touch.exe"

:# Special arguments for testing the script
:peek_next
%PEEKARG%
if "%ARG%"=="-X" %POPARG% & set "DO=echo" & goto :peek_next
if "%ARG%"=="-D" %POPARG% & set "touch.exe=DoNotFindTouch.exe" & goto :peek_next

:# Test if touch.exe is available in the PATH
for %%f in ("%touch.exe%") do set "touch.exe=%%~$PATH:f"
if defined touch.exe (	:# Invoke the real touch.exe, which is more efficient
  %DO% %touch.exe% %ARGS%
) else (		:# Invoke the pure batch version, which copies the file
  for %%a in (%ARGS%) do (
    set "SKIP="
    if "%%~a"=="-a" set "SKIP=Yes"	&rem :# Ignore touch.exe -a argument
    if "%%~a"=="-f" set "SKIP=Yes"	&rem :# Ignore touch.exe -f argument
    if "%%~a"=="-m" set "SKIP=Yes"	&rem :# Ignore touch.exe -m argument
    if "%%~a"=="--" set "SKIP=Yes"	&rem :# Ignore touch.exe -- argument
    if not defined SKIP %DO% copy /b %%a+NUL
  )
)
exit /b

:# One line versions for use in nmake files

:# The trick I had used in nmake files
touch $@ || copy /b $@+NUL &:# Use touch.exe if available, else use internal DOS commands for doing the same, albeit less efficiently.

:# Improved version that avoids double errors when invoked with a bad argument
touch $@ & if errorlevel 100 copy /b $@+NUL &:# Idem, but don't try again with copy if touch.exe exists, but did fail.

:# Improved version that tests the PATH, and runs only the right command
cmd /V:ON /c "set "T=" & for %f in (touch.exe) do set "T=%~$PATH:f" & if defined T (!T! $@) else (copy /b $@+NUL)"
