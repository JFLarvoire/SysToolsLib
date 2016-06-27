@echo off
:##############################################################################
:#                                                                            #
:#  Filename        AddPath.bat                                               #
:#                                                                            #
:#  Description     Get/Set the environment PATH lists                        #
:#                                                                            #
:#  Notes 	    Version supporting Windows cmd.exe only.                  #
:#                                                                            #
:#                  Uses setx.exe for setting the machine and user default    #
:#                  paths. Setx.exe is standard in Vista and later, and is    #
:#                  available for Windows XP in the XP Resource Kit.          #
:#                  The major advantage of using setx over simply updating    #
:#                  the registry key, is that setx sends a WM_SETTINGCHANGE   #
:#                  broadcast to all Windows applications, ensuring that the  #
:#                  new path is immediately reloaded in all of them.          #
:#                                                                            #
:#  History                                                                   #
:#   1994-12-07 JFL Created this script for DOS and Windows 95.               #
:#   2012-12-11 JFL Added the -m option to get/set the Windows machine path.  #
:#                  Fixed a problem when the added path contains spaces.      #
:#   2012-12-13 JFL Bugfix: Some machines paths are in a REG_SZ. (Non EXPAND) #
:#   2013-04-04 JFL Added option -r to remove an entry in the PATH.           #
:#                  Added options -v, -V, and -X.                             #
:#                  Added option -b to insert an entry before another one.    #
:#                  Display the updated PATH in all cases.                    #
:#                  Added option -u to get/set the user's default path.       #
:#   2013-04-08 JFL Fallback to using reg.exe in the absence of setx.exe, and #
:#                  added warnings and comments explaining the drawbacks.     #
:#                  Fixed the -u option operation.                            #
:#                  Avoid adding a path that's already there.                 #
:#                  Renamed option -m as -s, and added option -m to move a p. #
:#   2015-01-13 JFL Find setx.exe wherever it is in the PATH.                 #
:#                  Bug fix: reg.exe option /f works only if it comes first.  #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2015-01-13"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
goto Main

:Help
echo.
echo %SCRIPT% version %VERSION% - Get/Set the environment PATH lists
echo.
echo Usage: AddPath [OPTIONS] [PATHS]
echo.
echo Options:
echo   -?         Display this help
echo   -b PATH    Insert PATHS just before PATH. Default: Append to the end
echo   -m PATH    Move PATH to the end of the path
echo   -p         Get/Set the local cmd.exe path. (Default if no -m or -u)
echo   -r PATH    Remove PATH from the path
echo   -s         Get/Set the system's default path. (Must run as administrator)
echo   -u         Get/Set the user's default path.
echo   -v         Verbose mode
echo   -V         Display the script version and exit
echo   -X         Display the command generated and exit
echo.
echo PATHS        PATH[;PATH...]   List of paths separated by semi-colons
echo                               Must be quoted if it contains spaces
echo              Default: Display the current path list items, one per line 
goto :eof

:#----------------------------------------------------------------------------#
:# Main routine

:Main
set "EXEC="
set "NOEXEC=0"
set "VERBOSE=0"
set "OBJECT=LocalPath"
set "MENVKEY=HKLM\System\CurrentControlSet\Control\Session Manager\Environment"
set "UENVKEY=HKCU\Environment"
set "METHOD=Echo"
set "VALUE="
set "BEFORE="
goto :get_args
:next_2nd_arg
shift
:next_arg
shift
:get_args
if .%1.==.. goto :go
if "%~1"=="/?" goto :Help
if "%~1"=="-?" goto :Help
if "%~1"=="-b" set "BEFORE=%~2" & shift & goto :next_arg
if "%~1"=="-d" set "METHOD=Remove" & set "VALUE=%~2" & goto :next_2nd_arg
if "%~1"=="-m" set "METHOD=Move" & set "VALUE=%~2" & goto :next_2nd_arg
if "%~1"=="-p" set "OBJECT=LocalPath" & goto :next_arg
if "%~1"=="-r" set "METHOD=Remove" & set "VALUE=%~2" & goto :next_2nd_arg
:# Note: The XP version of setx.exe requires the option -m or -M, but fails with /M. The Win7 version supports all.
if "%~1"=="-s" set "OBJECT=MasterPath" & set "SETXOPT=-M" & set "MKEY=%MENVKEY%" & set "OWNER=system" & goto :next_arg
if "%~1"=="-u" set "OBJECT=MasterPath" & set "SETXOPT=" & set "MKEY=%UENVKEY%" & set "OWNER=user" & goto :next_arg
if "%~1"=="-v" set "VERBOSE=1" & goto :next_arg
if "%~1"=="-V" (echo.%VERSION%) & goto :eof
if "%~1"=="-X" set "NOEXEC=1" & set "EXEC=echo" & goto :next_arg
set "ARG=%~1"
if "%ARG:~0,1%"=="-" (>&2 echo Error: Invalid switch %1) & goto :Help
:# Now rebuild a list with ; separators, as cmd.exe removes the ; 
if "%VALUE%"=="" set "METHOD=Add"
if not "%VALUE%"=="" set "VALUE=%VALUE%;"
set "VALUE=%VALUE%%~1"
goto :next_arg

:go
goto :%OBJECT%.%METHOD%

:#----------------------------------------------------------------------------#

:LocalPath.Echo
if "%VERBOSE%"=="1" echo :# Local cmd.exe PATH list items
:# Display path list items, one per line
echo.%PATH:;=&echo.%
goto :eof

:LocalPath.Add
if "%VALUE%"=="" goto :LocalPath.Echo
:# First check if the path to add was already there
set "Path2=;!Path!;"		&:# Make sure all paths have a ; on both sides
set "Path2=!Path2:;%VALUE%;=;!"	&:# Remove the requested value
set "Path2=!Path2:~1,-1!"	&:# Remove the extra ; we added above
:# If the path was not already there, add it now.
if "!Path2!"=="!Path!" (
  if "%BEFORE%"=="" (
    set "Path=%Path%;%VALUE%"	&:# Append the requested value at the end
    set "Path=!Path:;;=;!"	&:# Work around a common problem: A trailing ';'
    rem
  ) else (
    set "Path=;!Path:;;=;!;"	&:# Make sure all paths have one ; on both sides
    set "Path=!Path:;%BEFORE%;=;%VALUE%;%BEFORE%;!" &:# Insert the requested value
    set "Path=!Path:~1,-1!"	&:# Remove the extra ; we added above
    rem
  )
)
:LocalPath.Set
:# endlocal is necessary for returning the modified value back to the caller
endlocal & %EXEC% set "Path=%Path%" & if "%NOEXEC%"=="0" goto :LocalPath.Echo
goto :eof

:LocalPath.Remove1
set "Path=;!Path:;;=;!;"	&:# Make sure all paths have one ; on both sides
set "Path=!Path:;%VALUE%;=;!"	&:# Remove the requested value
set "Path=!Path:~1,-1!"		&:# Remove the extra ; we added above
goto :eof

:LocalPath.Remove
if "%VALUE%"=="" goto :LocalPath.Echo
call :LocalPath.Remove1
goto :LocalPath.Set

:LocalPath.Move
if "%VALUE%"=="" goto :LocalPath.Echo
call :LocalPath.Remove1
goto :LocalPath.Add

:#----------------------------------------------------------------------------#

:MasterPath.Get
setlocal
:# Note: The Path is usuallly in a REG_EXPAND_SZ, but sometimes in a REG_SZ. 
set MCMD=reg query "%MKEY%" /v "Path" 2^>NUL ^| findstr REG_
for /f "tokens=1,2,*" %%a in ('"%MCMD%"') do set "MPATH=%%c"
endlocal & set "MPATH=%MPATH%" & goto :eof

:MasterPath.Echo
if "%VERBOSE%"=="1" echo :# Global %OWNER% PATH list items
call :MasterPath.Get
:# Display path list items, one per line
if not "%MPATH%"=="" echo.%MPATH:;=&echo.%
goto :eof

:MasterPath.Add
if "%VALUE%"=="" goto :MasterPath.Echo
call :MasterPath.Get
:MasterPath.Add1
:# First check if the path to add was already there
set "MPATH2=;!MPATH!;"		&:# Make sure all paths have a ; on both sides
set "MPATH2=!MPATH2:;%VALUE%;=;!" &:# Remove the requested value
set "MPATH2=!MPATH2:~1,-1!"	&:# Remove the extra ; we added above
:# If the path was not already there, add it now.
if "!MPATH2!"=="!MPATH!" (
  if "%BEFORE%"=="" (
    if not "!MPATH!"=="" set "MPATH=!MPATH!;"
    set "MPATH=!MPATH!%VALUE%"	&:# Append the requested value at the end
    rem
  ) else (
    set "MPATH=;!MPATH!;"	&:# Make sure all paths have a ; on both sides
    set "MPATH=!MPATH:;%BEFORE%;=;%VALUE%;%BEFORE%;!" &:# Insert the requested value
    set "MPATH=!MPATH:~1,-1!"	&:# Remove the extra ; we added above
    rem
  )
)
:MasterPath.Set
set "SETX="
for /f %%i in ("setx.exe") do set "SETX=%%~$PATH:i"
if not "%SETX%" == "" ( :# If setx.exe is in the PATH, then use it. (Preferred)
  :# Gotcha: regex.exe does interpret a trailing \" as escaping the "
  if "!MPATH:~-1!"=="\" set "MPATH=!MPATH!\"
  :# setx.exe updates the path, and _does_ broadcast a WM_SETTINGCHANGE to all apps
:# Note: The XP version of setx.exe requires the option -m or -M, but fails with /M. The Win7 version supports all.
  set CMD=setx Path "!MPATH!" %SETXOPT%
) else ( :# Fallback to updating the registry value manually using reg.exe.
  :# reg.exe updates the path, but does _not_ broadcast a WM_SETTINGCHANGE to all apps
  :# Note: On XP, /f does not work if it is the last option.
  set CMD=reg add "%MKEY%" /f /v Path /d "%MPATH%"
  echo Warning: setx.exe is not available on this system.
  echo The %OWNER%'s default PATH update will only be visible after a reboot.
  echo Note: setx.exe is standard in Windows Vista and later versions.
  echo       A version for Windows XP is available in the XP Resource Kit.
)
if "%NOEXEC%"=="0" (	:# Normal execution mode
  :# Redirect the "SUCCESS: Specified value was saved." message to NUL.
  :# Errors, if any, will still be output on stderr.
  if "%VERBOSE%"=="1" echo :# %CMD%
  %CMD% >NUL
) else (		:# NoExec mode. Just echo the command to execute.
  echo %CMD%
)
if "%NOEXEC%"=="0" goto :MasterPath.Echo
goto :eof

:MasterPath.Remove1
call :MasterPath.Get
set "MPATH=;!MPATH:;;=;!;"	&:# Make sure all paths have one ; on both sides
set "MPATH=!MPATH:;%VALUE%;=;!"	&:# Remove the requested value
set "MPATH=!MPATH:~1,-1!"	&:# Remove the extra ; we added above
goto :eof

:MasterPath.Remove
if "%VALUE%"=="" goto :MasterPath.Echo
call :MasterPath.Remove1
goto :MasterPath.Set

:MasterPath.Move
if "%VALUE%"=="" goto :MasterPath.Echo
call :MasterPath.Remove1
goto :MasterPath.Add1

:#----------------------------------------------------------------------------#


