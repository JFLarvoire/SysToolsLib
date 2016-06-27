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
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:#----------------------------------------------------------------------------#

:init_batch
setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2014-06-23"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"

set VERBOSE=
set DEBUG=0
set NOEXEC=0

set ARG0=%0
set RUN=call :Run
set RUNX=%COMSPEC% /c %ARG0% %VERBOSE% --call Run
set RETURN=goto :eof

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
:#  Description     Find the latest tclsh.exe                                 #
:#                                                                            #
:#  Note                                                                      #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-31 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:FindTclsh

:# Check prerequisites
call :EnableExpansion
if errorlevel 1 exit /b 1

:# Search in the list of drives, then in the possible \Tcl program directories.
set TCLEXE=
for %%d in (%SEARCHDRIVES%) do (
  for %%p in ("" "\Program Files" "\Program Files (x86)") do (
    set TCLBIN=%%d:%%~p\Tcl\bin
    if .%DEBUG%.==.1. echo Looking in !TCLBIN!
    :# Assume that the alphabetic order will be the same as the version order.
    :# So the last listed executable will be the most recent one.
    %FOREACHLINE% %%L in ('cmd /c "dir /b /on "!TCLBIN!\tclsh*.exe" 2>NUL"') do set TCLEXE=!TCLBIN!\%%L
    if not "!TCLEXE!"=="" (
      echo !TCLEXE!
      goto :EOF
    )
  )
)

%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Setup                                                     #
:#                                                                            #
:#  Description     Setup Windows for running *.tcl with the latest tclsh.exe #
:#                                                                            #
:#  Note                                                                      #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-31 JFL Created this routine.                                     #
:#   2011-09-06 JFL Setup now associates .tcl->tclsh and .tk->wish.           #
:#                  Added a test mode distinct from the setup mode.           #
:#   2012-04-23 JFL Setup now updates environment variable PATHEXT if needed. #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Setup
:# First check administrator rights
ren %SystemRoot%\win.ini win.ini >NUL 2>&1
if errorlevel 1 (
  >&2 echo Error: This must be run as Administrator.
  exit /b 1
)
call :DoSetup setup
endlocal & set "PATHEXT=%PATHEXT%" & set "PATH=%PATH%"
:# echo set "PATHEXT=%PATHEXT%"
%RETURN%

:TestSetup
call :DoSetup test
echo.
if .%NEEDSETUP%.==.0. (
  echo The setup is good.
) else (
  echo The setup must be updated.
)
%RETURN%

:# Common routine for test and setup. %1=test or %1=setup
:DoSetup
setlocal
set MODE=%1

:# Locate the latest Tcl shell
set TCLSH=""
for /f "delims=" %%p in ('call %ARG0% -f') do set TCLSH="%%p"
echo.
if %TCLSH%=="" (
  echo>&2 Failure. No Tcl shell found.
  exit /b 1
)
echo The Tcl shell is: %TCLSH%

:# First configure the Tcl text mode interpreter
echo.
set TCLCMD=%TCLSH% "%%1" %%*
call :Test1Setup tcl TCLSH TCLCMD TclScript
set NEEDSETUP1=%NEEDSETUP%

:# Repeat operation for the Tcl/Tk graphical mode interpreter
echo.
set WISH=%TCLSH:tclsh=wish%
set WISHCMD=%WISH% "%%1" %%*
call :Test1Setup tk WISH WISHCMD TclTkScript
set NEEDSETUP2=%NEEDSETUP%

:# Make sure the local PATH includes the Tcl's bin directory.
:# (It is set globally by ActiveTcl's setup, but not locally in each open cmd window.)
set NEEDSETUP=0
for %%B in (%TCLSH%) do set "TCLBIN=%%~dpB" &:# Deduce the Tcl bin path
:# Remove the trailing \ in that path
set "TCLBIN=%TCLBIN%::" &:# Note that :: cannot appear in a pathname
set "TCLBIN=%TCLBIN:\::=::%"
set "TCLBIN=%TCLBIN:::=%"
:# Now check if that path is present in the local PATH
set "PATH1=;%PATH%;"
set "PATH2=!PATH1:;%TCLBIN%;=;!"
if "%PATH1%"=="%PATH2%" (
  echo.
  if %MODE%==test (
    echo Error: The directory "%TCLBIN%" is missing in the local PATH.
    set NEEDSETUP=1
  )
  if %MODE%==setup (
    echo Adding "%TCLBIN%" to the local PATH
    set "PATH=%TCLBIN%;%PATH%"
  )
)
set NEEDSETUP3=%NEEDSETUP%

set /A NEEDSETUP=%NEEDSETUP1% ^| %NEEDSETUP2% ^| %NEEDSETUP3%
endlocal & set "NEEDSETUP=%NEEDSETUP%" & set "PATHEXT=%PATHEXT%" & set "PATH=%PATH%"
%RETURN%

:Test1Setup
setlocal
set EXT=%1
call set SH=%%%2%%
call set CMD=%%%3%%
set DEFCLASS=%4
set NEEDSETUP=0

echo Expecting .%EXT% script startup command: %CMD%

:# Check the class associated with the .%EXT% extension
set CLASS=
for /f "delims== tokens=2" %%c in ('%RUNX% assoc .%EXT%') do set CLASS=%%c
:# In case of error, displays: "File association not found for extension .%EXT%"
:# Create one if needed
if %MODE%==setup if .%CLASS%.==.. (
  set CLASS=%DEFCLASS%
  echo Associating it with: !CLASS!
  %RUN% assoc .%EXT%=!CLASS!
)

:# Check the open command associated with the class.
:# (Stored in HKEY_CLASSES_ROOT\%CLASS%\shell\open\command)
set CMD2=""
for /f "delims== tokens=2" %%c in ('%RUNX% ftype %CLASS% 2^>NUL') do set CMD2=%%c
if not .%CLASS%.==.. (
  echo The .%EXT% extension is associated with class: "%CLASS%"
  echo The open command for class %CLASS% is: %CMD2%
  :# Note: Replace " with '' to make it a single string for the if command parser.
  if not "%CMD2:"=''%"=="%CMD:"=''%" (
    if %MODE%==test (
      echo Error: Command mismatch.
      set NEEDSETUP=1
    )
    if %MODE%==setup (
      echo Setting it to the expected command.
      :# Note: Using delayed expansion to avoid issues with CMD paths containing parentheses.
      :# Note: Don't use %RUN% because it expands %1 and %*
      set CHGCMD=ftype %CLASS%=!CMD!
      if "%VERBOSE%"=="-v" echo !CHGCMD!
      if "%NOEXEC%"=="0" (
      	!CHGCMD!
        call :CheckError %ERRORLEVEL%
      )
    )
  )
) else (
  :# This can only happen in test mode.
  echo Error: No class associated with this extension.
  set NEEDSETUP=1
)

:# Check the open command associated with the application
:# (Stored in HKEY_CLASSES_ROOT\Applications\%EXE%\shell\open\command)
for %%P in (%SH%) do set EXE=%%~nxP
set KEY="HKEY_CLASSES_ROOT\Applications\%EXE%\shell\open\command"
set CMD3=""
for /f "skip=2 tokens=2,*" %%A in ('%RUNX% reg query %KEY% 2^>NUL') do set CMD3=%%B
call :CheckError %ERRORLEVEL%
echo The open command for application %EXE% is: %CMD3%
:# Note: Replace " with '' to make it a single string for the if command parser.
set EMPTY=""
if "%CMD3:"=''%"=="%EMPTY:"=''%" (
  echo Undefined, which is OK as this definition is optional.
) else if not "%CMD3:"=''%"=="%CMD:"=''%" (
  if %MODE%==test (
    echo Error: Command mismatch.
    set NEEDSETUP=1
  )
  if %MODE%==setup (
    echo Setting it to the expected command.
    :# Note: Using delayed expansion to avoid issues with TCLCMD paths containing parentheses.  
    :# Note: Don't use %RUN% because it expands %1 and %*
    set CHGCMD=reg add %KEY% /f /t REG_SZ /d "!CMD:"=\"!"
    if "%VERBOSE%"=="-v" echo !CHGCMD!
    if "%NOEXEC%"=="0" (
      !CHGCMD!
      call :CheckError %ERRORLEVEL%
    )
  )
)

:# Check the PATHEXT variable
:# 1) The global variable in the registry
set "KEY=HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment"
for /f "tokens=3" %%p in ('reg query "%KEY%" /v PATHEXT ^| findstr REG_SZ') do @set PE=%%p
echo The global environment variable PATHEXT is: %PE%
set "PEXTOK=0"
for %%e in (%PE:;= %) do @if /i %%e==.%EXT% set "PEXTOK=1"
if %PEXTOK%==0 (
  if %MODE%==test (
    echo Error: The .%EXT% extension is missing in the global PATHEXT.
    set NEEDSETUP=1
  )
  if %MODE%==setup (
    echo Updating global environment variable PATHEXT to: %PE%;.%EXT%
    %RUN% reg add "%KEY%" /v PATHEXT /d "%PE%;.%EXT%" /f
    :# Notify all windows of the environment block change
    findstr -rbv @echo "%ARG0%" 2>NUL | powershell -c -
  )
)
:# 2) The local variable in the command shell
echo The local environment variable PATHEXT is: %PATHEXT%
set "PEXTOK=0"
for %%e in (%PATHEXT:;= %) do @if /i %%e==.%EXT% set "PEXTOK=1"
if %PEXTOK%==0 (
  if %MODE%==test (
    echo Error: The .%EXT% extension is missing in the local PATHEXT.
    set NEEDSETUP=1
  )
  if %MODE%==setup (
    echo Updating local environment variable PATHEXT to: %PATHEXT%;.%EXT%
    set "PATHEXT=%PATHEXT%;.%EXT%"
  )
)

endlocal & set "NEEDSETUP=%NEEDSETUP%" & set "PATHEXT=%PATHEXT%"
%RETURN%

:CheckError
setlocal
set ERR=%1
if "%VERBOSE%"=="-v" if "%NOEXEC%"=="0" (
  if .%ERR%.==.0. (
    echo OK
  ) else (
    echo ERR=%ERR%
  )
)
endlocal
%RETURN%

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
echo   -s        Setup Windows for running .tcl scripts with the latest tclsh
echo   -t        Test the current setup. Tells if it's useful to use -s.
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
if %1==-f goto FindTclsh
if %1==-t goto TestSetup
if %1==-s goto CallSetup
if %1==-v set "VERBOSE=-v" & set RUNX=%COMSPEC% /c %ARG0% %VERBOSE% --call Run & goto nextarg
if %1==-V (echo %VERSION%) & goto :eof
if %1==-X set "NOEXEC=1" & goto nextarg
goto start

:# Call the setup routine. May update the parent cmd.exe PATHEXT variable.
:CallSetup
call :Setup
endlocal & set "PATHEXT=%PATHEXT%" & set "PATH=%PATH%" & exit /b 0

:# Mechanism for calling a subroutine from the command line
:Call
shift
set PROC=%1
shift
call :%PROC% %1 %2 %3 %4 %5 %6 %7 %8 %9
%RETURN%

:# Locate and start the Tcl interpreter
:Start
set TCLSH=""
for /f "delims=" %%p in ('%ARG0% %VERBOSE% -f') do set TCLSH="%%p"
:# Note: The set command above fails without the quotes around %%p. Why?!?
:#       (Is it linked to the parenthesis in the "C:\Program Files (x64)" path?)
if .%TCLSH%.==."". (
  echo>&2 Failure. No tclsh interpreter found.
  exit /b 1
)
:# Start it
%TCLSH% %*
goto :eof
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

