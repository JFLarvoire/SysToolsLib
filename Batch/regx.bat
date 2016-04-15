@echo off
:##############################################################################
:#                                                                            #
:#  Filename:	    regx.bat						      #
:#                                                                            #
:#  Description:    Browse the registry "directories" and "files"             #
:#                                                                            #
:#  Notes:	    Pure batch file, using just Windows' reg.exe.	      #
:#		    Outputs in my SML structured format.		      #
:#									      #
:#                  Registry pathnames may contain & characters, which break  #
:#                  the set command if not quoted. Be careful to use quoting  #
:#		    consistently everywhere.				      #
:#									      #
:#		    To do: Manage complex multi-line values.		      #
:#		           Ex: HKLM\Cluster\Exchange\DagNetwork		      #
:#									      #
:#  History:                                                                  #
:#   2010-03-31 JFL created this batch file.       			      #
:#   2011-12-01 JFL Restructured.                  			      #
:#   2011-12-12 JFL Finalized change and fixed bugs.			      #
:#                                                                            #
:##############################################################################

setlocal enableextensions enabledelayedexpansion
set "VERSION=2011-12-12"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"

:# Mechanism for calling subroutines in a second external script instance.
:# Done by {%XCALL% label [arguments]}.
if .%1.==.-call. goto :call
set XCALL=cmd /c call "%ARG0%" -call

:# FOREACHLINE macro. (Change the delimiter to none to catch the whole lines.)
set FOREACHLINE=for /f "delims=" 

:# Default definitions
if not "%VERBOSE:~0,0%"=="" set VERBOSE=0
if not "%FULLPATH:~0,0%"=="" set FULLPATH=0
if not .%NOEXEC%.==.1. set NOEXEC=0

call :debug_init
goto main

:call
shift
call :%1 %2 %3 %4 %5 %6 %7 %8 %9
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        debug routines					      #
:#                                                                            #
:#  Description     A collection of debug routines                            #
:#                                                                            #
:#  Notes 	    debug_init	    Initialize debugging. Call once at first. #
:#                  debug_off	    Disable debugging                         #
:#                  debug_on	    Enable debugging			      #
:#                  debug_entry	    Log entry into a routine		      #
:#                  debug_return    Log exit out of a routine		      #
:#                  putvars	    Display the values of a set of variables  #
:#                  putargs	    Display the values of all arguments       #
:#                                                                            #
:#                  %FUNCTION%	    Define and trace the entry in a function. #
:#                  %RETURN%        Return from a function and trace it.      #
:#                                                                            #
:#                  Always match uses of %FUNCTION% and %RETURN%. That is     #
:#                  never use %RETURN% if there was no %FUNCTION% before it.  #
:#                                                                            #
:#  History                                                                   #
:#   2011-11-15 JFL Split debug_init from debug_off, to improve clarity.      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:debug_init
set "INDENT="
set "RETVAR=RETVAL"
set "INDENT_ECHO=call :indent_echo"
set "PUTVARS=call :putvars"
:debug_off
set "DEBUG=0"
set "FUNCTION=rem"
set "RETURN=goto :eof"
set "DEBUGVARS=rem"
set "DEBUGECHO=rem"
goto :eof

:debug_on
set "DEBUG=1"
set "FUNCTION=call :debug_entry"
set "RETURN=call :debug_return & goto :eof"
set "DEBUGVARS=call :putvars"
set "DEBUGECHO=>&2 call :indent_echo"
goto :eof

:debug_entry
>&2 echo %INDENT%call :%*
set "INDENT=%INDENT%  "
goto :eof

:debug_return
>&2 echo %INDENT%return !RETVAL!
set "INDENT=%INDENT:~0,-2%"
goto :eof

:putvars
if .%~1.==.. goto :eof
>&2 echo %INDENT%set "%~1=!%~1!"
shift
goto putvars

:putargs
setlocal
set N=0
:putargs_loop
if .%1.==.. endlocal & goto :eof
set /a N=N+1
>&2 echo %INDENT%set "ARG%N%=%1"
shift
goto putargs_loop

:indent_echo
echo.%INDENT%%*
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        condquote                                                 #
:#                                                                            #
:#  Description     Add quotes around the content of a pathname if needed     #
:#                                                                            #
:#  Arguments       %1	    Source variable name                              #
:#                  %2	    Destination variable name (optional)              #
:#                                                                            #
:#  Notes 	    Quotes are necessary if the pathname contains special     #
:#                  characters, like spaces, &, |, etc.                       #
:#                                                                            #
:#  History                                                                   #
:#   2010-12-19 JFL Created this routine                                      #
:#   2011-12-12 JFL Rewrote using findstr. (Executes much faster.)	      #
:#		    Added support for empty pathnames.                        #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Quote file pathnames that require it. %1=Input variable. %2=Opt. output variable.
:condquote
%FUNCTION% condquote %1 %2
setlocal enableextensions enabledelayedexpansion
set RETVAR=%~2
if "%RETVAR%"=="" set RETVAR=%~1
:# call set "P=%%%~1%%"
set "P=!%~1!"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Remove double quotes inside P. (Fails if P is empty)
set "P=%P:"=%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Look for any special character that needs quoting
echo."%P%"|findstr /C:" " /C:"&" /C:"(" /C:")" /C:"@" /C:"," /C:";" /C:"[" /C:"]" /C:"{" /C:"}" /C:"=" /C:"'" /C:"+" /C:"`" /C:"~" >NUL
if not errorlevel 1 set P="%P%"
:condquote_ret
:# Contrary to the general rule, do NOT enclose the set commands below in "quotes",
:# because this interferes with the quoting already added above. This would
:# fail if the quoted string contained an & character.
:# But because of this, do not leave any space around & separators.
endlocal&set RETVAL=%P%&set %RETVAR%=%P%&%RETURN%

:#----------------------------------------------------------------------------#

:# Check that cmd extensions work
:check_exts
%FUNCTION% check %*
verify other 2>nul
setlocal enableextensions enabledelayedexpansion
if errorlevel 1 (
  >&2 echo Error: Unable to enable command extensions.
  endlocal & set "RETVAL=1" & %RETURN%
)
set VAR=before
if "%VAR%" == "before" (
  set VAR=after
  if not "!VAR!" == "after" (
    >&2 echo Error: Delayed environment variable expansion must be enabled.
    >&2 echo Please restart your cmd.exe shell with the /V option,
    >&2 echo or set HKLM\Software\Microsoft\Command Processor\DelayedExpansion=1
    endlocal & set "RETVAL=1" & %RETURN%
  )
)
endlocal & set "RETVAL=0" & %RETURN%

:#----------------------------------------------------------------------------#

:# List subkeys. %1=Parent key pathname.
:keys
:dirs
%FUNCTION% keys %*
setlocal enableextensions enabledelayedexpansion
set "PATTERN=*"
set "OPTS=/k"
:get_keys_args
if "%~1"=="" goto got_keys_args
if "%~1"=="-c" shift & set "OPTS=%OPTS% /c" & goto get_keys_args
if "%~1"=="-f" shift & set "PATTERN=%~1" & shift & goto get_keys_args
set "KEY=%~1" & shift & goto get_keys_args
:got_keys_args
%DEBUGVARS% KEY
set BEFORE=
if "%FULLPATH%"=="1" set "BEFORE=%KEY%\"
:# Use reg.exe to get the key information
set CMD=reg query "%KEY%" /f !PATTERN! !OPTS!
%DEBUGECHO% %CMD%
:# For each line in CMD output...
%FOREACHLINE% %%l in ('%CMD%') do (
  set "LINE=%%l"
  set "HEAD=!LINE:~0,2!"
  if "!HEAD!"=="HK" (
    set "NAME=%%~nxl"
    if "!NAME!"=="(Default)" set "NAME="
    set "NAME=!BEFORE!!NAME!"
    call :CondQuote NAME
    echo !NAME!
  )
)
endlocal & set "RETVAL=0" & %RETURN%

:#----------------------------------------------------------------------------#

:# List values. %1=Parent key pathname.
:values
:files
%FUNCTION% values %*
setlocal enableextensions enabledelayedexpansion
set "DETAILS=0"
set "PATTERN=*"
set "OPTS=/v"
:get_values_args
if "%~1"=="" goto got_values_args
if "%~1"=="-/" shift & set "DETAILS=1" & goto get_values_args
if "%~1"=="-c" shift & set "OPTS=%OPTS% /c" & goto get_values_args
if "%~1"=="-f" shift & set "PATTERN=%~1" & shift & goto get_values_args
set "KEY=%~1" & shift & goto get_values_args
:got_values_args
%DEBUGVARS% KEY
set BEFORE=
if "%FULLPATH%"=="1" set "BEFORE=%KEY%\"
:# Use reg.exe to get the key information
set CMD=reg query "%KEY%" /f !PATTERN! !OPTS!
%DEBUGECHO% %CMD%
:# For each line in CMD output... 
%FOREACHLINE% %%i in ('%CMD%') do (
  set "LINE=%%i"
  %DEBUGVARS% LINE
  :# Values are indented by 4 spaces.
  set "HEAD=!LINE:~0,4!"
  set "LINE=!LINE:~4!"
  :# But extra lines of multi-lined values are indented by >20 spaces.
  set "HEAD2=!LINE:~0,4!"
  if "!HEAD!"=="    " if not "!HEAD2!"=="    " (
    :# Some versions of reg.exe use 4 spaces as field separator; others use a TAB. 
    :# Change the 4-spaces around the REG_XX type word to a TAB.
    set "TOKENS=!LINE:    =	!"
    %DEBUGVARS% TOKENS
    :# Extract the value name as the first item before the first TAB.
    :# Names can contain spaces, but assume they don't contain TABs.
    for /f "tokens=1,2* delims=	" %%j in ("!TOKENS!") do (
      set "NAME=%%j"
      if "!NAME!"=="(Default)" set "NAME="
      set "TYPE=%%k"
      set "VALUE=%%l"
      %DEBUGVARS% NAME TYPE VALUE
      if %DETAILS%==0 (
	set "NAME=!BEFORE!!NAME!"
	call :CondQuote NAME
      	echo !NAME!
      ) else (
      	echo !NAME!/!TYPE!/!VALUE!
      )
    )
  )
)
endlocal & set "RETVAL=0" & %RETURN%

:#----------------------------------------------------------------------------#

:# List subkeys and values. %1=Parent key pathname.
:dir
:list
:ls
%FUNCTION% dir %*
setlocal
set "KEY=%~1" & shift
%DEBUGVARS% KEY
:# Then call subroutines to get subkeys, then values.
%FOREACHLINE% %%K in ('%XCALL% dirs "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9') do echo %%~K\
%FOREACHLINE% %%V in ('%XCALL% files "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9') do echo %%~V
endlocal & set "RETVAL=0" & %RETURN%

:#----------------------------------------------------------------------------#

:# Display a value. %1=Parent key pathname.
:type
:cat
:get
%FUNCTION% type %*
setlocal enableextensions enabledelayedexpansion
set "KEY=%~1"
:# Split the registry path and name. Uses a fake drive @ to prevent prepending the current disk drive path to the registry path.
for %%K in (@:"%KEY%") do (
  set "KEY=%%~dpK"
  set "NAME=%%~nxK"
)
:# Remove the head "@:\ and tail \ to the KEY, and tail " to the NAME
set "KEY=%KEY:~4,-1%"
set "NAME=%NAME:~0,-1%"
%DEBUGVARS% KEY NAME
if "%NAME%"=="" (
  set CMD=reg query "%KEY%" /ve
) else (
  set CMD=reg query "%KEY%" /v "%NAME%"
)
%DEBUGECHO% %CMD%
:# For each line in CMD output...
%FOREACHLINE% %%i in ('%CMD%') do (
  set "LINE=%%i"
  %DEBUGVARS% LINE
  :# Values are indented by 4 spaces.
  set "HEAD=!LINE:~0,4!"
  set "LINE=!LINE:~4!"
  :# But extra lines of multi-lined values are indented by >20 spaces.
  set "HEAD2=!LINE:~0,4!"
  if "!HEAD!"=="    " if not "!HEAD2!"=="    " (
    :# Some versions of reg.exe use 4 spaces as field separator; others use a TAB. 
    :# Change the 4-spaces around the REG_XX type word to a TAB.
    set "TOKENS=!LINE:    =	!"
    %DEBUGVARS% TOKENS
    :# Extract the value name as the first item before the first TAB.
    :# Names can contain spaces, but assume they don't contain TABs.
    for /f "tokens=1,2* delims=	" %%j in ("!TOKENS!") do (
      set "NAME=%%j"
      set "TYPE=%%k"
      set "VALUE=%%l"
      %DEBUGVARS% NAME TYPE VALUE
    )
    set BEFORE=
    if %VERBOSE%==1 set "BEFORE=%KEY%\!NAME! = "
    echo.!BEFORE!!VALUE!
  )
)
endlocal & set "RETVAL=%VALUE%" & %RETURN%

:#----------------------------------------------------------------------------#

:# Non-recursive show. %1=Parent key pathname.
:show
%FUNCTION% show %*
setlocal enableextensions enabledelayedexpansion
set "KEY=%~1" & shift
%DEBUGVARS% KEY
:# Then call subroutines to get subkeys, then values.
%FOREACHLINE% %%K in ('%XCALL% dirs "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9') do (
  set "SUBKEY=%%K\"
  call :CondQuote SUBKEY
  %INDENT_ECHO% !SUBKEY! {}
)
set "ATTRS= "
%FOREACHLINE% %%L in ('%XCALL% files -/ "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9 ^| sort') do (
  rem Gotcha: The name can be empty. So prefix the line with 1 extra character, then remove it from the name.
  for /f "tokens=1,2* delims=/" %%i in ("-%%L") do (
    set "NAME=%%i"
    set "NAME=!NAME:~1!"
    set "TYPE=%%j"
    set "VALUE=%%k"
  )
  %DEBUGVARS% NAME TYPE VALUE
  call :CondQuote NAME
  call :CondQuote VALUE
  if %VERBOSE%==1 SET "ATTRS=%ATTRS%type=!TYPE! "
  %INDENT_ECHO% !NAME!!ATTRS!!VALUE!
)
endlocal & set "RETVAL=0" & %RETURN%

:#----------------------------------------------------------------------------#

:# Recursive show. %1=Parent key pathname.
:tree
%FUNCTION% tree %*
setlocal enableextensions enabledelayedexpansion
set "KEY=%~1" & shift
%DEBUGVARS% KEY
:# Then call subroutines to get subkeys, then values.
%FOREACHLINE% %%K in ('%XCALL% dirs "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9') do (
  set "SUBKEY=%%~K\"
  call :CondQuote SUBKEY
  %INDENT_ECHO% !SUBKEY! {
    if "%DEBUG%"=="0" set "INDENT=%INDENT%  "
    call :tree "%KEY%\%%~K" %1 %2 %3 %4 %5 %6 %7 %8 %9
    if "%DEBUG%"=="0" set "INDENT=%INDENT%"
  %INDENT_ECHO% }
)
set "ATTRS= "
%FOREACHLINE% %%L in ('%XCALL% files -/ "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9 ^| sort') do (
  rem Gotcha: The name can be empty. So prefix the line with 1 extra character, then remove it from the name.
  for /f "tokens=1,2* delims=/" %%i in ("-%%L") do (
    set "NAME=%%i"
    set "NAME=!NAME:~1!"
    set "TYPE=%%j"
    set "VALUE=%%k"
  )
  %DEBUGVARS% NAME TYPE VALUE
  call :CondQuote NAME
  call :CondQuote VALUE
  if %VERBOSE%==1 SET "ATTRS=%ATTRS%type=!TYPE! "
  %INDENT_ECHO% !NAME!!ATTRS!!VALUE!
)
endlocal & set "RETVAL=0" & %RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        main                                                      #
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
echo %SCRIPT% version %VERSION%
echo.
echo Browse the registry "directories" and "files" using Windows' reg.exe
echo.
echo Usage: %ARG0% [options] {command} [command_options ...]
echo.
echo Options:
echo   -?^|-h            Display this help screen and exit
echo   -f               List the full pathname of keys and values
echo   -v               Verbose output
echo   -V               Display this script version and exit
echo.
echo Commands:
echo   dir {path}       List subkeys/ and values names
echo   keys {path}      List subkeys names
echo   show {path}      Display all values contents (1)
echo   tree {path}      Display the whole tree key values contents (1)
echo   type {path\name} Display the key value content 
echo   values {path}    List values names
echo.
echo (1): The unnamed "(Default)" value is displayed as name "".
echo.
goto :eof

:main
set "ACTION="
set "KEY="
set "OPTS="
goto test_args
:next_arg
shift
:test_args
if .%1.==.. goto go
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-c. goto case
if .%1.==.-d. goto debug
if .%1.==.-f. goto fullpath
if .%1.==.-p. goto pattern
if .%1.==.-t. goto test
if .%1.==.-v. goto verbose
if .%1.==.-V. goto Version
if .%1.==.-X. goto SetNoExec
if "%ACTION%"=="" set "ACTION=%1" & goto next_arg
if "%KEY%"=="" set "KEY=%~1" & goto next_arg
2>&1 echo Unexpected argument, ignored: %1
goto next_arg

:case
set "OPTS=%OPTS% -c"
goto next_arg

:debug
call :debug_on
goto next_arg

:fullpath
set FULLPATH=1
goto next_arg

:pattern
shift
set "OPTS=%OPTS% -f %1"
goto next_arg

:SetNoExec
set NOEXEC=1
set VERBOSE=1
goto next_arg

:verbose
set VERBOSE=1
goto next_arg

:Version
echo.%VERSION%
goto :eof

:test
shift
set "VAR=%~1"
%DEBUGVARS% VAR
call :CondQuote VAR
%DEBUGVARS% VAR
goto :eof

:# Execute the requested command
:go
call :check_exts
if not "%RETVAL%"=="0" exit /b 1

call :%ACTION% "%KEY%"!OPTS!

