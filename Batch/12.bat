@echo off
:##############################################################################
:#                                                                            #
:#  Filename        12.bat	                                              #
:#                                                                            #
:#  Description     Pipe clipboard data through a command and back            #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2006-10-21 JFL Created this script.		                      #
:#   2014-04-18 JFL Added the new -A options to both 1clip and 2clip, as data #
:#                  from GUI programs is likely to be encoded as ANSI, and    #
:#                  some of my filter programs still only support ANSI data.  #
:#   2017-05-10 JFL Removed the -A options as the filters are now fixed.      #
:#   2017-05-11 JFL Actually, no, the Tcl tools still assume that the input   #
:#                  and output pipes contain ANSI data. But my C tools now    #
:#                  follow cmd.exe's own convention of using the console code #
:#		    page for pipes. So to be compatible with both...          #
:#                  Temporarily change to the system's default ANSI codepage. #
:#                                                                            #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2017-05-11"

:# Get the current console code page
for /f "tokens=2 delims=:" %%n in ('chcp') do for %%p in (%%n) do set OLDCP=%%p
:# Get the default system code page
set "KEY=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage"
for /f "tokens=3" %%p in ('reg query "%KEY%" /v "ACP" ^| findstr REG_SZ') do set "NEWCP=%%p"
:# Change to the default system code page if needed
if not "%OLDCP%"=="%NEWCP%" chcp %NEWCP% >NUL

1clip | %* | 2clip

:# Return to the initial code page if it was different
if not "%OLDCP%"=="%NEWCP%" chcp %OLDCP% >NUL

