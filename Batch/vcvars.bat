@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    vcvars.bat						      *
:#                                                                            *
:#  Description:    Run vcvarsall.bat for the latest Visual C++ installed     *
:#                                                                            *
:#  Usage:	    Run this script with option -? to display a help screen.  *
:#                                                                            *
:#  Notes:								      *
:#									      *
:#  History:                                                                  *
:#   2011-12-30 JFL Created this script, based on my earlier make.bat.	      *
:#   2017-11-24 JFL Added paths for Visual Studio up to 2017.		      *
:#                  Fixed the display of variables set by the script.         *
:#   2018-01-10 JFL Added option -X.                           		      *
:#   2019-05-09 JFL Added support for Visual Studio 2019.		      *
:#                                                                            *
:#        © Copyright 2018 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license: www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal EnableExtensions EnableDelayedExpansion

set "VERSION=2019-05-09"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"
set "RETURN=goto :eof"

goto main

:#-----------------------------------------------------------------------------
:# Find all 32-bits Microsoft Visual Studio instances

:findvs
set "PGM32=%ProgramFiles(x86)%"
if not defined PGM32 set "PGM32=%ProgramFiles%"
if not defined PGM32 echo>&2 Error: Can't find 32-bits Program Files directory & exit /b 1
set "VSTUDIOS="
for %%s in ("\2019" "\2017" " 14.0" " 12.0" " 11.0" " 10.0" " 9.0" " 8" " .NET 2003" " .NET" "") do @(
  set "VSTUDIO=%PGM32%\Microsoft Visual Studio%%~s"
  if exist "!VSTUDIO!" (
    %ECHO.V% :# VSTUDIO dir "!VSTUDIO!"
    set VSTUDIOS=!VSTUDIOS! "!VSTUDIO!"
  )
)
%RETURN%

:#-----------------------------------------------------------------------------
:# Find the most recent 32-bits Microsoft Visual C++ instance

:findvc32
call :findvs
for %%s in (%VSTUDIOS%) do @(
  for %%c in ("Enterprise\VC" "Professional\VC" "Community\VC" "Preview\VC" "VC" "VC7" "VC98") do @(
    for %%p in ("\Auxiliary\Build" "") do (
      %ECHO.V% :# Searching vcvarsall.bat in "%%~s\%%~c%%~p"
      if exist "%%~s\%%~c%%~p\vcvarsall.bat" (
        set "VSTUDIO=%%~s"
        set "VC=%%~s\%%~c"
        set "VCVARSDIR=%%~s\%%~c%%~p"
        goto foundvc32
      )
    )
  )
)
:foundvc32
%ECHO.V% :# Found "%VCVARSDIR%\vcvarsall.bat"
%ECHOVARS.V% VSTUDIO VC
%RETURN%

:#-----------------------------------------------------------------------------

:echo
echo.%*
%RETURN%

:#-----------------------------------------------------------------------------
:# Display variables

:echo.vars
setlocal EnableExtensions EnableDelayedExpansion
for %%v in (%*) do @(
  echo set "%%~v=!%%~v!"
)
endlocal
goto :eof

:#-----------------------------------------------------------------------------
:# Display variables set by vcvars.bat.
:# Usable even after endlocal. So don't use any locally defined variable, including %RETURN%.

:echo.msvcvars
echo :# Environment variables used or set by Visual C++' script
call :echovars.v DevEnvDir FrameworkDir FrameworkVersion INCLUDE LIB LIBPATH PATH VCINSTALLDIR VSINSTALLDIR WindowsSdkDir
goto :eof

:#-----------------------------------------------------------------------------
:# Display a help screen

:help
echo.
echo Set Visual C++ environment variables
echo.
echo Usage: %ARG0% [OPTIONS] [ARCHI]
echo.
echo Options:
echo   -?^|-h          This help
echo   -v             Display verbose information
echo   -V             Display the script version and exit
echo   -x CMD [ARGS]  Run the command line with VC variables and return
echo   -X             Display the actual vcvars command to run, but don't run it
echo.
echo Archi:           Target architecture. Default: x86
echo   x86            32-bits x86
echo   ia64           64-bits IA64
echo   amd64          64-bits x86
echo   arm            ARM
echo   x86_amd64      Cross-compilation tools
echo   x86_ia64       Cross-compilation tools
echo   amd64_x86      Cross-compilation tools
echo   amd64_arm      Cross-compilation tools
echo.
echo Warning: Each time this script is run, it adds paths to existing variables.
echo          It is strongly recommended to run it in a sub-shell, and to exit
echo          that sub-shell and reopen another one, before running it again.
echo          Use the subcmd.bat script for doing that in a controlled way.
%RETURN%

:#-----------------------------------------------------------------------------
:# Execute a command in the context of VC environment variables and exit
:# %1 = Program
:# %2 ... = Command line arguments

:exec_cmd
set "VCINSTALLDIR="
call :findvc32
if exist "%VCVARSDIR%\vcvarsall.bat" call "%VCVARSDIR%\vcvarsall.bat" %ARCHI%
if "%VCINSTALLDIR%"=="" (
  >&2 echo Error: Cannot find Visual C++
  endlocal
  exit /b 1
)
%ECHOVARS.V% %*
%*
%RETURN%

:#-----------------------------------------------------------------------------
:# Main routine
:# %* = Command line arguments

:main
set "ECHOVARS=call :echo.vars"
set "ECHO.V=rem"
set "ECHOVARS.V=rem"
set "ECHO.MSVCVARS=rem"
set "ARCHI="
set "NOEXEC=0"
goto get_args

:next_arg
shift
:get_args
if .%1.==.. goto go
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-h. goto help
if .%1.==.-v. set "ECHO.V=call :echo" & set "ECHOVARS.V=call :echo.vars" & set "ECHO.MSVCVARS=call :echo.msvcvars" & goto next_arg
if .%1.==.-V. echo %VERSION% & exit /b 0
if .%1.==.-x. shift & goto exec_cmd
if .%1.==.-X. set "NOEXEC=1" & goto next_arg
if "%ARCHI%"=="" set "ARCHI=%1" & goto next_arg
>&2 echo Warning: Unexpected argument %1 ignored
goto next_arg

:go

:# Find 32-bits Visual C++
call :findvc32

%ECHO.V% :# Predefined context variables
%ECHOVARS.V% PROCESSOR_ARCHITECTURE
%ECHO.V%

%ECHO.V% :# Environment variables used or set by this script
%ECHOVARS.V% VSTUDIOS VSTUDIO VC VCVARSDIR
%ECHO.V%

:# Find and execute the corresponding vcvars.bat script
:# vcvarsall.bat $1=%PROCESSOR_ARCHITECTURE%, optional, default=x86
if not exist "%VCVARSDIR%\vcvarsall.bat" (
  >&2 echo Error: Could not find a vcvarsall.bat script
  exit /b 1
)

:# Visual Studio 2017 and later require the ARCHI parameter
if not "%VCVARSDIR%"=="%VC%" if "%ARCHI%"=="" set "ARCHI=x86"

:# Generate the final command
set CMD=call "%VCVARSDIR%\vcvarsall.bat" %ARCHI%
if "%NOEXEC%"=="1" (
  echo %CMD%
  exit /b 0
) else (
  %ECHO.V% %CMD%
)
endlocal & %CMD% & %ECHO.MSVCVARS%
