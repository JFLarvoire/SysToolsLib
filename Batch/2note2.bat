@echo off
:##############################################################################
:#                                                                            #
:#  Filename        2note2.bat                                                #
:#                                                                            #
:#  Description     Pipe data into Notepad2.exe                               #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  Author          Jean-François Larvoire, jf.larvoire@hpe.com               #
:#                                                                            #
:#  History                                                                   #
:#   2019-10-24 JFL Create this script.                                       #
:#		                                                              #
:##############################################################################

:# Rerun self in a sub-shell, to avoid breaking the original shell file handles
echo %0 | findstr :: >nul || (cmd /d /c ^""%~dp0\::\..\%~nx0" %*^" & exit /b)

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2019-11-01"
set "SCRIPT=%~nx0"		&:# Script name
set "SNAME=%~n0"		&:# Script name, without its extension
set "SPATH=%~dp0"		&:# Script path
set "SPATH=%SPATH:~0,-1%"	&:# Script path, without the trailing \
set "SFULL=%~f0"		&:# Script full pathname
set ^"ARG0=%0^"			&:# Script invokation name
set ^"ARGS=%*^"			&:# Argument line

call :debug.init
goto :Main

:debug.init
set "IFDEBUG=if "%DEBUG%"=="1""
set "ECHO.D=%IFDEBUG% echo"
set "ECHOVARS.D=%IFDEBUG% call :echovars"
set "RETURN=exit /b"
exit /b

:echovars %*=variables names
setlocal EnableExtensions EnableDelayedExpansion
for %%s in (%*) do echo set "%%s=!%%s!"
endlocal & exit /b

:#----------------------------------------------------------------------------#
:# Sleep N seconds. %1 = Number of seconds to wait.
:Sleep
%FUNCTION%
set /A N=%1+1
ping -n %N% 127.0.0.1 >NUL 2>&1
%RETURN%

:#----------------------------------------------------------------------------#

:# Note: In the routines below, we call "lists" batch variables with elements separated by a space.
:# The advantage of such lists is that they can be parsed, or looped on, using just the "for" command.
:# We build lists in a way that inserts an extra space ahead of the first element.
:# For performance reasons, we do not bother removing that space all along.
:# We remove it only in the final list returned in the end.
:# Also, we rely on the fact that elements in a list of handles here are all 1-digit numbers.

:#----------------------------------------------------------------------------#
:# Handle enumeration routine - Return lists of used and free file I/O handles
:EnumHandles %1=freeListVar %2=usedListVar %3=quantumLevelVar %4=knownList (Not including 0 1 2)
:#	     Returns %freeHandles%, %usedHandles%, %nUnknownHandles%
setlocal EnableExtensions EnableDelayedExpansion
%ECHO.D% call %0 %*

:# Search the top free handles
:# Our :TryRedir routine uses 2>NUL, so itself always uses the first free handle.
:# => Handle 3 will always be found in use: Either it was already, or 2>NUL will use it.
:# Using handle 3 for redirection tests forces cmd.exe to use the second free handle to save it.
:# => Handle 4 will always be found in use: Either it was already, or 3>&%%h will use it.
:# Try duplicating handles 5 to 9, to see if they exist.
set "freeHandles="	&:# List of free file handles
set "usedHandles= 3 4"	&:# List of used file handles. 3 and 4 will be in use.
for /L %%h in (5,1,9) do call :TryRedir 3 %%h freeHandles usedHandles
%ECHOVARS.D% freeHandles usedHandles

:# Search the second missed free handle, used for saving handle 3 above
if defined freeHandles ( :# This can only work by using another free handle
  set "firstFreeHandle=!freeHandles:~1,1!" &:# The first free one we've found so far
  set /a "tryLast=firstFreeHandle-1" &:# The last used handle before the first free one
  set "freeHandle="	&:# 1-element list with the free handle we missed in the above loop
  :# Again, no need to test handle 3, it's bound to be found in use.
  for /L %%h in (4,1,!tryLast!) do if not defined freeHandle call :TryRedir !firstFreeHandle! %%h freeHandle usedHandles2
  :# Move that free handle from the used list to the free list
  for %%h in ("!freeHandle!") do set "usedHandles=!usedHandles:%%~h=!"
  set "freeHandles=!freeHandle!!freeHandles!"
)
%ECHOVARS.D% freeHandles usedHandles

:# Search the first missed free handle, used for saving handle 2 above
:# If the first used handle is followed by the first free handle, then we know
:# it's the one that was used by the 2>NUL redirection. So it's actually free.
:# More generally, if all used handles before the first free one are known used
:# handles passed in %4, except for one, then that unknown one is actually free.
if defined freeHandles ( :# [Else there may actually be two unknown handles]
  set "firstFreeHandle=!freeHandles:~1,1!" &:# The first free one we've found so far
  set "knownUsedHandles=%~4" &:# This list may be empty, or partial
  set /a "nUnknownHandles=firstFreeHandle-3"
  %ECHOVARS.D% firstFreeHandle nUnknownHandles knownUsedHandles
  set "unknownHandles="
  set /a "tryLast=firstFreeHandle-1"
  for /l %%h in (3,1,!tryLast!) do set "unknownHandles=!unknownHandles! %%h"
  if defined knownUsedHandles for %%h in (!knownUsedHandles!) do (
    if %%h lss !firstFreeHandle! ( :# Then remove it from the unknown handle list
      set /a "nUnknownHandles-=1"
      set "unknownHandles=!unknownHandles: %%h=!"
    )
  )
  %ECHOVARS.D% nUnknownHandles unknownHandles
  if !nUnknownHandles!==1 (  :# OK, this single used handle is actually free.
    :# Move it from the used list to the free list
    for %%h in ("!unknownHandles!") do set "usedHandles=!usedHandles:%%~h=!"
    set "freeHandles=!unknownHandles!%freeHandles%"
    set "quantumLevel=0"
  ) else ( :# One unidentified handle in the used list is actually free.
    set "quantumLevel=1"
  )
) else ( :# No free handle found. Up to 2 used handles may actually be free.
  set "quantumLevel=2"
)
:# Cleanup and return
for %%v in (freeHandles usedHandles) do if defined %%v set "%%v=!%%v:~1!" &:# Remove the head space
endlocal & (
  set "%1=%freeHandles%"
  set "%2=%usedHandles%"
  set "%3=%quantumLevel%"
  %ECHOVARS.D% freeHandles usedHandles quantumLevel
) & (%ECHO.D% return %ERRORLEVEL%) & exit /b

:TryRedir %1=Handle to redirect; %2=Handle to duplicate; %3=Free var; %4=Used var
2>NUL ( :# Prevent error messages written to stderr from being visible.
  break %1>&%2 && (	:# The redirection succeeded.
    set "%4=!%4! %2"	&rem The handle %2 existed. Add it to the used list.
    (call,)		&rem Clear ERRORLEVEL, which might be non-0 despite success here.
  ) || (		:# The redirection failed, and an error message was written to stderr.
    set "%3=!%3! %2"	&rem The handle %2 did not exit. Add it to the free list.
  )
)
exit /b	  &:# Returns 0=Used, 1=Free

:#----------------------------------------------------------------------------#
:# Pipe creation routine - Returns the two handles selected, and updates the known handles list
:CreatePipe %1=PipeIn name; %2=PipeOut name; %3=Known handles list name (optional, list may be partial)
setlocal EnableExtensions EnableDelayedExpansion
%ECHO.D% call %0 %*
if not "%3"=="" (set "knownHandles=!%3!") else (set "knownHandles=")
call :EnumHandles freeHandles usedHandles quantumLevel "%knownHandles%"
:# Make sure there are at least 4 free handles. The list looks like: "0 2 4 6"
if %quantumLevel% gtr 1 endlocal & exit /b 1	 &:# No free handles
if "%freeHandles:~6,1%"=="" endlocal & exit /b 1 &:# Not enough free handles
:# Define an optional redirection, that plugs the hole in the usedHandles list, if any
set "FILL_HOLE="
if %quantumLevel% equ 1 set "FILL_HOLE=2>NUL"
:# Select the handles to use, in the first four free handles left
for /f "tokens=1,2,4" %%a in ("%freeHandles%") do (
  set "hIn=%%a"			&:# PipeIn handle
  set "hOut=%%b"		&:# PipeOut handle
  set "hTmp=%%c"		&rem Temporary handle, released after use
)
%ECHOVARS.D% hIn hOut hTmp
endlocal & (
  set "%1=%hIn%"		&:# PipeIn handle
  set "%2=%hOut%"		&:# PipeOut handle
  if not "%3"=="" (
    set "%3=!%3! %hIn% %hOut%"
    %ECHOVARS.D% %3
  )
) & %FILL_HOLE% (rundll32 1>&%hOut% %hOut%>&%hTmp% | rundll32 0>&%hIn% %hIn%>&%hTmp%)
%ECHO.D% return %ERRORLEVEL%
exit /b

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Main                                                      #
:#                                                                            #
:#  Description     Process command line arguments                            #
:#                                                                            #
:#  Arguments       %*	    Command line arguments                            #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Help
echo.
echo %SCRIPT% version %VERSION% - Pipe data into Notepad2.exe
echo.
echo Usage: ^<command^> ^| %SCRIPT% [OPTIONS]
echo.
echo Options:
echo   -?       Display this help
echo   -V       Display the script version and exit
goto :eof

:#----------------------------------------------------------------------------#
:# Main routine

:Main
goto :get_arg
:next_arg
shift
:get_arg
if [%1]==[] goto :Start
set "ARG=%~1"
if "!ARG!"=="-?" goto :Help
if "!ARG!"=="/?" goto :Help
if "!ARG!"=="-V" (echo.%VERSION%) & goto :eof
if "!ARG:~0,1!"=="-" (
  >&2 %ECHO% Warning: Unexpected option ignored: !ARG!
  goto :next_arg
)
>&2 %ECHO% Warning: Unexpected argument ignored: !"ARG"!
goto :next_arg

:#----------------------------------------------------------------------------#

:Start
:# Create a SUB variable containing a SUB <==> EOF <==> Ctrl-Z character
:# Not necessary anymore, now that 1clip.exe has a -Z option for generating a Ctrl-Z.
:# >NUL copy /y NUL + NUL /a "%TEMP%\1A.chr" /a
:# for /f %%c in (%TEMP%\1A.chr) do set "SUB=%%c"

call :CreatePipe P1IN P1OUT
if errorlevel 1 echo Error: Failed to find 4 free handles for pipe 1 & exit /b 1

:# Save the initial clipboard contents
>&%P1OUT% 1clip -U -Z
:# >&%P1OUT% echo.!SUB!

:# Pipe the standard input data into Notepad2
2clip		  &:# First pipe it into the clipboard
start notepad2 -c &:# -c option tells Notepad2 to copy data from the clipboard
		   :# Known issue: -c appends an extra \n after the clipboard data!

:# wait 1s, to give time to Notepad2 to start and paste the clipboard content
call :Sleep 1

:# Restore the initial clipboard contents
<&%P1IN% 2clip -U -z
