@echo off
:##############################################################################
:#                                                                            #
:#  Filename        n.bat	                                              #
:#                                                                            #
:#  Description     Start Windows Notepad				      #
:#                                                                            #
:#  Notes 	    Keep compatibility with Windows 95                        #
:#                                                                            #
:#  History                                                                   #
:#   1990s      JFL Created this script.			              #
:#   2019-04-09 JFL Work around trailing spaces issue in Windows 10 v 2019-03.#
:#   2020-09-23 JFL Added command-line arguments when running in NT cmd.exe.  #
:#                  Added ability to pipe names of the files to open.         #
:#                                                                            #
:##############################################################################

:# Keep compatibility with Windows 95
if not "%OS%"=="Windows_NT" goto :start_95

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2020-09-23"
set "SCRIPT=%~nx0"		&:# Script name
set "SNAME=%~n0"		&:# Script base name, without its extension

goto :Main

:#----------------------------------------------------------------------------#

:Help
echo.
echo %SCRIPT% - Start Windows Notepad
echo.
echo Usage: %SNAME% [OPTIONS] [NOTEPAD_OPTIONS] [PATHNAME]
echo.
echo Options:
echo   -?    Display this help
echo   -V    Display the script version
echo   -X    Display the command generated, but don't run it
echo.
echo Notepad options:
echo   /A    Open the file as ANSI
echo   /W    Open the file as UTF-16
echo   /P    Print the file
echo.
echo Pathname: Name of the file to open, or "-" to read names from stdin.
exit /b

:#----------------------------------------------------------------------------#

:Main
set "ARGS="
set "EXEC="

goto :get_arg
:next_arg
shift
:get_arg
if [%1]==[] goto :start_nt
if "%~1"=="-"  goto :start_from_pipe
if "%~1"=="/?" goto :Help
if "%~1"=="-?" goto :Help
if "%~1"=="-V" (echo.%VERSION%) & exit /b
if "%~1"=="-X" set "EXEC=echo" & goto :next_arg
set ARGS=%ARGS% %1
goto :next_arg

:# Get the names of the files to open from stdin
:start_from_pipe
for /f "delims=" %%n in ('findstr /R /C:"^"') do %EXEC% start notepad.exe "%%~n"
exit /b

:start_nt
:# In Windows 10 version 2019-03, notepad always appends .txt when creating a file if there are trailing spaces
%EXEC% start notepad.exe%ARGS%
exit /b

:start_95
:# Do not use %* to remain compatible with Windows 95
start notepad.exe %1 %2 %3 %4 %5
