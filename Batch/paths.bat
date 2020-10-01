<# :# PowerShell comment block protecting the batch section
@echo off
:##############################################################################
:#                                                                            #
:#  Filename        paths.bat                                                 #
:#                                                                            #
:#  Description     Manage the PATH and similar environment variables         #
:#                                                                            #
:#  Notes 	    Version supporting Windows cmd.exe only.                  #
:#                                                                            #
:#                  Uses setx.exe for setting the machine and user default    #
:#                  paths. Setx.exe is standard in Vista and later, and is    #
:#                  available for Windows XP in the XP Resource Kit.          #
:#                  The major advantage of using setx over simply updating    #
:#                  the registry key, is that setx sends a WM_SETTINGCHANGE   #
:#                  broadcast to all Windows applications, ensuring that the  #
:#                  new PATH is used for new shells started from explorer.exe.#
:#                  Else it'd be available only after logging off and back on.#
:#                                                                            #
:#  History                                                                   #
:#   1994-12-07 JFL Created ADDPATH.BAT for DOS and Windows 95.		      #
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
:#   2018-12-21 JFL Added option -q for a quiet mode.                         #
:#   ------------------------------------------------------------------------ #
:#   2012-03-01 JFL Created paths.bat.					      #
:#   2019-08-14 JFL Added an optional variable name, the default being PATH.  #
:#   ------------------------------------------------------------------------ #
:#   2019-09-18 JFL Merged ADDPATH.BAT into paths.bat.			      #
:#                  Renamed option -l as -c.				      #
:#                  Added options -h, -l, -n, -t.			      #
:#                  Avoid duplications when adding multiple paths.	      #
:#                  Added support for moving and removing multiple paths.     #
:#   2019-12-11 JFL Avoid displaying empty lines when PATH ends with a ;      #
:#   2020-03-13 JFL Corrected one message and two comments.                   #
:#   2020-10-01 JFL Bug fix: Use reg.exe to set system paths for large PATHs, #
:#                  as setx.exe fails if the PATH is > 1KB. If using reg.exe, #
:#                  still use setx on another variable to broadcast the change.
:#                  Added PowerShell as an alternative broadcast mechanism.   #
:#                  							      #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2020-10-01"

set "SCRIPT=%~nx0"				&:# Script name
set "SNAME=%~n0"				&:# Script name, without the extension
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"	&:# Script path, without the trailing \
set "SFULL=%~f0"				&:# Script full pathname
set ^"ARG0=%0^"					&:# Script invokation name

set "ECHO=call :echo"
set "ECHO.D=call :noop"
set "ECHOVARS=call :EchoVars"
set "ECHOVARS.D=call :noop"
set "ECHOVAL=call :EchoVal"
set "ECHOVAL.D=call :noop"
goto :main

:Debug.on
set "DEBUG=1"
set "ECHO.D=call :Echo.d"
set "ECHOVARS.D=call :EchoVars.d"
set "ECHOVAL.D=call :EchoVal.d"
exit /b

:EchoVars.d
if not "%DEBUG%"=="1" exit /b
:EchoVars	%1=VARNAME %2=VARNAME %3=VARNAME ...
setlocal EnableExtensions EnableDelayedExpansion
:EchoVars.loop
if "%~1"=="" endlocal & goto :eof
%>DEBUGOUT% echo %INDENT%set "%~1=!%~1!"
if defined LOGFILE %>>LOGFILE% echo %INDENT%set "%~1=!%~1!"
shift
goto EchoVars.loop

:EchoVal.d
if not "%DEBUG%"=="1" exit /b
:EchoValue	%1=VARNAME
setlocal EnableExtensions EnableDelayedExpansion
%>DEBUGOUT% echo.%INDENT%!%~1!
if defined LOGFILE %>>LOGFILE% echo %INDENT%!%~1!
exit /b

:Echo.d
if not "%DEBUG%"=="1" exit /b
:Echo
echo %*
:noop
exit /b

:#----------------------------------------------------------------------------#

:Help
echo.
echo %SCRIPT% version %VERSION% - Manage the PATH ^& similar environment variables
echo.
echo Usage: %SNAME% [PATHVAR] [OPTIONS] [PATHS]
echo.
echo PATHVAR:     Paths environment variable name. Default: PATH
echo.
echo Options:
echo   -?         Display this help
echo   -a PATHS   Add PATHS to PATHVAR
echo   -b PATH    Insert PATHS just before PATH. Default: Append them to the end
echo   -c         Manage the local cmd.exe path. (Default if no -s or -u)
echo   -h         Put PATHS at the head of PATHVAR
echo   -l         List paths, one per line. (Default)
echo   -m PATHS   Move PATHS within PATHVAR. Default: Move them to the end
echo   -n PATHVAR Paths environment variable name. Default: PATH
echo   -q         Quiet mode: Don't display the updated path list in the end
echo   -r PATHS   Remove PATHS from PATHVAR
echo   -s         Manage the system's default PATHVAR. (Must run as administrator)
echo   -t         Put PATHS at the tail of PATHVAR. (Default)
echo   -u         Manage the user's default PATHVAR
echo   -v         Verbose mode
echo   -V         Display the script version and exit
echo   -X         Display the command generated and exit
echo.
echo PATHS:       PATH[;PATH...]   List of paths separated by semi-colons or spaces
echo                               Must be quoted if any PATH contains spaces
exit /b

:#----------------------------------------------------------------------------#
:# Main routine

:Main
set "EXEC="
set "NOEXEC=0"
set "VERBOSE=0"
set "QUIET=0"
set "OBJECT=LocalPath"
set "MENVKEY=HKLM\System\CurrentControlSet\Control\Session Manager\Environment"
set "UENVKEY=HKCU\Environment"
set "METHOD=Echo"
set "VALUE="
set "BEFORE="
set "PATHVAR="
set "WHERE=tail"

goto :get_args
:next_2nd_arg
shift
:next_arg
shift
:get_args
if .%1.==.. goto :go
if "%~1"=="/?" goto :Help
if "%~1"=="-?" goto :Help
if "%~1"=="-a" set "METHOD=Add" & set "VALUE=%~2" & goto :next_2nd_arg
if "%~1"=="-b" set "WHERE=before" & set "BEFORE=%~2" & goto :next_2nd_arg
if "%~1"=="-c" set "OBJECT=LocalPath" & goto :next_arg
if "%~1"=="-d" call :Debug.on & goto :next_arg
if "%~1"=="-h" set "WHERE=head" & goto :next_arg
if "%~1"=="-l" set "METHOD=Echo" & goto :next_arg
if "%~1"=="-m" set "METHOD=Move" & set "VALUE=%~2" & goto :next_2nd_arg
if "%~1"=="-n" set "PATHVAR=%~2" & goto :next_2nd_arg
if "%~1"=="-q" set "QUIET=1" & goto :next_arg
if "%~1"=="-r" set "METHOD=Remove" & set "VALUE=%~2" & goto :next_2nd_arg
:# Note: The XP version of setx.exe requires the option -m or -M, but fails with /M. The Win7 version supports all.
if "%~1"=="-s" set "OBJECT=MasterPath" & set "SETXOPT=-M" & set "MKEY=%MENVKEY%" & set "OWNER=system" & goto :next_arg
if "%~1"=="-t" set "WHERE=tail" & goto :next_arg
if "%~1"=="-tb" goto :TestBroadcast
if "%~1"=="-u" set "OBJECT=MasterPath" & set "SETXOPT=" & set "MKEY=%UENVKEY%" & set "OWNER=user" & goto :next_arg
if "%~1"=="-v" set "VERBOSE=1" & goto :next_arg
if "%~1"=="-V" (echo.%VERSION%) & exit /b
if "%~1"=="-X" set "NOEXEC=1" & set "EXEC=echo" & goto :next_arg
set "ARG=%~1"
if "%ARG:~0,1%"=="-" (>&2 echo Error: Invalid switch %1) & goto :Help
if not defined PATHVAR if "%METHOD%"=="Echo" set "PATHVAR=%ARG%" & goto :next_arg
:# Now rebuild a list with ; separators, as cmd.exe removes the ; 
if not defined VALUE set "METHOD=Add"
if defined VALUE set "VALUE=%VALUE%;"
set "VALUE=%VALUE%%~1"
goto :next_arg

:go
if not defined PATHVAR set "PATHVAR=PATH"
goto :%OBJECT%.%METHOD%

:#----------------------------------------------------------------------------#
:#                      Manage paths in the local shell                       #
:#----------------------------------------------------------------------------#

:LocalPath.Echo %1=Optional variable name. Default=PATHVAR
setlocal EnableExtensions EnableDelayedExpansion
if not "%~1"=="" set "PATHVAR=%~1"
if "%VERBOSE%"=="1" echo :# Local cmd.exe %PATHVAR% list items
:# Display path list items, one per line
set "VALUE=!%PATHVAR%!"
echo.%VALUE:;=&echo.%
endlocal
exit /b

:LocalPath.Add1 %1=path to add to %PATHVAR%
setlocal EnableExtensions EnableDelayedExpansion
%ECHO.D% call %0 %*
:# First check if the path to add was already there
set "VALUE=!%PATHVAR%:;;=;!"	&:# The initial paths list value
set "VALUE2=;!VALUE!;"		&:# Make sure all paths have a ; on both sides
set "VALUE2=!VALUE2:;%~1;=;!"	&:# Remove the requested value
set "VALUE2=!VALUE2:~1,-1!"	&:# Remove the extra ; we added above
:# If the path was not already there, add it now.
if "!VALUE2!"=="!VALUE!" (
  if "%WHERE%"=="tail" ( :# Append the requested value at the end
    if defined VALUE set "VALUE=!VALUE!;"
    set "VALUE=!VALUE!%~1"	&:# Append the requested value at the end
    set "VALUE=!VALUE:;;=;!"	&:# Work around a common problem: A trailing ';'
    rem
  ) else (		  :# Insert it before the specified entry
    set "VALUE=;!VALUE:;;=;!;"	&:# Make sure all paths have one ; on both sides
    set "VALUE=!VALUE:;%BEFORE%;=;%~1;%BEFORE%;!" &:# Insert the requested value
    set "VALUE=!VALUE:~1,-1!"	&:# Remove the extra ; we added above
    rem
  )
)
endlocal & set "%PATHVAR%=%VALUE%"
exit /b

:# Change WHERE==head to WHERE=before to preserve multiple paths ordering
:UpdateWhere %1=PATHS value
if "%WHERE%"=="head" (
  if not "%~1"=="" (
    for /f "tokens=1 delims=;" %%p in ("%~1") do (
      set "BEFORE=%%~p"
      set "WHERE=before"
    )
  ) else ( :# The PATH is empty, so head==tail
      set "WHERE=tail"
  )
)
exit /b

:LocalPath.Add
if not defined VALUE goto :LocalPath.Echo
call :UpdateWhere "!%PATHVAR%!" &:# Change WHERE=head to WHERE=before to preserve multiple paths ordering
for %%p in ("!VALUE:;=" "!") do ( :# For each individual path to add
  call :LocalPath.Add1 "%%~p"
)
:# goto :LocalPath.Set

:LocalPath.Set
:# endlocal is necessary for returning the modified value back to the caller
set "VALUE=!%PATHVAR%!"
endlocal & %EXEC% set "%PATHVAR%=%VALUE%" & if "%NOEXEC%"=="0" if "%QUIET%"=="0" call :LocalPath.Echo %PATHVAR%
exit /b

:LocalPath.Remove1 %1=path to remove from %PATHVAR%
setlocal EnableExtensions EnableDelayedExpansion
%ECHO.D% call %0 %*
set "VALUE=;!%PATHVAR%:;;=;!;"	&:# Make sure all paths have one ; on both sides
set "VALUE=!VALUE:;%~1;=;!"	&:# Remove the requested value
set "VALUE=!VALUE:~1,-1!"	&:# Remove the extra ; we added above
endlocal & set "%PATHVAR%=%VALUE%"
exit /b

:LocalPath.Remove
if not defined VALUE goto :LocalPath.Echo
call :UpdateWhere "!%PATHVAR%!" &:# Change WHERE=head to WHERE=before to preserve multiple paths ordering
for %%p in ("!VALUE:;=" "!") do ( :# For each individual path to remove
  call :LocalPath.Remove1 "%%~p"
)
goto :LocalPath.Set

:LocalPath.Move
if not defined VALUE goto :LocalPath.Echo
call :UpdateWhere "!%PATHVAR%!" &:# Change WHERE=head to WHERE=before to preserve multiple paths ordering
for %%p in ("!VALUE:;=" "!") do ( :# For each individual path to move
  call :LocalPath.Remove1 "%%~p"
  call :LocalPath.Add1 "%%~p"
)
goto :LocalPath.Set

:#----------------------------------------------------------------------------#
:#                  Manage persistent user and system paths                   #
:#----------------------------------------------------------------------------#

:MasterPath.Get
setlocal EnableExtensions DisableDelayedExpansion
:# Note: The Path is usuallly in a REG_EXPAND_SZ, but sometimes in a REG_SZ. 
set MCMD=reg query "%MKEY%" /v "%PATHVAR%" 2^>NUL ^| findstr REG_
%ECHOVAL.D% MCMD
for /f "tokens=1,2,*" %%a in ('"%MCMD%"') do set "MPATH=%%c"
:MasterPath.Get.TrimR
if "%MPATH:~-1%"==";" set "MPATH=%MPATH:~0,-1%" & goto :MasterPath.Get.TrimR
endlocal & set "MPATH=%MPATH%" & exit /b

:MasterPath.Echo
if "%VERBOSE%"=="1" echo :# Global %OWNER% PATH list items
call :MasterPath.Get
:# Display path list items, one per line
if defined MPATH echo.%MPATH:;=&echo.%
exit /b

:MasterPath.Add1 %1=path to add to MPATH
:# First check if the path to add was already there
set "MPATH2=;!MPATH!;"		&:# Make sure all paths have a ; on both sides
set "MPATH2=!MPATH2:;%~1;=;!"	&:# Remove the requested value
set "MPATH2=!MPATH2:~1,-1!"	&:# Remove the extra ; we added above
:# If the path was not already there, add it now.
if "!MPATH2!"=="!MPATH!" (
  if "%WHERE%"=="tail" ( :# Append the requested value at the end
    if defined MPATH set "MPATH=!MPATH!;"
    set "MPATH=!MPATH!%~1"	&:# Append the requested value at the end
    set "MPATH=!MPATH:;;=;!"	&:# Work around a common problem: A trailing ';'
    rem
  ) else (		  :# Insert it before the specified entry
    set "MPATH=;!MPATH!;"	&:# Make sure all paths have a ; on both sides
    set "MPATH=!MPATH:;%BEFORE%;=;%~1;%BEFORE%;!" &:# Insert the requested value
    set "MPATH=!MPATH:~1,-1!"	&:# Remove the extra ; we added above
    rem
  )
)
exit /b

:MasterPath.Add
if not defined VALUE goto :MasterPath.Echo
call :MasterPath.Get
call :UpdateWhere "!MPATH!" &:# Change WHERE=head to WHERE=before to preserve multiple paths ordering
for %%p in ("!VALUE:;=" "!") do ( :# For each individual path to add
  call :MasterPath.Add1 "%%~p"
)
:# goto :MasterPath.Set

:MasterPath.Set
set "MORE_THAN_1KB=!MPATH:~1024!" &:# Defined if the new PATH is larger than 1 KB
:# Gotcha: reg.exe and setx.exe interpret a trailing \" as escaping the "
if "!MPATH:~-1!"=="\" set "MPATH=!MPATH!\"
set "CMD="
set "NEED_BROADCAST=0"
for /f %%i in ("setx.exe") do set "SETX=%%~$PATH:i"
if not defined MORE_THAN_1KB ( :# If the PATH is less than 1KB long, then try using setx
  if defined SETX ( :# If setx.exe is in the PATH, then use it. (Preferred if within the 1KB limit)
    :# setx.exe updates the %PATHVAR%, and _does_ broadcast a WM_SETTINGCHANGE to all apps
    :# Note: The XP version of setx.exe requires the option -m or -M, but fails with /M. The Win7 version supports all.
    set ^"CMD=setx %PATHVAR% "!MPATH!" %SETXOPT%^"
  )
)  
if not defined CMD ( :# Fallback to updating the registry value manually using reg.exe.
  :# reg.exe updates the %PATHVAR%, but does _not_ broadcast a WM_SETTINGCHANGE to all apps
  :# Note: On XP, /f does not work if it is the last option.
  set ^"CMD=reg add "%MKEY%" /f /v %PATHVAR% /d "%MPATH%"^"
  set "NEED_BROADCAST=1"
)
if "%NOEXEC%"=="0" (	:# Normal execution mode
  :# Redirect the "SUCCESS: Specified value was saved." message to NUL.
  :# Errors, if any, will still be output on stderr.
  if "%VERBOSE%"=="1" echo :# %CMD%
  %CMD% >NUL
) else (		:# NoExec mode. Just echo the command to execute.
  echo %CMD%
)
:# If the update was done with reg.exe, we need to tell Windows Explorer about the change
if "%NEED_BROADCAST%"=="1" (
  :# Find a way to broadcast a WM_SETTINGCHANGE message
  set "CMD="
  if defined SETX ( :# If setx.exe is in the PATH, then use it. (Preferred, as this is faster)
    :# setx.exe updates _does_ broadcast a WM_SETTINGCHANGE to all apps
    :# Note: The XP version of setx.exe requires the option -m or -M, but fails with /M. The Win7 version supports all.
    :# Change a system variable, for both cases of the user or system PATHs.
    set "VAR=PROCESSOR_ARCHITECTURE" &:# This system variable is always short, and is unlikely to ever change
    set ^"CMD=setx !VAR! -k "%MENVKEY%\!VAR!" -m^"
  ) else ( :# If powershell.exe is in the PATH, then use it to run the PS routine at the send of this script. (Slower)
    for /f %%i in ("powershell.exe") do set "POWERSHELL=%%~$PATH:i"
    if defined POWERSHELL set ^"CMD=powershell -c "Invoke-Expression $([System.IO.File]::ReadAllText('%SFULL%'))"^"
  )
  if defined CMD (
    if "%NOEXEC%"=="0" (	:# Normal execution mode
      :# Redirect the "SUCCESS: Specified value was saved." message to NUL.
      :# Errors, if any, will still be output on stderr.
      if "%VERBOSE%"=="1" echo :# !CMD!   ^&:# Broadcast a WM_SETTINGCHANGE message
      !CMD! >NUL
    ) else (		:# NoExec mode. Just echo the command to execute.
      echo !CMD!   ^&:# Broadcast a WM_SETTINGCHANGE message
    )
  ) else if "%QUIET%"=="0" ( :# Only happens in Windows XP or Windows 7 without setx.exe and PowerShell
    echo Warning: Could not find a way to broadcast the change to other applications.
    echo New shells will only have the updated PATH after you log off and log back in.
  )
)
if "%NOEXEC%"=="0" if "%QUIET%"=="0" goto :MasterPath.Echo
exit /b

:MasterPath.Remove1 %1=path to remove from MPATH
set "MPATH=;!MPATH:;;=;!;"	&:# Make sure all paths have one ; on both sides
set "MPATH=!MPATH:;%~1;=;!"	&:# Remove the requested value
set "MPATH=!MPATH:~1,-1!"	&:# Remove the extra ; we added above
exit /b

:MasterPath.Remove
if not defined VALUE goto :MasterPath.Echo
call :MasterPath.Get
call :UpdateWhere "!MPATH!" &:# Change WHERE=head to WHERE=before to preserve multiple paths ordering
for %%p in ("!VALUE:;=" "!") do ( :# For each individual path to remove
  call :MasterPath.Remove1 "%%~p"
)
goto :MasterPath.Set

:MasterPath.Move
if not defined VALUE goto :MasterPath.Echo
call :MasterPath.Get
call :UpdateWhere "!MPATH!" &:# Change WHERE=head to WHERE=before to preserve multiple paths ordering
for %%p in ("!VALUE:;=" "!") do ( :# For each individual path to move
  call :MasterPath.Remove1 "%%~p"
  call :MasterPath.Add1 "%%~p"
)
goto :MasterPath.Set

:#----------------------------------------------------------------------------#
:#           Broadcast a WM_SETTINGCHANGE message using PowerShell            #
:#----------------------------------------------------------------------------#

:# Test the WM_SETTINGCHANGE broadcast
:TestBroadcast
:# Run a second instance of this script in PowerShell, to do the broadcast
:# This first command is simpler, but does not work in all versions of PowerShell
:# type %ARG0% | PowerShell -c -
:# Instead, use this one that works from Win7/PSv2 to Win10/PSv5
PowerShell -c "Invoke-Expression $([System.IO.File]::ReadAllText('%SFULL%'))"
exit /b

:# End of the PowerShell comment section protecting the batch section.
#>

# PowerShell subroutine called from the batch setup routine

# Redefine the colors for a few message types
$colors = (Get-Host).PrivateData
if ($colors) { # Exists for ConsoleHost, but not for ServerRemoteHost
  $colors.VerboseForegroundColor = "white"	# Less intrusive than the default yellow
  $colors.DebugForegroundColor = "cyan"		# Distinguish debug messages from yellow warning messages
}

# Inherit the DEBUG mode from the batch instance
if ($env:DEBUG -eq 1) {
  $Debug = $true
  $DebugPreference = "Continue"		# Make sure Write-Debug works
} else {
  $Debug = $false
  $DebugPreference = "SilentlyContinue"	# Write-Debug does nothing
}

Write-Debug "# This is PowerShell, broadcasting WM_SETTINGCHANGE for the Environment"

# import sendmessagetimeout from win32
add-type -Namespace Win32 -Name NativeMethods -MemberDefinition @"
  [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
  public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
    uint fuFlags, uint uTimeout, out UIntPtr lpdwResult
  );
"@

function Invoke-WMSettingsChanged {
  $HWND_BROADCAST = [intptr]0xffff;
  $WM_SETTINGCHANGE = 0x1a;
  $result = [uintptr]::zero
  # notify all windows of environment block change
  $done = [win32.nativemethods]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE,
	    [uintptr]::Zero, "Environment", 2, 5000, [ref]$result);
  return $done # 0=Failed. Check GetLastError(); !0=Success
}

$done = Invoke-WMSettingsChanged

return
