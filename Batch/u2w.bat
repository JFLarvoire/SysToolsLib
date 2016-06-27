@echo off
:#****************************************************************************#
:#									      #
:#  File name	    u2w.bat						      #
:#									      #
:#  Description	    Unix to Windows text converter		              #
:#									      #
:#  History								      #
:#   2004-10-18 JFL Use .%1. instead of "%1" to detect empty arguments.	      #
:#		    Allows to support long file names within quotes.          #
:#   2006-04-03 JFL Allow not passing any argument, for full piping support.  #
:#   2009-09-21 JFL Preserve the file time stamp, if modifying the same file. #
:#   2010-10-14 JFL Added the -q option.				      #
:#   2011-01-10 JFL Preserve the file time stamp using remplace new -t option.#
:#   2011-01-11 JFL Added the -X option.				      #
:#									      #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:#****************************************************************************#

setlocal
goto main

:help
echo.
echo Unix to Windows text converter.
echo.
echo Usage:
echo.
echo u2w [OPTIONS] [input_file [output_file]]
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

%EXEC% remplace %QUIET% %COPYTIME% \r?\n \r\n %IN% %OUT%

:end
