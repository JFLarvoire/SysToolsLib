@echo off
:##############################################################################
:#                                                                            #
:#  Filename        AutoRun.cmd                                               #
:#                                                                            #
:#  Description     Manage multiple cmd.exe AutoRun scripts		      #
:#                                                                            #
:#  Notes 	    This script manages cmd.exe AutoRun initializations, in   #
:#		    an easily extensible way, inspired by Unix standards.     #
:#		    It makes extensions independent of each other, so that    #
:#		    they can be installed or removed in any order.	      #
:#                  It also defines installation directories, as specified in #
:#      https://www.gnu.org/prep/standards/html_node/Directory-Variables.html #
:#                                                                            #
:#                  See 'cmd /?' for information about its AutoRun features.  #
:#                                                                            #
:#                  To install AutoRun.cmd, copy it to your favorite cmd      #
:#                  scripts directory (A directory that's in your PATH);      #
:#		    Then run as Administrator: AutoRun -i		      #
:#                  If you first want to know what this installation would    #
:#                  do, run: AutoRun -X -i                                    #
:#                  In case that another AutoRun script is already present,   #
:#		    this installation will detect it, and refuse to run.      #
:#		    It's possible to override that by using the -f option. In #
:#		    that case the previous AutoRun script is moved to one of  #
:#                  the extension directories. (See below)		      #
:#                                                                            #
:#                  This script does as little as possible, to run as fast    #
:#                  as possible.                                              #
:#                  Do not customize it. Instead, put your own extension      #
:#		    scripts in "%ALLUSERSPROFILE%\AutoRun.cmd.d\" for all     #
:#                  users, or in "%USERPROFILE%\AutoRun.cmd.d\" for you only. #
:#                  This is inspired from the way Linux bashrc and login      #
:#		    scripts are organized, and how they can be extended.      #
:#                                                                            #
:#                  This script, or any extension add-on in AutoRun.cmd.d     #
:#                  directories, must NEVER output anything during their      #
:#                  initialization. Else this may severely affect outside     #
:#                  scripts that don't expect sub-cmd instances to display    #
:#		    anything before they start.				      #
:#                  Likewise, they must not change directory, nor do anything #
:#                  that may break existing scripts if they run a sub-cmd.    #
:#                                                                            #
:#  Authors         JFL Jean-François Larvoire, jf.larvoire@free.fr           #
:#                                                                            #
:#  History                                                                   #
:#   2019-02-03 JFL Created this script.                                      #
:#   2019-02-04 JFL Added options -a and -c.                                  #
:#		    Make sure bindir* directories are in the PATH.            #
:#   2019-02-05 JFL Fixed a problem with %CMDCMDLINE%, that caused errors.    #
:#		    Enforce the fact that AutoRun scripts must be output      #
:#		    anything, nor change directory.                           #
:#		                                                              #
:#         © Copyright 2019 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2019-02-05"
set "SCRIPT=%~nx0"				&:# Script name
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"	&:# Script path, without the trailing \
set "SFULL=%~f0"				&:# Script full pathname
set ^"ARG0=%0^"					&:# Script invokation name
set ^"ARGS=%*^"					&:# Argument line
goto :main

:#----------------------------------------------------------------------------#
:# make sure a directory is in the PATH
:PutInPath %1=directory that should be in the PATH
setlocal EnableExtensions EnableDelayedExpansion
if not "!PATH:~0,1!"==";" set "PATH=;!PATH!"
if not "!PATH:~-1!"==";" set "PATH=!PATH!;"
if "!PATH:;%~1;=;!"=="!PATH!" set "PATH=!PATH!%~1;"
set "PATH=!PATH:~1,-1!" &:# Trim the extra ; added above
endlocal & set "PATH=%PATH%" & exit /b

:#----------------------------------------------------------------------------#
:# Check if the user has system administrator rights. %ERRORLEVEL% 0=Yes; 5=No
:IsAdmin
>NUL 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
exit /b

:#----------------------------------------------------------------------------#
:# Extract the ARG0 and ARG1 from %CMDCMDLINE% using cmd.exe own parser
:SplitArgs
set "ARG0=%1"
set "ARG1=%2"
exit /b

:#----------------------------------------------------------------------------#
:# Installation routine
:install
setlocal EnableExtensions EnableDelayedExpansion

:# Check prerequisites
if /i "%PROCESSOR_ARCHITECTURE%" neq "x86" if /i "%PROCESSOR_ARCHITECTURE%" neq "AMD64" (
  if not "%FORCE%"=="1" (
    >&2 echo Error: This script does not support the %PROCESSOR_ARCHITECTURE% processor architecture
    exit /b 1
  )
)

:# Create the extension directories
for %%h in (HKLM HKCU) do (
  if not exist "!AUTORUN.D[%%h]!" %EXEC% md "!AUTORUN.D[%%h]!"
)

:# Test if there are previously installed AutoRun scripts
for %%h in (HKLM HKCU) do (
  call :query %%h AutoRun
  if defined AutoRun (
    if not "%SFULL%"=="!AutoRun!" (
      if not "%FORCE%"=="1" (
	>&2 echo Error: Another AutoRun script is already installed for !USER[%%h]!: !AutoRun!
	endlocal & exit /b 1
      )
      :# In force mode, move the previous autorun to AutoRun.cmd.d
      %ECHO.COMMENT% Moving the existing AutoRun script !AutoRun! to !AUTORUN.D[%%h]!
      %EXEC% move "!AutoRun!" "!AUTORUN.D[%%h]!"
      :# Also delete its key, else the next call to :query will display an error
      set "KEY=%%h\Software\Microsoft\Command Processor"
      %ECHO.COMMENT% Deleting "!KEY!\AutoRun"
      %EXEC% reg delete "!KEY!" /v "AutoRun" /f
    ) else (
      %ECHO.COMMENT% Already installed
      endlocal & exit /b 0
    )
  )
)
:# OK, no previous instance installed. Try installing ourselves
if not defined HIVE (
  call :IsAdmin
  if not errorlevel 1 (	:# Admin user. Install for all users.
    set "HIVE=HKLM"
  ) else (		:# Normal user. Install for himself.
    set "HIVE=HKCU"
  )
)
set "KEY=%HIVE%\Software\Microsoft\Command Processor"
%ECHO.COMMENT% Creating registry key "%KEY%" value "AutoRun"
%EXEC% reg add "%KEY%" /v "AutoRun" /t REG_SZ /d "%SFULL%" /f

endlocal & exit /b

:#----------------------------------------------------------------------------#
:# Uninstallation routine
:uninstall
setlocal EnableExtensions EnableDelayedExpansion
set "HIVES=%HIVE%"
if not defined HIVES set "HIVES=HKLM HKCU"
:# Test if there are previously installed AutoRun scripts
for %%h in (%HIVES%) do (
  call :query %%h AutoRun
  if defined AutoRun (
    if not "%SFULL%"=="!AutoRun!" if not "%FORCE%"=="1" (
      >&2 echo Error: Another AutoRun script is installed for !USER[%%h]!: "!AutoRun!"
      endlocal & exit /b 1
    )
    set "KEY=%%h\Software\Microsoft\Command Processor"
    %ECHO.COMMENT% Deleting regisry key "!KEY!" value "AutoRun"
    %EXEC% reg delete "!KEY!" /v "AutoRun" /f
  )
)
endlocal & exit /b

:#----------------------------------------------------------------------------#
:# Installation test routine
:query %1=Hive %2=OutputVarName
setlocal EnableExtensions EnableDelayedExpansion
set "KEY=%1\Software\Microsoft\Command Processor"
for /f "skip=2 tokens=2,3*" %%a in ('reg query "!KEY!" /v "AutoRun" 2^>NUL') do (
  set "TYPE=%%a"
  set "VALUE=%%b"
  if "%~2"=="" echo !USER[%1]!: !VALUE!
  if "!TYPE!"=="REG_EXPAND_SZ" call set "VALUE=!VALUE!"
)
endlocal & (if not "%~2"=="" (set "%~2=%VALUE%")) & exit /b

:#----------------------------------------------------------------------------#
:# Installation enumeration routine
:list
setlocal EnableExtensions EnableDelayedExpansion
:# First display the AutoRun scripts configured
%ECHO.COMMENT% AutoRun scripts:
for %%h in (HKLM HKCU) do call :query %%h
:# Then display available extension scripts
%ECHO.COMMENT%.
%ECHO.COMMENT% Extension scripts:
for %%h in (HKLM HKCU) do if exist "!AUTORUN.D[%%h]!" (
  for %%b in ("!AUTORUN.D[%%h]!\*.bat" "!AUTORUN.D[%%h]!\*.cmd") do (
    echo !USER[%%h]!: %%~b
  )
)
endlocal & exit /b

:#----------------------------------------------------------------------------#
:# Help routine
:help
echo %SCRIPT% - Extensible initialization of cmd.exe
echo.
echo Usage %SCRIPT% [OPTIONS]
echo.
echo Options:
echo   -?    Display this help
echo   -a    Do install/uninstall for all users (Default if running as admin.)
echo   -c    Do install/uninstall for the current user (Default if non-admin.)
echo   -f    Force install/uninstall in the presence of another AutoRun script
echo   -i    Install %SCRIPT%, to run automatically every time cmd.exe starts
echo   -l    List AutoRun scripts installed
echo   -u    Uninstall %SCRIPT%, or any other AutoRun script with option -f
echo   -V    Display this script version
echo   -X    No-eXec mode: Display commands to execute, but don't run them
echo.
echo Add your own *.bat or *.cmd extension scripts in:
echo   "%%ALLUSERSPROFILE%%\AutoRun.cmd.d\" for all users
echo   "%%USERPROFILE%%\AutoRun.cmd.d\" for yourself only
echo These extension scripts must not display anything, or change directory, or
echo do anything that might surprise other scripts that start a sub-cmd shell.
exit /b

:#----------------------------------------------------------------------------#
:# Main routine
:main
set "ECHO.COMMENT=echo"
set "DEBUG.LOG=rem"
set "DEBUG.LOG=if exist "%TEMP%\AutoRun.log" >>"%TEMP%\AutoRun.log" echo"
%DEBUG.LOG% :#------------------- %SCRIPT% - %DATE% %TIME% -------------------#
%DEBUG.LOG% set "CMDCMDLINE=!CMDCMDLINE!"
%DEBUG.LOG% set "%%0 %%*=%0 %*"
set "FORCE=0"
set "EXEC="
set "ACTION=start"
set "AUTORUN.D[HKLM]=%ALLUSERSPROFILE%\AutoRun.cmd.d"
set "AUTORUN.D[HKCU]=%USERPROFILE%\AutoRun.cmd.d"
set "USER[HKLM]=all users"
set "USER[HKCU]=%USERNAME%"
set "HIVE="

:# Parse script arguments
goto :get_arg
:next_arg
shift
:get_arg
if [%1]==[] goto :%ACTION% &:# No more arguments. Run the selected action
if "%~1"=="-?" goto :help
if "%~1"=="/?" goto :help
if "%~1"=="-a" set "HIVE=HKLM" & goto :next_arg
if "%~1"=="-c" set "HIVE=HKCU" & goto :next_arg
if "%~1"=="-f" set "FORCE=1" & goto :next_arg
if "%~1"=="-i" set "ACTION=install" & goto :next_arg
if "%~1"=="-l" set "ACTION=list" & goto :next_arg
if "%~1"=="-u" set "ACTION=uninstall" & goto :next_arg
if "%~1"=="-V" (echo.%VERSION%) & exit /b
if "%~1"=="-X" set "EXEC=echo" & set "ECHO.COMMENT=echo :#" & goto :next_arg
>&2 echo Unknown argument: %1
exit /b 1

:#----------------------------------------------------------------------------#
:# Initialize cmd.exe every time a new instance starts
:start
:# if not defined bindir_x86 goto :init_now &:# It's never been done, so it's needed
:# Caution: The AutoRun script is also invoked when running: for /f %%l in ('some command') do ...
:#          We do not want to run the initializations in that case, as this can have
:#	    unexpected side effects on the parent script variables.
:#	    Also this severly affects the performance of scripts that heavily sue that kind of for /f.
:# TO DO: The actual command for running a new cmd instance is available in the "Command Prompt.lnk" shortcut.
:#        It would be nice to extract it from there, and compare it to %CMDCMDLINE%.
:#        This is feasible using JScript WshShell.Createshortcut().
:#        But the performance penalty each time a cmd.exe starts would not be acceptable,
:#        unless we do it only once at install time.
:# Workaround for now: Check that a full quoted COMSPEC was used, _not_ using /c.
:# Important: Do not use %CMDCMDLINE%, as is may contain unprotected | & > < characters. Use !CMDCMDLINE! instead.
set "CMD=!CMDCMDLINE!"
set "CMD=!CMD:|=\x7C!"
set "CMD=!CMD:>=\x3E!"
set "CMD=!CMD:<=\x3C!"
set "CMD=!CMD:&=\x36!"
call :SplitArgs !CMD!
if "!ARG0!"=="%COMSPEC%" ( :# for /f invokes %COMSPEC% without quotes, whereas new shells' ARG0 have quotes.
  %DEBUG.LOG% :# This is not a new top cmd.exe instance
  exit /b 0
)
if /i "!ARG1!"=="/c" (
  %DEBUG.LOG% :# This is not a new top cmd.exe instance
  exit /b 0
)
:init_now
%DEBUG.LOG% :# This is a new top cmd.exe instance. Initialize it.

if exist "%TEMP%\AutoRun.log" (
  set "CAPTURE_OUTPUT=>>"%TEMP%\AutoRun.log" 2>&1"
) else (
  set "CAPTURE_OUTPUT=>NUL 2>&1"
)

:# Now on, every variable will be created in the parent cmd.exe instance
endlocal & (
  :# Create common PATH variables
  if not defined HOME set "HOME=%HOMEDRIVE%%HOMEPATH%"
  
  :# Call all extension scripts in AutoRun.cmd.d directories
  for %%p in ("%AUTORUN.D[HKLM]%" "%AUTORUN.D[HKCU]%") do if exist "%%~p" (
    for %%b in ("%%~p\*.bat" "%%~p\*.cmd") do (
      call "%%~b" %CAPTURE_OUTPUT%
      cd /d "%CD%" &rem Make sure the current directory is not changed
    )
  )
)

:# Standard variables for installation directories
:# See: https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
:# They can be overridden individually by defining them in an extension script above
if not defined prefix set "prefix=%ProgramFiles%" &:# Where to install everything
if not defined exec_prefix set "exec_prefix=%prefix%" &:# Base dir for executables
if not defined bindir ( :# Where to put programs and scripts
  for %%p in ("%~nx0") do ( :# Use the directory where this AutoRun.cmd script was installed
    if "%%~f$PATH:p"=="%~f0" (	:# OK, this script path is in the PATH. Use it.
      set "bindir=%~dp0:" &rem Append a : to flag the trailing \ in %~dp0
    ) else (			:# Else use the standard default
      set "bindir=%exec_prefix%\bin"
    )
  )
)
set "bindir=%bindir:\:=%" &:# Trim the trailing \ inserted by %~dp0 above
if not defined libdir set "libdir=%exec_prefix%\lib"
if not defined includedir set "includedir=%prefix%\include"
:# Extensions for Windows, in case we want to install both 32 and 64-bits versions of programs
:# Use a suffix with the %PROCESSOR_ARCHITECTURE%: x86 or AMD64 
:# Use _ for the suffix separator, as it is valid in nmake.exe variable names => Usable in an 'nmake install' rule.
if not defined bindir_%PROCESSOR_ARCHITECTURE% set "bindir_%PROCESSOR_ARCHITECTURE%=%exec_prefix%\bin"
if not defined libdir_%PROCESSOR_ARCHITECTURE% set "libdir_%PROCESSOR_ARCHITECTURE%=%exec_prefix%\lib"
if /i "%PROCESSOR_ARCHITECTURE%" neq "x86" ( :# This one supports dual installs
  if not defined exec_prefix86 (
    if "%prefix%"=="%ProgramFiles%" (	:# Use Windows' default
      set "exec_prefix86=%ProgramFiles(x86)%"
    ) else (				:# Use a path within the requested %prefix% directory
      set "exec_prefix86=%prefix%\x86"
    )
  )
)
if /i "%PROCESSOR_ARCHITECTURE%" neq "x86" ( :# This one supports dual installs
  if not defined bindir_x86 set "bindir_x86=%exec_prefix86%\bin"
  if not defined libdir_x86 set "libdir_x86=%exec_prefix86%\lib"
)

:# Make sure bindir* directories are in the PATH
call :PutInPath "%bindir%"
if /i "%PROCESSOR_ARCHITECTURE%"=="AMD64" if /i "%bindir_AMD64%" neq "%bindir%" call :PutInPath "%bindir_AMD64%"
if /i "%bindir_x86%" neq "%bindir%" call :PutInPath "%bindir_x86%"

:# Done
exit /b
