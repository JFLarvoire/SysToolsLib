@echo off
:##############################################################################
:#                                                                            #
:#  Filename        u2a.bat                                                   #
:#                                                                            #
:#  Description     Convert a Unicode text file to Ansi			      #
:#                                                                            #
:#  Notes 	    Based on an idea found on				      #
:#		    http://www.robvanderwoude.com/type.php.		      #
:#                                                                            #
:#                  Improvement on their code:				      #
:#		    This script can be used as a filter in a pipe.	      #
:#                  Note however that I've not found a way to do that         #
:#                  filtering using pure batch only. It's using remplace.exe. #
:#		    My initial idea of using "more" was outputing an extra    #
:#                  CRLF in the end.                                          #
:#                                                                            #
:#  History                                                                   #
:#   2012-02-28 JFL Created this script. (jf.larvoire@hp.com)                 #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal enableextensions enabledelayedexpansion
set "VERSION=2012-02-28"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"

goto main

:help
echo.
echo Unicode to Ansi text converter.
echo.
echo Usage:
echo.
echo %SCRIPT% [OPTIONS] [input_file [output_file]]
echo.
echo.Options:
echo.  -?    This help screen
echo.  -X    Display the commands to execute, but don't run them
echo.
echo Default input file: stdin
echo Default output file: Same as the input file, or stdout for stdin.
echo Use - to force input from stdin or output to stdout.
echo.
echo Caution: Do not use ^<input_file, as this forces a preliminary conversion
echo that defeats the whole purpose of this script.
goto :eof

:main
set "EXEC="
goto get_args

:next_arg
shift
:get_args
if .%1.==./?. goto help
if .%1.==.-?. goto help
if .%1.==.-X. set "EXEC=echo" & goto next_arg
goto go

:go
set IN=%1
set OUT=%2

:# Save the initial code page
for /f "tokens=2 delims=:" %%n in ('chcp') do set "OLDCP=%%n"

:# Switch to code page 1252.
if not %OLDCP%==1252 %EXEC% chcp 1252 | findstr /v ":"

if .%IN%.==.-. set "IN="
if .%IN%.==.. (
  rem set "CMD=more"
  set "CMD=remplace "ÿþ" "" | remplace -."
) else (
  set "CMD=type %1"
)
set SAME=0
if .%OUT%.==.. if not .%IN%.==.. (
  set SAME=1
  set OUT=%TEMP%\%RANDOM%%RANDOM%%RANDOM%%RANDOM%.u2a
)
if .%OUT%.==.-. set "OUT="
if not .%OUT%.==.. set "CMD=%CMD% >%OUT%"

:# Convert the file
if .%EXEC%.==.echo. (
  set "CMD=%CMD:>=^>%"
  set "CMD=%CMD:|=^|%"
)
%EXEC% %CMD%

:# Replace the initial file if needed
if %SAME%==1 (
  %EXEC% del %IN%
  %EXEC% move %OUT% %IN% >NUL
)

:# Restore the initial code page
if not %OLDCP%==1252 %EXEC% chcp %OLDCP% | findstr /v ":"

