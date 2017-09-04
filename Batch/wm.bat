@echo off
:##############################################################################
:#                                                                            #
:#  Filename:       wm.bat                                                    #
:#                                                                            #
:#  Description:    Invoke WinMerge, even if it's not in the PATH             #
:#                                                                            #
:#  Notes:	    							      #
:#		    							      #
:#  History:                                                                  #
:#   2007-05-09 JFL Added the --, -?, -h, -R and -X options.                  #
:#   2010-09-07 JFL Search WinMerge.exe independantly of the µP architecture. #
:#   2017-09-04 JFL Simplified the argument processing.                       #
:#                  Changed the default to non-recursive.                     #
:#                  Added option -V.                                          #
:#                                                                            #
:#         © Copyright 2017 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2017-09-04"

goto :main

:help
echo.
echo Invoke WinMerge, even if it's not in the PATH
echo.
echo Usage: wm [options] PATH_1 PATH_2 [WinMerge switches]
echo.
echo Options:
echo   --            Stop processing options. Use -- -? to get WinMerge help.
echo   -?^|-h         Display this help screen.
echo   -r            Enable recursion.
echo   -R            No recursion. (Default)
echo   -V            Display the script version and exit.
echo   -X            Display command to execute, but don't execute it.
goto :eof

:main
:# Default definitions
set "WM_RECUR=0"
set "EXEC="
:# Search for WinMerge EXE in various possible places.
for %%d in ("%ProgramFiles%" "%ProgramFiles(x86)%" "U:\Program Files (x86)") do (
  for %%f in ("WinMerge\WinMergeU.exe" "WinMerge\WinMerge.exe") do (
    if exist "%%~d\%%~f" (
      set WM_PROG=start "WinMerge" "%%~d\%%~f"
      goto found_wm
    )
  )
)
>&2 echo Cannot find the WinMerge executable
exit /b 1
:found_wm
goto :tst_args

:next_arg
shift
:tst_args
if .%1.==.--. shift && goto :go
if .%1.==./?. goto :help
if .%1.==.-?. goto :help
if .%1.==.-h. goto :help
if .%1.==.-r. set "WM_RECUR=1" & goto :next_arg
if .%1.==.-R. set "WM_RECUR=0" & goto :next_arg
if .%1.==.-V. (echo.%VERSION%) & goto :eof
if .%1.==.-X. set "EXEC=echo" & goto :next_arg
goto :go

:# Start the action
:go
:# Manage recursion
if %WM_RECUR%==1 set WM_PROG=%WM_PROG% -r

%EXEC% %WM_PROG% %1 %2 %3 %4 %5 %6 %7 %8 %9
