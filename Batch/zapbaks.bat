@echo off
:#****************************************************************************#
:#                                                                            #
:#  Filename:	    zapbaks.bat						      #
:#                                                                            #
:#  Description:    Delete all *.bak and *~ and #*# backup files.	      #
:#                                                                            #
:#  Notes:          Uses GNU find.                                            #
:#                                                                            #
:#  History:                                                                  #
:#   2010-03-24 JFL Added options -i, -q, -R, and -X.			      #
:#   2011-01-04 JFL Fixed bug with parenthesis passed to xfind.bat.           #
:#   2013-05-16 JFL Added option -v.                                          #
:#                  Fixed compatibility with the ezWinPorts of Unix find.     #
:#   2014-04-22 JFL Also delete files like #*#.                               #
:#                  Fixed the -q option.                                      #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:#****************************************************************************#

setlocal enableextensions enabledelayedexpansion
set "VERSION=2014-04-22"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*
goto main

:help
echo Delete all *.bak and *~ backup files.
echo.
echo Usage:
echo   %0 [OPTIONS]
echo.
echo Options:
echo   -i PATH   Delete files in PATH. Default: In the current directory. 
echo   -q        Quiet mode. Don't display the files deleted.
echo   -r        Recursively delete files in subdirectories too.
echo   -R        Do not recurse to subdirectories. (Default)
echo   -v        Display the Unix find command executed.
echo   -V        Display the script version.
echo   -X        Do not execute. List files, but don't delete them.
goto :eof

:main
set INDIR=.
set MAXDEPTH=-maxdepth 1
set NOEXEC=
set VFLAG=
set QUIET=0
goto get_args

:next_arg
shift
:get_args
if .%1.==.. goto :go
if .%1.==./?. goto help
if .%1.==.-?. goto help
if .%1.==.-i. set "INDIR=%2" & shift & goto next_arg
if .%1.==.-q. set "QUIET=1" & goto next_arg
if .%1.==.-r. set "MAXDEPTH=" & goto next_arg
if .%1.==.-R. set "MAXDEPTH=-maxdepth 1" & goto next_arg
if .%1.==.-v. set "VFLAG=-v" & goto next_arg
if .%1.==.-V. (echo %VERSION%) & goto :eof
if .%1.==.-X. set "NOEXEC=1" & goto next_arg
>&2 echo Warning: Unexpected argument ignored: %1
goto :next_arg

:go
set DOECHO=-exec echo -E {} ";"
set DOECHO=-printf %%%%P\n
if "%QUIET%"=="1" set "DOECHO="
:# Note: The Windows' ports of Unix find are incompatible with each other in the
:#       way they generate quotes around the {} marker.
:#       These quotes are necessary to correctly handle pathnames including spaces.
:#       The UnixUtils port xfind.bat used by default initially requires three
:#       quotes to work properly: """{}"""
:#       The (faster) ezWinPort xfind uses now by default works fine without quotes,
:#       works fine also with one quote, but fails with three quotes.
set DOZAP=-exec rm -f {} ";"
if "%NOEXEC%"=="1" set "DOZAP="
call xfind %VFLAG% %INDIR% %MAXDEPTH% -type f "(" -iname "*.bak" -o -name "*~" -o -name "#*#" ")" %DOECHO% %DOZAP%

