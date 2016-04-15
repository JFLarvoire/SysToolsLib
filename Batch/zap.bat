@echo off
:##############################################################################
:#                                                                            #
:#  Filename:       zap.bat                                                   #
:#                                                                            #
:#  Description:    Delete files or directories, with style                   #
:#                                                                            #
:#  Notes:	    Remarks begin with :# instead of rem, to avoid having     #
:#		    them displayed while single-stepping though the batch.    #
:#		    This also makes the batch more readable.                  #
:#									      #
:#		    Known bug: Files containing ! or % characters may be      #
:#		    processed incorrectly.				      #
:#									      #
:#  History:                                                                  #
:#   2010-03-10 JFL Created this file.                                        #
:#   2010-03-31 JFL Added option -r.                                          #
:#   2012-01-17 JFL Make sure to use the redo.exe instance in the path.       #
:#                  (Avoids issues when zapping redo.exe in its build dir!)   #
:#                  Give up using DOS-compatible subroutine calls, since      #
:#                  restoring this batch compatibility with DOS is hopeless.  #
:#   2013-08-26 JFL Added my debugging framework.			      #
:#		    Allow deleting directories, while displaying all files    #
:#                  deleted in the process.                                   #
:#                  Rewrote as pure batch, to avoid dependancy on redo.exe    #
:#                  and remplace.exe.					      #
:#   2013-08-30 JFL Force deleting a directory, even if we missed some files  #
:#                  in it, like those named with a ! or % character.          #
:#                  Fixed a bug when deleting subdirs with spaces.            #
:#                                                                            #
:##############################################################################

setlocal enableextensions enabledelayedexpansion

set "VERSION=2013-08-30"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*

call :Debug.Init
goto main

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
if .%VERBOSE%.==.1. goto :Echo
goto :Echo.Log

:Echo.Debug
if .%DEBUG%.==.1. goto :Echo
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
if .%VERBOSE%.==.1. (
  call :EchoVars %*
) else (
  call :EchoVars %* >NUL 2>NUL
)
goto :eof

:EchoVars.Debug
if .%DEBUG%.==.1. (
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
:#  Function        strlen						      #
:#                                                                            #
:#  Description     Measure the length of a string                            #
:#                                                                            #
:#  Arguments       %1	    String variable name                              #
:#                  %2	    Ouput variable name                               #
:#                                                                            #
:#  Notes 	    Inspired from C string management routines                #
:#                                                                            #
:#                  Many thanks to 'sowgtsoi', but also 'jeb' and 'amel27'    #
:#		    dostips forum users helped making this short and efficient#
:#  History                                                                   #
:#   2008-11-22     Created on dostips.com.                                   #
:#   2010-11-16     Changed.                                                  #
:#   2012-10-08 JFL Adapted to my %FUNCTION% library.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:strlen string len                -- returns the length of a string
%FUNCTION% strlen %1 %2
setlocal enabledelayedexpansion
set "RETVAR=%~2"
if "%RETVAR%"=="" set "RETVAR=RETVAL"
set "str=A!%~1!" &:# keep the A up front to ensure we get the length and not the upper bound
		  :# it also avoids trouble in case of empty string
set "len=0"
for /L %%A in (12,-1,0) do (
    set /a "len|=1<<%%A"
    for %%B in (!len!) do if "!str:~%%B,1!"=="" set /a "len&=~1<<%%A"
)
endlocal & set "%RETVAR%=%len%" & set "RETVAL=%len%" & %RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        basename						      #
:#                                                                            #
:#  Description     Get the file name part of a pathname                      #
:#                                                                            #
:#  Arguments       %1	    Input pathname variable name                      #
:#                  %2	    Ouput file name variable name                     #
:#                                                                            #
:#  Notes 	    Inspired from Unix' basename command                      #
:#                                                                            #
:#                  Works even when the base name contains wild cards,        #
:#                  which prevents using commands such as                     #
:#                  for %%f in (%ARG%) do set NAME=%%~nxf                     #
:#                                                                            #
:#  History                                                                   #
:#   2013-08-27 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:basename pathnameVar filenameVar :# Returns the file name part of the pathname
%FUNCTION% basename %*
setlocal enabledelayedexpansion
set "RETVAR=%~2"
if "%RETVAR%"=="" set "RETVAR=RETVAL"
set "NAME=!%~1!"
:basename.trim_dir
set "NAME=%NAME:*\=%"
if not "%NAME%"=="%NAME:\=%" goto :basename.trim_dir
endlocal & set "%RETVAR%=%NAME%" & set "RETVAL=%NAME%" & %RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        is_dir						      #
:#                                                                            #
:#  Description     Check if a pathname refers to an existing directory       #
:#                                                                            #
:#  Arguments       %1	    pathname                                          #
:#                                                                            #
:#  Notes 	    Returns errorlevel 0 if it's a valid directory.           #
:#                                                                            #
:#  History                                                                   #
:#   2013-08-27 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:is_dir pathname       -- Check if a pathname refers to an existing directory
%FUNCTION% is_dir %*
setlocal
pushd "%~1" 2>NUL
if errorlevel 1 (
  set "ERROR=1"
) else (
  set "ERROR=0"
  popd
)
endlocal & set "RETVAL=%ERROR%" & %RETURN% %ERROR%

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

:help
echo.
echo Delete files or directories, with style
echo.
echo Usage: %SCRIPT% [options] {pathname} [pathname2 ...]
echo.
echo Options:
echo   -?^|-h         Display this help screen.
echo   -p {prefix}   Prefix string to insert ahead of output file names
echo   -r            Delete files recursively.
echo   -V            Display the script version and exit
echo   -X            No exec, just display the command generated.
echo.
echo List the files actually deleted.
echo Don't complain if a requested file does not exist.
echo.
echo Known bug: Pathnames containing ^^! or %% characters may be processed incorrectly.
goto :eof

:main

:# Default definitions
set "RECUR=0"
set "NOEXEC=0"
set "FLAGS="
set "PREFIX="

goto tst_args

:next_arg
shift
:tst_args
if .%1.==.. goto help
if .%1.==./?. goto help
if %1==-? goto help
if %1==-d call :Debug.On & set "FLAGS=%FLAGS% %~1" & goto next_arg
if %1==-h goto help
if %1==-p set "PREFIX=%~2" & shift & goto next_arg
if %1==-r set "RECUR=1" & goto next_arg
if %1==-V echo.%VERSION%& goto :eof
if %1==-X set "NOEXEC=1" & set "FLAGS=%FLAGS% %~1" & goto next_arg
goto go

:# Delete files. %1=Target Pathname. Wild cards allowed.
:zapFiles
%FUNCTION% zapFiles %*
setlocal enableextensions enabledelayedexpansion
:# Exit silently if the file is already deleted.
if not exist %1 endlocal & set "RETVAL=" & %RETURN%
for %%f in (%1) do (
  :# Build the command to delete each file
  set CMD=del "%%~f"
  %ECHO.D% !CMD!
  :# echo the name of the file that will be deleted
  echo %PREFIX%%%~f
  :# But skip the deletion in no-exec mode
  if %NOEXEC%==0 !CMD!
)
endlocal & set "RETVAL=" & %RETURN%

:# Delete files recursively. %1=Target Pathname. %2=File names. Optional. Wild cards allowed.
:zapRecur
%FUNCTION% zapRecur %*
setlocal enableextensions enabledelayedexpansion
set "DIR=%~1"
set "FILES=%~2"
if "%FILES%"=="" (
  :# Then extract the FILES part from the DIR pathname
  call :basename DIR FILES
  :# Trim the filename off the DIR pathname
  call :strlen FILES L
  set /a L=L+1
  for %%l in (!L!) do set "DIR=!DIR:~0,-%%l!"
)
%ECHOVARS.D% DIR FILES
:# Exit silently if the directory does not exist
:# Note: Making the test with the unquoted short name, because NUL is _NOT_ present in NTFS name spaces like it is in FAT 8.3 name spaces.
for %%d in ("%DIR%") do if not exist %%~sd\NUL endlocal & set "RETVAL=" & %RETURN%
:# Else delete recursively all the files contained in the target directory
set "FILES2ZAP=%FILES%"
if not "%DIR%"=="" set "FILES2ZAP=%DIR%\%FILES%"
call :zapFiles "%FILES2ZAP%"
set "SUBDIRS=*"
if not "%DIR%"=="" set "SUBDIRS=%DIR%\*"
for /d %%d in ("%SUBDIRS%") do call :zapRecur "%%~d" "%FILES%"
endlocal & set "RETVAL=" & %RETURN%

# Delete one directory. %1=Target Pathname. Wild cards NOT allowed.
:zapDir
%FUNCTION% zapDir %*
setlocal enableextensions enabledelayedexpansion
:# Exit silently if the directory is already deleted.
call :is_dir "%~1"
if errorlevel 1 endlocal & set "RETVAL=" & %RETURN%
:# Else delete recursively all the files contained in the target directory
for /d %%d in ("%~1\*") do call :zapDir "%%~d"
call :zapFiles "%~1\*.*"
:# And finally delete the target directory itself
:# Note: Use /S /Q in case this script missed a few files, like those named with ! or % characters.
set CMD=rd /S /Q %1
%ECHO.D% %CMD%
:# Echo the name of the directory that will be deleted
echo %~1\
:# But skip the deletion in no-exec mode. 
if %NOEXEC%==0 %CMD%
endlocal & set "RETVAL=" & %RETURN%

:next_path
shift
:go
set "ARG=%~1"
%ECHOVARS.D% ARG
if "%ARG%"=="" goto :eof
:# Check if the target path contains wild cards
set WILDCARD=0
if not "%ARG%"=="%ARG:**=%" set WILDCARD=1
%ECHOVARS.D% WILDCARD
:# First check if it's a directory name. In this case, delete the whole tree.
if %WILDCARD%==0 (
  call :is_dir "%~1"
  if not errorlevel 1 (
    call :zapDir "%~1"
    goto next_path
  )
)
:# If we were asked for a recursive action, do it recursively.
if %RECUR%==1 (
  call :zapRecur "%~1"
  goto next_path
)
:# Else just delete files in the target (default: current) directory
call :zapFiles "%~1"
goto next_path

