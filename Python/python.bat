@echo off
:##############################################################################
:#                                                                            #
:#  Filename        python.bat                                                #
:#                                                                            #
:#  Description     Run python.exe, even when it's not in the PATH            #
:#                                                                            #
:#  Notes 	    The default instance of Python is considered to be        #
:#		    the one associated with the .py file extension in         #
:#		    the registry; Not necessarily the one in the PATH.        #
:#                                                                            #
:#  History                                                                   #
:#   2017-01-16 JFL Created this file.                                        #
:#   2019-09-30 JFL Added option -X.                                          #
:#   2019-10-01 JFL Added options --, -# and -l.                              #
:#   2019-10-02 JFL Use DisableDelayedExpansion to avoid issues w. ! in args. #
:#                  Added option -V.                                          #
:#   2019-11-29 JFL Make the python instance number inheritable.              #
:#   2020-02-27 JFL Search python.exe in more locations.                      #
:#   2021-02-25 JFL Added option -d to enabled debugging.                     #
:#                  Removed a dependency on my VMs host drive configuration.  #
:#                  Use short names to compare instances reliably.            #
:#   2021-03-02 JFL Restructured the instances enumeration like in PySetup.bat.
:#   2021-03-04 JFL Synchronized changes with pip.bat.			      #
:#                                                                            #
:#         © Copyright 2019 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2021-03-04"
set "SCRIPT=%~nx0"		&:# Script name
set "SNAME=%~n0"		&:# Script name, without its extension
set "SPATH=%~dp0"		&:# Script path
set "SPATH=%SPATH:~0,-1%"	&:# Script path, without the trailing \
set "SFULL=%~f0"		&:# Script full pathname
set ^"ARG0=%0^"			&:# Script invokation name
set ^"ARGS=%*^"			&:# Argument line

call :debug.off
goto :main

:debug.off
set "DEBUG=0"
set "ECHO.D=echo >NUL"
set "ECHOVARS.D=echo >NUL"
goto :debug.common
:debug.on
set "DEBUG=1"
set "ECHO.D=echo"
set "ECHOVARS.D=call :EchoVars"
:debug.common
set "FUNCTION=call %ECHO.D% call %%0 %%*&setlocal"
exit /b

:EchoVars	%1=VARNAME %2=VARNAME %3=VARNAME ...
setlocal EnableExtensions EnableDelayedExpansion
:EchoVars.loop
if "%~1"=="" endlocal & goto :eof
%>DEBUGOUT% echo %INDENT%set "%~1=!%~1!"
if defined LOGFILE %>>LOGFILE% echo %INDENT%set "%~1=!%~1!"
shift
goto EchoVars.loop

:# Get the first (#0) trailing argument
:GetArg0 %1=VARNAME %2=ARG0 %3... ignored
set "%~1=%~2"
exit /b

:# Get the Nth trailing argument
:GetArgN %1=VARNAME %2=N %3=ARG0 %4=ARG1 ...
setlocal DisableDelayedExpansion
set "VAR=%~1"
for /l %%n in (1,1,%~2) do shift
endlocal & set "%VAR%=%~3"
exit /b

:# Append an element to a list. The list is walkable using a for %%e in (!LIST!) ...
:lappend %1=LISTNAME %2=value
if defined %~1 (
  call set %%~1=%%%1%% %%2
) else (
  set %~1=%2
)
exit /b

:# Get the Nth element of a list.
:lindex %1=LISTNAME %2=index %3=VARNAME
call call :GetArgN %%3 %%2 %%%~1%%
exit /b

:# Get the command line used to open a given file type
:GetOpenCommand %1=type %2=VARNAME
set "%~2="
for /f "delims== tokens=2" %%c in ('ftype %~1 2^>NUL') do set "%~2=%%c"
exit /b

:# Get the default python.exe executable
:GetPythonExe %1=VARNAME
%ECHO.D% call :GetPythonExe %1
call :GetOpenCommand Python.File %1
if defined %~1 call call :GetArg0 %%1 %%%~1%%
%ECHOVARS.D% %1
exit /b

:# Get a list of all python.exe executables
:GetAllPythonExe %1=LISTNAME
%FUNCTION% EnableDelayedExpansion
set "LIST=" &:# List of instances found.

:# First list the default instance
call :GetPythonExe DEFAULT
set "SHORT_DEFAULT=" &:# Short version of the same, used to avoid reporting it twice.
if defined DEFAULT for %%p in ("!DEFAULT!") do (
  set "SHORT_DEFAULT=%%~sp"
  call :lappend LIST "%%~fp"
)

:# Then list py.exe if it's not the default
if defined DEFAULT for %%p in ("%DEFAULT%") do if not "%%~nxp"=="py.exe" (
  for %%x in (py.exe) do set "PY=%%~$PATH:x"
  if defined PY for %%p in ("!PY!") do (
    call :lappend LIST "%%~fp"
  )
)

:# Then list all instances that match the requested criteria
call :GetAllPythonDirs ALL_DIRS
for %%d in (%ALL_DIRS%) do (
  for %%p in ("%%~d\python.exe") do (
    if not "%%~sp"=="%SHORT_DEFAULT%" (
      call :lappend LIST "%%~fp"
    )
  )
)

endlocal & set %~1=%LIST%
%ECHOVARS.D% %~1
exit /b

:# Get a list of all python.exe installation directories in well known places
:GetAllPythonDirs %1=LISTNAME
%FUNCTION% EnableDelayedExpansion
set "LISTNAME=%~1"
set "DRIVES=C:"		&:# List of drives where to search for python.exe
if not "%HOMEDRIVE%"=="C:" set "DRIVES=%HOMEDRIVE% %DRIVES%" &:# Prepend the home drive, if it's different
for /f "tokens=1" %%d in ('net use ^| findstr /C:"\\vmware-host\Shared Folders\\"') do ( :# And append VM host drives, if any
  if exist "%%d%ProgramFiles:~2%" set "DRIVES=!DRIVES! %%d"
)
%ECHOVARS.D% DRIVES
set "LIST="
if not defined ProgramFiles(x86) set "ProgramFiles(x86)=%ProgramFiles% (x86)" &:# For x86 VMs searching on amd64 hosts
for %%d in (%DRIVES%) do (
  rem :# The default install dir is %LOCALAPPDATA%\Programs\Python\Python%VER% for the current user,
  rem :# Or %ProgramFiles%\Python%VER% or %ProgramFiles(x86)%\Python%VER% for all users.
  for %%p in ("" "%ProgramFiles:~2%" "%ProgramFiles(x86):~2%" "%LOCALAPPDATA:~2%\Programs") do (
    for %%y in ("Python" "Python\Python" "Microsoft Visual Studio\Shared\Python") do (
      for /d %%b in ("%%d%%~p\%%~y*") do (
	%ECHO.D% :# Looking in %%~b
	for %%x in ("%%~b\python.exe") do if exist "%%~x" (
	  if defined LISTNAME (
	    call :lappend LIST "%%~b"
	  ) else (
	    set "B=%%~b"
	    echo !B!
	  )
	)
      )
    )
  )
)
if defined LISTNAME (
  set SET_RESULT=set %LISTNAME%=%LIST%
  %ECHO.D% !SET_RESULT!
) else (
  set SET_RESULT=rem
)
endlocal & %SET_RESULT%
exit /b

:# List python.exe instances
:list
%FUNCTION% EnableDelayedExpansion
call :GetAllPythonExe ALL_PYTHON
set "N=0"
set ^"PY_VER_ARCH_CMD=import sys, os; print (sys.version.split()[0]+\" \"+os.environ[\"PROCESSOR_ARCHITECTURE\"])^"
for %%e in (!ALL_PYTHON!) do (
  set "PY_VER=" & set "PY_ARCH="
  for /f "tokens=1,2" %%v in ('^"%%e -c "!PY_VER_ARCH_CMD!" 2^>NUL^"') do set "PY_VER=%%v" & set "PY_ARCH=%%w"
  if defined PY_VER if defined PY_ARCH (
    set "INDEX_=!N!       "
    set "PY_VER_=!PY_VER!       "
    set "PY_ARCH_=!PY_ARCH!       "
    echo #!INDEX_:~0,3! !PY_VER_:~0,8! !PY_ARCH_:~0,7! %%~e
    set /a "N+=1"
  )
)
endlocal
exit /b

:#----------------------------------------------------------------------------#
:# Main routine

:usage
echo.
echo %SCRIPT% - Run python.exe, even when it's not in the PATH
echo.
echo Usage: %SNAME% [OPTIONS] [PYTHON.EXE OPTIONS AND ARGUMENTS]
echo.
echo Options:
echo   -?          Display this help
echo   --          End of %SCRIPT% arguments
echo   -# N ^| #N   Use python instance #N in the list. Default=instance #0
echo   -l          List python instances installed, with the default instance first
echo   -X          Display the python command line to execute, but don't run it
echo.
echo python.exe options:
exit /b

:main
set "EXEC="
set "PYARGS="
if not defined PY# set "PY#=0"

goto :get_arg
:next_arg
shift
:get_arg
if [%1]==[] goto :go
set "ARG=%~1"
if "%ARG%"=="-?" call :usage & set "PYARGS=-?" & goto :go
if "%ARG%"=="--" goto :next_pyarg &:# All remaining arguments are for python.exe
if "%ARG%"=="-#" set "PY#=%~2" & shift & goto :next_arg
if "%ARG%"=="-d" call :debug.on & goto :next_arg
if "%ARG:~0,1%"=="#" set "PY#=%ARG:~1%" & goto :next_arg
if "%ARG%"=="-l" call :list & exit /b
if "%ARG%"=="-V" (echo %SCRIPT% %VERSION%) & set "PYARGS=-V" & goto :go
if "%ARG%"=="-X" set "EXEC=echo" & goto :next_arg
:get_pyarg
if [%1]==[] goto :go
call :lappend PYARGS %1
:next_pyarg
shift
goto :get_pyarg

:go

:# Get the Nth Python instance
set "PYTHON="
if %PY#%==0 ( :# Performance optimization for instance #0
  call :GetPythonExe PYTHON
)
if not defined PYTHON ( :# For %PY#% > 0, or if the above search for #0 failed
  call :GetAllPythonExe LIST
  call :lindex LIST %PY#% PYTHON
)
if not defined PYTHON (
  echo %SCRIPT%: Error: No python interpreter found
  exit /b 1
)
%EXEC% "%PYTHON%" %PYARGS%
