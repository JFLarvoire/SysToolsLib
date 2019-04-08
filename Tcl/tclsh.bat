@echo off & goto :init_batch
<# PowerShell comment block protecting the batch section
:#----------------------------------------------------------------------------#
:#                                                                            #
:#  File name       tclsh.bat                                                 #
:#                                                                            #
:#  Description     Run the most recent tcl shell available                   #
:#                                                                            #
:#  Note            Work around shortcomings of ActiveTCL distributions,      #
:#                  which do not set environment variables correctly for      #
:#                  running console-based scripts with tclsh.                 #
:#                  Also after updates, the interpreter name changes, like    #
:#                  from tclsh84.exe to tclsh85.exe.                          #
:#                                                                            #
:#                  Run "tclsh -?" to get a list of available options.        #
:#                                                                            #
:#  History                                                                   #
:#   2010-04-02 JFL Created this script.                                      #
:#   2011-08-17 JFL Explicitely enable CMD extensions and delayed expansion.  #
:#   2011-09-06 JFL Setup now associates .tcl->tclsh and .tk->wish.           #
:#                  Added a -t option to test setup without changing it.      #
:#   2012-04-23 JFL Setup now updates environment variable PATHEXT if needed. #
:#   2012-04-24 JFL Added a PowerShell routine to notify all windows of the   #
:#                  environment block change.                                 #
:#   2014-05-13 JFL Fixed the setting of variable PATHEXT.		      #
:#   2014-06-23 JFL Make sure the local path includes the Tcl's bin directory.#
:#                  Bug fix: Avoid adding trailing spaces to variable PATHEXT.#
:#   2018-11-19 JFL Use the improved FindTclsh routine from TclSetup.bat.     #
:#                  Removed options -s and -t.                                #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:#----------------------------------------------------------------------------#

:init_batch
setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2018-11-19"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"

set VERBOSE=
set DEBUG=0
set NOEXEC=0

set ARG0=%0
set RUN=call :Run
set RETURN=exit /b

:# FOREACHLINE macro. (Change delimiter to none to catch the whole lines.)
set FOREACHLINE=for /f "delims=" 

:# List of drives where to search for tclsh*.exe.
set SEARCHDRIVES=C U

goto Main

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Run                                                       #
:#                                                                            #
:#  Description     Run a command, possibly echoing it.                       #
:#                                                                            #
:#  Arguments       %1 = String to output.                                    #
:#                                                                            #
:#  Note            If invoked at script scope by a goto :Run, then the       #
:#                  script exits with the error code of the command executed. #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-19 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Run
if "%VERBOSE%"=="-v" echo>con %*
if "%NOEXEC%"=="0" %*
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        EnableExpansion                                           #
:#                                                                            #
:#  Description     Test if cmd.exe delayed variable expansion can be enabled #
:#                                                                            #
:#  Returns         %ERRORLEVEL% == 0 if success, else error.                 #
:#                                                                            #
:#  Note            Allows testing if enabling delayed expansion works.       #
:#                  But, contrary to what I thought when I created the        #
:#		    routine, the effect does not survive the return.          #
:#                  So this routine CANNOT be used to enable variable         #
:#                  expansion.                                                #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-31 JFL Created this routine.                                     #
:#   2014-05-13 JFL Only do a single setlocal.                                #
:#                  Tested various return methods, but none of them preserves #
:#                  the expansion state changed inside.                       #
:#                                                                            #
:#----------------------------------------------------------------------------#

:EnableExpansion
:# Enable command extensions
verify other 2>nul
setlocal EnableExtensions
if errorlevel 1 (
  >&2 echo Error: Unable to enable cmd.exe command extensions.
  >&2 echo Please restart your cmd.exe shell with the /E:ON option,
  >&2 echo or set HKLM\Software\Microsoft\Command Processor\EnableExtensions=1
  >&2 echo or set HKCU\Software\Microsoft\Command Processor\EnableExtensions=1
  exit /b 1
)
endlocal &:# Disable that first setting, as we do another setlocal just below.
:# Enable delayed variable expansion
verify other 2>nul
setlocal EnableExtensions EnableDelayedExpansion
if errorlevel 1 (
  :EnableExpansionFailed
  >&2 echo Error: Unable to enable cmd.exe delayed variable expansion.
  >&2 echo Please restart your cmd.exe shell with the /V option,
  >&2 echo or set HKLM\Software\Microsoft\Command Processor\DelayedExpansion=1
  >&2 echo or set HKCU\Software\Microsoft\Command Processor\DelayedExpansion=1
  exit /b 1
)
:# Check if delayed variable expansion works now 
set VAR=before
if "%VAR%" == "before" (
  set VAR=after
  if not "!VAR!" == "after" goto :EnableExpansionFailed
)
:# Success
exit /b 0

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        FindTclsh                                                 #
:#                                                                            #
:#  Description     Find the latest tclsh.exe in %SEARCHDRIVES%               #
:#                                                                            #
:#  Note                                                                      #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-31 JFL Created this routine.                                     #
:#   2018-10-02 JFL Search in the PATH first.                                 #
:#                  Then search in both Tcl and ActiveTcl subdirectories.     #
:#                  And skip threaded versions, like tclsh86t.exe             #
:#                  Return errorlevel 0 in case of success, else errorlevel 1.#
:#                                                                            #
:#----------------------------------------------------------------------------#

:FindTclsh  %1=VARNAME. Default=Display all
setlocal EnableExtensions EnableDelayedExpansion

set "RETVAR=%~1"		&:# Variable where to store the result
set "RETVAL=0"			&:# Assume success
set "TCLEXE="

:# First search it in the PATH
if "%DEBUG%"=="1" echo :# Searching in %%PATH%%
for %%p in (tclsh.exe) do set "TCLEXE=%%~$PATH:p"
if defined TCLEXE goto :FindTclsh.Done

:# If the PATH has not been set (which is optional), then scan likely places.
:# Search in the list of drives, then in the possible \Tcl program directories.
for %%d in (%SEARCHDRIVES%) do (
  for %%p in ("" "\Program Files" "\Program Files (x86)") do (
    for %%s in ("ActiveTcl" "Tcl") do (
      set TCLBIN=%%d:%%~p\%%~s\bin
      if "%DEBUG%"=="1" echo :# Searching in !TCLBIN!
      if exist "!TCLBIN!" (
	if exist "!TCLBIN!\tclsh.exe" (
	  set TCLEXE=!TCLBIN!\tclsh.exe
	  goto :FindTclsh.Done
	)
	:# In some cases, there are several tclsh.exe versions side-by-side.
	:# Assume that the alphabetic order will be the same as the version order.
	:# So the last listed executable will be the most recent one.
	:# Skip threaded versions, like tclsh86t.exe.
	%FOREACHLINE% %%L in ('cmd /c "dir /b /on "!TCLBIN!\tclsh*.exe" 2^>NUL ^| findstr /V t.exe"') do set TCLEXE=!TCLBIN!\%%L
	if defined TCLEXE goto :FindTclsh.Done
      )
    )
  )
)

:# Failed to find tclsh.exe
%RETURN% 1

:FindTclsh.Done
if defined TCLEXE (
  if defined RETVAR (
    if "%DEBUG%"=="1" echo set "%RETVAR%=%TCLEXE%"
    endlocal & set "%RETVAR%=%TCLEXE%"
  ) else (
    echo !TCLEXE!
    endlocal
  )
)
%RETURN% 0

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Main                                                      #
:#                                                                            #
:#  Description     Main routine                                              #
:#                                                                            #
:#  Note                                                                      #
:#                                                                            #
:#  History                                                                   #
:#   2010-04-02 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Help
echo.
echo Usage: tclsh [options] [command]
echo.
echo Options:
echo   -?        Display this help
echo   -f        Find the latest tclsh*.exe, and display its pathname
goto :EOF

:Main
goto getarg

:# Process the command line options
:nextarg
shift
:getarg
if .%1.==.. goto Start
if .%1.==.-?. goto Help
if .%1.==./?. goto Help
if %1==--call goto Call
if %1==-d set "DEBUG=1" & goto nextarg
if %1==-f call :FindTclsh & exit /b
if %1==-t echo Please use TclSetup.bat instead & exit /b 1
if %1==-s echo Please use TclSetup.bat instead & exit /b 1
if %1==-v set "VERBOSE=-v" & goto nextarg
if %1==-V (echo %VERSION%) & goto :eof
if %1==-X set "NOEXEC=1" & goto nextarg
goto start

:# Mechanism for calling a subroutine from the command line
:Call
shift
set PROC=%1
shift
call :%PROC% %1 %2 %3 %4 %5 %6 %7 %8 %9
%RETURN%

:# Locate and start the Tcl interpreter
:Start
set "TCLSH="
call :FindTclsh TCLSH
if not defined TCLSH (
  >&2 echo Failed. No tclsh interpreter found.
  exit /b 1
)
:# Start it
"%TCLSH%" %*
exit /b
:# End of the PowerShell comment section protecting the batch section.
#>

# PowerShell subroutine called from the batch setup routine

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
  [win32.nativemethods]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE,
	    [uintptr]::Zero, "Environment", 2, 5000, [ref]$result);
}

Invoke-WMSettingsChanged

