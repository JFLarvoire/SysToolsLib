@echo off
:##############################################################################
:#                                                                            #
:#  Filename:       mountw.bat                                                #
:#                                                                            #
:#  Description:    Mount a .wim Windows boot Image using a Unix-like command.#
:#                                                                            #
:#  Notes:	    Requires Windows AIK's dism.exe.                          #
:#		    Install the Windows AIK if it's missing.                  #
:#                                                                            #
:#  History:                                                                  #
:#   2011-03-30 JFL jf.larvoire@hpe.com created this script.                  #
:#   2011-04-14 JFL Added the optional mountpoint argument.		      #
:#   2011-06-30 JFL Added options -i -l -L -q -r -v -X.                       #
:#                  Choose default values intelligently.                      #
:#   2012-02-01 JFL Fixed a bug if variable IMAGE was already defined.        #
:#                  Make sure the script returns the dism command exit code.  #
:#                  Changed the verbosity default back to VERBOSE=0.	      #
:#                  Added options -d -V.				      #
:#   2012-06-27 JFL Fixed another bug if variable IMAGE contained spaces.     #
:#   2013-09-20 JFL Added option -c to cleanup corrupted images.              #
:#   2014-07-30 JFL Make sure the mount point exists.			      #
:#   2016-12-01 JFL Fixed debug messages in routine :enum_mwi.		      #
:#                                                                            #
:#         © Copyright 2018 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal enableextensions enabledelayedexpansion
set "VERSION=2016-12-01"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"
set  ARGS=%*

:# Mechanism for calling subroutines in a second external script instance.
:# Done by %XCALL% :label [arguments]
if '%1'=='-call' !ARGS:~1! & exit /b
set XCALL=call "%ARG0%" -call
set XEXEC=%XCALL% :Exec

if .%DEBUG%.==.1. (
  call :debug_on
) else (
  call :debug_off
)
set "PUTVARS=call :putvars"
set "EXEC=call :Exec"
goto main

:debug_on
set "DEBUG=1"
set "DEBUGVARS=call :putvars"
goto :eof

:debug_off
set "DEBUG=0"
set "DEBUGVARS=rem"
goto :eof

:putvars
if .%~1.==.. goto :eof
>&2 echo %INDENT%set "%~1=!%~1!"
shift
goto putvars

:# Execute critical operations that should not be run in NOEXEC mode.
:Exec
set "EXEC.ECHO=rem"
if .%VERBOSE%.==.1. set "EXEC.ECHO=>&2 echo"
if .%DEBUG%.==.1.   set "EXEC.ECHO=>&2 echo"
if .%NOEXEC%.==.1.  set "EXEC.ECHO=>&2 echo"
%EXEC.ECHO% %INDENT%%*
if not .%NOEXEC%.==.1. %*
exit /b

:# Enumerate mounted wim images, and record information about each of them
:# Returns:
:#    LAST_MWI	Index of the last mounted image. -1=None.
:#    MWI#.DIR	Mount directory
:#    MWI#.IMG	Image file
:#    MWI#.IX	Image index in the image file
:#    MWI#.MODE	RW=Read/Write RO=Read-only
:#    MWI#.STAT	Status
:enum_mwi
set LAST_MWI=-1
for /f "tokens=1* delims=:" %%a in ('%XEXEC% dism /Get-MountedWimInfo') do (
  set NAME=%%a
  set VALUE=%%b
  :# Trim the tail space after the name, and the head space ahead of the value
  if "!NAME:~-1!!NAME:~-1!"=="  " set NAME=!NAME:~0,-1!
  if defined VALUE if "!VALUE:~0,1!!VALUE:~0,1!"=="  " set VALUE=!VALUE:~1!
  if .%DEBUG%.==.1. (
    if defined VALUE (
      echo !NAME!=!VALUE!
    ) else (
      echo !NAME!
    )
  )
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
goto :eof

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
echo %SCRIPT% version %VERSION% - Manage .wim Windows boot Images
echo.
echo Usage: mount [OPTIONS] [IMAGE.WIM [MOUNTPOINT]]
echo.
echo Options:
echo   -?    Display this help
echo   -c    Cleanup corrupted images that can't be unmounted
echo   -i    Get image information
echo   -l    List mounted images (Default if no .wim image is specified)
echo   -L    List mounted images, showing the raw dism output
echo   -q    Quiet mode. Display only errors
echo   -r    Mount the image as read-only
echo   -v    Verbose mode. Display the dism command and its output
echo   -V    Display the script version and exit
echo   -X    Generate the dism command, but don't execute it
echo.
echo Default mount point: %MNTPT0%
echo.
echo Requires Windows AIK's dism.exe. Install the Windows AIK if it's missing.
echo.
echo Author: jf.larvoire@hpe.com
goto :eof

:main
if not .%NOEXEC%.==.1. set NOEXEC=0
if not .%VERBOSE%.==.1. set VERBOSE=0
if not .%QUIET%.==.1. set QUIET=0
set "CMD=mount"
set "MNTPT0=C:\mnt\wim"
set "MNTPT="
set "DISMOPT= /Quiet"
set "MNTOPT="
set "IMAGE="

goto test_args
:next_arg
shift
:test_args
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-c. set "CMD=cleanup" & goto next_arg
if .%1.==.-d. call :debug_on & goto next_arg
if .%1.==.-i. set "CMD=info" & goto next_arg
if .%1.==.-l. set "CMD=list" & goto next_arg
if .%1.==.-L. set "CMD=list0" & goto next_arg
if .%1.==.-q. set "VERBOSE=0" & set "QUIET=1" & goto next_arg
if .%1.==.-r. set "MNTOPT=%MNTOPT% /ReadOnly" & goto next_arg
if .%1.==.-v. set "VERBOSE=1" & set "QUIET=0" & goto next_arg
if .%1.==.-V. echo.%VERSION%& goto :eof
if .%1.==.-X. set "NOEXEC=1" & set "VERBOSE=1" & goto next_arg
:# Process optional positional arguments
if not .%1.==.. if "%IMAGE%"=="" set "IMAGE=%~1" & goto next_arg
if not .%1.==.. if "%MNTPT%"=="" set "MNTPT=%~1" & goto next_arg

%DEBUGVARS% VERBOSE NOEXEC CMD MNTPT0 MNTPT DISMOPT MNTOPT IMAGE
:# Set mode-specific variables
set "ERROR_ECHO=>&2 echo"
if .%VERBOSE%.==.1. (
  set "IF_VERBOSE="
  set "VERBOSE_ECHO=echo"
) else (
  set "IF_VERBOSE=rem"
  set "VERBOSE_ECHO=rem"
)
if .%QUIET%.==.1. (
  set "ECHO=rem"
) else (
  set "ECHO=echo"
)

:# Run the requested command
goto %CMD%

:# List mounted images
:list
if .%NOEXEC%.==.1. goto list0
call :enum_mwi
echo Mount point	Image file					Index	Access	Status
for /l %%n in (0,1,%LAST_MWI%) do (
  set N=%%n
  call echo %%MWI!N!.DIR%%	%%MWI!N!.IMG%%	%%MWI!N!.IX%%	%%MWI!N!.MODE%%	%%MWI!N!.STAT%%
)
goto :eof

:# List mounted images, showing the raw dism output
:list0
%EXEC% dism /Get-MountedWimInfo
goto :eof

:# Get information about a .wim image
:info
%EXEC% dism /Get-WimInfo /WimFile:"%IMAGE%"
goto :eof

:# Mount a .wim image
:mount
if "%IMAGE%"=="" goto list
if .%MNTPT%.==.. (
  call :enum_mwi
  if "!LAST_MWI!"=="-1" (
    :# There's no previous mount point. Use the default.
    set "MNTPT=%MNTPT0%"
  ) else (
    :# Else there's at least one mounted wim. If there's 1, last==0 ==> N=2; etc.
    set /a N=!LAST_MWI!+2
    set "MNTPT=%MNTPT0%!N!"
  )
  :# Make sure the mount point exists
  if not exist "!MNTPT!" md "!MNTPT!"
)
:# In verbose mode, remove the dism /Quiet option
%IF_VERBOSE% set "DISMOPT=%DISMOPT: /Quiet=%"
%ECHO% Mounting "%IMAGE%" at %MNTPT%
%EXEC% dism%DISMOPT% /Mount-Wim%MNTOPT% /WimFile:"%IMAGE%" /index:1 /MountDir:"%MNTPT%"
goto :eof

:# Cleanup corrupted images
:cleanup
%EXEC% dism /Cleanup-Wim
goto :eof

