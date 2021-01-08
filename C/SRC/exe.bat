@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    exe.bat						      *
:#                                                                            *
:#  Description:    Front end to make.bat, to simply build multiple targets   *
:#                                                                            *
:#  Arguments:	    Use option -? to display a help screen		      *
:#                                                                            *
:#  Notes:	    Builds the 16-bits MS-DOS version if Visual C++ 1.52 is   *
:#		    installed in its default location in C:\MSVC.	      *
:#									      *
:#  History:                                                                  *
:#   2003-03-31 JFL Adapted from previous projects			      *
:#   2014-03-21 JFL Builds the 16-bits MS-DOS version if Visual C++ 1.52 is   *
:#		    installed in its default location in C:\MSVC.	      *
:#   2014-03-27 JFL Changed option -f to use nmake option /A.                 *
:#		    Added option -r for completeness.                         *
:#   2015-11-13 JFL Adapted to the new multitarget make system.               *
:#   2020-12-15 JFL Accept either program or program.exe as target arguments. *
:#   2021-01-07 JFL Avoid unnecessary spaces in the make command generated.   *
:#   2021-01-08 JFL Allow building multiple programs.                         *
:#		    Delete all local variables before running make.bat.       *
:#		    	                                                      *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal enableextensions enabledelayedexpansion
set "VERSION=2021-01-08"
goto main

:main
set "EXEC="
set "REL=1"		&:# 1=Build release versions
set "DBG=1"		&:# 1=Build debug versions
set "MAKEOPTS="		&:# nmake.exe /options
set "TARGETS="		&:# nmake targets to build

goto :get_arg
:next_arg
shift
:get_arg
if [%1]==[] goto :start
if .%1.==.-?. goto :help
if .%1.==./?. goto :help
if .%1.==.-a. set "REL=1" & set "DBG=1" & goto :next_arg
if .%1.==.-d. set "REL=0" & set "DBG=1" & goto :next_arg
if .%1.==.-f. set ^"MAKEOPTS=%MAKEOPTS% /A^" & goto :next_arg
if .%1.==.-r. set "REL=1" & set "DBG=0" & goto :next_arg
if .%1.==.-V. (echo.%VERSION%) & exit /b
if .%1.==.-X. set "EXEC=echo" & goto next_arg
set "ARG=%~1"
if "%ARG:~0,1%%ARG:~0,1%"=="--" >&2 echo Unsupported option %1 & goto :next_arg
if "%ARG:~0,1%%ARG:~0,1%"=="//" set ^"MAKEOPTS=%MAKEOPTS% %1^" & goto :next_arg &:# nmake /option
for /f "tokens=2 delims==" %%v in ("%ARG%") do set ^"MAKEOPTS=%MAKEOPTS% %1^" & goto :next_arg &:# nmake macro=value
set "PROGRAM=%~1.exe"
set "PROGRAM=%PROGRAM:.exe.exe=.exe%"
if %REL%==1 set "TARGETS=%TARGETS% %PROGRAM%"
if %DBG%==1 set "TARGETS=%TARGETS% debug\%PROGRAM%"
goto :next_arg

:help
echo.
echo..exe program builder from a C or C++ source
echo.
echo.Usage: exe [-options] [/nmake_options] [macro_defs] program [program ...]
echo.
echo.Options:
echo.
echo.  -?    Display this help page
echo.  -a    Builds all release ^& debug versions (default)
echo.  -d    Builds all debug versions only
echo.  -f^|/A Force building all program targets, irrespective of file dates
echo.  -r    Builds all release versions only
echo.  -X    Display the make command generated and exit
echo.
echo.Notes:
echo.* exe myprog ^<==^> make myprog.exe debug\myprog.exe
echo.* By default, it builds all possible OS targets in DOS, WIN95, WIN32, WIN64
echo.* Macro definitions must be quoted. Ex: "OS=DOS" "MEM=L"
exit /b

:start
if not defined TARGETS (>&2 echo Error: No target program specified) & exit /b 1
set CMDLINE=make%MAKEOPTS%%TARGETS%
:# The endlocal below deletes all local variables, which sometimes did confuse make.bat.
endlocal & %EXEC% %CMDLINE%
