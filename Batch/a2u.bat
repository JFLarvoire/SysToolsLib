@echo off
:##############################################################################
:#                                                                            #
:#  Filename        a2u.bat                                                   #
:#                                                                            #
:#  Description     Convert an Ansi text file to Unicode		      #
:#                                                                            #
:#  Notes 	    Based on an idea of Carlos M., found on		      #
:#		    http://www.robvanderwoude.com/type.php.		      #
:#                                                                            #
:#                  Improvement on their code:				      #
:#		    This script can be used as a filter in a pipe.	      #
:#                  Note however that I've not found a way to do that         #
:#                  filtering using pure batch only. It's using remplace.exe. #
:#		    My initial idea of using "cmd /u more" failed to output   #
:#                  Unicode characters as I had hoped.                        #
:#                                                                            #
:#  History                                                                   #
:#   2012-02-28 JFL Created this script. (jf.larvoire@hp.com)                 #
:#                                                                            #
:##############################################################################

setlocal enableextensions enabledelayedexpansion
set "VERSION=2012-02-28"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"

goto main

:help
echo.
echo Ansi to Unicode text converter.
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

:# Switch to code page 1252. Must be done _before_ allocating the CMD1 variable,
:# to preserve the \xFF\xFE characters encoding.
if not %OLDCP%==1252 %EXEC% chcp 1252 | findstr /v ":"

set "CMD1=cmd /D /A /C (SET/P=ÿþ) ^<NUL 2>NUL"

if .%IN%.==.-. set "IN="
if .%IN%.==.. (
  rem set "CMD2=cmd /D /U /C more"
  set "CMD2=remplace . \\0\0"
) else (
  set "CMD2=cmd /D /U /C type %1"
)
set SAME=0
if .%OUT%.==.. if not .%IN%.==.. (
  set SAME=1
  set OUT=%TEMP%\%RANDOM%%RANDOM%%RANDOM%%RANDOM%.u2a
)
if .%OUT%.==.-. set "OUT="
if not .%OUT%.==.. (
  set "CMD1=%CMD1% >%OUT%"
  set "CMD2=%CMD2% >>%OUT%"
)

:# Convert the file
:# Note: For some reason, substituting chars around > work, but around < does not.
if not .%EXEC%.==.echo. set "CMD1=%CMD1:^=%"
if .%EXEC%.==.echo. (
  set "CMD1=%CMD1:>=^>%"
  set "CMD2=%CMD2:>=^>%"
)
%EXEC% %CMD1%
%EXEC% %CMD2%

:# Replace the initial file if needed
if %SAME%==1 (
  %EXEC% del %IN%
  %EXEC% move %OUT% %IN% >NUL
)

:# Restore the initial code page
if not %OLDCP%==1252 %EXEC% chcp %OLDCP% | findstr /v ":"

