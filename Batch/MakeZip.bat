@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    MakeZip.bat						      *
:#                                                                            *
:#  Description:    Create a new Tools.zip for exporting all JFL tools        *
:#                                                                            *
:#  Notes:	    Requires 7-Zip.                                           *
:#                                                                            *
:#  History:                                                                  *
:#   2014-05-12 JFL Created this script.          			      *
:#   2014-12-05 JFL Added the ability to specify the input file name.	      *
:#                                                                            *
:#*****************************************************************************

setlocal enableextensions enabledelayedexpansion
set "VERSION=2014-12-05"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*

set RETURN=goto :eof
set LOG=call :log

set "POPARG=call :PopArg"
goto Main

:#-----------------------------------------------------------------------------

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

:#-----------------------------------------------------------------------------

:help
echo %SCRIPT% - Create a new zip file for exporting a set of tools
echo.
echo Usage: %SCRIPT% [options] [LIST_FILE]
echo.
echo Options:
echo   -?^|-h           This help
echo   -V              Display %SCRIPT% version
echo   -X              Display the 7-zip command generated, but don't run it
echo.
echo List File:
echo   PATHNAME[.lst]  Create PATHNAME.zip with files listed in PATHNAME.lst
echo                   Default: Tools.lst
%RETURN%

:#-----------------------------------------------------------------------------

:main
set "INPUT_FILE=Tools.lst"
set "EXEC="

:next_arg
%POPARG%
if .!ARG!.==.. goto go
if .!ARG!.==.-?. goto help
if .!ARG!.==./?. goto help
if .!ARG!.==.-h. goto help
if .!ARG!.==.-V. (echo %VERSION%) & goto :eof
if .!ARG!.==.-X. set "EXEC=echo" & goto :next_arg
if "!ARG:~0,1!"=="-" (
  >&2 %ECHO% Warning: Unexpected option ignored: !ARG!
  goto :next_arg
)
set "INPUT_FILE=!ARG!"
goto next_arg

:go
if not exist "%INPUT_FILE%" (
  if "%INPUT_FILE:.lst=%"=="%INPUT_FILE%" (
    :# If the input file is missing, and if it does not already ends with .lst, append .lst.
    set "INPUT_FILE=%INPUT_FILE%.lst"
  )
)
set "ZIP_FILE=%INPUT_FILE:.lst=%.zip"
if not exist "%INPUT_FILE%" (
  >&2 echo Error: This must be run in a directory containing a %INPUT_FILE% file,
  >&2 echo  with the list of files to copy into %ZIP_FILE%.
  exit /b 1
)
echo Building %ZIP_FILE%
if exist "%ZIP_FILE%" del "%ZIP_FILE%"
set PATH=$(PATH);C:\Program Files\7-zip
%EXEC% 7z.exe a -r0 "%ZIP_FILE%" @"%INPUT_FILE%"
