@echo off
:******************************************************************************
:*									      *
:*  File name	    u2w.bat						      *
:*									      *
:*  Description	    Windows to Unix text converter		              *
:*									      *
:*  History								      *
:*   2004-10-18 JFL Use %1. instead of "%1" to detect empty arguments.	      *
:*		    Allows to support long file names within quotes.          *
:*   2011-01-11 JFL Merged in changes from u2w.bat.			      *
:*									      *
:******************************************************************************

setlocal
goto main

:help
echo.
echo Windows to Unix text converter.
echo.
echo Usage:
echo.
echo w2u [OPTIONS] [input_file [output_file]]
echo.
echo.Options:
echo.  -?    This help screen
echo.  -q    Quiet mode: Do not display the number of changes done
echo.  -t    Copy the input file time to the output. (Default if same file)
echo.  -X    Display the command to execute, but don't execute it
echo.
echo Default output file: Same as the input file.
goto end

:quiet
set QUIET=-q
goto next_arg

:copytime
set COPYTIME=-t
goto next_arg

:noexec
set EXEC=echo
goto next_arg

:main
set QUIET=
set COPYTIME=
set EXEC=
goto get_args

:next_arg
shift
:get_args
if .%1.==./?. goto help
if .%1.==.-?. goto help
if .%1.==.-q. goto quiet
if .%1.==.-t. goto copytime
if .%1.==.-X. goto noexec
goto go

:go
set IN=%1
set OUT=%2
if .%OUT%.==.. if not .%IN%.==.. set OUT=-same

if .%OUT%.==.-same. set COPYTIME=-t

%EXEC% remplace %QUIET% %COPYTIME% \r\n \n %IN% %OUT%

:end
