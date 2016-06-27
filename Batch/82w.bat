@echo off
:##############################################################################
:#                                                                            #
:#  Filename	    82w.bat						      #
:#                                                                            #
:#  Description	    Convert UTF-8 characters to Windows character.	      #
:#                                                                            #
:#  Notes	                                                              #
:#                                                                            #
:#  History	                                                              #
:#   2004-01-01 JFL Created this program as m2w.bat.                          #
:#   2006-09-10 JFL Use the new conv.exe tool instead of remplace.exe.        #
:#                  (Full UTF-8 conversion)				      #
:#   2008-12-10 JFL The non-standard \xC3\x20 is now managed in conv.exe.     #
:#   2012-03-01 JFL Cleanup up obsolete features.			      #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal
goto main

:help
echo.
echo.82w version 2012-03-01
echo.
echo Convert a UTF-8 text file into the Windows character set.
echo.
echo Usage: 82w [OPTIONS] [INPUT_FILE [OUTPUT_FILE]]
echo.
echo Default output file: Same as the input file.
echo Default input file: stdin. Then output to stdout, for use in a pipe.
goto :eof

:main
set "DEBUG.ECHO=rem"
goto get_args
:next_arg
shift
:get_args
if .%1.==..   goto pipe
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-d. set "DEBUG.ECHO=echo" & goto next_arg
if .%1.==.-h. goto help

if not exist %1 goto help

:# Replace the input file if no output file specified.
set OUT=%2
if .%2.==.. set "OUT="%TEMP%\%~nx1""

%DEBUG.ECHO% conv 8 w ^<%1 ^>%OUT%
conv 8 w <%1 >%OUT%
if .%2.==.. (
  del %1
  move %OUT% %1 >NUL
)
goto :eof

:pipe
>&2 %DEBUG.ECHO% conv 8 w
conv 8 w
