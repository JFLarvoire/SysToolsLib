@if (@Language == @Batch) @end /* JScript comment protecting the Batch section
@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename	    $.bat						      *
:#                                                                            *
:#  Description     Set a variable with the output of a command		      *
:#                                                                            *
:#  Notes	                                                              *
:#		                                                              *
:#  History                                                                   *
:#   2021-01-11 JFL Created this script.				      *
:#   2021-01-22 JFL Added a dirty work around the pipe issue by generating    *
:#                  key strokes using JScript WScript.Shell.SendKeys().       *
:#   2021-01-27 JFL Capture every line of output.                             *
:#                  Quote lines that contain spaces, or other special chars.  *
:#                  Added option -l to revert to the initial behavior.        *
:#                                                                            *
:#        © Copyright 2021 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license: www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2021-01-27"
set "SCRIPT=%~nx0"		&:# Script name
set "SNAME=%~n0"		&:# Script name without the extension
set "SPATH=%~dp0"
set "SPATH=%SPATH:~0,-1%"	&:# Script path, without the trailing \
set "SFULL=%~f0"		&:# Script full pathname
set ^"ARG0=%0^"			&:# Script invokation name
set ^"ARGS=%*^"			&:# Argument line

set "POPARG=call :PopArg"
call :Entities.init
goto :main

:#----------------------------------------------------------------------------#

:PopArg
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
:# Note: The following call doubles ^ within "quotes", but not those outside of quotes.
:# So :PopArg.Helper will correctly record ^ within quotes, but miss those outside. (Unless quadrupled!)
:# The only way to fix this would be to completely rewrite :PopArg as a full fledged batch parser written in batch!
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

:# Quote strings that require it.
:condquote	 %1=Input variable. %2=Opt. output variable.
setlocal EnableExtensions Disabledelayedexpansion
set "RETVAR=%~2"
if not defined RETVAR set "RETVAR=%~1" &:# By default, change the input variable itself
call set "P=%%%~1%%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Remove double quotes inside P. (Fails if P is empty)
set "P=%P:"=%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Look for any special character that needs quoting
:# Added "@" that needs quoting ahead of commands.
:# Added "|&<>" that are not valid in file names, but that do need quoting if used in an argument string.
echo."%P%"|findstr /C:" " /C:"&" /C:"(" /C:")" /C:"[" /C:"]" /C:"{" /C:"}" /C:"^^" /C:"=" /C:";" /C:"!" /C:"'" /C:"+" /C:"," /C:"`" /C:"~" /C:"@" /C:"|" /C:"&" /C:"<" /C:">" >NUL
if not errorlevel 1 set P="%P%"
:condquote_ret
:# Contrary to the general rule, do NOT enclose the set commands below in "quotes",
:# because this interferes with the quoting already added above. This would
:# fail if the quoted string contained an & character.
:# But because of this, do not leave any space around & separators.
endlocal&set %RETVAR%=%P%&exit /b 0

:#----------------------------------------------------------------------------#

:# Build a list of strings that can be used in a for loop
:lappend	%1=List variable name. %2=String to add to the list
setlocal DisableDelayedExpansion
set "STRING=%~2"
call :condquote STRING
setlocal EnableDelayedExpansion
set "LIST=!%~1!"
if defined LIST set "LIST=!LIST! "
endlocal & endlocal & set %~1=%LIST%%STRING%
exit /b

:#----------------------------------------------------------------------------#

:Entities.init
:# Define variables for problematic characters, that cause parsing issues.
:# Use the ASCII control character name, or the html entity name.
:# Warning: The excl and hat characters need different quoting depending on context.
set  "$percnt=%%"	&:# One percent sign
set  "$excl=^!"		&:# One exclamation mark
set  "$hat=^"		&:# One caret, aka. circumflex accent, or hat sign
set ^"$quot=""		&:# One double quote
set  "$apos='"		&:# One apostrophe
set  "$amp=&"		&:# One ampersand
set  "$vert=|"		&:# One vertical bar
set  "$gt=>"		&:# One greater than sign
set  "$lt=<"		&:# One less than sign
set  "$lpar=("		&:# One left parenthesis
set  "$rpar=)"		&:# One right parenthesis
set  "$lbrack=["	&:# One left bracket
set  "$rbrack=]"	&:# One right bracket
set  "$sp= "		&:# One space
set  "$tab=	"	&:# One tabulation
set  "$quest=?"		&:# One question mark
set  "$ast=*"		&:# One asterisk

set  "E0$percnt=%%"	&:# One percent sign
set  "E0$excl=^!"	&:# One exclamation mark
set  "E0$hat=^"		&:# One caret, aka. circumflex accent, or hat sign
set ^"E0$quot=""	&:# One double quote
set  "E0$apos='"	&:# One apostrophe
set  "E0$amp=&"		&:# One ampersand
set  "E0$vert=|"	&:# One vertical bar
set  "E0$gt=>"		&:# One greater than sign
set  "E0$lt=<"		&:# One less than sign
set  "E0$lpar=("	&:# One left parenthesis
set  "E0$rpar=)"	&:# One right parenthesis
set  "E0$lbrack=["	&:# One left bracket
set  "E0$rbrack=]"	&:# One right bracket
set  "E0$sp= "		&:# One space
set  "E0$tab=	"	&:# One tabulation
set  "E0$quest=?"	&:# One question mark
set  "E0$ast=*"		&:# One asterisk

set  "E1$percnt=%%"	&:# One percent sign
set  "E1$excl=^!"	&:# One exclamation mark
set  "E1$hat=^"		&:# One caret, aka. circumflex accent, or hat sign
set ^"E1$quot=""	&:# One double quote
set  "E1$apos='"	&:# One apostrophe
set  "E1$amp=&"		&:# One ampersand
set  "E1$vert=|"	&:# One vertical bar
set  "E1$gt=>"		&:# One greater than sign
set  "E1$lt=<"		&:# One less than sign
set  "E1$lpar=("	&:# One left parenthesis
set  "E1$rpar=)"	&:# One right parenthesis
set  "E1$lbrack=["	&:# One left bracket
set  "E1$rbrack=]"	&:# One right bracket
set  "E1$sp= "		&:# One space
set  "E1$tab=	"	&:# One tabulation
set  "E1$quest=?"	&:# One question mark
set  "E1$ast=*"		&:# One asterisk

exit /b

:Entities.decode %1=INPUTVAR [%2=OUTPUTVAR]
for %%x in ("!!") do setlocal EnableDelayedExpansion & set "EXP=^^%%~x" &:# Exp.On->"^^" Exp.Off->"^"
set "EXP=!EXP:~1!" & if defined EXP (set "EXP=E1") else set "EXP=E0"
for %%x in ("!EXP!") do (
  setlocal EnableDelayedExpansion
  set "STRING=!%1!"
  for %%e in (percnt excl hat quot apos amp vert gt lt lpar rpar lbrack rbrack sp tab quest ast) do (
    for %%c in ("!%%x$%%e!") do set "STRING=!STRING:[%%e]=%%~c!"
  )
)
set "OUTVAR=%2"
if not defined OUTVAR set "OUTVAR=%1"
endlocal & set ^"%OUTVAR%=%STRING%^" ! &:# The ! forces always having !escaping ^ removal in delayed expansion mode
exit /b

:#----------------------------------------------------------------------------#

:help
setlocal DisableDelayedExpansion
echo %SCRIPT% - Set a variable with the output of a command
echo.
echo Usage 1: %SNAME% [OPTIONS] VARIABLE COMMAND [ARGUMENTS]
echo Usage 2: COMMAND [ARGUMENTS] ^| %SNAME% [OPTIONS] VARIABLE
echo.
echo Options:
echo   -?    Display this help
echo   -l    Capture the last non-empty line. Default: Build a list w. all such lines
echo   -q    Quiet mode: Don't display the set command executed
echo   -X    Display the command generated, but don't run it
echo.
echo In the first usage case, special characters can be entered in arguments as html-like entities.
echo For that, surround the [entity] name in brackets instead of html's ^&entity;. Supported entities:
echo   [percnt]=%% [excl]=^! [quot]=" [hat]=^ [lt]=< [gt]=> [amp]=& [vert]=| [lpar]=( [rpar]=) [lbrack]=[ [rbrack]=] [tab]=\t
echo.
echo The second usage case allows capturing the output of a complex sequence
echo of commands with pipes. But the performance isn't as good.
echo.
echo In both cases, tricky characters like "^&<|>!%% may cause unexpected results.
exit /b

:#----------------------------------------------------------------------------#

:main
set "EXEC="
set "IF_NOEXEC=if defined EXEC"
set "SET=call :EchoAndSet"	&:# Command used in the end to set the environment variable
set "CAPTURE=:CaptureAll"
:next_arg
%POPARG%
if "%ARG%"=="--" %POPARG% & goto :start &:# Allows defining a -X variable, etc
if "%ARG%"=="-?" goto :help
if "%ARG%"=="/?" goto :help
if "%ARG%"=="-l" set "CAPTURE=:CaptureLast" & goto :next_arg
if "%ARG%"=="-q" set "DO=do" & goto :next_arg
if "%ARG%"=="-X" set "EXEC=echo" & set "SET=set" & goto :next_arg

:start

:# The first argument is the variable name
set "VAR=%ARG%"
:# The remaining of the command-line is the command and arguments to run
set CMDLINE=%ARGS%
if defined CMDLINE (
  call :Entities.decode ARGS CMDLINE
  set CMDLINE=%CMDLINE%^^^| findstr /R "^."
  set "SENDKEYS="
) else ( :# No command. Get the input from standard input.
  set CMDLINE=findstr /R "^."
  if "%CAPTURE%"==":CaptureLast" set SET=CScript //nologo //E:JScript "%SFULL%"
  set "SENDKEYS=%EXEC% CScript //nologo //E:JScript "%SFULL%" -s %VAR% RESULT"
)
:# %IF_NOEXEC% set CMDLINE
:# In no-exec mode, correct the # of ^ so that the ^| displays right
%IF_NOEXEC% set CMDLINE=%CMDLINE: ^| = ^^^^^^^| %

goto %CAPTURE%

:# Capture all output lines, quoting those with spaces or special characters
:CaptureAll
setlocal EnableDelayedExpansion
set "RESULT="
%EXEC% for /f "delims=" %%s in ('%CMDLINE%') do call :lappend RESULT "%%~s"

:# NOTE: The findstr CMDLINE, used by default above, works for capturing stdin.
:# What does not work is the export of the variable to the parent cmd.exe using pure batch.
:# The problem is that when using a pipe, ex: `hostname | $ HOST`, an
:# additional cmd runs at the right side of the pipe, and its updated variable
:# is lost when the pipe is closed and that cmd shell exits.
:# I tried using known tricks like '(goto) 2>NUL' but this did not help.
:# As a workaround, I'm now using JScript to generate a set command that's
:# fed back into this shell's keyboard input stream.
%SENDKEYS%

%IF_NOEXEC% set "RESULT=%%RESULT%%"
endlocal & endlocal & %EXEC% %SET% %VAR%=%RESULT%
exit /b

:# Capture only the last non-empty line
:CaptureLast
endlocal & %EXEC% for /f "delims=" %%v in ('%CMDLINE%') do %SET% %VAR%=%%v
exit /b

:EchoAndSet
echo set %*
     set %*
exit /b

:#############################################################################:
End of the JScript comment around the Batch section; Beginning of the JScript section */

/* Get the value of an environment variable */
var shell;
function getenv(varname, envname) {  // envname = "Process" | "System" | "User" | "Volatile"
  if (!envname) envname = "Process"; // By default, use the current environment
  if (!shell) shell = new ActiveXObject("WScript.Shell"); // Reuse the shell object
  var env = shell.Environment(envname);
  return env(varname);
}

/* Escape the reserved characters used by shell.SendKeys */
function EscapeKeys(string) {
  keys = "";
  for (var j = 0; j < string.length; j++) {
    var c = string.charAt(j);
    switch (c) {
    case '+':
    case '~':
    case '(':
    case ')':
    case '{':
    case '}':
      c = "{" + c + "}";
      break;
    case '^':
    case '%':
      c = "{" + c + "}";
      c = c + c;
      break;
    }
    keys += c;
  }
  return keys;
}

/* Process JScript command-line arguments */
function main(argc, argv) {
  var keys = "";
  for (var i = 1; i < argc; i++) {
    var arg = argv[i];
    switch (arg) {
    case "-s":
      var cmdline = "set " + argv[++i] + "=" + getenv(argv[++i]);
      cmdline = EscapeKeys(cmdline); // Escape the reserved characters used by SendKeys
      if (!shell) shell = WScript.CreateObject("WScript.Shell");
      shell.SendKeys("{ESC}" + cmdline + "{ENTER}"); // The ESC character empties cmd's input buffer
      return 0;
    default:
      WScript.Echo("Unexpected argument:" + arg);
      return 1;
    }
  }
}

/* Top level code, building a C-like environment */
var argv = [WScript.ScriptFullName];
var argc = 1 + WScript.Arguments.Length;
for (var i=1; i<argc; i++) argv[i] = WScript.Arguments.item(i-1);
var exitCode = main(argc, argv);
WScript.Quit(exitCode);
