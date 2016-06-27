@echo off
:##############################################################################
:#                                                                            #
:#  Filename        timex.bat                                                 #
:#                                                                            #
:#  Description     Measure the duration of a task                            #
:#                                                                            #
:#  Notes 	    Inspired by Unix' time program.                           #
:#                                                                            #
:#                  Cannot be named time.bat, because cmd.exe will run its    #
:#                  internal time command, even when explicitly invoked as    #
:#                  time.bat. (Unless time.bat is in the current directory.)  #
:#                                                                            #
:#  History                                                                   #
:#   2014-12-02 JFL Created this script, reusing my Library.bat routines.     #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2014-12-02"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*

set "POPARG=call :PopArg"
call :Debug.Init
call :Exec.Init
goto Main

:PopArg
set "ARG="
for /f "tokens=1,*" %%a in ('echo.%ARGS%') do (
  set ARG=%%a
  if not defined ARG (
    set "ARGS="
  ) else (
    set "ARG=%%~a"
    set ARGS=%%b
  )
)
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Debug routines					      #
:#                                                                            #
:#  Description     A collection of debug routines                            #
:#                                                                            #
:#  Functions       Debug.Init	    Initialize debugging. Call once at first. #
:#                  Debug.Off	    Disable the debugging mode		      #
:#                  Debug.On	    Enable the debugging mode		      #
:#                  Debug.SetLog    Set the log file         		      #
:#                  Debug.Entry	    Log entry into a routine		      #
:#                  Debug.Return    Log exit from a routine		      #
:#                  Verbose.Off	    Disable the verbose mode                  #
:#                  Verbose.On	    Enable the verbose mode		      #
:#                  Echo	    Echo and log strings, indented            #
:#                  EchoVars	    Display the values of a set of variables  #
:#                  EchoArgs	    Display the values of all arguments       #
:#                                                                            #
:#  Macros          %FUNCTION%	    Define and trace the entry in a function. #
:#                  %RETURN%        Return from a function and trace it       #
:#                  %RETVAL%        Default return value                      #
:#                  %RETVAR%        Name of the return value. Default=RETVAL  #
:#                                                                            #
:#                  Always match uses of %FUNCTION% and %RETURN%. That is     #
:#                  never use %RETURN% if there was no %FUNCTION% before it.  #
:#                                                                            #
:#                  :# Example of a factorial routine using this framework    #
:#                  :Fact                                                     #
:#                  %FUNCTION% Fact %*                                        #
:#                  setlocal enableextensions enabledelayedexpansion          #
:#                  set N=%1                                                  #
:#                  if .%N%.==.0. (                                           #
:#                    set RETVAL=1                                            #
:#                  ) else (                                                  #
:#                    set /A M=N-1                                            #
:#                    call :Fact !M!                                          #
:#                    set /A RETVAL=N*RETVAL                                  #
:#                  )                                                         #
:#                  endlocal & set "RETVAL=%RETVAL%" & %RETURN%               #
:#                                                                            #
:#                  %ECHO%	    Echo and log a string, indented           #
:#                  %LOG%	    Log a string, indented                    #
:#                  %ECHO.V%	    Idem, but display it in verbose mode only #
:#                  %ECHO.D%	    Idem, but display it in debug mode only   #
:#                                                                            #
:#                  %ECHOVARS%	    Indent, echo and log variables values     #
:#                  %ECHOVARS.V%    Idem, but display them in verb. mode only #
:#                  %ECHOVARS.D%    Idem, but display them in debug mode only #
:#                                                                            #
:#                  %IF_DEBUG%      Execute a command in debug mode only      #
:#                  %IF_VERBOSE%    Execute a command in verbose mode only    #
:#                                                                            #
:#  Variables       %>DEBUGOUT%     Debug output redirect. Either "" or ">&2".#
:#                  %LOGFILE%       Log file name. Inherited. Default=NUL.    #
:#                  %DEBUG%         Debug mode. 0=Off; 1=On. Use functions    #
:#                                  Debug.Off and Debug.On to change it.      #
:#                                  Inherited. Default=0.                     #
:#                  %VERBOSE%       Verbose mode. 0=Off; 1=On. Use functions  #
:#                                  Verbose.Off and Verbose.On to change it.  #
:#                                  Inherited. Default=0.                     #
:#                  %INDENT%        Spaces to put ahead of all debug output.  #
:#                                  Inherited. Default=. (empty string)       #
:#                                                                            #
:#  Notes           All output from these routines is sent to the log file.   #
:#                  In debug mode, the debug output is also sent to stderr.   #
:#                                                                            #
:#                  Traced functions are indented, based on the call depth.   #
:#                  Use %ECHO% to get the same indentation of normal output.  #
:#                                                                            #
:#                  The output format matches the batch language syntax       #
:#                  exactly. This allows copying the debug output directly    #
:#                  into another command window, to check troublesome code.   #
:#                                                                            #
:#  History                                                                   #
:#   2011-11-15 JFL Split Debug.Init from Debug.Off, to improve clarity.      #
:#   2011-12-12 JFL Output debug information to stderr, so that stdout can be #
:#                  used for returning information from the subroutine.       #
:#   2011-12-13 JFL Standardize use of RETVAR/RETVAL, and display it on return.
:#   2012-07-09 JFL Restructured functions to a more "object-like" style.     #
:#                  Added the three flavors of the Echo and EchoVars routines.#
:#   2012-07-19 JFL Added optimizations to improve performance in non-debug   #
:#                  and non-verbose mode. Added routine Debug.SetLog.         #
:#   2012-11-13 JFL Added macro LOG. Fixed setlocal bug in :EchoVars.         #
:#   2013-08-27 JFL Changed %RETURN% to do exit /b. This allows returning     #
:#                  an errorlevel by doing: %RETURN% %ERRORLEVEL%             #
:#   2013-11-12 JFL Added macros %IF_DEBUG% and %IF_VERBOSE%.                 #
:#   2013-12-04 JFL Added variable %>DEBUGOUT% to allow sending debug output  #
:#                  either to stdout or to stderr.                            #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Debug.Init
goto :Debug.End

:Debug.Init
set "RETVAR=RETVAL"
set "ECHO=call :Echo"
set "ECHOVARS=call :EchoVars"
set ">DEBUGOUT=" &:# ""=output debug messages to stdout; ">&2"=output to stderr
:Debug.Init.2
set "LOG=call :Echo.Log"
if .%LOGFILE%.==.NUL. set "LOG=rem"
set "ECHO.V=call :Echo.Verbose"
set "ECHO.D=call :Echo.Debug"
set "ECHOVARS.V=call :EchoVars.Verbose"
set "ECHOVARS.D=call :EchoVars.Debug"
:# Variables inherited from the caller...
:# Preserve INDENT if it contains just spaces, else clear it.
for /f %%s in ('echo.%INDENT%') do set "INDENT="
:# Preserve the log file name, else by default use NUL.
if .%LOGFILE%.==.. set "LOGFILE=NUL"
:# VERBOSE mode can only be 0 or 1. Default is 0.
if not .%VERBOSE%.==.1. set "VERBOSE=0"
call :Verbose.%VERBOSE%
:# DEBUG mode can only be 0 or 1. Default is 0.
if not .%DEBUG%.==.1. set "DEBUG=0"
goto :Debug.%DEBUG%

:Debug.SetLog
set "LOGFILE=%~1"
goto :Debug.Init.2

:Debug.Off
:Debug.0
set "DEBUG=0"
set "IF_DEBUG=if .%DEBUG%.==.1."
set "FUNCTION=rem"
set "RETURN=exit /b"
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -d=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Optimization to speed things up in non-debug mode
if .%LOGFILE%.==.NUL. set "ECHO.D=rem"
if .%LOGFILE%.==.NUL. set "ECHOVARS.D=rem"
goto :eof

:Debug.On
:Debug.1
set "DEBUG=1"
set "IF_DEBUG=if .%DEBUG%.==.1."
set "FUNCTION=call :Debug.Entry"
set "RETURN=call :Debug.Return & exit /b"
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -d=% -d"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Reverse the above optimization
set "ECHO.D=call :Echo.Debug"
set "ECHOVARS.D=call :EchoVars.Debug"
goto :eof

:Debug.Entry
%>DEBUGOUT% echo %INDENT%call :%*
>>%LOGFILE% echo %INDENT%call :%*
set "INDENT=%INDENT%  "
goto :eof

:Debug.Return
%>DEBUGOUT% echo %INDENT%return !RETVAL!
>>%LOGFILE% echo %INDENT%return !RETVAL!
set "INDENT=%INDENT:~0,-2%"
goto :eof

:# Routine to set the VERBOSE mode, in response to the -v argument.
:Verbose.Off
:Verbose.0
set "VERBOSE=0"
set "IF_VERBOSE=if .%VERBOSE%.==.1."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -v=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Optimization to speed things up in non-verbose mode
if .%LOGFILE%.==.NUL. set "ECHO.V=rem"
if .%LOGFILE%.==.NUL. set "ECHOVARS.V=rem"
goto :eof

:Verbose.On
:Verbose.1
set "VERBOSE=1"
set "IF_VERBOSE=if .%VERBOSE%.==.1."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -v=% -v"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Reverse the above optimization
set "ECHO.V=call :Echo.Verbose"
set "ECHOVARS.V=call :EchoVars.Verbose"
goto :eof

:# Echo and log a string, indented at the same level as the debug output.
:Echo
echo.%INDENT%%*
:Echo.Log
>>%LOGFILE% 2>&1 echo.%INDENT%%*
goto :eof

:Echo.Verbose
%IF_VERBOSE% goto :Echo
goto :Echo.Log

:Echo.Debug
%IF_DEBUG% goto :Echo
goto :Echo.Log

:# Echo and log variable values, indented at the same level as the debug output.
:EchoVars
setlocal EnableExtensions EnableDelayedExpansion
:EchoVars.loop
if "%~1"=="" endlocal & goto :eof
%>DEBUGOUT% echo %INDENT%set "%~1=!%~1!"
>>%LOGFILE% echo %INDENT%set "%~1=!%~1!"
shift
goto EchoVars.loop

:EchoVars.Verbose
%IF_VERBOSE% (
  call :EchoVars %*
) else (
  call :EchoVars %* >NUL 2>NUL
)
goto :eof

:EchoVars.Debug
%IF_DEBUG% (
  call :EchoVars %*
) else (
  call :EchoVars %* >NUL 2>NUL
)
goto :eof

:# Echo a list of arguments.
:EchoArgs
setlocal EnableExtensions DisableDelayedExpansion
set N=0
:EchoArgs.loop
if .%1.==.. endlocal & goto :eof
set /a N=N+1
%>DEBUGOUT% echo %INDENT%set "ARG%N%=%1"
shift
goto EchoArgs.loop

:Debug.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Exec                                                      #
:#                                                                            #
:#  Description     Run a command, logging its output to the log file.        #
:#                                                                            #
:#                  In VERBOSE mode, display the command line first.          #
:#                  In DEBUG mode, display the command line and the exit code.#
:#                  In NOEXEC mode, display the command line, but don't run it.
:#                                                                            #
:#  Arguments       -t          Tee all output to the log file if there's a   #
:#                              usable tee.exe. Default: Redirect all >> log. #
:#                              Known limitation: The exit code is always 0.  #
:#                  %*          The command and its arguments                 #
:#                              Quote redirection operators. Ex:              #
:#                              %EXEC% find /I "error" "<"logfile.txt ">"NUL  #
:#                                                                            #
:#  Functions       Exec.Init	Initialize Exec routines. Call once at 1st    #
:#                  Exec.Off	Disable execution of commands		      #
:#                  Exec.On	Enable execution of commands		      #
:#                  Do          Always execute a command, logging its output  #
:#                  Exec	Conditionally execute a command, logging it.  #
:#                                                                            #
:#  Macros          %DO%        Always execute a command, logging its output  #
:#                  %EXEC%      Conditionally execute a command, logging it.  #
:#                  %ECHO.X%    Echo and log a string, indented, in -X mode.  #
:#                  %ECHO.XVD%  Echo a string, indented, in -X or -V or -D    #
:#                              modes; Log it always.                         #
:#                              Useful to display commands in cases where     #
:#                              %EXEC% can't be used, like in for ('cmd') ... #
:#                  %IF_NOEXEC% Execute a command in NOEXEC mode only         #
:#                                                                            #
:#  Variables       %LOGFILE%	Log file name.                                #
:#                  %NOEXEC%	Exec mode. 0=Execute commands; 1=Don't. Use   #
:#                              functions Exec.Off and Exec.On to change it.  #
:#                              Inherited from the caller. Default=On.	      #
:#                  %NOREDIR%   0=Log command output to the log file; 1=Don't #
:#                              Default: 0                                    #
:#                              Useful in cases where the output must be      #
:#                              shown to the user, and no tee.exe is available.
:#                  %EXEC.ARGS%	Arguments to recursively pass to subcommands  #
:#                              with the same execution options conventions.  #
:#                                                                            #
:#  Notes           This framework can't be used from inside () blocks.       #
:#                  This is because these blocks are executed separately      #
:#                  in a child shell.                                         #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-19 JFL Created this routine.                                     #
:#   2012-05-04 JFL Support logging ">" redirections.                         #
:#   2012-07-09 JFL Restructured functions to a more "object-like" style.     #
:#   2012-07-11 JFL Support logging both "<" and ">" redirections.            #
:#   2012-09-18 JFL Added macro %ECHO.X% for cases where %EXEC% can't be used.#
:#   2012-11-13 JFL Support for "|" pipes too.                                #
:#   2013-11-12 JFL Added macro %IF_NOEXEC%.                                  #
:#   2013-12-04 JFL Added option -t to tee the output if possible.            #
:#                  Split %ECHO.X% and %ECHO.XVD%.                            #
:#   2014-05-13 JFL Call tee.exe explicitely, to avoid problems if there's    #
:#                  also a tee.bat script in the path.                        #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Exec.Init
goto :Exec.End

:# Global variables initialization, to be called first in the main routine
:Exec.Init
set "DO=call :Do"
set "EXEC=call :Exec"
set "ECHO.X=call :Echo.NoExec"
set "ECHO.XVD=call :Echo.XVD"
if .%LOGFILE%.==.. set "LOGFILE=NUL"
if not .%NOEXEC%.==.1. set "NOEXEC=0"
if not .%NOREDIR%.==.1. set "NOREDIR=0"
:# Check if there's a tee.exe program available
set "Exec.HaveTee=0"
tee.exe --help >NUL 2>NUL
if not errorlevel 1 set "Exec.HaveTee=1"
goto :NoExec.%NOEXEC%

:Exec.On
:NoExec.0
set "NOEXEC=0"
set "IF_NOEXEC=if .%NOEXEC%.==.1."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -X=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
goto :eof

:# Routine to set the NOEXEC mode, in response to the -X argument.
:Exec.Off
:NoExec.1
set "NOEXEC=1"
set "IF_NOEXEC=if .%NOEXEC%.==.1."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -X=% -X"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
goto :eof

:Echo.NoExec
%IF_NOEXEC% goto :Echo
goto :eof

:Echo.XVD
%IF_NOEXEC% goto :Echo
%IF_VERBOSE% goto :Echo
%IF_DEBUG% goto :Echo
goto :Echo.Log

:# Execute a command, logging its output.
:# Use for informative commands that should always be run, even in NOEXEC mode. 
:Do
setlocal EnableExtensions DisableDelayedExpansion
set NOEXEC=0
goto :Exec.Start

:# Execute critical operations that should not be run in NOEXEC mode.
:# Version supporting input and output redirections, and pipes.
:# Redirection operators MUST be surrounded by quotes. Ex: "<" or ">" or "2>>"
:Exec
setlocal EnableExtensions DisableDelayedExpansion
:Exec.Start
set "Exec.Redir=>>%LOGFILE%,2>&1"
if .%NOREDIR%.==.1. set "Exec.Redir="
if .%LOGFILE%.==.NUL. set "Exec.Redir="
:# Process optional arguments
set "Exec.GotCmd=Exec.GotCmd"   &:# By default, the command line is %* for :Exec
goto :Exec.GetArgs
:Exec.NextArg
set "Exec.GotCmd=Exec.BuildCmd" &:# An :Exec argument was found, we'll have to rebuild the command line
shift
:Exec.GetArgs
if "%~1"=="-t" ( :# Tee the output to the log file
  :# Warning: This prevents from getting the command exit code!
  if .%Exec.HaveTee%.==.1. set "Exec.Redir= 2>&1 | tee.exe -a %LOGFILE%"
  goto :Exec.NextArg
)
set Exec.Cmd=%*
goto :%Exec.GotCmd%
:Exec.BuildCmd
:# Build the command list. Cannot use %*, which still contains the :Exec switches processed above.
set Exec.Cmd=%1
:Exec.GetCmdLoop
shift
if not .%1.==.. set Exec.Cmd=%Exec.Cmd% %1& goto :Exec.GetCmdLoop
:Exec.GotCmd
:# First stage: Split multi-char ops ">>" "2>" "2>>". Make sure to keep ">" signs quoted every time.
:# Do NOT use surrounding quotes for these set commands, else quoted arguments will break.
set Exec.Cmd=%Exec.Cmd:">>"=">"">"%
set Exec.Cmd=%Exec.Cmd:">>&"=">"">""&"%
set Exec.Cmd=%Exec.Cmd:">&"=">""&"%
:# Second stage: Convert quoted redirection operators (Ex: ">") to a usable (Ex: >) and a displayable (Ex: ^>) value.
:# Must be once for each of the four < > | & operators.
:# Since each operation removes half of ^ escape characters, then insert
:# enough ^ to still protect the previous characters during the subsequent operations.
set Exec.toEcho=%Exec.Cmd:"|"=^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|%
set Exec.toEcho=%Exec.toEcho:"&"=^^^^^^^^^^^^^^^&%
set Exec.toEcho=%Exec.toEcho:">"=^^^^^^^>%
set Exec.toEcho=%Exec.toEcho:"<"=^^^<%
:# Finally create the usable command, by removing the last level of ^ escapes.
set Exec.Cmd=%Exec.toEcho%
set "Exec.Echo=rem"
%IF_NOEXEC% set "Exec.Echo=echo"
%IF_DEBUG% set "Exec.Echo=echo"
%IF_VERBOSE% set "Exec.Echo=echo"
%>DEBUGOUT% %Exec.Echo%.%INDENT%%Exec.toEcho%
>>%LOGFILE% echo.%INDENT%%Exec.toEcho%
:# Constraints at this stage:
:# The command exit code must make it through, back to the caller.
:# The local variables must disappear before return.
:# But the new variables created by the command must make it through.
:# This should work whether :Exec is called with delayed expansion on or off.
endlocal & if not .%NOEXEC%.==.1. (
  %Exec.Cmd%%Exec.Redir%
  call :Exec.ShowExitCode
)
goto :eof

:Exec.ShowExitCode
%IF_DEBUG% %>DEBUGOUT% echo.%INDENT%  exit %ERRORLEVEL%
>>%LOGFILE% echo.%INDENT%  exit %ERRORLEVEL%
exit /b %ERRORLEVEL%

:Exec.End

:#----------------------------------------------------------------------------#
:#                        End of the debugging library                        #
:#----------------------------------------------------------------------------#

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Now                                                       #
:#                                                                            #
:#  Description     Locale-independant routine to parse the current date/time #
:#                                                                            #
:#  Returns         Environment variables YEAR MONTH DAY HOUR MINUTE SECOND MS#
:#                                                                            #
:#  Notes 	    This routine is a pure-batch attempt at parsing the date  #
:#                  and time in a way compatible with any language and locale.#
:#                  Forces the output variables widths to fixed widths,       #
:#		    suitable for use in ISO 8601 date/time format strings.    #
:#                  Note that it would have been much easier to cheat and     #
:#                  do all this by invoking a PowerShell command!             #
:#                                                                            #
:#                  The major difficulty is that the cmd.exe date and time    #
:#                  are localized, and the year/month/day order and separator #
:#                  vary a lot between countries and languages.               #
:#                  Workaround: Use the short date format from the registry   #
:#                  as a template to analyse the date and time strings.       #
:#                  Tested in English, French, German, Spanish, Simplified    #
:#		    Chinese, Japanese.                                        #
:#                                                                            #
:#                  Uses %TIME% and not "TIME /T" because %TIME% gives more:  #
:#                  %TIME% returns [H]H:MM:SS.hh			      #
:#		    "TIME /T" returns MM:SS only.                             #
:#                                                                            #
:#                  Set DEBUG=1 before calling this routine, to display       #
:#                  the values of intermediate results.                       #
:#                                                                            #
:#  History                                                                   #
:#   2012-02-14 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Now
setlocal enableextensions enabledelayedexpansion
:# First get the short date format from the Control Panel data in the registry
for /f "tokens=3" %%a in ('reg query "HKCU\Control Panel\International" /v sShortDate 2^>NUL ^| findstr "REG_SZ"') do set "SDFTOKS=%%a"
if .%DEBUG%.==.1. echo set "SDFTOKS=!SDFTOKS!"
:# Now simplify this (ex: "yyyy/MM/dd") to a "YEAR MONTH DAY" format
for %%a in ("yyyy=y" "yy=y" "y=YEAR" "MMM=M" "MM=M" "M=MONTH" "dd=d" "d=DAY" "/=-" ".=-" "-= ") do set "SDFTOKS=!SDFTOKS:%%~a!"
if .%DEBUG%.==.1. echo set "SDFTOKS=!SDFTOKS!"
:# From the actual order, generate the token parsing instructions
for /f "tokens=1,2,3" %%t in ("!SDFTOKS!") do set "SDFTOKS=set %%t=%%a&set %%u=%%b&set %%v=%%c"
if .%DEBUG%.==.1. echo set "SDFTOKS=!SDFTOKS!"
:# Then get the current date and time. (Try minimizing the risk that they get off by 1 day around midnight!)
set "D=%DATE%" & set "T=%TIME%"
if .%DEBUG%.==.1. echo set "D=%D%" & echo set "T=%T%"
:# Remove the day-of-week that appears in some languages (US English, Chinese...)
for /f %%d in ('for %%a in ^(%D%^) do @^(echo %%a ^| findstr /r [0-9]^)') do set "D=%%d"
if .%DEBUG%.==.1. echo set "D=%D%"
:# Extract the year/month/day components, using the token indexes set in %SDFTOKS%
for /f "tokens=1,2,3 delims=/-." %%a in ("%D%") do (%SDFTOKS%)
:# Make sure the century is specified, and the month and day have 2 digits.
set "YEAR=20!YEAR!"  & set "YEAR=!YEAR:~-4!"
set "MONTH=0!MONTH!" & set "MONTH=!MONTH:~-2!"
set "DAY=0!DAY!"     & set "DAY=!DAY:~-2!"
:# Remove the leading space that appears for time in some cases. (Spanish...)
set "T=%T: =%"
:# Split seconds and milliseconds
for /f "tokens=1,2 delims=,." %%a in ("%T%") do (set "T=%%a" & set "MS=%%b")
if .%DEBUG%.==.1. echo set "T=%T%" & echo set "MS=%MS%"
:# Split hours, minutes and seconds. Make sure they all have 2 digits.
for /f "tokens=1,2,3 delims=:" %%a in ("%T%") do (
  set "HOUR=0%%a"   & set "HOUR=!HOUR:~-2!"
  set "MINUTE=0%%b" & set "MINUTE=!MINUTE:~-2!"
  set "SECOND=0%%c" & set "SECOND=!SECOND:~-2!"
  set "MS=!MS!000"  & set "MS=!MS:~0,3!"
)
if .%DEBUG%.==.1. echo set "YEAR=%YEAR%" ^& set "MONTH=%MONTH%" ^& set "DAY=%DAY%" ^& set "HOUR=%HOUR%" ^& set "MINUTE=%MINUTE%" ^& set "SECOND=%SECOND%" ^& set "MS=%MS%"
endlocal & set "YEAR=%YEAR%" & set "MONTH=%MONTH%" & set "DAY=%DAY%" & set "HOUR=%HOUR%" & set "MINUTE=%MINUTE%" & set "SECOND=%SECOND%" & set "MS=%MS%" & goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Time.Delta                                                #
:#                                                                            #
:#  Description     Compute the difference between two times                  #
:#                                                                            #
:#  Returns         Environment variables DC DH DM DS DMS                     #
:#                  for carry, hours, minutes, seconds, milliseconds          #
:#                                                                            #
:#  Notes 	    Carry == 0, or -1 if the time flipped over midnight.      #
:#                                                                            #
:#  History                                                                   #
:#   2012-10-08 JFL Created this routine.                                     #
:#   2012-10-12 JFL Renamed variables. Added support for milliseconds.        #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Time.Delta %1=T0 %2=T1 [%3=-f]. Input times in HH:MM:SS[.mmm] format.
setlocal enableextensions enabledelayedexpansion
for /f "tokens=1,2,3,4 delims=:." %%a in ("%~1") do set "H0=%%a" & set "M0=%%b" & set "S0=%%c" & set "MS0=%%d000" & set "MS0=!MS0:~0,3!"
for /f "tokens=1,2,3,4 delims=:." %%a in ("%~2") do set "H1=%%a" & set "M1=%%b" & set "S1=%%c" & set "MS1=%%d000" & set "MS1=!MS1:~0,3!"
:# Remove the initial 0, to avoid having numbers interpreted in octal afterwards. (MS may have 2 leading 0s!)
for %%n in (0 1) do for %%c in (H M S MS MS) do if "!%%c%%n:~0,1!"=="0" set "%%c%%n=!%%c%%n:~1!"
:# Compute differences
for %%c in (H M S MS) do set /a "D%%c=%%c1-%%c0"
set "DC=0" & :# Carry  
:# Report carries if needed
if "%DMS:~0,1%"=="-" set /a "DMS=DMS+1000" & set /a "DS=DS-1"
if "%DS:~0,1%"=="-" set /a "DS=DS+60" & set /a "DM=DM-1"
if "%DM:~0,1%"=="-" set /a "DM=DM+60" & set /a "DH=DH-1"
if "%DH:~0,1%"=="-" set /a "DH=DH+24" & set /a "DC=DC-1"
:# If requested, convert the results back to a 2-digit format.
if "%~3"=="-f" for %%c in (H M S MS) do if "!D%%c:~1!"=="" set "D%%c=0!D%%c!"
if "!DMS:~2!"=="" set "DMS=0!DMS!"
endlocal & set "DC=%DC%" & set "DH=%DH%" & set "DM=%DM%" & set "DS=%DS%" & set "DMS=%DMS%" & goto :eof

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

:Help
echo.
echo %SCRIPT% version %VERSION% - Time the execution of a program
echo.
echo Usage: %SCRIPT% [OPTIONS] [PROGRAM [ARGUMENTS]]
echo.
echo Options:
echo   -?       Display this help
echo   --       End of options
echo   -n N     Run the commands N times and display the start and end times
echo   -t       Get the current time, with seconds resolution
echo   -v       Verbose mode. Display commands executed
echo   -V       Display the script version and exit
echo   -X       Display commands to execute, but don't execute them
echo.
goto :eof

:#----------------------------------------------------------------------------#
:# Main routine

:Main
set "NLOOPS=1"

:next_arg
%POPARG%
if .!ARG!.==.. goto :SetTime
if .!ARG!.==.--. goto :Start
if .!ARG!.==.-?. goto :Help
if .!ARG!.==./?. goto :Help
if .!ARG!.==.-d. call :Debug.On & goto next_arg
if .!ARG!.==.-n. %POPARG% & set "NLOOPS=!ARG!" & goto next_arg
if .!ARG!.==.-t. goto :GetTime
if .!ARG!.==./t. goto :GetTime
if .!ARG!.==./T. goto :GetTime
if .!ARG!.==.-v. call :Verbose.On & goto next_arg
if .!ARG!.==.-V. echo.%VERSION%& goto :eof
if .!ARG!.==.-X. call :Exec.Off & goto next_arg
if "!ARG:~0,1!"=="-" (
  >&2 %ECHO% Warning: Unexpected argument ignored: !ARG!
  goto :next_arg
)
set "ARGS=!ARG! !ARGS!"
goto Start

:GetTime
call :Now
set "NOW=!HOUR!:!MINUTE!:!SECOND!"
%IF_VERBOSE% set "NOW=!NOW!.!MS!"
echo !NOW!
goto :eof

:SetTime
time
goto :eof

:#----------------------------------------------------------------------------#
:# Start the real work

:Start
call :Now
set STARTED=%HOUR%:%MINUTE%:%SECOND%.%MS%
%ECHO.V% Starting at %STARTED%

for /l %%n in (1,1,%NLOOPS%) do (
  %EXEC% %ARGS%
)

call :Now
set ENDED=%HOUR%:%MINUTE%:%SECOND%.%MS%
%ECHO.V% Ended at %ENDED%

call :Time.Delta %STARTED% %ENDED% -f
%ECHO% Duration: %DH%:%DM%:%DS%.%DMS%

