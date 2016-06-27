@echo off
:#****************************************************************************#
:#                                                                            #
:#  Filename	    72w.bat						      #
:#                                                                            #
:#  Description	    Convert UTF-7 characters to Windows character.	      #
:#                                                                            #
:#  Notes	    The built-in conversion with repeated calls to remplace   #
:#                  converts only the most common characters for French.      #
:#                  Also it only works for isolated UTF-7 characters.         #
:#                                                                            #
:#  History                                                                   #
:#   2008/11/18 JFL Created this program as x2w.bat.                          #
:#   2008/12/02 JFL Use the new conv.exe tool instead. (Full UTF-7 conversion)#
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:#****************************************************************************#

set R=remplace

goto getargs

:help
echo.
echo Usage: 72w [input file] [output file]
echo.
echo Convert a UTF-7 text file into the Windows character set.
echo.
echo Default output file: Same as the input file.
echo Default input file: stdin. Then output to stdout, for use in a pipe.
goto end

:getargs
if .%1.==..   goto pipe
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-h. goto help
if .%1.==.--help. goto help

if not exist %1 goto help

:# Replace the input file if no output file specified.
set OUT=%2
if .%2.==.. set OUT=%1.tmp

if not .%DEBUG%.==.. if not %DEBUG%==0 echo  Input file: %1
if not .%DEBUG%.==.. if not %DEBUG%==0 echo Output file: %OUT%

:# Recursively call ourselves in pipe mode
%COMSPEC% /c %0 <%1 >%OUT%
if .%2.==.. del %1
if .%2.==.. ren %OUT% %1
goto end

:pipe
conv 7 w
goto end

:# Cannot use non ASCII characters on the command line. Using hexadecimal escape sequences instead.
:#           ç    |              Ç    | %R%          €    |              à    |              â    |              é    |              è    |              ê    |              ë    |              î    |              ï    |              ô    |              ù    |              û    |              ü    |              '    |              ""   | %R%          œ    |              °    |               !  |               ;  |               |  |               <  |               >  |               [  |               ]  |               @  |               &  |               %  |           + 
%R% "+AOc-?" \xE7 | %R% "+AMc-?" \xC7 | %R% "+IKw-?" \x80 | %R% "+AOA-?" \xE0 | %R% "+AOI-?" \xE2 | %R% "+AOk-?" \xE9 | %R% "+AOg-?" \xE8 | %R% "+AOo-?" \xEA | %R% "+AOs-?" \xEB | %R% "+AO4-?" \xEE | %R% "+AO8-?" \xEF | %R% "+APQ-?" \xF4 | %R% "+APk-?" \xF9 | %R% "+APs-?" \xFB | %R% "+APw-?" \xFC | %R% "+IBk-?" \x27 | %R% "+ACI-?" \x22 | %R% "+AMc-?" \x9C | %R% "+ALo-?" \xB0 | %R% "+ACE-?" "!" | %R% "+ADs-?" ";" | %R% "+AHw-?" "|" | %R% "+ADw-?" "<" | %R% "+AD4-?" ">" | %R% "+AFs-?" "[" | %R% "+AF0-?" "]" | %R% "+AEA-?" "@" | %R% "+ACY-?" "&" | %R% "+ACU-?" "%" | %R% "+-" "+"
goto end

:end
