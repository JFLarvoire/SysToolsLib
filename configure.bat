@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.bat / make.bat				      *
:#                                                                            *
:#  Description:    Search for the eponym script in NMaker\include and run it *
:#                                                                            *
:#  Notes:	    If the %NMINCLUDE% variable is not defined, search for    *
:#		    the NMaker\include directory, and define %NMINCLUDE%.     *
:#                                                                            *
:#                  The NMINCLUDE name comes from JFL's Systems Tools Library,*
:#                  for which this make system was originally created.        *
:#                                                                            *
:#                  Make any change needed regarding the actual configuration *
:#		    process in %NMINCLUDE%\configure.bat. Idem for the make   *
:#                  process in make.bat.				      *
:#                                                                            *
:#  History:                                                                  *
:#   2016-10-10 JFL jf.larvoire@hpe.com created this file.		      *
:#   2016-12-15 JFL Search for the real make.bat in [.|..|../..]\include.     *
:#   2017-03-12 JFL Avoid defining STINCLUDE as a side effect.                *
:#   2017-11-13 JFL Bug fix: Avoid breaking the cmd windows title.            *
:#                  Added a search in the PATH, to be forward-compatible.     *
:#                  Added the :CheckDir subroutine to search consistently.    *
:#   2018-01-11 JFL Also search in win32 subdirectories, which are standard   *
:#                  in many Unix+Windows open source projects.                *
:#                  Fixed several :# comments, which should have been &:#.    *
:#   2018-01-12 JFL Improved comments and the debugging output.               *
:#   2020-12-10 JFL Search for batchs in [.|..|..\..]\[.|WIN32|C]\include.    *
:#   2021-01-28 JFL Search in [.|..|..\..]\[.|NMaker|WIN32|C]\include.        *
:#                  Update the STINCLUDE variable in the caller's scope.      *
:#   2021-02-03 JFL Renamed variable STINCLUDE as NMINCLUDE.                  *
:#                                                                            *
:#        © Copyright 2016 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2021-02-03"
set "SCRIPT=%~nx0"				&:# Script name

goto main

:# Check if a directory contains the NMaker make system
:CheckDir DIRNAME [DIRNAME ...] :# Sets NMINCLUDE and ERRORLEVEL=0 if success
for %%a in (%*) do (
  %ECHO.D% Searching for NMINCLUDE in "%%~a"
  set "NMINCLUDE="
  if exist "%%~a\make.bat" if exist "%%~a\configure.bat" if exist "%%~a\win32.mak" (
    for /f "delims=" %%d in ('pushd %%a ^& cd ^& popd') do set "NMINCLUDE=%%d"
  )
  if defined NMINCLUDE %ECHO.D% set "NMINCLUDE=!NMINCLUDE!" & exit /b 0
)
exit /b 1

:#*****************************************************************************

:main
set "CHECKDIR=call :CheckDir"
set "ECHO.D=call rem" &:# The call ensures that the rem ends at the first &
set "ECHO.V=call rem"

goto :get_arg
:next_arg
shift
:get_arg
if [%1]==[] goto :done_arg
if "%~1"=="-d" set "ECHO.D=echo" & goto :next_arg
if "%~1"=="-v" set "ECHO.V=echo" & goto :next_arg
goto :next_arg
:done_arg

:# Get the full pathname of the NMINCLUDE library directory
if defined NMINCLUDE %CHECKDIR% "%NMINCLUDE%" &:# If pre-defined, make sure the value is valid

:# As a first choice, use the make.bat provided in this project
if not defined NMINCLUDE %CHECKDIR% include NMaker\include win32\include C\include &:# If we have one here, use it
:# Else try in the near context
for %%p in (.. ..\..) do if not defined NMINCLUDE %CHECKDIR% %%p\include %%p\NMaker\include %%p\win32\include %%p\C\include &:# Default: Search it the parent directory, and 2nd level.
:# We might also store the information in the registry
if not defined NMINCLUDE ( :# Try getting the copy in the master environment
  for /f "tokens=3" %%v in ('reg query "HKCU\Environment" /v NMINCLUDE 2^>NUL') do set "NMINCLUDE=%%v"
)
:# Else try finding it in the PATH
set P=!PATH!
:NextP
if not defined NMINCLUDE (
  for /f "tokens=1* delims=;" %%a in ("!P!") do (
    set "DIR=%%a
    set "P=%%b"
    if defined DIR %CHECKDIR% "!DIR!"
    if defined P goto :NextP
  )
)
:# If we still can't find it, admit defeat
if not exist %NMINCLUDE%\make.bat (
  >&2 echo %0 Error: Cannot find NMaker's global C include directory. Please define variable NMINCLUDE.
  exit /b 1
)

%ECHO.D% "%NMINCLUDE%\%SCRIPT%" %*
:# Update the NMINCLUDE variable in the caller's scope. This ensures that child instances don't search it again.
:# Must call the target batch, not run it directly, else the cmd window title is not removed in the end!
endlocal & set "NMINCLUDE=%NMINCLUDE%" & call "%NMINCLUDE%\%SCRIPT%" %*
