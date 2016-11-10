@echo off
:##############################################################################
:#                                                                            #
:#  Filename:	    regx.bat						      #
:#                                                                            #
:#  Description:    Browse the registry "directories" and "files"             #
:#                                                                            #
:#  Notes:	    Pure batch file, using just Windows' reg.exe.	      #
:#		    Outputs in my SML structured format.		      #
:#									      #
:#                  Registry pathnames may contain & characters, which break  #
:#                  the set command if not quoted. Be careful to use quoting  #
:#		    consistently everywhere.				      #
:#									      #
:#		    To do: Manage complex multi-line values.		      #
:#		           Ex: HKLM\Cluster\Exchange\DagNetwork		      #
:#									      #
:#  History:                                                                  #
:#   2010-03-31 JFL created this batch file.       			      #
:#   2011-12-01 JFL Restructured.                  			      #
:#   2011-12-12 JFL Finalized change and fixed bugs.			      #
:#   2016-10-21 JFL Updated the debugging framework to the latest version.    #
:#                  Added commands del and rd.      			      #
:#   2016-11-09 JFL Changed all routines to return correct errorlevels.       #
:#                  Fixed the display of values containing '%' characters.    #
:#                  Fixed the display of values containing '?' characters.    #
:#                  Option -f now also forces deletions.                      #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2016-11-10"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set "ARGS=%*"

:# Mechanism for calling subroutines in a second external script instance.
:# Done by {%XCALL% label [arguments]}.
if .%1.==.-call. goto :call
set XCALL=cmd /c call "%ARG0%" -call

:# FOREACHLINE macro. (Change the delimiter to none to catch the whole lines.)
set FOREACHLINE=for /f "delims=" 

:# Default definitions
if not "%FULLPATH:~0,0%"=="" set FULLPATH=0

set "POPARG=call :PopArg"
call :Macro.Init
call :Debug.Init
call :Exec.Init
goto main

:call
shift
call :%1 %2 %3 %4 %5 %6 %7 %8 %9
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        PopArg                                                    #
:#                                                                            #
:#  Description     Pop the first arguments from %ARGS% into %ARG%            #
:#                                                                            #
:#  Arguments       %ARGS%	    Command line arguments                    #
:#                                                                            #
:#  Returns         %ARG%           The unquoted argument                     #
:#                  %"ARG"%         The actual argument, possibly quoted      #
:#                                                                            #
:#  Notes 	    Works around the defect of the shift command, which       #
:#                  pops the first argument from the %* list, but does not    #
:#                  remove it from %*.                                        #
:#                                                                            #
:#                  Use an inner call to make sure the argument parsing is    #
:#                  done by the actual cmd.exe parser. This guaranties that   #
:#                  arguments are split exactly as shift would have done.     #
:#                                                                            #
:#                  But call itself has a quirk, which requires a convoluted  #
:#                  workaround to process the /? argument.                    #
:#                                                                            #
:#                  To do: Detect if the last arg has mismatched quotes, and  #
:#                  if it does, append one.                                   #
:#                  Right now such mismatched quotes will cause an error here.#
:#                  It is easily feasible to work around this, but this is    #
:#                  useless as passing back an invalid argument like this     #
:#                  will only cause more errors further down.                 #
:#                                                                            #
:#  History                                                                   #
:#   2015-04-03 JFL Bug fix: Quoted args with an & inside failed to be poped. #
:#   2015-07-06 JFL Bug fix: Call quirk prevented inner call from popping /?. #
:#                                                                            #
:#----------------------------------------------------------------------------#

:PopArg
:# Gotcha: The call parser first scans its command line for an unquoted /?.
:# If it finds one anywhere on the command line, then it ignores the target label and displays call help.
:# To work around that, we initialize %ARG% and %"ARG"% with an impossible combination of values.
set "ARG=Yes"
set ""ARG"=No"
call :PopArg.Helper %ARGS% >NUL 2>NUL &:# Output redirections ensure the call help is not actually output.
:# Finding that impossible combination now is proof that the call was not executed.
:# In this case, try again with the /? quoted, to prevent the call parser from processing it.
:# Note that we can not systematically do this /? enquoting, else it's "/?" that would break the call.
if "%ARG%"=="Yes" if [%"ARG"%]==[No] call :PopArg.Helper %ARGS:/?="/?"% 
goto :eof
:PopArg.Helper
set "ARG=%~1"	&:# Remove quotes from the argument
set ""ARG"=%1"	&:# The same with quotes, if any, should we need them
:# Rebuild the tail of the argument line, as shift does not do it
:# Never quote the set ARGS command, else some complex quoted strings break
set ARGS=%2
:PopArg.GetNext
shift
if [%2]==[] goto :eof
:# Leave quotes in the tail of the argument line
set ARGS=%ARGS% %2
goto :PopArg.GetNext

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Inline macro functions                                    #
:#                                                                            #
:#  Description     Tools for defining inline functions,                      #
:#                  also known as macros by analogy with Unix shells macros   #
:#                                                                            #
:#  Macros          %MACRO%         Define the prolog code of a macro         #
:#                  %/MACRO%        Define the epilog code of a macro         #
:#                                                                            #
:#  Variables       %LF%            A Line Feed ASCII character '\x0A'        #
:#                  %LF2%           Generates a LF when expanded twice        #
:#                  %LF3%           Generates a LF when expanded 3 times      #
:#                                  Etc...                                    #
:#                  %\n%            Macro command line separator              #
:#                                                                            #
:#  Notes           The principle is to define a variable containing the      #
:#                  complete body of a function, like this:                   #
:#                  set $macro=for %%$ in (1 2) do if %%$==2 ( %\n%           #
:#                    :# Define the body of your macro here %\n%              #
:#                    :# Then return the result to the caller %\n%            #
:#                    for /f "delims=" %%r in ('echo.%!%RETVAL%!%') do ( %\n% #
:#                      endlocal %&% set "RETVAL=%%~r" %\n%                   #
:#                    ) %\n%                                                  #
:#                  ) else setlocal enableDelayedExpansion %&% set ARGS=      #
:#                                                                            #
:#                  It is then invoked just like an external command:         #
:#                  %$macro% ARG1 ARG2 ...                                    #
:#                                                                            #
:#                  The ideas that make all this possible were published on   #
:#                  the dostips.com web site, in multiple messages exchanged  #
:#                  by community experts.                                     #
:#                  By convention on the dostips.com web site, macro names    #
:#                  begin by a $ character; And the %\n% variable ends lines. #
:#                  The other variables are mine.                             #
:#                                                                            #
:#                  The use of a for loop executed twice, is critical for     #
:#                  allowing to place arguments behind the macro.             #
:#                  The first loop executes only the tail line, which defines #
:#                  the arguments; The second loop executes the main body of  #
:#                  the macro, which processes the arguments, and returns the #
:#                  result(s).                                                #
:#                  To improve the readability of macros, replace the code in #
:#                  the first line by %MACRO%, and the code in the last line  #
:#                  by %/MACRO%                                               #
:#                                                                            #
:#                  The use of the Line Feed character as command separator   #
:#                  within macros is a clever trick, that helps debugging,    #
:#                  but it is not necessary for macros to work.               #
:#                  This helps debugging, because this allows to output the   #
:#                  macro definition as a structured string spanning several  #
:#                  lines, looking exactly like a normal function with one    #
:#                  instruction per line.                                     #
:#                  But it would be equally possible to define macros using   #
:#                  the documented & character as command separator.          #
:#                                                                            #
:#                  Limitations:                                              #
:#                  - A macro cannot call another macro.                      #
:#                    (This would require escaping all control characters in  #
:#                     the sub-macro, so that they survive an additional      #
:#                     level of expansion.)                                   #
:#                                                                            #
:#  History                                                                   #
:#   2015-04-15 JFL Initial version, based on dostips.com samples, with       #
:#                  changes so that they work with DelayedExpansion on.       #
:#   2015-11-27 JFL Added a primitive macro debugging capability.             #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Macro.Init
goto :Macro.End

:Macro.Init
:# Define a LF variable containing a Line Feed ('\x0A')
:# The two blank lines below are necessary.
set LF=^


:# End of define Line Feed. The two blank lines above are necessary.

:# LF generator variables, that become an LF after N expansions
:# %LF1% == %LF% ; %LF2% == To expand twice ; %LF3% == To expand 3 times ; Etc
:# Starting with LF2, the right # of ^ doubles on every line,
:# and the left # of ^ is 3 times the right # of ^.
set ^"LF1=^%LF%%LF%"
set ^"LF2=^^^%LF1%%LF1%^%LF1%%LF1%"
set ^"LF3=^^^^^^%LF2%%LF2%^^%LF2%%LF2%"
set ^"LF4=^^^^^^^^^^^^%LF3%%LF3%^^^^%LF3%%LF3%"
set ^"LF5=^^^^^^^^^^^^^^^^^^^^^^^^%LF4%%LF4%^^^^^^^^%LF4%%LF4%"

:# Variables for use in inline macro functions
set ^"\n=%LF3%^^^"	&:# Insert a LF and continue macro on next line
set "^!=^^^^^^^!"	&:# Define a %!%DelayedExpansion%!% variable
set "'^!=^^^!"		&:# Idem, but inside a quoted string
set ">=^^^>"		&:# Insert a redirection character
set "&=^^^&"		&:# Insert a command separator in a macro
:# Idem, to be expanded twice, for use in macros within macros
set "^!2=^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^!"
set "'^!2=^^^^^^^!"
set "&2=^^^^^^^^^^^^^^^&"

set "MACRO=for %%$ in (1 2) do if %%$==2"				&:# Prolog code of a macro
set "/MACRO=else setlocal enableDelayedExpansion %&% set MACRO.ARGS="	&:# Epilog code of a macro

set "ON_MACRO_EXIT=for /f "delims=" %%r in ('echo"	&:# Begin the return variables definitions 
set "/ON_MACRO_EXIT=') do endlocal %&% %%r"		&:# End the return variables definitions

:# Primitive macro debugging definitions
:# Macros, usable anywhere, including within other macros, for conditionally displaying debug information
:# Use option -xd to set a > 0 macro debugging level.
:# Usage: %IF_XDLEVEL% N command
:# Runs command if the current macro debugging level is at least N.
:# Ex: %IF_XDLEVEL% 2 set VARIABLE
:# Recommended: Use set, instead of echo, to display variable values. This is sometimes
:# annoying because this displays other unwanted variables. But this is the only way
:# to be sure to display _all_ tricky characters correctly in any expansion mode. 
:# Note: These debugging macros slow down a lot their enclosing macro.
:#       They should be removed from the released code.
set "XDLEVEL=0" &:# 0=No macro debug; 1=medium debug; 2=full debug; 3=Even more debug
set "IF_XDLEVEL=for /f %%' in ('call echo.%%XDLEVEL%%') do if %%' GEQ"

:# While at it, and although this is unrelated to macros, define other useful ASCII control codes
:# Define a CR variable containing a Carriage Return ('\x0D')
for /f %%a in ('copy /Z %COMSPEC% nul') do set "CR=%%a"

:# Define a BS variable containing a BackSpace ('\x08')
:# Use prompt to store a  backspace+space+backspace into a DEL variable.
for /F "tokens=1 delims=#" %%a in ('"prompt #$H# & echo on & for %%b in (1) do rem"') do set "DEL=%%a"
:# Then extract the first backspace
set "BS=%DEL:~0,1%"

goto :eof
:Macro.end

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Debug routines					      #
:#                                                                            #
:#  Description     A collection of debug routines                            #
:#                                                                            #
:#  Functions       Debug.Init	    Initialize debugging. Call once at first. #
:#                  Debug.Off	    Disable the debugging mode		      #
:#                  Debug.On	    Enable the debugging mode		      #
:#                  Debug.SetLog    Set the log file         		      #
:#                  Debug.Entry	    Log entry into a routine		      #
:#                  Debug.Return    Log exit from a routine		      #
:#                  Verbose.Off	    Disable the verbose mode                  #
:#                  Verbose.On	    Enable the verbose mode		      #
:#                  Echo	    Echo and log strings, indented            #
:#                  EchoVars	    Display the values of a set of variables  #
:#                  EchoArgs	    Display the values of all arguments       #
:#                                                                            #
:#  Macros          %FUNCTION%	    Define and trace the entry in a function. #
:#                  %UPVAR%         Declare a var. to pass back to the caller.#
:#                  %RETURN%        Return from a function and trace it       #
:#                                                                            #
:#                  Always match uses of %FUNCTION% and %RETURN%. That is     #
:#                  never use %RETURN% if there was no %FUNCTION% before it.  #
:#                                                                            #
:#                  :# Example of a factorial routine using this framework    #
:#                  :Fact                                                     #
:#                  %FUNCTION% enableextensions enabledelayedexpansion        #
:#		    %UPVAR% RETVAL					      #
:#                  set N=%1                                                  #
:#                  if .%N%.==.0. (                                           #
:#                    set RETVAL=1                                            #
:#                  ) else (                                                  #
:#                    set /A M=N-1                                            #
:#                    call :Fact !M!                                          #
:#                    set /A RETVAL=N*RETVAL                                  #
:#                  )                                                         #
:#                  %RETURN%					              #
:#                                                                            #
:#                  %ECHO%	    Echo and log a string, indented           #
:#                  %LOG%	    Log a string, indented                    #
:#                  %ECHO.V%	    Idem, but display it in verbose mode only #
:#                  %ECHO.D%	    Idem, but display it in debug mode only   #
:#                                                                            #
:#                  %ECHOVARS%	    Indent, echo and log variables values     #
:#                  %ECHOVARS.V%    Idem, but display them in verb. mode only #
:#                  %ECHOVARS.D%    Idem, but display them in debug mode only #
:#                                                                            #
:#                  %IF_DEBUG%      Execute a command in debug mode only      #
:#                  %IF_VERBOSE%    Execute a command in verbose mode only    #
:#                                                                            #
:#                  %FUNCTION0%	    Weak functions with no local variables.   #
:#                  %RETURN0%       Return from a %FUNCTION0% and trace it    #
:#                  %RETURN#%       Idem, with comments after the return      #
:#                                                                            #
:#  Variables       %>DEBUGOUT%     Debug output redirect. Either "" or ">&2".#
:#                  %LOGFILE%       Log file name. Inherited. Default=""==NUL #
:#                                  Always set using call :Debug.SetLog       #
:#                  %DEBUG%         Debug mode. 0=Off; 1=On. Use functions    #
:#                                  Debug.Off and Debug.On to change it.      #
:#                                  Inherited. Default=0.                     #
:#                  %VERBOSE%       Verbose mode. 0=Off; 1=On. Use functions  #
:#                                  Verbose.Off and Verbose.On to change it.  #
:#                                  Inherited. Default=0.                     #
:#                  %INDENT%        Spaces to put ahead of all debug output.  #
:#                                  Inherited. Default=. (empty string)       #
:#                                                                            #
:#  Notes           All output from these routines is sent to the log file.   #
:#                  In debug mode, the debug output is also sent to stderr.   #
:#                                                                            #
:#                  Traced functions are indented, based on the call depth.   #
:#                  Use %ECHO% to get the same indentation of normal output.  #
:#                                                                            #
:#                  The output format matches the batch language syntax       #
:#                  exactly. This allows copying the debug output directly    #
:#                  into another command window, to check troublesome code.   #
:#                                                                            #
:#  History                                                                   #
:#   2011-11-15 JFL Split Debug.Init from Debug.Off, to improve clarity.      #
:#   2011-12-12 JFL Output debug information to stderr, so that stdout can be #
:#                  used for returning information from the subroutine.       #
:#   2011-12-13 JFL Standardize use of RETVAR/RETVAL, and display it on return.
:#   2012-07-09 JFL Restructured functions to a more "object-like" style.     #
:#                  Added the three flavors of the Echo and EchoVars routines.#
:#   2012-07-19 JFL Added optimizations to improve performance in non-debug   #
:#                  and non-verbose mode. Added routine Debug.SetLog.         #
:#   2012-11-13 JFL Added macro LOG. Fixed setlocal bug in :EchoVars.         #
:#   2013-08-27 JFL Changed %RETURN% to do exit /b. This allows returning     #
:#                  an errorlevel by doing: %RETURN% %ERRORLEVEL%             #
:#   2013-11-12 JFL Added macros %IF_DEBUG% and %IF_VERBOSE%.                 #
:#   2013-12-04 JFL Added variable %>DEBUGOUT% to allow sending debug output  #
:#                  either to stdout or to stderr.                            #
:#   2015-10-29 JFL Added macro %RETURN#% to return with a comment.           #
:#   2015-11-19 JFL %FUNCTION% now automatically generates its name & %* args.#
:#                  (Simplifies usage, but comes at a cost of about a 5% slow #
:#                   down when running in debug mode.)                        #
:#                  Added an %UPVAR% macro allowing to define the list of     #
:#                  variables that need to make it back to the caller.        #
:#                  %RETURN% (Actually the Debug.return routine) now handles  #
:#                  this variable back propagation using the (goto) trick.    #
:#                  This works well, but the performance is poor.             #
:#   2015-11-25 JFL Rewrote the %FUNCTION% and %RETURN% macros to manage      #
:#                  most common cases without calling a subroutine. This      #
:#                  resolves the performance issues of the previous version.  #
:#   2015-11-27 JFL Redesigned the problematic character return mechanism     #
:#                  using a table of predefined generic entities. Includes    #
:#                  support for returning strings with CR & LF.		      #
:#   2015-11-29 JFL Streamlined the macros and added lots of comments.        #
:#                  The FUNCTION macro now runs with expansion enabled, then  #
:#                  does a second setlocal in the end as requested.           #
:#                  The RETURN macro now displays strings in debug mode with  #
:#                  delayed expansion enabled. This fixes issues with CR & LF.#
:#                  Added a backspace entity.                                 #
:#   2015-12-01 JFL Bug fix: %FUNCTION% with no arg did change the exp. mode. #
:#   2016-09-01 JFL Bug fix: %RETURN% incorrectly returned empty variables.   #
:#   2016-11-02 JFL Bug fix: Avoid log file redirection failures in recursive #
:#                  scripts.                                                  #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Debug.Init
goto :Debug.End

:Debug.Init
:# Preliminary checks to catch common problems
if exist echo >&2 echo WARNING: The file "echo" in the current directory will cause problems. Please delete it and retry.
:# Inherited variables from the caller: DEBUG, VERBOSE, INDENT, >DEBUGOUT
:# Initialize other debug variables
set "ECHO=call :Echo"
set "ECHOVARS=call :EchoVars"
:# Define variables for problematic characters, that cause parsing issues
:# Use the ASCII control character name, or the html entity name.
:# Warning: The excl and hat characters need different quoting depending on context.
set "DEBUG.percnt=%%"	&:# One percent sign
set "DEBUG.excl=^!"	&:# One exclamation mark
set "DEBUG.hat=^"	&:# One caret, aka. circumflex accent, or hat sign
set ^"DEBUG.quot=""	&:# One double quote
set "DEBUG.apos='"	&:# One apostrophe
set "DEBUG.amp=&"	&:# One ampersand
set "DEBUG.vert=|"	&:# One vertical bar
set "DEBUG.gt=>"	&:# One greater than sign
set "DEBUG.lt=<"	&:# One less than sign
set "DEBUG.lpar=("	&:# One left parenthesis
set "DEBUG.rpar=)"	&:# One right parenthesis
set "DEBUG.lbrack=["	&:# One left bracket
set "DEBUG.rbrack=]"	&:# One right bracket
set "DEBUG.sp= "	&:# One space
set "DEBUG.tab=	"	&:# One tabulation
set "DEBUG.quest=?"	&:# One question mark
set "DEBUG.ast=*"	&:# One asterisk
set "DEBUG.cr=!CR!"	&:# One carrier return
set "DEBUG.lf=!LF!"	&:# One line feed
set "DEBUG.bs=!BS!"	&:# One backspace
:# The FUNCTION, UPVAR, and RETURN macros should work with delayed expansion on or off
set MACRO.GETEXP=(if "%'!2%%'!2%"=="" (set MACRO.EXP=EnableDelayedExpansion) else set MACRO.EXP=DisableDelayedExpansion)
set UPVAR=call set DEBUG.RETVARS=%%DEBUG.RETVARS%%
set RETURN=call set "DEBUG.ERRORLEVEL=%%ERRORLEVEL%%" %&% %MACRO% ( %\n%
  set DEBUG.EXITCODE=%!%MACRO.ARGS%!%%\n%
  if defined DEBUG.EXITCODE set DEBUG.EXITCODE=%!%DEBUG.EXITCODE: =%!%%\n%
  if not defined DEBUG.EXITCODE set DEBUG.EXITCODE=%!%DEBUG.ERRORLEVEL%!%%\n%
  set "DEBUG.SETARGS=" %\n%
  for %%v in (%!%DEBUG.RETVARS%!%) do ( %\n%
    set "DEBUG.VALUE=%'!%%%v%'!%" %# We must remove problematic characters in that value #% %\n%
    if defined DEBUG.VALUE ( %# Else the following lines will generate phantom characters #% %\n%
      set "DEBUG.VALUE=%'!%DEBUG.VALUE:%%=%%DEBUG.percnt%%%'!%"	%# Encode percent #% %\n%
      for %%e in (sp tab cr lf quot) do for %%c in ("%'!%DEBUG.%%e%'!%") do ( %# Encode named character entities #% %\n%
	set "DEBUG.VALUE=%'!%DEBUG.VALUE:%%~c=%%DEBUG.%%e%%%'!%" %\n%
      ) %\n%
      set "DEBUG.VALUE=%'!%DEBUG.VALUE:^^=%%DEBUG.hat%%%'!%"	%# Encode carets #% %\n%
      call set "DEBUG.VALUE=%%DEBUG.VALUE:%!%=^^^^%%" 		%# Encode exclamation points #% %\n%
      set "DEBUG.VALUE=%'!%DEBUG.VALUE:^^^^=%%DEBUG.excl%%%'!%"	%# Encode exclamation points #% %\n%
      set "DEBUG.VALUE=%'!%DEBUG.VALUE:?=%%DEBUG.quest%%%'!%"	%# Encode question marks #% %\n%
    ) %\n%
    set DEBUG.SETARGS=%!%DEBUG.SETARGS%!% "%%v=%'!%DEBUG.VALUE%'!%" %\n%
  ) %\n%
  if %!%DEBUG%!%==1 ( %# Build the debug message and display it #% %\n%
    set "DEBUG.MSG=return %'!%DEBUG.EXITCODE%'!%" %\n%
    for %%v in (%!%DEBUG.SETARGS%!%) do ( %\n%
      set "DEBUG.MSG=%'!%DEBUG.MSG%'!% %%DEBUG.amp%% set %%v" %!% %\n%
    ) %\n%
    call set "DEBUG.MSG=%'!%DEBUG.MSG:%%=%%DEBUG.excl%%%'!%" %# Change all percent to ! #%  %\n%
    if defined ^^%>%DEBUGOUT ( %# If we use a debugging stream distinct from stdout #% %\n%
      call :Echo.Eval2DebugOut %!%DEBUG.MSG%!%%# Use a helper routine, as delayed redirection does not work #%%\n%
    ) else ( %# Output directly here, which is faster #% %\n%
      for /f "delims=" %%c in ("%'!%INDENT%'!%%'!%DEBUG.MSG%'!%") do echo %%c%# Use a for loop to do a double !variable! expansion #% %\n%
    ) %\n%
    if defined LOGFILE ( %# If we have to send a copy to a log file #% %\n%
      call :Echo.Eval2LogFile %!%DEBUG.MSG%!%%# Use a helper routine, as delayed redirection does not work #%%\n%
    ) %\n%
  ) %\n%
  for %%r in (%!%DEBUG.EXITCODE%!%) do ( %# Carry the return values through the endlocal barriers #% %\n%
    for /f "delims=" %%a in (""" %'!%DEBUG.SETARGS%'!%") do ( %# The initial "" makes sure the body runs even if the arg list is empty #% %\n%
      endlocal %&% endlocal %&% endlocal %# Exit the RETURN and FUNCTION local scopes #% %\n%
      if "%'!%%'!%"=="" ( %# Delayed expansion is ON #% %\n%
	set "DEBUG.SETARGS=%%a" %\n%
	call set "DEBUG.SETARGS=%'!%DEBUG.SETARGS:%%=%%DEBUG.excl%%%'!%" %# Change all percent to ! #%  %\n%
	for %%v in (%!%DEBUG.SETARGS:~3%!%) do ( %\n%
	  set %%v %# Set each upvar variable in the caller's scope #% %\n%
	) %\n%
	set "DEBUG.SETARGS=" %\n%
      ) else ( %# Delayed expansion is OFF #% %\n%
	set "DEBUG.hat=^^^^" %# Carets need to be doubled to be set right below #% %\n%
	for %%v in (%%a) do if not %%v=="" ( %\n%
	  call set %%v %# Set each upvar variable in the caller's scope #% %\n%
	) %\n%
	set "DEBUG.hat=^^" %# Restore the normal value with a single caret #% %\n%
      ) %\n%
      exit /b %%r %# Return to the caller #% %\n%
    ) %\n%
  ) %\n%
) %/MACRO%
:Debug.Init.2
set "LOG=call :Echo.Log"
set ">>LOGFILE=>>%LOGFILE%"
if not defined LOGFILE set "LOG=rem" & set ">>LOGFILE=rem"
if .%LOGFILE%.==.NUL. set "LOG=rem" & set ">>LOGFILE=rem"
if .%NOREDIR%.==.1. set "LOG=rem" & set ">>LOGFILE=rem" &:# A parent script is already redirecting output. Trying to do it again here would fail. 
set "ECHO.V=call :Echo.Verbose"
set "ECHO.D=call :Echo.Debug"
set "ECHOVARS.V=call :EchoVars.Verbose"
set "ECHOVARS.D=call :EchoVars.Debug"
:# Variables inherited from the caller...
:# Preserve INDENT if it contains just spaces, else clear it.
for /f %%s in ('echo.%INDENT%') do set "INDENT="
:# Preserve the log file name, else by default use NUL.
:# if not defined LOGFILE set "LOGFILE=NUL"
:# VERBOSE mode can only be 0 or 1. Default is 0.
if not .%VERBOSE%.==.1. set "VERBOSE=0"
call :Verbose.%VERBOSE%
:# DEBUG mode can only be 0 or 1. Default is 0.
if not .%DEBUG%.==.1. set "DEBUG=0"
goto :Debug.%DEBUG%

:Debug.SetLog
set "LOGFILE=%~1"
goto :Debug.Init.2

:Debug.Off
:Debug.0
set "DEBUG=0"
set "DEBUG.ENTRY=rem"
set "IF_DEBUG=if .%DEBUG%.==.1."
set "FUNCTION0=rem"
set FUNCTION=%MACRO.GETEXP% %&% %MACRO% ( %\n%
  call set "FUNCTION.NAME=%%0" %\n%
  call set "ARGS=%%*"%\n%
  set "DEBUG.RETVARS=" %\n%
  if not defined MACRO.ARGS set "MACRO.ARGS=%'!%MACRO.EXP%'!%" %\n%
  setlocal %!%MACRO.ARGS%!% %\n%
) %/MACRO%
set "RETURN0=exit /b"
set "RETURN#=exit /b & rem"
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -d=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Optimization to speed things up in non-debug mode
if not defined LOGFILE set "ECHO.D=rem"
if .%LOGFILE%.==.NUL. set "ECHO.D=rem"
if not defined LOGFILE set "ECHOVARS.D=rem"
if .%LOGFILE%.==.NUL. set "ECHOVARS.D=rem"
goto :eof

:Debug.On
:Debug.1
set "DEBUG=1"
set "DEBUG.ENTRY=:Debug.Entry"
set "IF_DEBUG=if .%DEBUG%.==.1."
set "FUNCTION0=call call :Debug.Entry0 %%0 %%*"
set FUNCTION=%MACRO.GETEXP% %&% %MACRO% ( %\n%
  call set "FUNCTION.NAME=%%0" %\n%
  call set "ARGS=%%*"%\n%
  if %!%DEBUG%!%==1 ( %# Build the debug message and display it #% %\n%
    if defined ^^%>%DEBUGOUT ( %# If we use a debugging stream distinct from stdout #% %\n%
      call :Echo.2DebugOut call %!%FUNCTION.NAME%!% %!%ARGS%!%%# Use a helper routine, as delayed redirection does not work #%%\n%
    ) else ( %# Output directly here, which is faster #% %\n%
      echo%!%INDENT%!% call %!%FUNCTION.NAME%!% %!%ARGS%!%%\n%
    ) %\n%
    if defined LOGFILE ( %# If we have to send a copy to a log file #% %\n%
      call :Echo.2LogFile call %!%FUNCTION.NAME%!% %!%ARGS%!%%# Use a helper routine, as delayed redirection does not work #%%\n%
    ) %\n%
    call set "INDENT=%'!%INDENT%'!%  " %\n%
  ) %\n%
  set "DEBUG.RETVARS=" %\n%
  if not defined MACRO.ARGS set "MACRO.ARGS=%'!%MACRO.EXP%'!%" %\n%
  setlocal %!%MACRO.ARGS%!% %\n%
) %/MACRO%
set "RETURN0=call :Debug.Return0 & exit /b"
:# Macro for displaying comments on the return log line
set RETURN#=set "RETURN#ERR=%'!%ERRORLEVEL%'!%" %&% %MACRO% ( %\n%
  set RETVAL=%!%MACRO.ARGS:~1%!%%\n%
  call :Debug.Return0 %!%RETURN#ERR%!% %\n%
  %ON_MACRO_EXIT% set "INDENT=%'!%INDENT%'!%" %/ON_MACRO_EXIT% %&% set "RETURN#ERR=" %&% exit /b %\n%
) %/MACRO%
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -d=% -d"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Reverse the above optimization
set "ECHO.D=call :Echo.Debug"
set "ECHOVARS.D=call :EchoVars.Debug"
goto :eof

:Debug.Entry0
setlocal DisableDelayedExpansion
%>DEBUGOUT% echo %INDENT%call %*
if defined LOGFILE %>>LOGFILE% echo %INDENT%call %*
endlocal
set "INDENT=%INDENT%  "
goto :eof

:Debug.Entry
setlocal DisableDelayedExpansion
%>DEBUGOUT% echo %INDENT%call %FUNCTION.NAME% %ARGS%
if defined LOGFILE %>>LOGFILE% echo %INDENT%call %FUNCTION.NAME% %ARGS%
endlocal
set "INDENT=%INDENT%  "
goto :eof

:Debug.Return0 %1=Optional exit code
%>DEBUGOUT% echo %INDENT%return !RETVAL!
if defined LOGFILE %>>LOGFILE% echo %INDENT%return !RETVAL!
set "INDENT=%INDENT:~0,-2%"
exit /b %1

:# Routine to set the VERBOSE mode, in response to the -v argument.
:Verbose.Off
:Verbose.0
set "VERBOSE=0"
set "IF_VERBOSE=if .%VERBOSE%.==.1."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -v=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Optimization to speed things up in non-verbose mode
if not defined LOGFILE set "ECHO.V=rem"
if .%LOGFILE%.==.NUL. set "ECHO.V=rem"
if not defined LOGFILE set "ECHOVARS.V=rem"
if .%LOGFILE%.==.NUL. set "ECHOVARS.V=rem"
goto :eof

:Verbose.On
:Verbose.1
set "VERBOSE=1"
set "IF_VERBOSE=if .%VERBOSE%.==.1."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -v=% -v"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Reverse the above optimization
set "ECHO.V=call :Echo.Verbose"
set "ECHOVARS.V=call :EchoVars.Verbose"
goto :eof

:# Echo and log a string, indented at the same level as the debug output.
:Echo
echo.%INDENT%%*
:Echo.Log
if defined LOGFILE %>>LOGFILE% echo.%INDENT%%*
goto :eof

:Echo.Verbose
%IF_VERBOSE% goto :Echo
goto :Echo.Log

:Echo.Debug
%IF_DEBUG% goto :Echo
goto :Echo.Log

:Echo.Eval2DebugOut %*=String with variables that need to be evaluated first
:# Must be called with delayed expansion on, so that !variables! within %* get expanded
:Echo.2DebugOut	%*=String to output to the DEBUGOUT stream
%>DEBUGOUT% echo.%INDENT%%*
goto :eof

:Echo.Eval2LogFile %*=String with variables that need to be evaluated first
:# Must be called with delayed expansion on, so that !variables! within %* get expanded
:Echo.2LogFile %*=String to output to the LOGFILE
%>>LOGFILE% echo.%INDENT%%*
goto :eof

:# Echo and log variable values, indented at the same level as the debug output.
:EchoVars
setlocal EnableExtensions EnableDelayedExpansion
:EchoVars.loop
if "%~1"=="" endlocal & goto :eof
%>DEBUGOUT% echo %INDENT%set "%~1=!%~1!"
if defined LOGFILE %>>LOGFILE% echo %INDENT%set "%~1=!%~1!"
shift
goto EchoVars.loop

:EchoVars.Verbose
%IF_VERBOSE% (
  call :EchoVars %*
) else ( :# Make sure the variables are logged
  call :EchoVars %* >NUL 2>NUL
)
goto :eof

:EchoVars.Debug
%IF_DEBUG% (
  call :EchoVars %*
) else ( :# Make sure the variables are logged
  call :EchoVars %* >NUL 2>NUL
)
goto :eof

:# Echo a list of arguments.
:EchoArgs
setlocal EnableExtensions DisableDelayedExpansion
set N=0
:EchoArgs.loop
if .%1.==.. endlocal & goto :eof
set /a N=N+1
%>DEBUGOUT% echo %INDENT%set "ARG%N%=%1"
shift
goto EchoArgs.loop

:Debug.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Exec                                                      #
:#                                                                            #
:#  Description     Run a command, logging its output to the log file.        #
:#                                                                            #
:#                  In VERBOSE mode, display the command line first.          #
:#                  In DEBUG mode, display the command line and the exit code.#
:#                  In NOEXEC mode, display the command line, but don't run it.
:#                                                                            #
:#  Arguments       -t          Tee all output to the log file if there's a   #
:#                              usable tee.exe. Default: Redirect all >> log. #
:#                              Known limitation: The exit code is always 0.  #
:#                  %*          The command and its arguments                 #
:#                              Quote redirection operators. Ex:              #
:#                              %EXEC% find /I "error" "<"logfile.txt ">"NUL  #
:#                                                                            #
:#  Functions       Exec.Init	Initialize Exec routines. Call once at 1st    #
:#                  Exec.Off	Disable execution of commands		      #
:#                  Exec.On	Enable execution of commands		      #
:#                  Do          Always execute a command, logging its output  #
:#                  Exec	Conditionally execute a command, logging it.  #
:#                                                                            #
:#  Macros          %DO%        Always execute a command, logging its output  #
:#                  %EXEC%      Conditionally execute a command, logging it.  #
:#                  %ECHO.X%    Echo and log a string, indented, in -X mode.  #
:#                  %ECHO.XVD%  Echo a string, indented, in -X or -V or -D    #
:#                              modes; Log it always.                         #
:#                              Useful to display commands in cases where     #
:#                              %EXEC% can't be used, like in for ('cmd') ... #
:#                  %IF_EXEC%   Execute a command if _not_ in NOEXEC mode     #
:#                  %IF_NOEXEC% Execute a command in NOEXEC mode only         #
:#                                                                            #
:#  Variables       %NOEXEC%	Exec mode. 0=Execute commands; 1=Don't. Use   #
:#                              functions Exec.Off and Exec.On to change it.  #
:#                              Inherited from the caller. Default=On.	      #
:#                  %NOREDIR%   0=Log command output to the log file; 1=Don't #
:#                              Inherited. Default=0.                         #
:#                              Useful in cases where the output must be      #
:#                              shown to the user, and no tee.exe is available.
:#                  %EXEC.ARGS%	Arguments to recursively pass to subcommands  #
:#                              with the same execution options conventions.  #
:#                                                                            #
:#  Notes           This framework can't be used from inside () blocks.       #
:#                  This is because these blocks are executed separately      #
:#                  in a child shell.                                         #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-19 JFL Created this routine.                                     #
:#   2012-05-04 JFL Support logging ">" redirections.                         #
:#   2012-07-09 JFL Restructured functions to a more "object-like" style.     #
:#   2012-07-11 JFL Support logging both "<" and ">" redirections.            #
:#   2012-09-18 JFL Added macro %ECHO.X% for cases where %EXEC% can't be used.#
:#   2012-11-13 JFL Support for "|" pipes too.                                #
:#   2013-11-12 JFL Added macro %IF_NOEXEC%.                                  #
:#   2013-12-04 JFL Added option -t to tee the output if possible.            #
:#                  Split %ECHO.X% and %ECHO.XVD%.                            #
:#   2014-05-13 JFL Call tee.exe explicitely, to avoid problems if there's    #
:#                  also a tee.bat script in the path.                        #
:#   2015-03-12 JFL If there are output redirections, then cancel any attempt #
:#		    at redirecting output to the log file.		      #
:#   2016-10-19 JFL Bug fix: Make sure the :Exec initialization preserves the #
:#                  errorlevel that was there on entrance.                    #
:#   2016-11-02 JFL Bug fix: Avoid log file redirection failures in recursive #
:#                  scripts.                                                  #
:#   2016-11-05 JFL Fixed :Exec bug in XP/64.				      #
:#                  Indent sub-scripts output in debug mode.                  #
:#   2016-11-06 JFL Updated the 10/19 errorlevel fix to work for DO and EXEC. #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Exec.Init
goto :Exec.End

:# Global variables initialization, to be called first in the main routine
:Exec.Init
set "DO=call :Do"
set "EXEC=call :Exec"
set "ECHO.X=call :Echo.NoExec"
set "ECHO.XVD=call :Echo.XVD"
if not .%NOEXEC%.==.1. set "NOEXEC=0"
:# Check if there's a tee.exe program available
set "Exec.HaveTee=0"
tee.exe --help >NUL 2>NUL
if not errorlevel 1 set "Exec.HaveTee=1"
:# Initialize ERRORLEVEL with known values
set "TRUE.EXE=(call,)"	&:# Macro to silently set ERRORLEVEL to 0
set "FALSE.EXE=(call)"	&:# Macro to silently set ERRORLEVEL to 1
goto :NoExec.%NOEXEC%

:Exec.On
:NoExec.0
set "NOEXEC=0"
set "IF_NOEXEC=if .%NOEXEC%.==.1."
set "IF_EXEC=if .%NOEXEC%.==.0."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -X=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
goto :eof

:# Routine to set the NOEXEC mode, in response to the -X argument.
:Exec.Off
:NoExec.1
set "NOEXEC=1"
set "IF_NOEXEC=if .%NOEXEC%.==.1."
set "IF_EXEC=if .%NOEXEC%.==.0."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -X=% -X"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
goto :eof

:Echo.NoExec
%IF_NOEXEC% goto :Echo
goto :eof

:Echo.XVD
%IF_NOEXEC% goto :Echo
%IF_VERBOSE% goto :Echo
%IF_DEBUG% goto :Echo
goto :Echo.Log

:Exec.SetErrorLevel %1
exit /b %1

:# Execute a command, logging its output.
:# Use for informative commands that should always be run, even in NOEXEC mode. 
:Do
set "Exec.RestoreErr=call :Exec.SetErrorLevel %ERRORLEVEL%" &:# Save the initial errorlevel: Build a command for restoring it later
setlocal EnableExtensions DisableDelayedExpansion
set NOEXEC=0
set "IF_NOEXEC=if .%NOEXEC%.==.1."
goto :Exec.Start

:# Execute critical operations that should not be run in NOEXEC mode.
:# Version supporting input and output redirections, and pipes.
:# Redirection operators MUST be surrounded by quotes. Ex: "<" or ">" or "2>>"
:Exec
set "Exec.RestoreErr=call :Exec.SetErrorLevel %ERRORLEVEL%" &:# Save the initial errorlevel: Build a command for restoring it later
setlocal EnableExtensions DisableDelayedExpansion
:Exec.Start
set "NOREDIR0=%NOREDIR%"
set "Exec.Redir=>>%LOGFILE%,2>&1"
if .%NOREDIR%.==.1. set "Exec.Redir="
if not defined LOGFILE set "Exec.Redir="
if /i .%LOGFILE%.==.NUL. set "Exec.Redir="
:# Process optional arguments
set "Exec.GotCmd=Exec.GotCmd"   &:# By default, the command line is %* for :Exec
goto :Exec.GetArgs
:Exec.NextArg
set "Exec.GotCmd=Exec.BuildCmd" &:# An :Exec argument was found, we'll have to rebuild the command line
shift
:Exec.GetArgs
if "%~1"=="-L" set "Exec.Redir=" & goto :Exec.NextArg :# Do not send the output to the log file
if "%~1"=="-t" if defined LOGFILE ( :# Tee the output to the log file
  :# Warning: This prevents from getting the command exit code!
  if .%Exec.HaveTee%.==.1. if not .%NOREDIR%.==.1. set "Exec.Redir= 2>&1 | tee.exe -a %LOGFILE%"
  goto :Exec.NextArg
)
set Exec.Cmd=%*
goto :%Exec.GotCmd%
:Exec.BuildCmd
:# Build the command list. Cannot use %*, which still contains the :Exec switches processed above.
set Exec.Cmd=%1
:Exec.GetCmdLoop
shift
if not .%1.==.. set Exec.Cmd=%Exec.Cmd% %1& goto :Exec.GetCmdLoop
:Exec.GotCmd
:# First stage: Split multi-char ops ">>" "2>" "2>>". Make sure to keep ">" signs quoted every time.
:# Do NOT use surrounding quotes for these set commands, else quoted arguments will break.
set Exec.Cmd=%Exec.Cmd:">>"=">"">"%
set Exec.Cmd=%Exec.Cmd:">>&"=">"">""&"%
set Exec.Cmd=%Exec.Cmd:">&"=">""&"%
:# If there are output redirections, then cancel any attempt at redirecting output to the log file.
set "Exec.Cmd1=%Exec.Cmd:"=%" &:# Remove quotes in the command string, to allow quoting the whole string.
if not "%Exec.Cmd1:>=%"=="%Exec.Cmd1%" set "Exec.Redir="
if defined Exec.Redir set "NOREDIR=1" &:# make sure child scripts do not try to redirect output again 
:# Second stage: Convert quoted redirection operators (Ex: ">") to a usable (Ex: >) and a displayable (Ex: ^>) value.
:# Must be once for each of the four < > | & operators.
:# Since each operation removes half of ^ escape characters, then insert
:# enough ^ to still protect the previous characters during the subsequent operations.
set Exec.toEcho=%Exec.Cmd:"|"=^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^|%
set Exec.toEcho=%Exec.toEcho:"&"=^^^^^^^^^^^^^^^&%
set Exec.toEcho=%Exec.toEcho:">"=^^^^^^^>%
set Exec.toEcho=%Exec.toEcho:"<"=^^^<%
:# Finally create the usable command, by removing the last level of ^ escapes.
set Exec.Cmd=%Exec.toEcho%
set "Exec.Echo=rem"
%IF_NOEXEC% set "Exec.Echo=echo"
%IF_DEBUG% set "Exec.Echo=echo"
%IF_VERBOSE% set "Exec.Echo=echo"
%>DEBUGOUT% %Exec.Echo%.%INDENT%%Exec.toEcho%
if defined LOGFILE %>>LOGFILE% echo.%INDENT%%Exec.toEcho%
:# Constraints at this stage:
:# The command exit code must make it through, back to the caller.
:# The local variables must disappear before return.
:# But the new variables created by the command must make it through.
:# This should work whether :Exec is called with delayed expansion on or off.
endlocal & if not .%NOEXEC%.==.1. (
  set "NOREDIR=%NOREDIR%"
  %IF_DEBUG% set "INDENT=%INDENT%  "
  %Exec.RestoreErr% &:# Restore the errorlevel we had on :Exec entrance
  %Exec.Cmd%%Exec.Redir%
  set "Exec.ErrorLevel=!ERRORLEVEL!"
  set "NOREDIR=%NOREDIR0%" &:# Sets ERRORLEVEL=1 in Windows XP/64
  %IF_DEBUG% set "INDENT=%INDENT%"
  call :Exec.ShowExitCode !Exec.ErrorLevel!
)
goto :eof

:Exec.ShowExitCode %1
set "Exec.ErrorLevel="
set "Exec.RestoreErr="
%IF_DEBUG% %>DEBUGOUT% echo.%INDENT%  exit %1
if defined LOGFILE %>>LOGFILE% echo.%INDENT%  exit %1
exit /b %1

:Exec.End

:#----------------------------------------------------------------------------#
:#                        End of the debugging library                        #
:#----------------------------------------------------------------------------#

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        condquote                                                 #
:#                                                                            #
:#  Description     Add quotes around the content of a pathname if needed     #
:#                                                                            #
:#  Arguments       %1	    Source variable name                              #
:#                  %2	    Destination variable name (optional)              #
:#                                                                            #
:#  Notes 	    Quotes are necessary if the pathname contains special     #
:#                  characters, like spaces, &, |, etc.                       #
:#                                                                            #
:#                  See "cmd /?" for information about characters needing to  #
:#                  be quoted.                                                #
:#                  I've added "@" that needs quoting if first char in cmd.   #
:#                  I've removed "!" as quoting does NOT prevent expansions.  #
:#                                                                            #
:#  History                                                                   #
:#   2010-12-19 JFL Created this routine                                      #
:#   2011-12-12 JFL Rewrote using findstr. (Executes much faster.)	      #
:#		    Added support for empty pathnames.                        #
:#   2016-11-09 JFL Fixed this routine, which was severely broken :-(	      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Quote file pathnames that require it. %1=Input variable. %2=Opt. output variable.
:condquote
%FUNCTION% enableextensions enabledelayedexpansion
set "RETVAR=%~2"
if not defined RETVAR set "RETVAR=%~1" &:# By default, change the input variable itself
%UPVAR% %RETVAR%
set "P=!%~1!"
:# Remove double quotes inside P. (Fails if P is empty, so skip this in this case)
if defined P set "P=!P:"=!"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Look for any special character that needs quoting
echo."!P!"|findstr /C:" " /C:"&" /C:"|" /C:"(" /C:")" /C:"@" /C:"," /C:";" /C:"[" /C:"]" /C:"{" /C:"}" /C:"=" /C:"'" /C:"+" /C:"`" /C:"~" >NUL
if not errorlevel 1 set P="!P!"
:condquote_ret
set "%RETVAR%=!P!"
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        list						      #
:#                                                                            #
:#  Description     List management routines                                  #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    Inspired from Tcl list management routines                #
:#                                                                            #
:#  History                                                                   #
:#   2016-11-09 JFL Added routine lsort.                                      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:lsort LIST_NAME [RETVAR]
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "RETVAR=%~2"
if not defined RETVAR set "RETVAR=%~1"
%UPVAR% %RETVAR%
set "SORTED_LIST="
%ECHOVARS.D% %~1
set "LIST=!%~1!"
:# No idea why %FOREACHLINE% appends a space to every line
if defined LIST %FOREACHLINE% %%l in ('^(for %%e in ^(!%~1!^) do @echo %%e^) ^| sort') do set "SORTED_LIST=!SORTED_LIST!%%l"
if defined SORTED_LIST set "SORTED_LIST=!SORTED_LIST:~0,-1!"
set "%RETVAR%=!SORTED_LIST!"
%RETURN%

:list2lines LIST_NAME
if defined %~1 for %%e in (!%~1!) do (echo.%%e)
exit /b

:#----------------------------------------------------------------------------#

:# Check that cmd extensions work
:check_exts
verify other 2>nul
setlocal enableextensions enabledelayedexpansion
if errorlevel 1 (
  >&2 echo Error: Unable to enable command extensions.
  endlocal & exit /b 1
)
set VAR=before
if "%VAR%" == "before" (
  set VAR=after
  if not "!VAR!" == "after" (
    >&2 echo Error: Delayed environment variable expansion must be enabled.
    >&2 echo Please restart your cmd.exe shell with the /V option,
    >&2 echo or set HKLM\Software\Microsoft\Command Processor\DelayedExpansion=1
    endlocal & exit /b 1
  )
)
endlocal & exit /b 0

:#----------------------------------------------------------------------------#

:# List subkeys. Args: [-c] [-f] KEY [RETVAR]
:keys
:dirs
%FUNCTION% enableextensions enabledelayedexpansion
set "PATTERN=*"
set "OPTS=/k"
set "KEY="
set "RETVAR="
:get_keys_args
if "%~1"=="" goto got_keys_args
if "%~1"=="-c" shift & set "OPTS=%OPTS% /c" & goto get_keys_args
if "%~1"=="-f" shift & set "PATTERN=%~1" & shift & goto get_keys_args
if not defined KEY set "KEY=%~1" & shift & goto get_keys_args
if not defined RETVAR set "RETVAR=%~1" & shift & goto get_keys_args
:got_keys_args
%ECHOVARS.D% KEY RETVAR
if defined RETVAR set "RETVAL="
:# First check if the key exists
%EXEC% reg query "%KEY%" ">"NUL
if errorlevel 1 %RETURN%
:# OK, so now list subkeys
set BEFORE=
if "%FULLPATH%"=="1" set "BEFORE=%KEY%\"
:# Use reg.exe to get the key information
set CMD=reg query "%KEY%" /f !PATTERN! !OPTS!
%>DEBUGOUT% %ECHO.XVD% %CMD%
:# For each line in CMD output...
%IF_EXEC% %FOREACHLINE% %%l in ('%CMD%') do (
  set "LINE=%%l"
  set "HEAD=!LINE:~0,2!"
  if "!HEAD!"=="HK" (
    set "NAME=%%~nxl"
    if "!NAME!"=="(Default)" set "NAME="
    set "NAME=!BEFORE!!NAME!"
    call :CondQuote NAME
    if defined RETVAR (
      set "RETVAL=!RETVAL! !NAME!"
    ) else (
      echo !NAME!
    )
  )
)
if defined RETVAR (
  %UPVAR% %RETVAR%
  if defined RETVAL set "RETVAL=!RETVAL:~1!"
  set "%RETVAR%=!RETVAL!"
)
%RETURN% 0

:#----------------------------------------------------------------------------#

:# List values. Args: [-/] [-c] [-f] KEY [RETVAR]
:values
:files
%FUNCTION% enableextensions enabledelayedexpansion
set "DETAILS=0"
set "PATTERN=*"
set "OPTS=/v"
set "KEY="
set "RETVAR="
:get_values_args
if "%~1"=="" goto got_values_args
if "%~1"=="-/" shift & set "DETAILS=1" & goto get_values_args
if "%~1"=="-c" shift & set "OPTS=%OPTS% /c" & goto get_values_args
if "%~1"=="-f" shift & set "PATTERN=%~1" & shift & goto get_values_args
if not defined KEY set "KEY=%~1" & shift & goto get_values_args
if not defined RETVAR set "RETVAR=%~1" & shift & goto get_values_args
:got_values_args
%ECHOVARS.D% KEY
if defined RETVAR set "RETVAL="
:# First check if the key exists
%EXEC% reg query "%KEY%" ">"NUL
if errorlevel 1 %RETURN%
:# OK, so now list values
set BEFORE=
if "%FULLPATH%"=="1" set "BEFORE=%KEY%\"
:# Use reg.exe to get the key information
set CMD=reg query "%KEY%" /f !PATTERN! !OPTS!
%>DEBUGOUT% %ECHO.XVD% %CMD%
:# For each line in CMD output...
%IF_EXEC% %FOREACHLINE% %%i in ('%CMD%') do (
  set "LINE=%%i"
  %ECHOVARS.D% LINE
  :# Values are indented by 4 spaces.
  set "HEAD=!LINE:~0,4!"
  set "LINE=!LINE:~4!"
  :# But extra lines of multi-lined values are indented by >20 spaces.
  set "HEAD2=!LINE:~0,4!"
  if "!HEAD!"=="    " if not "!HEAD2!"=="    " (
    :# Some versions of reg.exe use 4 spaces as field separator; others use a TAB.
    :# Change the 4-spaces around the REG_XX type word to a TAB.
    set "TOKENS=!LINE:    =	!"
    %ECHOVARS.D% TOKENS
    :# Extract the value name as the first item before the first TAB.
    :# Names can contain spaces, but assume they don't contain TABs.
    for /f "tokens=1,2* delims=	" %%j in ("!TOKENS!") do (
      set "NAME=%%j"
      if "!NAME!"=="(Default)" set "NAME="
      set "TYPE=%%k"
      set "VALUE=%%l"
      %ECHOVARS.D% NAME TYPE VALUE
      if %DETAILS%==0 (
	set "NAME=!BEFORE!!NAME!"
      ) else (
      	set "NAME=!NAME!/!TYPE!/!VALUE!"
      )
      if defined RETVAR (
	call :CondQuote NAME
	set "RETVAL=!RETVAL! !NAME!"
      ) else (
	echo !NAME!
      )
    )
  )
)
if defined RETVAR (
  %UPVAR% %RETVAR%
  if defined RETVAL set "RETVAL=!RETVAL:~1!"
  set "%RETVAR%=!RETVAL!"
)
%RETURN% 0

:#----------------------------------------------------------------------------#

:# List subkeys and values. %1=Parent key pathname.
:dir
:list
:ls
%FUNCTION%
set "KEY=%~1" & shift
%ECHOVARS.D% KEY
:# Then call subroutines to get subkeys, then values.
call :dirs !ARGS! SUBKEYS
if errorlevel 1 %RETURN%
for %%K in (!SUBKEYS!) do echo %%~K\
call :files !ARGS! VALUES
if errorlevel 1 %RETURN%
for %%V in (!VALUES!) do (
  if "%%~V"=="" ( :# The default value is returned as ""
    echo ""
  ) else (
    echo.%%~V
  )
)
%RETURN% 0

:#----------------------------------------------------------------------------#

:# Display a value. %1=Parent key pathname.
:type
:cat
:get
%FUNCTION% enableextensions enabledelayedexpansion
set "KEY=%~1"
:# Split the registry path and name. Uses a fake drive @ to prevent prepending the current disk drive path to the registry path.
for %%K in (@:"%KEY%") do (
  set "KEY=%%~dpK"
  set "NAME=%%~nxK"
)
:# Remove the head "@:\ and tail \ to the KEY, and tail " to the NAME
set "KEY=%KEY:~4,-1%"
set "NAME=%NAME:~0,-1%"
%ECHOVARS.D% KEY NAME
if "%NAME%"=="" (
  set CMD=reg query "%KEY%" /ve
) else (
  set CMD=reg query "%KEY%" /v "%NAME%"
)
%>DEBUGOUT% %ECHO.XVD% %CMD%
:# For each line in CMD output...
set "ERROR=1"
%FOREACHLINE% %%i in ('%CMD%') do (
  set "LINE=%%i"
  %ECHOVARS.D% LINE
  :# Values are indented by 4 spaces.
  set "HEAD=!LINE:~0,4!"
  set "LINE=!LINE:~4!"
  :# But extra lines of multi-lined values are indented by >20 spaces.
  set "HEAD2=!LINE:~0,4!"
  if "!HEAD!"=="    " if not "!HEAD2!"=="    " (
    :# Some versions of reg.exe use 4 spaces as field separator; others use a TAB. 
    :# Change the 4-spaces around the REG_XX type word to a TAB.
    set "TOKENS=!LINE:    =	!"
    %ECHOVARS.D% TOKENS
    :# Extract the value name as the first item before the first TAB.
    :# Names can contain spaces, but assume they don't contain TABs.
    for /f "tokens=1,2* delims=	" %%j in ("!TOKENS!") do (
      set "NAME=%%j"
      set "TYPE=%%k"
      set "VALUE=%%l"
      %ECHOVARS.D% NAME TYPE VALUE
    )
    set BEFORE=
    if %VERBOSE%==1 set "BEFORE=%KEY%\!NAME! = "
    echo.!BEFORE!!VALUE!
    set "ERROR=0"
  )
)
%RETURN% %ERROR%

:#----------------------------------------------------------------------------#

:# Non-recursive show. %1=Parent key pathname.
:show
:showdir
%FUNCTION% enableextensions enabledelayedexpansion
set "KEY=%~1" & shift
%ECHOVARS.D% KEY
:# Then call subroutines to get subkeys, then values.
call :dirs "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9 SUBKEYS
if errorlevel 1 %RETURN%
for %%K in (!SUBKEYS!) do (
  set "SUBKEY=%%~K\"
  call :CondQuote SUBKEY
  %ECHO% !SUBKEY! {}
)
set "ATTRS= "
call :files -/ "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9 VALUES
if errorlevel 1 %RETURN%
set NAMES=
set "VALUES=!VALUES:%%=%%debug.percnt%%!"	&:# Encode percent characters
set "VALUES=!VALUES:?=%%debug.quest%%!" 	&:# Encode question marks (else the for loop below ignores them)
for %%V in (!VALUES!) do (
  set "VALUE=%%~V"
  :# Gotcha: The name can be empty. So prefix the line with 1 extra character, then remove it from the name.
  for /f "tokens=1,2* delims=/" %%i in ("-!VALUE!") do (
    setlocal DisableDelayedExpansion
    endlocal & set "NAME=%%i" & set "TYPE=%%j" & set "VALUE=%%k"
    set "NAME=!NAME:~1!"
    :# %ECHOVARS.D% NAME TYPE VALUE
    set "NAMES=!NAMES! "!NAME!""
    set "TYPE[!NAME!]=!TYPE!"
    call set "VALUE[!NAME!]=!VALUE!"
  )
)
if defined NAMES set "NAMES=!NAMES:~1!"
call :lsort NAMES
for %%n in (!NAMES!) do (
  set "NAME=%%~n"
  set "TYPE=!TYPE[%%~n]!"
  set "VALUE=!VALUE[%%~n]!"
  :# %ECHOVARS.D% NAME TYPE VALUE
  call :CondQuote NAME
  call :CondQuote VALUE
  if %VERBOSE%==1 SET "ATTRS=%ATTRS%type=!TYPE! "
  %ECHO% !NAME!!ATTRS!!VALUE:%%=%%%%!
)
%RETURN% 0

:#----------------------------------------------------------------------------#

:# Recursive show. %1=Parent key pathname.
:tree
:showtree
%FUNCTION% enableextensions enabledelayedexpansion
set "KEY=%~1" & shift
%ECHOVARS.D% KEY
:# Then call subroutines to get subkeys, then values.
call :dirs "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9 SUBKEYS
if errorlevel 1 %RETURN%
for %%K in (!SUBKEYS!) do (
  set "SUBKEY=%%~K\"
  call :CondQuote SUBKEY
  %ECHO% !SUBKEY! {
    if "%DEBUG%"=="0" set "INDENT=%INDENT%  "
    call :tree "%KEY%\%%~K" %1 %2 %3 %4 %5 %6 %7 %8 %9
    if "%DEBUG%"=="0" set "INDENT=%INDENT%"
  %ECHO% }
)
set "ATTRS= "
call :files -/ "%KEY%" %1 %2 %3 %4 %5 %6 %7 %8 %9 VALUES
if errorlevel 1 %RETURN%
set NAMES=
set "VALUES=!VALUES:%%=%%debug.percnt%%!"	&:# Encode percent characters
set "VALUES=!VALUES:?=%%debug.quest%%!" 	&:# Encode question marks (else the for loop below ignores them)
for %%V in (!VALUES!) do (
  set "VALUE=%%~V"
  :# Gotcha: The name can be empty. So prefix the line with 1 extra character, then remove it from the name.
  for /f "tokens=1,2* delims=/" %%i in ("-!VALUE!") do (
    setlocal DisableDelayedExpansion
    endlocal & set "NAME=%%i" & set "TYPE=%%j" & set "VALUE=%%k"
    set "NAME=!NAME:~1!"
    :# %ECHOVARS.D% NAME TYPE VALUE
    set "NAMES=!NAMES! "!NAME!""
    set "TYPE[!NAME!]=!TYPE!"
    call set "VALUE[!NAME!]=!VALUE!"
  )
)
if defined NAMES set "NAMES=!NAMES:~1!"
call :lsort NAMES
for %%n in (!NAMES!) do (
  set "NAME=%%~n"
  set "TYPE=!TYPE[%%~n]!"
  set "VALUE=!VALUE[%%~n]!"
  %ECHOVARS.D% NAME TYPE VALUE
  call :CondQuote NAME
  call :CondQuote VALUE
  if %VERBOSE%==1 SET "ATTRS=%ATTRS%type=!TYPE! "
  %ECHO% !NAME!!ATTRS!!VALUE!
)
%RETURN% 0

:#----------------------------------------------------------------------------#

:# Delete a key value. %1=Parent key pathname.
:del
:rm
%FUNCTION% enableextensions enabledelayedexpansion
set "KEY=%~1"
:# Split the registry path and name. Uses a fake drive @ to prevent prepending the current disk drive path to the registry path.
for %%K in (@:"%KEY%") do (
  set "KEY=%%~dpK"
  set "NAME=%%~nxK"
)
:# Remove the head "@:\ and tail \ to the KEY, and tail " to the NAME
set "KEY=%KEY:~4,-1%"
set "NAME=%NAME:~0,-1%"
%ECHOVARS.D% KEY NAME
if "%NAME%"=="" (
  set CMD=reg delete "%KEY%" /ve
) else (
  set CMD=reg delete "%KEY%" /v "%NAME%"
)
if "%FORCE%"=="1" set "CMD=!CMD! /f"
%EXEC% %CMD%
%RETURN%

:#----------------------------------------------------------------------------#

:# Delete a key and all its contents. %1=Parent key pathname.
:rd
:rmdir
%FUNCTION%
set "KEY=%~1"
%ECHOVARS.D% KEY
set CMD=reg delete "%KEY%"
if "%FORCE%"=="1" set "CMD=!CMD! /f"
%EXEC% %CMD%
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        main                                                      #
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

:help
echo %SCRIPT% version %VERSION%
echo.
echo Browse the registry "directories" and "files" using Windows' reg.exe
echo.
echo Usage: %ARG0% [GLOBAL_OPTIONS] COMMAND [COMMAND_OPTIONS]
echo.
echo Global options:
echo   -?^|-h            Display this help screen and exit
echo   -f               List the full pathname of keys and values
echo   -v               Verbose output
echo   -V               Display this script version and exit
echo.
echo Commands:
echo   del {path\name}  Delete a key value
echo   dir {path}       List subkeys/ and values names (1)
echo   keys {path}      List subkeys names
echo   rd {path}        Delete a key, and all sub-keys and value
echo   show {path}      Display all values contents (1)
echo   tree {path}      Display the whole tree key values contents (1)
echo   type {path\name} Display the key value content 
echo   values {path}    List values names (1)
echo.
echo dir, keys, values options:
echo   -c               Case-sensitive search
echo   -p PATTERN       Search for a specific pattern. Default: *
echo.
echo del, rd options:
echo   -f               Force deletion without confirmation
echo.
echo (1): The unnamed "(Default)" value is displayed as name "".
echo.
goto :eof

:main
set "ACTION="
set "KEY="
set "OPTS="
set ">DEBUGOUT=>&2"	&:# Send debug output to stderr, so that it does not interfere with subroutines output capture
set "FORCE=0"

:next_arg
if not defined ARGS set "ARG=" & goto :go
%POPARG%
if "!ARG!"=="-?" goto :Help
if "!ARG!"=="/?" goto :Help
if "!ARG!"=="-c" set "OPTS=%OPTS% -c" & goto :next_arg
if "!ARG!"=="-d" call :Debug.on & goto :next_arg
if "!ARG!"=="-f" set "FULLPATH=1" & set "FORCE=1" & goto :next_arg
if "!ARG!"=="-p" %POPARG% & set "OPTS=%OPTS% -f %"ARG"%" & goto :next_arg
if "!ARG!"=="-t" goto test
if "!ARG!"=="-v" call :Verbose.on & goto :next_arg
if "!ARG!"=="-V" (echo.%VERSION%) & goto :eof
if "!ARG!"=="-X" call :Exec.off & goto :next_arg
if "%ACTION%"=="" set "ACTION=!ARG!" & goto next_arg
if "%KEY%"=="" set "KEY=!ARG!" & goto next_arg
2>&1 echo Unexpected argument, ignored: %"ARG"%
goto next_arg

:test
goto :eof

:# Execute the requested command
:go
call :check_exts
if errorlevel 1 exit /b

call :%ACTION% "%KEY%"!OPTS!
