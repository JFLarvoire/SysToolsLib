@echo off
:##############################################################################
:#                                                                            #
:#  Filename:       umountw.bat                                               #
:#                                                                            #
:#  Description:    Unmount a .wim Windows boot Image using a Unix-like cmd.  #
:#                                                                            #
:#  Notes:	    Requires Windows AIK's dism.exe.                          #
:#		    Install the Windows AIK if it's missing.                  #
:#                                                                            #
:#  History:                                                                  #
:#   2011-03-30 JFL jf.larvoire@hpe.com created this script.                  #
:#   2011-04-14 JFL Added the optional mountpoint argument.		      #
:#   2011-06-30 JFL Added options -v -X.                                      #
:#                  Choose default values intelligently.                      #
:#   2011-07-12 JFL Fixed a bug with the options -c and -d.                   #
:#                  Added option -V to display the script version.            #
:#   2012-02-01 JFL Make sure the script returns the dism command exit code.  #
:#                  Changed the verbosity default back to VERBOSE=0.	      #
:#                  Display a message telling which image is unmounted.       #
:#                  Added option -q to avoid that message.		      #
:#                  Display a notice and stop if there's no mounted image.    #
:#   2013-11-12 JFL Use my latest debugging framework.                        #
:#                  Fixed bug in detection of mount RW/RO mode.               #
:#   2018-06-14 JFL Fixed bug with default /commit or /discard.		      #
:#                  Fixed bug if the mount point is relative or has no drive. #
:#                                                                            #
:#         © Copyright 2018 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal enableextensions enabledelayedexpansion
set "VERSION=2018-06-14"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"

call :Debug.Init
call :Exec.Init
goto Main

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
:#  Variables       %LOGFILE%       Log file name. Inherited. Default=NUL.    #
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
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Debug.Init
goto :Debug.End

:Debug.Init
set "RETVAR=RETVAL"
set "ECHO=call :Echo"
set "ECHOVARS=call :EchoVars"
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
>&2         echo %INDENT%call :%*
>>%LOGFILE% echo %INDENT%call :%*
set "INDENT=%INDENT%  "
goto :eof

:Debug.Return
>&2         echo %INDENT%return !RETVAL!
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
>&2         echo %INDENT%set "%~1=!%~1!"
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
>&2 echo %INDENT%set "ARG%N%=%1"
shift
goto EchoArgs.loop

:Debug.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Exec                                                      #
:#                                                                            #
:#  Description     Run a command, logging its output to the log file.        #
:#                                                                            #
:#  Arguments       %*          The command and its arguments                 #
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
:#                  %ECHO.X%	Echo and log a string, indented, in -X mode   #
:#                  %IF_NOEXEC% Execute a command in NOEXEC mode only         #
:#                                                                            #
:#  Variables       %LOGFILE%	Log file name.                                #
:#                  %NOEXEC%	Exec mode. 0=Execute commands; 1=Don't. Use   #
:#                              functions Exec.Off and Exec.On to change it.  #
:#                              Inherited from the caller. Default=On.	      #
:#                  %NOREDIR%   0=Log command output to the log file; 1=Don't #
:#                              Default: 0                                    #
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
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Exec.Init
goto :Exec.End

:# Global variables initialization, to be called first in the main routine
:Exec.Init
set "DO=call :Do"
set "EXEC=call :Exec"
set "ECHO.X=call :Echo.NoExec"
if .%LOGFILE%.==.. set "LOGFILE=NUL"
if not .%NOEXEC%.==.1. set "NOEXEC=0"
if not .%NOREDIR%.==.1. set "NOREDIR=0"
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
:# First stage: Split multi-char ops ">>" "2>" "2>>". Make sure to keep ">" signs quoted every time.
:# Do NOT use surrounding quotes for these set commands, else quoted arguments will break.
set Exec.Cmd=%*
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
>&2 %Exec.Echo%.%INDENT%%Exec.toEcho%
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
%IF_DEBUG% >&2 echo.%INDENT%  exit %ERRORLEVEL%
>>%LOGFILE% echo.%INDENT%  exit %ERRORLEVEL%
exit /b %ERRORLEVEL%

:Exec.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        enum_mwi                                                  #
:#                                                                            #
:#  Description     Enumerate mounted wim images                              #
:#                                                                            #
:#  Arguments       None                                                      #
:#                                                                            #
:#  Returns         Environment variables with information about mounted imgs:#
:#                  LAST_MWI	Index of the last mounted image. -1=None.     #
:#                  MWI#.DIR	Mount directory                               #
:#                  MWI#.IMG	Image file                                    #
:#                  MWI#.IX	Image index in the image file                 #
:#                  MWI#.MODE	RW=Read/Write RO=Read-only                    #
:#                  MWI#.STAT	Status                                        #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2011-03-30 JFL Created this routine.                                     #
:#   2013-11-12 JFL Fix trimming to remove only spaces if present.            #
:#                                                                            #
:#----------------------------------------------------------------------------#

:enum_mwi
%FUNCTION% :enum_mwi %*
set LAST_MWI=-1
for /f "tokens=1* delims=:" %%a in ('dism /Get-MountedWimInfo') do (
  set NAME=%%a
  set VALUE=%%b
  :# Trim the tail space after the name, and the head space ahead of the value
  if "!NAME:~-1!"==" " set NAME=!NAME:~0,-1!
  if "!VALUE:~0,1!"==" " set VALUE=!VALUE:~1!
  %ECHO.D% :# Processing: !NAME!=!VALUE!
  if "!NAME!"=="Mount Dir" (
    set /a LAST_MWI=LAST_MWI+1
    set "MWI!LAST_MWI!.DIR=!VALUE!"
  )
  if "!NAME!"=="Image File" set MWI!LAST_MWI!.IMG=!VALUE!
  if "!NAME!"=="Image Index" set "MWI!LAST_MWI!.IX=!VALUE!"
  if "!NAME!"=="Mounted Read/Write" (
    if .!VALUE!.==.Yes. set VALUE=RW
    if .!VALUE!.==.No. set VALUE=RO
    set "MWI!LAST_MWI!.MODE=!VALUE!"
  )
  if "!NAME!"=="Status" set "MWI!LAST_MWI!.STAT=!VALUE!"
)
%ECHOVARS.D% MWI!LAST_MWI!.DIR MWI!LAST_MWI!.IMG MWI!LAST_MWI!.IX MWI!LAST_MWI!.MODE MWI!LAST_MWI!.STAT
%RETURN%

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
echo.
echo %SCRIPT% version %VERSION% - Unmount a .wim Windows boot Image
echo.
echo Usage: %SCRIPT% [OPTIONS] [MOUNTPOINT]
echo.
echo Options:
echo   -c    Commit changes (Default for images mounted read/write)
echo   -d    Discard changes (Default for images mounted read-only)
echo   -v    Verbose mode. Display the dism command and its output
echo   -V    Display the script version and exit
echo   -X    Generate the dism command, but don't execute it
echo.
echo Mountpoint: The image mount point. Default: The first image mount point
echo.
echo Requires Windows AIK's dism.exe. Install the Windows AIK if it's missing.
goto :eof

:main
set "/COMMIT="
set "MNTPT="
set "DISMOPT= /Quiet"

goto test_args
:next_arg
shift
:test_args
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-c. set "/COMMIT=/commit" & goto next_arg
if .%1.==.-d. set "/COMMIT=/discard" & goto next_arg
if .%1.==.--debug. call :Debug.On & goto next_arg
if .%1.==.-q. call :Verbose.Off & goto next_arg
if .%1.==.-v. call :Verbose.On & goto next_arg
if .%1.==.-V. echo.%VERSION%& goto :eof
if .%1.==.-X. call :Exec.Off & goto next_arg
:# Process optional positional arguments
if not .%1.==.. if not defined MNTPT for %%p in (%1) do set "MNTPT=%%~fp" & goto next_arg

:# Enumerate current mounts
call :enum_mwi

:# Default mount point: The mount point for the first mounted image
if "%MNTPT%"=="" set "MNTPT=%MWI0.DIR%"
if "%MNTPT%"=="" (
  %ECHO% Notice: There is no image mounted
  exit /b 0
)

:# Default changes saving: Based on the image mount mode (RO or RW)
if .%/COMMIT%.==.. (
  for /l %%n in (0,1,%LAST_MWI%) do (
    set N=%%n
    call set DIR=%%MWI!N!.DIR%%
    call set MODE=%%MWI!N!.MODE%%
    if "!DIR!"=="%MNTPT%" (
      if "!MODE!"=="RW" (
      	set "/COMMIT=/commit"
      ) else (
      	set "/COMMIT=/discard"
      )
    )
    %ECHOVARS.D% N DIR MODE COMMIT
  )
)

:# Unmount a .wim image
:# In verbose mode, remove the dism /Quiet option
%IF_VERBOSE% set "DISMOPT=%DISMOPT: /Quiet=%"
%ECHO% Unmounting image from %MNTPT%
%EXEC% dism%DISMOPT% /Unmount-Wim /MountDir:"%MNTPT%" %/COMMIT%

