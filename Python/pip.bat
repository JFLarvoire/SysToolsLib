@echo off
:##############################################################################
:#                                                                            #
:#  Filename        pip.bat                                                   #
:#                                                                            #
:#  Description     Run Python's pip.exe, even when it's not in the PATH      #
:#                                                                            #
:#  Notes 	    The default instance of Python is considered to be        #
:#		    the one associated with the .py file extension in         #
:#		    the registry; Not necessarily the one in the PATH.        #
:#                                                                            #
:#  History                                                                   #
:#   2019-09-30 JFL Created this file based on python.bat.                    #
:#   2019-10-01 JFL Added options --, -# and -l.                              #
:#   2019-10-02 JFL Use DisableDelayedExpansion to avoid issues w. ! in args. #
:#                  Added option -V.                                          #
:#   2019-11-29 JFL Make the python instance number inheritable.              #
:#   2020-02-27 JFL Search python.exe in more locations.                      #
:#   2020-10-28 JFL Added option -d to enabled debugging.                     #
:#   2021-02-25 JFL Removed a dependency on my VMs host drive configuration.  #
:#                  Use short names to compare instances reliably.            #
:#                                                                            #
:#         © Copyright 2019 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2021-02-25"
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
goto :debug.common
:debug.on
set "DEBUG=1"
:debug.common
set "ECHO.D=if %DEBUG%==1 echo"
set "ECHOVARS.D=if %DEBUG%==1 call :EchoVars"
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
%ECHO.D% :GetPythonExe %1
call :GetOpenCommand Python.File %1
if defined %~1 call call :GetArg0 %%1 %%%~1%%
%ECHOVARS.D% %1
exit /b

:# Get a list of all python.exe executables
:GetAllPythonExe %1=LISTNAME
setlocal EnableDelayedExpansion
set "SEARCHDRIVES=C:"		&:# List of drives where to search for python.exe.
if not "%HOMEDRIVE%"=="C:" set "SEARCHDRIVES=%HOMEDRIVE% %SEARCHDRIVES%" &:# Prepend the home drive, if it's different.
for /f "tokens=1" %%d in ('net use ^| findstr /C:"\\vmware-host\Shared Folders\\"') do ( :# And append VM host drives, if any.
  if exist "%%d%ProgramFiles:~2%" set "SEARCHDRIVES=!SEARCHDRIVES! %%d"
)
%ECHOVARS.D% SEARCHDRIVES
set "SHORT_LIST=" &:# List of instances found. Used to avoid reporting one twice.
set "LIST=" &:# List of instances found.
:# List first the default instance
call :GetPythonExe PYTHON
if defined PYTHON (
  for %%p in ("%PYTHON%") do call :lappend SHORT_LIST "%%~sp"
  call :lappend LIST "%PYTHON%"
)
:# Then look for other instances in well known places
for %%d in (%SEARCHDRIVES%) do (
  rem :# The default install dir is %LOCALAPPDATA%\Programs\Python\Python%VER% for the current user,
  rem :# Or %ProgramFiles%\Python%VER% or %ProgramFiles(x86)%\Python%VER% for all users.
  for %%p in ("" "%ProgramFiles:~2%" "%ProgramFiles(x86):~2%" "%LOCALAPPDATA:~2%\Programs") do (
    for %%y in ("Python" "Python\Python" "Microsoft Visual Studio\Shared\Python") do (
      for /d %%b in ("%%d%%~p\%%~y*") do (
        %ECHO.D% :# Looking in %%b
	for %%x in ("%%~b\python.exe") do if exist "%%~x" (
	  set "FOUND=" &:# Avoid duplications, for example with the default instance
	  for %%s in (!SHORT_LIST!) do if not defined FOUND if /i "%%~sx"=="%%~s" set "FOUND=1"
	  if not defined FOUND call :lappend SHORT_LIST "%%~sx" & call :lappend LIST "%%~fx"
	)
      )
    )
  )
)
endlocal & set %~1=%LIST%
exit /b

:# Get the corresponding pip.exe executable if it exists
:GetPythonPipExe %1=PYTHON %2=VARNAME
set "%~2="
for %%p in ("%~dp1Scripts\pip.exe") do if exist %%p set "%~2=%%~p"  
exit /b

:# Get a list of all pip.exe executables
:GetAllPipExe %1=LISTNAME
setlocal DisableDelayedExpansion
call :GetAllPythonExe PLIST
set "LIST="
for %%e in (%PLIST%) do (
  call :GetPythonPipExe %%e PIP
  if defined PIP call call :lappend LIST "%%PIP%%"
)
endlocal & set %~1=%LIST%
exit /b

:# List pip.exe instances
:list
setlocal EnableDelayedExpansion
call :GetAllPipExe LIST
set "N=0"
for %%e in (!LIST!) do (
  echo #!N! %%~e
  set /a "N+=1"
)
endlocal
exit /b

:#----------------------------------------------------------------------------#
:# Main routine

:usage
echo.
echo %SCRIPT% - Run Python's pip.exe, even when it's not in the PATH
echo.
echo Usage: %SNAME% [OPTIONS] [PIP.EXE OPTIONS AND ARGUMENTS]
echo.
echo Options:
echo   -?          Display this help
echo   --          End of %SCRIPT% arguments
echo   -# N ^| #N   Use pip instance #N in the list. Default=instance #0
echo   -l          List pip instances installed, with the default instance first
echo   -X          Display the pip command line to execute, but don't run it
echo.
echo pip.exe options:
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
if "%ARG%"=="-?" call :usage & set "PYARGS=-h" & goto :go
if "%ARG%"=="--" goto :next_pyarg &:# All remaining arguments are for pip.exe
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

:# Get the Nth Pip instance
set "PIP="
if %PY#%==0 ( :# Performance optimization for instance #0
  call :GetPythonExe PYTHON
  if defined PYTHON call call :GetPythonPipExe "%%PYTHON%%" PIP
)
if not defined PIP ( :# For %PY#% > 0, or if the above search for #0 failed
  call :GetAllPipExe LIST
  call :lindex LIST %PY#% PIP
)
if not defined PIP (
  echo %SCRIPT%: Error: No pip command found
  exit /b 1
)
%EXEC% "%PIP%" %PYARGS%
