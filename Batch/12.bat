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
:#   2018-09-11 JFL Use codepage 65001 = UTF-8, to handle all Unicode chars   #
:#                  in the pipe. This is possible now that flipmails.tcl      #
:#                  correctly handles code pages, like my C tools did.        #
:#   2018-10-18 JFL Added options -?, -A, -U, -V, -X.			      #
:#                                                                            #
:##############################################################################

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2018-10-18"
set ^"ARGS=%*^"					&:# Argument line
goto :main

:#----------------------------------------------------------------------------#

:PopArg
if "!!"=="" goto :PopArg.Eon
:PopArg.Eoff
:PopSimpleArg :# Will corrupt result if expansion is on and ARG contains ^ or ! characters.
:# Gotcha: The call parser first scans its command line for an unquoted /?.
:# If it finds one anywhere on the command line, then it ignores the target label and displays call help.
:# To work around that, we initialize %ARG% and %"ARG"% with an impossible combination of values.
set "ARG=Yes"
set ""ARG"=No"
set "PopArg.ARGS="
if defined ARGS (
  setlocal EnableDelayedExpansion
  for /f "delims=" %%a in ("!ARGS:%%=%%%%!") do endlocal & set ^"PopArg.ARGS=%%a^"
)
call :PopArg.Helper %PopArg.ARGS% >NUL 2>NUL &:# Output redirections ensure the call help is not actually output.
:# Finding that impossible combination now is proof that the call was not executed.
:# In this case, try again with the /? quoted, to prevent the call parser from processing it.
:# Note that we can not systematically do this /? enquoting, else it's "/?" that would break the call.
if "%ARG%"=="Yes" if [%"ARG"%]==[No] call :PopArg.Helper %PopArg.ARGS:/?="/?"%
set "PopArg.ARGS="
goto :eof
:PopArg.Helper
set "ARG=%~1"		&:# Remove quotes from the argument
set ^""ARG"=%1^"	&:# The same with quotes, if any, should we need them
if defined ARG set "ARG=%ARG:^^=^%"
if defined "ARG" set ^""ARG"=%"ARG":^^=^%^"
:# Rebuild the tail of the argument line, as shift does not do it
:# Never quote the set ARGS command, else some complex quoted strings break
set ARGS=%2
:PopArg.GetNext
shift
if defined ARGS set ^"ARGS=%ARGS:^^=^%^"
if [%2]==[] goto :eof
:# Leave quotes in the tail of the argument line
set ARGS=%ARGS% %2
goto :PopArg.GetNext

:#----------------------------------------------------------------------------#

:help
echo Filter clipboard contents. Runs: 1clip.exe ^| FILTER [OPTIONS] ^| 2clip.exe
echo.
echo Usage:
echo   12 [12_OPTIONS] FILTER [FILTER_OPTIONS]
echo.
echo 12 Options:
echo   -?    Display this help and exit
echo   -A    Filter ANSI characters. Default only for 82w.bat
echo   -U    Filter UTF-8 characters. Default
echo   -V    Display this script version and exit
echo   -X    Display the commands to execute, but don't run them
exit /b

:#----------------------------------------------------------------------------#

:main
:# Get the default system code page
:# set "KEY=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage"
:# for /f "tokens=3" %%p in ('reg query "%KEY%" /v "ACP" ^| findstr REG_SZ') do set "ACP=%%p"
:# set "NEWCP=%ACP%"
:# Actually use UTF-8 in all cases now
set "NEWCP=65001"
set "_CLIPMODE="
set "EXEC="
set "|=|"
set ">NUL=>NUL"

:next_arg
set ^"LASTARGS=%ARGS%^"
call :PopArg
if [%ARG%]==[-?] goto :help
if [%ARG%]==[/?] goto :help
if [%ARG%]==[-A] set "_CLIPMODE= -A" & goto next_arg
if [%ARG%]==[-U] set "_CLIPMODE= -U" & goto next_arg
if [%ARG%]==[-V] (echo.%VERSION%) & exit /b
if [%ARG%]==[-X] set "EXEC=echo" & set "|=^|" & set ">NUL=" & goto next_arg

if [%ARG%]==[82w] set "_CLIPMODE= -A"	  &:# Special case for the 82w.bat script
if [%ARG%]==[82w.bat] set "_CLIPMODE= -A" &:# Special case for the 82w.bat script
set ^"ARGS=%LASTARGS%^"

:# Get the current console code page
for /f "tokens=2 delims=:" %%n in ('chcp') do for %%p in (%%n) do set OLDCP=%%p
:# Change to the default system code page if needed
if not "%OLDCP%"=="%NEWCP%" %EXEC% chcp %NEWCP% %>NUL%

%EXEC% 1clip%_CLIPMODE% %|% %ARGS% %|% 2clip%_CLIPMODE%

:# Return to the initial code page if it was different
if not "%OLDCP%"=="%NEWCP%" %EXEC% chcp %OLDCP% %>NUL%

