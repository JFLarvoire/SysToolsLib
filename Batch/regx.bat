@echo off
:##############################################################################
:#                                                                            #
:#  Filename:	    regx.bat						      #
:#                                                                            #
:#  Description:    Manage the registry "directories"=keys and "files"=values #
:#                                                                            #
:#  Notes:	    Front end to Windows' reg.exe.			      #
:#		    Uses the familiar dir, type, tree, set, del, rd commands  #
:#		    for managing registry keys and values as if they were     #
:#		    directories and files.				      #
:#		    The show command output is in my SML structured format.   #
:#		    							      #
:#                  Registry pathnames and values may contain tricky          #
:#                  characters like '%' '!' '^', which break the set, call,   #
:#		    echo, etc., commands if not quoted correctly. This script #
:#		    demonstates techniques for doing this consistently.	      #
:#		    							      #
:#		    Uses external calls to other instances of this script.    #
:#		    This proved necessary for several reasons:		      #
:#		    - Work around the impossibility of passing arguments      #
:#		      containing % characters to for /f ('commands').	      #
:#		    - Work around the impossibility of calling subroutines    #
:#		      in for /f ('commands').				      #
:#		    - Avoid issues when piping the output of some subroutines #
:#		      into the sort program. (cmd.exe even crashes sometimes!)#
:#		    							      #
:#		    Tricky keys that influenced the design:		      #
:#		    A key with subkeys containing the % character:	      #
:#		        HKCU\Console					      #
:#		    A key with data containing the % character:		      #
:#		        HKCU\Environment				      #
:#		    A key with subkeys containing the ^ character:	      #
:#		        HKCU\Control Panel\Desktop\PerMonitorSettings	      #
:#		    A key with data containing the ^ character:		      #
:#		    HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Extensions #
:#		    A key with subkeys containing the ! character:	      #
:#		        HKLM\SOFTWARE\Microsoft\DirectDraw\Compatibility      #
:#		    A key with subkeys containing the ! character:	      #
:#			HKCR\Acrobat.aaui\shell\Open\command		      #
:#		    							      #
:#		    To do: Manage complex multi-line values.		      #
:#		           Ex: HKLM\Cluster\Exchange\DagNetwork		      #
:#		    							      #
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
:#                  Fixed the -X option, which was not respected by many cmds.#
:#   2016-11-15 JFL Added functions :set and :open.                           #
:#   2016-11-17 JFL Don't display the command for show & tree in verbose mode.#
:#                  Fixed several tracing bugs. (The code itself worked fine.)#
:#   2016-11-18 JFL Major redesign to allow passing arguments containing %    #
:#                  characters to for /f ('commands'). Necessary because some #
:#                  keys do have a % in their name.                           #
:#                  Walk down the registry tree using callbacks.              #
:#   2016-11-22 JFL Fixed at last the errorlevel for good keys with nothing   #
:#                  inside.                                                   #
:#   2016-11-23 JFL Factored out routine :Prep2ExpandVars, and use it before  #
:#                  displaying any name or value. Fixes issues with ! and ^.  #
:#   2016-12-16 JFL Updated the library framework. Features unchanged.        #
:#   2017-08-29 JFL Fixed the type command output.			      #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2017-08-29"
set "SCRIPT=%~nx0"				&:# Script name
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"	&:# Script path, without the trailing \
set  "ARG0=%~f0"				&:# Script full pathname
set ^"ARGS=%*^"					&:# Argument line

:# Mechanism for calling subroutines in a second instance of a script, from its main instance.
:# Done by (%XCALL% :label [arguments]), with XCALL defined in the Call module below.
if '%1'=='-call' !ARGS:~1!& exit /b

:# Initialize the most commonly used library components.
call :Library.Init

:# Go process command-line arguments
goto main

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function	    Library.Init					      #
:#                                                                            #
:#  Description     Initialize the most commonly used library components      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Library.Init
:# Initialize this library modules definitions.
:# Each one depends on the preceding ones, so if you need one, you need all the preceding ones as well.
call :Call.Init			&:# Function calls and argument extraction
call :Macro.Init		&:# Inline macros generation
call :Debug.Init		&:# Debug routines
call :Exec.Init			&:# Conditional execution routines

:# FOREACHLINE macro. (Changes the delimiter to none to catch the whole lines.)
set FOREACHLINE=for /f "delims="

:# HOME variable. For analogy with Unix systems.
if not defined HOME set "HOME=%HOMEDRIVE%%HOMEPATH%"

goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Module	    Call						      #
:#                                                                            #
:#  Description     Manage function calls and argument extraction             #
:#                                                                            #
:#  Functions	    PopArg          Pop the first argument from %ARGS% into   #
:#				     %ARG% and %"ARG"%			      #
:#		    PopSimpleArg    Simpler and faster version, incompatible  #
:#                                   with ! or ^ characters in ARG values.    #
:#		    Prep2ExpandVars Prepare variables to return from the      #
:#		 		    local scope (with expansion on or off)    #
:#				    to a parent scope with expansion on.      #
:#		    PrepArgVars     Prepare variables containing pathnames    #
:#				    that will be passed as arguments.	      #
:#                                                                            #
:#  Macros	    %POPARG%        Pop one argument using :PopArg            #
:#                  %POPSARG%       Pop one argument using :PopSimpleArg      #
:#                  %LCALL%         Call a routine in this library, either    #
:#                                   locally, or from an outside script.      #
:#                  %XCALL%         Call an outside script routine, from      #
:#                                   another instance of that outside script. #
:#                  %XCALL@%        Idem, but with all args stored in one var.#
:#                                                                            #
:#  Variables	    %ARG%           The unquoted argument                     #
:#                  %"ARG"%         The actual argument, possibly quoted      #
:#                  %ARGS%	    Remaining command line arguments          #
:#                                                                            #
:#                  %CR%            An ASCII Carrier Return character '\x0D'  #
:#                  %LF%            An ASCII Line Feed character '\x0A'       #
:#                  %BS%            An ASCII Back Space character '\x08'      #
:#                  %FF%            An ASCII Form Feed character '\x0C'       #
:#                                                                            #
:#  Notes 	    PopArg works around the defect of the shift command,      #
:#                  which pops the first argument from the %* list, but does  #
:#                  not remove it from %*.                                    #
:#                  Also works around another defect with tricky characters   #
:#                  like ! or ^ being lost when variable expansion is on.     #
:#                                                                            #
:#                  Important: The performance of this routine is much better #
:#                  when invoked with variable expansion disabled. This is    #
:#                  due to the complex processing done to avoid issues with   #
:#                  tricky characters like ! or ^ when expansion is enabled.  #
:#                  If you're sure that NONE of the arguments contain such    #
:#                  tricky characters, then call :PopSimpleArg.               #
:#                                                                            #
:#                  Uses an inner call to make sure the argument parsing is   #
:#                  done by the actual cmd.exe parser. This guaranties that   #
:#                  arguments are split exactly as shift would have done.     #
:#                                                                            #
:#                  But call itself has a quirk, which requires a convoluted  #
:#                  workaround to process the /? argument.                    #
:#                                                                            #
:#                  Known limitation: After using :PopArg, all consecutive    #
:#                  argument separators in %ARGS% are replaced by one space.  #
:#                  For example: "A==B" becomes "A B"                         #
:#                  This does not change the result of subsequent calls to    #
:#                  :PopArg, but this prevents from using the tail itself as  #
:#                  an argument. => Do not use :PopArg to get :Exec args!     #
:#                                                                            #
:#                  To do: Detect if the last arg has mismatched quotes, and  #
:#                  if it does, append one.                                   #
:#                  Right now such mismatched quotes will cause an error here.#
:#                  Do not work around this error to only pass back the bad   #
:#                  argument, as this will only cause more errors further down#
:#                                                                            #
:#  History                                                                   #
:#   2015-04-03 JFL Bug fix: Quoted args with an & inside failed to be poped. #
:#   2015-07-06 JFL Bug fix: Call quirk prevented inner call from popping /?. #
:#   2016-11-18 JFL Fixed popping arguments containing % characters.          #
:#   2016-11-21 JFL Fixed popping quoted arguments containing &|<> characters.#
:#   2016-11-22 JFL Fixed popping arguments containing ^ characters.          #
:#   2016-11-24 JFL Updated %POPARG% to work with trick characters ! and ^ in #
:#                  delayed expansion mode. The old and faster version is now #
:#		    called %POPSARG%.                                         #
:#		    Added routine :Prep2ExpandVars allowing to pass any       #
:#		    tricky string across call or endlocal barriers.           #
:#   2016-12-01 JFL Added a %FF% Form Feed character variable.                #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Call.Init
goto Call.end

:Call.Init
if not defined LCALL set "LCALL=call"	&:# Macro to call functions in this library
set "POPARG=%LCALL% :PopArg"
set "POPSARG=%LCALL% :PopSimpleArg"

:# Mechanism for calling subroutines in a second external instance of the top script.
set ^"XCALL=call "!ARG0!" -call^"	&:# This is the top script's (or this lib's if called directly) ARG0
set ^"XCALL@=!XCALL! :CallVar^"		&:# Indirect call, with the label and arguments in a variable

:# Define a LF variable containing a Line Feed ('\x0A')
set LF=^
%# The two blank lines here are necessary. #%
%# The two blank lines here are necessary. #%

:# Define a CR variable containing a Carriage Return ('\x0D')
for /f %%a in ('copy /Z %COMSPEC% nul') do set "CR=%%a"

:# Define a BS variable containing a BackSpace ('\x08')
:# Use prompt to store a  backspace+space+backspace into a DEL variable.
for /F "tokens=1 delims=#" %%a in ('"prompt #$H# & echo on & for %%b in (1) do rem"') do set "DEL=%%a"
:# Then extract the first backspace
set "BS=%DEL:~0,1%"

:# Define a FF variable containing a Form Feed ('\x0C')
for /f %%A in ('cls') do set "FF=%%A"

:# Define variables for problematic characters, that cause parsing issues.
:# Use the ASCII control character name, or the html entity name.
:# Warning: The excl and hat characters need different quoting depending on context.
set  "DEBUG.percnt=%%"	&:# One percent sign
set  "DEBUG.excl=^!"	&:# One exclamation mark
set  "DEBUG.hat=^"	&:# One caret, aka. circumflex accent, or hat sign
set ^"DEBUG.quot=""	&:# One double quote
set  "DEBUG.apos='"	&:# One apostrophe
set  "DEBUG.amp=&"	&:# One ampersand
set  "DEBUG.vert=|"	&:# One vertical bar
set  "DEBUG.gt=>"	&:# One greater than sign
set  "DEBUG.lt=<"	&:# One less than sign
set  "DEBUG.lpar=("	&:# One left parenthesis
set  "DEBUG.rpar=)"	&:# One right parenthesis
set  "DEBUG.lbrack=["	&:# One left bracket
set  "DEBUG.rbrack=]"	&:# One right bracket
set  "DEBUG.sp= "	&:# One space
set  "DEBUG.tab=	"	&:# One tabulation
set  "DEBUG.quest=?"	&:# One question mark
set  "DEBUG.ast=*"	&:# One asterisk
set  "DEBUG.cr=!CR!"	&:# One carrier return
set  "DEBUG.lf=!LF!"	&:# One line feed
set  "DEBUG.bs=!BS!"	&:# One backspace
set  "DEBUG.ff=!FF!"	&:# One form feed
goto :eof

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

:PopArg.Eon
setlocal DisableDelayedExpansion
call :PopArg
call :Prep2ExpandVars ARG ^""ARG"^" ARGS
setlocal EnableDelayedExpansion
for /f %%a in ("-!ARG!") do for /f %%b in ("-!"ARG"!") do for /f %%c in ("-!ARGS!") do (
  endlocal
  endlocal
  set "ARG=%%a"
  set "ARG=!ARG:~1!"
  set ^""ARG"=%%b^"
  set ^""ARG"=!"ARG":~1!^"
  set ^"ARGS=%%c^"
  set "ARGS=!ARGS:~1!"
)
goto :eof

:# Prepare variables to return from the local scope (with expansion on or off) to a parent scope with expansion on
:Prep2ExpandVars VAR [VAR ...]
if "!!"=="" goto :Prep2ExpandVars.Eon
:Prep2ExpandVars.Eoff	:# The local scope has expansion off
setlocal EnableDelayedExpansion
set "VALUE=!%~1!"
call :Prep2ExpandVars.Eon VALUE
endlocal & set "%~1=%VALUE%"
if not [%2]==[] shift & goto :Prep2ExpandVars.Eoff
goto :eof

:# Prepare variables, assuming the local scope itself has expansion on
:Prep2ExpandVars.Eon VAR [VAR ...]
if defined %1 (
  for %%e in (sp tab cr lf quot amp vert lt gt hat percnt) do ( :# Encode named character entities
    for %%c in ("!DEBUG.%%e!") do (
      set "%~1=!%~1:%%~c= DEBUG.%%e !"
    )
  )
  call set "%~1=%%%~1:^!= DEBUG.excl %%" 	& rem :# Encode exclamation points
  call set "%~1=%%%~1: =^!%%"			& rem :# Encode final expandable entities
)
if not [%2]==[] shift & goto :Prep2ExpandVars.Eon
goto :eof

:# Prepare variables containing pathnames that will be passed as "arguments"
:PrepArgVars
set "%~1=!%~1:%%=%%%%!"				&:# Escape percent signs
if not [%2]==[] shift & goto :PrepArgVars
goto :eof

:# Indirect call, with the label and arguments in a variable
:CallVar CMDVAR
call !%1:%%=%%%%!
exit /b

:Call.end

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Module          Macro						      #
:#                                                                            #
:#  Description     Tools for defining inline functions,                      #
:#                  also known as macros by analogy with Unix shells macros   #
:#                                                                            #
:#  Macros          %MACRO%         Define the prolog code of a macro         #
:#                  %/MACRO%        Define the epilog code of a macro         #
:#                                                                            #
:#  Variables       %LF1%           A Line Feed ASCII character '\x0A'        #
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
set "<=^^^<"		&:# Insert a redirection character
set "&=^^^&"		&:# Insert a command separator in a macro
:# Idem, to be expanded twice, for use in macros within macros
set "^!2=^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^!"
set "'^!2=^^^^^^^!"
set "&2=^^^^^^^^^^^^^^^&"

set "MACRO=for %%$ in (1 2) do if %%$==2"				&:# Prolog code of a macro
set "/MACRO=else setlocal enableDelayedExpansion %&% set MACRO.ARGS="	&:# Epilog code of a macro
set "ENDMACRO=endlocal"	&:# Ends the macro local scope started in /MACRO. Necessary before macro exit.

set "ON_MACRO_EXIT=for /f "delims=" %%r in ('echo"	&:# Begin the return variables definitions 
set "/ON_MACRO_EXIT=') do %ENDMACRO% %&% %%r"		&:# End the return variables definitions

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

goto :eof
:Macro.end

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Module	    Debug						      #
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
:#   2016-11-13 JFL Bug fix: Correctly return special characters & | < > ? *  #
:#   2016-11-24 JFL Fixed tracing %FUNCTION% arguments with ^ and % chars.    #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Debug.Init
goto :Debug.End

:Debug.Init
:# Preliminary checks to catch common problems
if exist echo >&2 echo WARNING: The file "echo" in the current directory will cause problems. Please delete it and retry.
:# Inherited variables from the caller: DEBUG, VERBOSE, INDENT, >DEBUGOUT
:# Initialize other debug variables
set "ECHO=%LCALL% :Echo"
set "ECHOVARS=%LCALL% :EchoVars"
:# The FUNCTION, UPVAR, and RETURN macros should work with delayed expansion on or off
set MACRO.GETEXP=(if "%'!2%%'!2%"=="" (set MACRO.EXP=EnableDelayedExpansion) else set MACRO.EXP=DisableDelayedExpansion)
set UPVAR=call set DEBUG.RETVARS=%%DEBUG.RETVARS%%
set RETURN=call set "DEBUG.ERRORLEVEL=%%ERRORLEVEL%%" %&% %MACRO% ( %\n%
  set DEBUG.EXITCODE=%!%MACRO.ARGS%!%%\n%
  if defined DEBUG.EXITCODE set DEBUG.EXITCODE=%!%DEBUG.EXITCODE: =%!%%\n%
  if not defined DEBUG.EXITCODE set DEBUG.EXITCODE=%!%DEBUG.ERRORLEVEL%!%%\n%
  for %%l in ("%'!%LF%'!%") do ( %# Make it easy to insert line-feeds in any mode #% %\n%
    set "DEBUG.SETARGS=""" %# The initial "" makes sure that for loops below never get an empty arg list #% %\n%
    for %%v in (%!%DEBUG.RETVARS%!%) do ( %\n%
      set "DEBUG.VALUE=%'!%%%v%'!%" %# We must remove problematic characters in that value #% %\n%
      if defined DEBUG.VALUE ( %# Else the following lines will generate phantom characters #% %\n%
	set "DEBUG.VALUE=%'!%DEBUG.VALUE:%%=%%DEBUG.percnt%%%'!%"	%# Encode percent #% %\n%
	for %%e in (sp tab cr lf quot amp vert lt gt) do for %%c in ("%'!%DEBUG.%%e%'!%") do ( %# Encode named character entities #% %\n%
	  set "DEBUG.VALUE=%'!%DEBUG.VALUE:%%~c=%%DEBUG.%%e%%%'!%" %\n%
	) %\n%
	set "DEBUG.VALUE=%'!%DEBUG.VALUE:^^=%%DEBUG.hat%%%'!%"	%# Encode carets #% %\n%
	call set "DEBUG.VALUE=%%DEBUG.VALUE:%!%=^^^^%%" 		%# Encode exclamation points #% %\n%
	set "DEBUG.VALUE=%'!%DEBUG.VALUE:^^^^=%%DEBUG.excl%%%'!%"	%# Encode exclamation points #% %\n%
      ) %\n%
      set DEBUG.SETARGS=%!%DEBUG.SETARGS%!% "%%v=%'!%DEBUG.VALUE%'!%"%\n%
    ) %\n%
    if %!%DEBUG%!%==1 ( %# Build the debug message and display it #% %\n%
      set "DEBUG.MSG=return %'!%DEBUG.EXITCODE%'!%" %\n%
      for /f "delims=" %%v in ("%'!%DEBUG.SETARGS: =%%~l%'!%") do if not %%v=="" ( %# for /f avoids issues with ? and * #% %\n%
	set "DEBUG.MSG=%'!%DEBUG.MSG%'!% %%DEBUG.amp%% set %%v" %!% %\n%
      ) %\n%
      call set "DEBUG.MSG=%'!%DEBUG.MSG:%%=%%DEBUG.excl%%%'!%" %# Change all percent to ! #%  %\n%
      if defined ^^%>%DEBUGOUT ( %# If we use a debugging stream distinct from stdout #% %\n%
	%!%LCALL%!% :Echo.Eval2DebugOut DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
      ) else ( %# Output directly here, which is faster #% %\n%
	for /f "delims=" %%c in ("%'!%INDENT%'!%%'!%DEBUG.MSG%'!%") do echo %%c%# Use a for loop to do a double !variable! expansion #%%\n%
      ) %\n%
      if defined LOGFILE ( %# If we have to send a copy to a log file #% %\n%
	%!%LCALL%!% :Echo.Eval2LogFile DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
      ) %\n%
    ) %\n%
    for %%r in (%!%DEBUG.EXITCODE%!%) do ( %# Carry the return values through the endlocal barriers #% %\n%
      for /f "delims=" %%a in ("%'!%DEBUG.SETARGS%'!%") do ( %\n%
	endlocal %&% endlocal %&% endlocal %# Exit the RETURN and FUNCTION local scopes #% %\n%
	set "DEBUG.SETARGS=%%a" %\n%
	if "%'!%%'!%"=="" ( %# Delayed expansion is ON #% %\n%
	  call set "DEBUG.SETARGS=%'!%DEBUG.SETARGS:%%=%%DEBUG.excl%%%'!%" %# Change all percent to ! #%  %\n%
	  for /f "delims=" %%v in ("%'!%DEBUG.SETARGS: =%%~l%'!%") do if not %%v=="" ( %# for /f avoids issues with ? and * #% %\n%
	    set %%v %# Set each upvar variable in the caller's scope #% %\n%
	  ) %\n%
	) else ( %# Delayed expansion is OFF #% %\n%
	  setlocal EnableDelayedExpansion %\n%
	  for /f "delims=" %%v in ("%'!%DEBUG.SETARGS: =%%~l%'!%") do if %%v=="" ( %# for /f avoids issues with ? and * #% %\n%
	    endlocal %\n%
	  ) else ( %\n%
	    call set %%v %# Set each upvar variable in the caller's scope #% %\n%
	  ) %\n%
	) %\n%
	set "DEBUG.SETARGS=" %\n%
	exit /b %%r %# Return to the caller #% %\n%
      ) %\n%
    ) %\n%
  ) %\n%
) %/MACRO%
:Debug.Init.2
set "LOG=%LCALL% :Echo.Log"
set ">>LOGFILE=>>%LOGFILE%"
if not defined LOGFILE set "LOG=rem" & set ">>LOGFILE=rem"
if .%LOGFILE%.==.NUL. set "LOG=rem" & set ">>LOGFILE=rem"
if .%NOREDIR%.==.1. set "LOG=rem" & set ">>LOGFILE=rem" &:# A parent script is already redirecting output. Trying to do it again here would fail. 
set "ECHO.V=%LCALL% :Echo.Verbose"
set "ECHO.D=%LCALL% :Echo.Debug"
set "ECHOVARS.V=%LCALL% :EchoVars.Verbose"
set "ECHOVARS.D=%LCALL% :EchoVars.Debug"
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
  call set ARGS=%%*%# Do not quote this, to keep string/non string aternance #%%\n%
  if defined ARGS set ARGS=%!%ARGS:^^^^^^^^^^^^^^^^=^^^^^^^^%!%%# ^carets are doubled in quoted strings, halved outside. => Quadruple them if using unquoted ones #%%\n%
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
set "FUNCTION0=call %LCALL% :Debug.Entry0 %%0 %%*"
set FUNCTION=%MACRO.GETEXP% %&% %MACRO% ( %\n%
  call set "FUNCTION.NAME=%%0" %\n%
  call set ARGS=%%*%# Do not quote this, to keep string/non string aternance #%%\n%
  if defined ARGS set ARGS=%!%ARGS:^^^^^^^^^^^^^^^^=^^^^^^^^%!%%# ^carets are doubled in quoted strings, halved outside. => Quadruple them if using unquoted ones #%%\n%
  if %!%DEBUG%!%==1 ( %# Build the debug message and display it #% %\n%
    set DEBUG.MSG=call %!%FUNCTION.NAME%!% %!%ARGS%!%%\n%
    if defined ^^%>%DEBUGOUT ( %# If we use a debugging stream distinct from stdout #% %\n%
      %!%LCALL%!% :Echo.2DebugOut DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
    ) else ( %# Output directly here, which is faster #% %\n%
      echo%!%INDENT%!% %!%DEBUG.MSG%!%%\n%
    ) %\n%
    if defined LOGFILE ( %# If we have to send a copy to a log file #% %\n%
      %!%LCALL%!% :Echo.2LogFile DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
    ) %\n%
    call set "INDENT=%'!%INDENT%'!%  " %\n%
  ) %\n%
  set "DEBUG.RETVARS=" %\n%
  if not defined MACRO.ARGS set "MACRO.ARGS=%'!%MACRO.EXP%'!%" %\n%
  setlocal %!%MACRO.ARGS%!% %\n%
) %/MACRO%
set "RETURN0=call %LCALL% :Debug.Return0 %%ERRORLEVEL%% & exit /b"
:# Macro for displaying comments on the return log line
set RETURN#=call set "RETURN.ERR=%%ERRORLEVEL%%" %&% %MACRO% ( %\n%
  %LCALL% :Debug.Return# %# Redirections can't work in macro. Do it in a function. #% %\n%
  for %%r in (%!%RETURN.ERR%!%) do %ENDMACRO% %&% set "RETURN.ERR=" %&% call set "INDENT=%%INDENT:~2%%" %&% exit /b %%r %\n%
) %/MACRO%
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -d=% -d"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Reverse the above optimization
set "ECHO.D=%LCALL% :Echo.Debug"
set "ECHOVARS.D=%LCALL% :EchoVars.Debug"
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

:Debug.Return0 %1=Exit code
%>DEBUGOUT% echo %INDENT%return %1
if defined LOGFILE %>>LOGFILE% echo %INDENT%return %1
set "INDENT=%INDENT:~0,-2%"
exit /b %1

:Debug.Return# :# %RETURN.ERR% %MACRO.ARGS%
setlocal DisableDelayedExpansion
%>DEBUGOUT% echo %INDENT%return %RETURN.ERR% ^&:#%MACRO.ARGS%
if defined LOGFILE %>>LOGFILE% echo %INDENT%return %RETURN.ERR% ^&:#%MACRO.ARGS%
endlocal
goto :eof &:# %RETURN.ERR% will be processed in the %DEBUG#% macro.

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
set "ECHO.V=%LCALL% :Echo.Verbose"
set "ECHOVARS.V=%LCALL% :EchoVars.Verbose"
goto :eof

:# Echo and log a string, indented at the same level as the debug output.
:Echo
echo.%INDENT%%*
:Echo.Log
if defined LOGFILE %>>LOGFILE% echo.%INDENT%%*
goto :eof

:Echo.Verbose
:Echo.V
%IF_VERBOSE% goto :Echo
goto :Echo.Log

:Echo.Debug
:Echo.D
%IF_DEBUG% %>DEBUGOUT% echo.%INDENT%%*
goto :Echo.Log

:Echo.Eval2DebugOut %1=Name of string, with !variables! that need to be evaluated first
setlocal EnableDelayedExpansion &:# Make sure that !variables! get expanded
set "STRING=!%1!" &:# !variables! not yet expanded; They will be on next line
%>DEBUGOUT% echo.%INDENT%%STRING%
goto :eof

:Echo.2DebugOut	%1=Name of string to output to the DEBUGOUT stream
setlocal EnableDelayedExpansion &:# Make sure that !variables! get expanded
%>DEBUGOUT% echo.%INDENT%!%1!
goto :eof

:Echo.Eval2LogFile %1=Name of string, with variables that need to be evaluated first
setlocal EnableDelayedExpansion &:# Make sure that !variables! get expanded
set "STRING=!%1!" &:# !variables! not yet expanded; They will be on next line
%>>LOGFILE% echo.%INDENT%%STRING%
goto :eof

:Echo.2LogFile %1=Name of string to output to the LOGFILE
setlocal EnableDelayedExpansion &:# Make sure that !variables! get expanded
%>>LOGFILE% echo.%INDENT%!%1!
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
:#  Module	    Exec                                                      #
:#                                                                            #
:#  Description     Run a command, logging its output to the log file.        #
:#                                                                            #
:#                  In VERBOSE mode, display the command line first.          #
:#                  In DEBUG mode, display the command line and the exit code.#
:#                  In NOEXEC mode, display the command line, but don't run it.
:#                                                                            #
:#  Functions       Exec.Init	Initialize Exec routines. Call once at 1st    #
:#                  Exec.Off	Disable execution of commands		      #
:#                  Exec.On	Enable execution of commands		      #
:#                  Do          Always execute a command, logging its output  #
:#                  Exec	Conditionally execute a command, logging it.  #
:#                  Exec.SetErrorLevel	Change the current ERRORLEVEL	      #
:#                                                                            #
:#  Exec Arguments  -l          Log the output to the log file.               #
:#                  -L          Do not send the output to the log file. (Dflt)#
:#                  -t          Tee all output to the log file if there's a   #
:#                              usable tee.exe.                               #
:#                              Known limitation: The exit code is always 0.  #
:#                  -e          Always echo the command.		      #
:#                  -v          Trace the command in verbose mode. (Default)  #
:#                  -V          Do not trace the command in verbose mode.     #
:#                  %*          The command and its arguments                 #
:#                              Quote redirection operators. Ex:              #
:#                              %EXEC% find /I "error" "<"logfile.txt ">"NUL  #
:#                              Note: Quote redirections, NOT file numbers.   #
:#                              Ex: 2">&"1 will work; "2>&1" will NOT work.   #
:#                                                                            #
:#  Macros          %DO%        Always execute a command, logging its output  #
:#                  %EXEC%      Conditionally execute a command, logging it.  #
:#                  %ECHO.X%    Echo a string indented in -X mode, and log it.#
:#                  %ECHO.XD%   Idem in -X or -D modes.                       #
:#                  %ECHO.XVD%  Idem in -X or -V or -D modes.                 #
:#                              Useful to display commands in cases where     #
:#                              %EXEC% can't be used, like in for ('cmd') ... #
:#                  %IF_EXEC%   Execute a command if _not_ in NOEXEC mode     #
:#                  %IF_NOEXEC% Execute a command in NOEXEC mode only         #
:#                  %_DO%       Echo and run a command. No opts. No logging.  #
:#                  %_DO.D%     Idem, echoing it in debug mode only.          #
:#                  %_DO.XVD%   Idem, echoing it in -X or -V or -D modes only.#
:#                  %XEXEC%     Call :Exec from an external scriptlet, such   #
:#                               one in a (for /f in ('commands')) block.     #
:#                  %XEXEC@%    Idem, but with all args stored in one var.    #
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
:#  Notes           %EXEC% can't be used from inside ('command') blocks.      #
:#                  This is because these blocks are executed separately in   #
:#                  a child shell. Use %XEXEC% or %XEXEC@% instead.	      #
:#		    These macros rely on the %XCALL% mechanism for calling    #
:#		    subroutines in a second instance of a script. They depend #
:#		    on the following line being present after the ARGS	      #
:#		    variable definition at the top of your script:	      #
:#		    if '%1'=='-call' !ARGS:~1!& exit /b			      #
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
:#   2016-11-17 JFL Fixed tracing the exit code when caller has exp. disabled.#
:#		    Added option -V to disable tracing exec in verbose mode.  #
:#		    Added macro %ECHO.XD%.                                    #
:#		    Faster and more exact method for separating the %EXEC%    #
:#		    optional arguments from the command line to run. (The old #
:#		    method lost non-white batch argument separators = , ; in  #
:#		    some cases.)                                              #
:#   2016-11-24 JFL Fixed executing commands containing a ^ character.        #
:#		    Added routine :_Do.                                       #
:#   2016-12-13 JFL Rewrote _DO as a pure macro.                              #
:#   2016-12-15 JFL Changed the default to NOT redirecting the output to log. #
:#		                                                              #
:#----------------------------------------------------------------------------#

call :Exec.Init
goto :Exec.End

:# Global variables initialization, to be called first in the main routine
:Exec.Init
set "DO=%LCALL% :Do"
set "EXEC=%LCALL% :Exec"
set "ECHO.X=%LCALL% :Echo.X"
set "ECHO.XD=%LCALL% :Echo.XD"
set "ECHO.XVD=%LCALL% :Echo.XVD"
if not .%NOEXEC%.==.1. set "NOEXEC=0"
:# Quick and simple DO macros, supporting a single command, no redirections, no tricky chars!
set _DO=%MACRO%     ( %LCALL% :Echo     %!%MACRO.ARGS%!% %&% %ON_MACRO_EXIT%%!%MACRO.ARGS%!%%/ON_MACRO_EXIT% ) %/MACRO%
set _DO.D=%MACRO%   ( %LCALL% :Echo.D   %!%MACRO.ARGS%!% %&% %ON_MACRO_EXIT%%!%MACRO.ARGS%!%%/ON_MACRO_EXIT% ) %/MACRO%
set _DO.XD=%MACRO%  ( %LCALL% :Echo.XD  %!%MACRO.ARGS%!% %&% %ON_MACRO_EXIT%%!%MACRO.ARGS%!%%/ON_MACRO_EXIT% ) %/MACRO%
set _DO.XVD=%MACRO% ( %LCALL% :Echo.XVD %!%MACRO.ARGS%!% %&% %ON_MACRO_EXIT%%!%MACRO.ARGS%!%%/ON_MACRO_EXIT% ) %/MACRO%
:# Execute commands from another instance of the main script
set "XEXEC=%XCALL% :Exec"
set "XEXEC@=%XCALL% :Exec.ExecVar"
:# Check if there's a tee.exe program available
:# set "Exec.HaveTee=0"
:# tee.exe --help >NUL 2>NUL
:# if not errorlevel 1 set "Exec.HaveTee=1"
for %%t in (tee.exe) do set "Exec.tee=%%~$PATH:t"
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

:Echo.XVD
%IF_VERBOSE% goto :Echo
:Echo.XD
%IF_DEBUG% goto :Echo
:Echo.X
%IF_NOEXEC% goto :Echo
goto :Echo.Log

:Exec.SetErrorLevel %1
exit /b %1

:# Execute a command, logging its output.
:# Use for informative commands that should always be run, even in NOEXEC mode. 
:Do
set "Exec.ErrorLevel=%ERRORLEVEL%" &:# Save the initial errorlevel
setlocal EnableExtensions DisableDelayedExpansion &:# Clears the errorlevel
%IF_NOEXEC% call :Exec.On
goto :Exec.Start

:# Execute critical operations that should not be run in NOEXEC mode.
:# Version supporting input and output redirections, and pipes.
:# Redirection operators MUST be surrounded by quotes. Ex: "<" or ">" or ">>"
:Exec
set "Exec.ErrorLevel=%ERRORLEVEL%" &:# Save the initial errorlevel
setlocal EnableExtensions DisableDelayedExpansion &:# Clears the errorlevel
:Exec.Start
set "Exec.NOREDIR=%NOREDIR%"
set "Exec.Redir="				&:# The selected redirection. Default: none
set "Exec.2Redir=>>%LOGFILE%,2>&1"		&:# What to change it to, to enable redirection
if .%NOREDIR%.==.1. set "Exec.2Redir="		&:# Several cases forbid redirection
if not defined LOGFILE set "Exec.2Redir="
if /i .%LOGFILE%.==.NUL. set "Exec.2Redir="
set "Exec.IF_VERBOSE=%IF_VERBOSE%"		&:# Echo the command in verbose mode
:# Record the command-line to execute.
:# Never comment (set Exec.cmd) lines themselves, to avoid appending extra spaces.
:# Use %*, but not %1 ... %9, because %N miss non-white argument separators like = , ;
set ^"Exec.Cmd=%*^" &:# Doubles ^carets within "quoted" strings, and halves those outside
set ^"Exec.Cmd=%Exec.Cmd:^^=^%^" &:# Fix the # of ^carets within "quoted" strings
:# Process optional arguments
goto :Exec.GetArgs
:Exec.NextArg
:# Remove the %EXEC% argument and following spaces from the head of the command line
setlocal EnableDelayedExpansion &:# The next line works because no :exec own argument may contain an '=' or a '!'
for /f "tokens=1* delims= " %%a in ("-!Exec.Cmd:*%1=!") do endlocal & set Exec.Cmd=%%b
shift
:Exec.GetArgs
if "%~1"=="-l" set "Exec.Redir=%Exec.2Redir%" & goto :Exec.NextArg :# Do send the output to the log file
if "%~1"=="-L" set "Exec.Redir=" & goto :Exec.NextArg :# Do not send the output to the log file
if "%~1"=="-t" if defined Exec.2Redir ( :# Tee the output to the log file
  :# Warning: This prevents from getting the command exit code!
  if defined Exec.tee set "Exec.Redir= 2>&1 | %Exec.tee% -a %LOGFILE%"
  goto :Exec.NextArg
)
if "%~1"=="-e" set "Exec.IF_VERBOSE=if 1==1" & goto :Exec.NextArg :# Always echo the command
if "%~1"=="-v" set "Exec.IF_VERBOSE=%IF_VERBOSE%" & goto :Exec.NextArg :# Echo the command in verbose mode
if "%~1"=="-V" set "Exec.IF_VERBOSE=if 0==1" & goto :Exec.NextArg :# Do not echo the command in verbose mode
:# Anything else is part of the command. Prepare to display it and run it.
:# First stage: Split multi-char ops ">>" "2>" "2>>". Make sure to keep ">" signs quoted every time.
:# Do NOT use surrounding quotes for these set commands, else quoted arguments will break.
set Exec.Cmd=%Exec.Cmd:">>"=">"">"%
set Exec.Cmd=%Exec.Cmd:">>&"=">"">""&"%
set Exec.Cmd=%Exec.Cmd:">&"=">""&"%
:# If there are output redirections, then cancel any attempt at redirecting output to the log file.
set "Exec.Cmd1=%Exec.Cmd:"=%" &:# Remove quotes in the command string, to allow quoting the whole string.
if not "%Exec.Cmd1:>=%"=="%Exec.Cmd1%" set "Exec.Redir="
if defined Exec.Redir set "Exec.NOREDIR=1" &:# make sure child scripts do not try to redirect output again 
:# Second stage: Convert quoted redirection operators (Ex: ">") to a usable (Ex: >) and a displayable (Ex: ^>) value.
:# Must be done once for each of the four < > | & operators.
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
%Exec.IF_VERBOSE% set "Exec.Echo=echo"
%>DEBUGOUT% %Exec.Echo%.%INDENT%%Exec.toEcho%
if defined LOGFILE %>>LOGFILE% echo.%INDENT%%Exec.toEcho%
:# Constraints at this stage:
:# The command exit code must make it through, back to the caller.
:# The local variables must disappear before return.
:# But the new variables created by the command must make it through.
:# This should work whether :Exec is called with delayed expansion on or off.
endlocal & if not .%NOEXEC%.==.1. (
  set "NOREDIR=%Exec.NOREDIR%"
  %IF_DEBUG% set "INDENT=%INDENT%  "
  call :Exec.SetErrorLevel %Exec.ErrorLevel% &:# Restore the errorlevel we had on :Exec entrance
  %Exec.Cmd%%Exec.Redir%
  call set "Exec.ErrorLevel=%%ERRORLEVEL%%"  &:# Save the new errorlevel set by the command executed
  set "NOREDIR=%NOREDIR%" &:# Sets ERRORLEVEL=1 in Windows XP/64
  %IF_DEBUG% set "INDENT=%INDENT%"
  call :Exec.TraceExit
)
exit /b

:Exec.TraceExit
for %%e in (%Exec.ErrorLevel%) do (
  set "Exec.ErrorLevel="
  %IF_DEBUG% %>DEBUGOUT% echo.%INDENT%  exit %%e
  if defined LOGFILE %>>LOGFILE% echo.%INDENT%  exit %%e
  exit /b %%e
)

:Exec.ExecVar CMDVAR
call :Exec !%1:%%=%%%%!
exit /b

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
:#                                                                            #
:#                  Although this is not the objective of this function,      #
:#                  some effort is made to also produce a usable string if    #
:#                  the input contains characters that are invalid in file    #
:#                  names. Inner '"' are removed. "|&<>" are quoted.	      #
:#                                                                            #
:#  History                                                                   #
:#   2010-12-19 JFL Created this routine                                      #
:#   2011-12-12 JFL Rewrote using findstr. (Executes much faster.)	      #
:#		    Added support for empty pathnames.                        #
:#   2016-11-09 JFL Fixed this routine, which was severely broken :-(	      #
:#   2016-11-21 JFL Fixed the "!" quoting, and added "|&<>" quoting.	      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Quote file pathnames that require it. %1=Input variable. %2=Opt. output variable.
:condquote
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "RETVAR=%~2"
if not defined RETVAR set "RETVAR=%~1" &:# By default, change the input variable itself
%UPVAR% %RETVAR%
set "P=!%~1!"
:# Remove double quotes inside P. (Fails if P is empty, so skip this in this case)
if defined P set ^"P=!P:"=!"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Look for any special character that needs "quoting". See list from (cmd /?).
:# Added "@" that needs quoting ahead of commands.
:# Added "|&<>" that are not valid in file names, but that do need quoting if used in an argument string.
echo."!P!"|findstr /C:" " /C:"&" /C:"(" /C:")" /C:"[" /C:"]" /C:"{" /C:"}" /C:"^^" /C:"=" /C:";" /C:"!" /C:"'" /C:"+" /C:"," /C:"`" /C:"~" /C:"@" /C:"|" /C:"&" /C:"<" /C:">" >NUL
if not errorlevel 1 set P="!P!"
:condquote_ret
set "%RETVAR%=!P!"
%RETURN% 

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        dir type tree del rd set start ...			      #
:#                                                                            #
:#  Description     Registry file system management routines                  #
:#                                                                            #
:#  Notes 	    Several homonyms are provided for each function.          #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# List subkeys. Args: [-c] [-p PATTERN] [-cb CALLBACK] KEY [RETVAR]
:# Warning: Some keys (Ex: HKCR) have thousand of subkeys, which overflows a RETVAR
:keys
:dirs
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "PATTERN=*"
set "OPTS=/k"
set "KEY="
set "RETVAR="
set "BEFORE="
set "CALLBACK="
:dirs.get_args
if not defined ARGS goto :dirs.got_args
%POPARG%
%ECHOVARS.D% ARG
if "!ARG!"=="-c" set "OPTS=!OPTS! /c" & goto dirs.get_args
if "!ARG!"=="-cb" %POPARG% & set "CALLBACK=!ARG!" & goto dirs.get_args
if "!ARG!"=="-p" %POPARG% & set "PATTERN=!"ARG"!" & goto dirs.get_args
if not defined KEY set "KEY=!ARG!" & goto dirs.get_args
if not defined RETVAR set "RETVAR=!ARG!" & goto dirs.get_args
:dirs.got_args
%ECHOVARS.D% KEY RETVAR
if defined RETVAR set "RETVAL="
if "%FULLPATH%"=="1" set "BEFORE=!KEY!\"
:# Use reg.exe to get the key information
set CMD=reg query "!KEY!" /f !PATTERN! !OPTS!
:# For each line in CMD output...
set "ERROR="
set "^ERRORLEVEL=" &:# Make sure the first parsing phase does not expand this in the for 'command' below
%FOREACHLINE% %%l in ('%XEXEC@% CMD ^& call echo. ERRORLEVEL %%^^ERRORLEVEL%%') do (
  setlocal DisableDelayedExpansion
  set "LINE=%%l" &:# Must be executed with expansion disabled, else ! are lost
  call :Prep2ExpandVars LINE
  setlocal EnableDelayedExpansion
  for /f "delims=" %%m in ("!LINE!") do endlocal & endlocal & set "LINE=%%m" & set "NAME=%%~nxm"
  %ECHOVARS.D% LINE
  set "HEAD=!LINE:~0,2!"
  :# Special case of the exit code, appended to the data as a uniquely formatted text line " ERRORLEVEL N"
  if "!LINE:~0,12!"==" ERRORLEVEL " (
    :# Gotcha: reg.exe returns errorlevel 1 if the key is valid, but contains no subkey.
    :#         We override this by looking further down for the summary line with the number of keys found.
    :#         When the key is invalid, that summary line is not displayed.
    :#         If it is displayed, and if its value is 0, then ignore the errorlevel 1 here.
    if not defined ERROR %_DO.D% set "ERROR=!LINE:~12!"
  ) else if "!HEAD!"=="HK" (
    if "!NAME!"=="(Default)" set "NAME="
    set "NAME=!BEFORE!!NAME!"
    %ECHOVARS.D% NAME
    if defined CALLBACK (
      if defined NAME set "NAME=!NAME:%%=%%%%!"
      call %CALLBACK% "!NAME!"
    ) else if defined RETVAR (
      call :CondQuote NAME
      set "RETVAL=!RETVAL! !NAME!"
    ) else (
      call :Prep2ExpandVars NAME &:# Make sure %ECHO% displays tricky characters correctly
      %ECHO% !NAME!
    )
  ) else if not "!HEAD!"=="  " (
    call :dirs.ParseSummaryCount & rem :# If success, sets "ERROR=0" are returns 0
  )
)
if defined RETVAR if "%ERROR%"=="0" (
  %UPVAR% %RETVAR%
  if defined RETVAL set "RETVAL=!RETVAL:~1!"
  set "%RETVAR%=!RETVAL!"
)
%RETURN% %ERROR%

:# Parse the summary line, making sure it has the right format, and contains a 0 count.
:# It's a localized string, looking like: "End of search: 2 match(es) found"
:# In some languages, the count is not the first word, like in: "Found 2 matches"
:# This routine is also used when enumerating files.
:dirs.ParseSummaryCount 
for /f "tokens=1* delims=:" %%a in ("!LINE!") do (
  set PART1=%%a
  set PART2=%%b
  %ECHOVARS.D% PART1 PART2
  if defined PART2 if not "!PART2: 0=!"=="!PART2!" (
    %_DO.D% set "ERROR=0"
    exit /b 0
  )
)
exit /b 1

:#----------------------------------------------------------------------------#

:# Dump unordered values. Args: [-c] [-p PATTERN] KEY
:GetValues
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "PATTERN=*"
set "OPTS=/v"
set "KEY="
:GetValues.get_args
if not defined ARGS goto :GetValues.got_args
%POPARG%
if "!ARG!"=="-c" set "OPTS=%OPTS% /c" & goto GetValues.get_args
if "!ARG!"=="-p" %POPARG% & set "PATTERN=!"ARG"!" & goto GetValues.get_args
if not defined KEY set "KEY=!ARG!" & goto GetValues.get_args
>&2 %ECHO% %SCRIPT%: Error: Invalid argument !"ARG"! in function !FUNCTION.NAME!
%RETURN% 1
:GetValues.got_args
%ECHOVARS.D% KEY RETVAR
:# Use reg.exe to get the key information
set CMD=reg query "!KEY!" /f !PATTERN! !OPTS!
:# For each line in CMD output...
set "ERROR="
set "^ERRORLEVEL=" &:# Make sure the first parsing phase does not expand this in the for 'command' below
%FOREACHLINE% %%l in ('%XEXEC@% CMD ^& call echo./ERRORLEVEL/%%^^ERRORLEVEL%%') do (
  setlocal DisableDelayedExpansion
  set "LINE=%%l" &:# Must be executed with expansion disabled, else ! are lost
  call :Prep2ExpandVars LINE
  setlocal EnableDelayedExpansion
  for /f "delims=" %%m in ("!LINE!") do endlocal & endlocal & set "LINE=%%m"
  %ECHOVARS.D% LINE
  set "IDENTIFIED=0"
  :# Special case of the exit code, appended to the data as a uniquely formatted text line " ERRORLEVEL N"
  if "!LINE:~0,12!"=="/ERRORLEVEL/" (
    if not defined ERROR (
      %_DO.D% set "ERROR=!LINE:~12!" & rem :# May also be forced to 0 when a 0-file count is detected below
      echo.!LINE!
    )
    set "IDENTIFIED=1"
  )
  :# Special case of the count of values returned.
  set "HEAD=!LINE:~0,4!"
  if "!IDENTIFIED!"=="0" if not "!HEAD!"=="    " if not "!HEAD!"=="HKEY" (
    call :dirs.ParseSummaryCount &:# If success, sets "ERROR=0" are returns 0
    if not errorlevel 1 (
      echo./ERRORLEVEL/0
      set "IDENTIFIED=1"
    )
  )
  :# Values are indented by 4 spaces.
  set "LINE=!LINE:~4!"
  :# But extra lines of multi-lined values are indented by >20 spaces.
  set "HEAD2=!LINE:~0,4!"
  set "TOKENS="
  if "!HEAD!"=="    " if not "!HEAD2!"=="    " (
    :# Some versions of reg.exe use 4 spaces as field separator; others use a TAB.
    :# Change the 4-spaces around the REG_XX type word to a TAB.
    set "TOKENS=!LINE:    =	!"
    %ECHOVARS.D% TOKENS
    :# Extract the value name as the first item before the first TAB.
    :# Names can contain spaces, but assume they don't contain TABs.
    for /f "tokens=1,2* delims=	" %%j in ("!TOKENS!") do (
      setlocal DisableDelayedExpansion
      set "NAME=%%j"
      set "TYPE=%%k"
      set "VALUE=%%l"
      setlocal EnableDelayedExpansion
      if "!NAME!"=="(Default)" set "NAME="
      echo !NAME!/!TYPE!/!VALUE!
      endlocal
      endlocal
      set "IDENTIFIED=1"
    )
  )
)
%RETURN% %ERROR%

:# List sorted values. Args: [-c] [-p PATTERN] [-a] [-cb CALLBACK] KEY [RETVAR]
:values
:files
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "DETAILS=0"
set "PATTERN=*"
set "OPTS="
set "KEY="
set "RETVAR="
set "CALLBACK="
:get_values_args
if not defined ARGS goto :got_values_args
%POPARG%
if "!ARG!"=="-a" set "DETAILS=1" & goto get_values_args
if "!ARG!"=="-c" set "OPTS=%OPTS% -c" & goto get_values_args
if "!ARG!"=="-cb" %POPARG% & set "CALLBACK=!ARG!" & goto get_values_args
if "!ARG!"=="-p" %POPARG% & set "OPTS=%OPTS% -p !"ARG"!" & goto get_values_args
if not defined KEY set "KEY=!ARG!" & goto get_values_args
if not defined RETVAR set "RETVAR=!ARG!" & goto get_values_args
>&2 %ECHO% %SCRIPT%: Error: Invalid argument !"ARG"! in function !FUNCTION.NAME!
%RETURN% 1
:got_values_args
%ECHOVARS.D% KEY RETVAR
if defined RETVAR set "RETVAL="
set BEFORE=
if "%FULLPATH%"=="1" set "BEFORE=!KEY!\"
set "ERROR="
set CMD=:GetValues "!KEY!" !OPTS!
%FOREACHLINE% %%l in ('%XCALL@% CMD ^| sort') do (
  setlocal DisableDelayedExpansion
  set "LINE=%%l" &:# Must be executed with expansion disabled, else ! are lost
  call :Prep2ExpandVars LINE
  setlocal EnableDelayedExpansion
  for /f "delims=" %%m in ("!LINE!") do endlocal & endlocal & set "LINE=%%m"
  %ECHOVARS.D% LINE
  for /f "tokens=1,2* delims=/" %%j in ("-!LINE!") do ( :# Prepend a - to make sure the NAME is never empty
    setlocal DisableDelayedExpansion
    set "NAME=%%j"
    set "TYPE=%%k"
    set "VALUE=%%l"
    call :Prep2ExpandVars NAME TYPE VALUE
    setlocal EnableDelayedExpansion
    for /f "delims=" %%n in ("!NAME!") do (
      for /f "delims=" %%t in ("!TYPE!") do (
	for /f "delims=" %%v in ("-!VALUE!") do ( :# Prepend a - to make sure the VALUE is never empty
	  endlocal & endlocal
          set "NAME=%%n"
          set "TYPE=%%t"
          set "VALUE=%%v"
        )
      )
    )
    set "NAME=!NAME:~1!" &:# Remove the - inserted above, possibly leaving an empty name
    set "VALUE=!VALUE:~1!" &:# Remove the - inserted above, possibly leaving an empty value
    %ECHOVARS.D% NAME TYPE VALUE
    if "!TYPE!"=="ERRORLEVEL" ( :# Special case of the exit code
      %_DO.D% set "ERROR=!VALUE!"
    ) else (			:# General case of all REG_* values
      if %DETAILS%==0 (
	set "OUTPUT=!BEFORE!!NAME!"
      ) else (
	set "OUTPUT=!NAME!/!TYPE!/!VALUE!"
      )
      if defined CALLBACK (
      	if defined NAME set "NAME=!NAME:%%=%%%%!"
	call %CALLBACK% "!NAME!" "!TYPE!" VALUE
      ) else if defined RETVAR (
	call :CondQuote OUTPUT
	set "RETVAL=!RETVAL! !OUTPUT!"
      ) else (
      	call :Prep2ExpandVars OUTPUT &:# Make sure %ECHO% displays tricky characters correctly
	%ECHO% !OUTPUT!
      )
    )
  )
)
if defined RETVAR if "%ERROR%"=="0" (
  %UPVAR% %RETVAR%
  if defined RETVAL set "RETVAL=!RETVAL:~1!"
  set "%RETVAR%=!RETVAL!"
)
%RETURN% %ERROR%

:#----------------------------------------------------------------------------#

:# List subkeys and values. %1=Parent key pathname.
:dir
:list
:ls
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1" & shift
%ECHOVARS.D% KEY
:# Then call subroutines to get subkeys, then values.
call :dirs -cb :dirs.dirCB !KEY:%%=%%%%!
if errorlevel 1 %RETURN%
call :files -cb :dirs.fileCB !KEY:%%=%%%%!
if errorlevel 1 %RETURN%
%RETURN% 0

:dirs.dirCB DIRNAME
%FUNCTION% DisableDelayedExpansion
set "NAME=%~1" &:# Quoting this set command doubles ^carets, which will then be halved in the %ECHO%
setlocal EnableDelayedExpansion
call :Prep2ExpandVars NAME &:# Make sure %ECHO% displays tricky characters correctly
%ECHO% !NAME!\
endlocal
%RETURN% 0

:dirs.fileCB NAME TYPE VALUEVAR
%FUNCTION% DisableDelayedExpansion
set "NAME=%~1" &:# Quoting this set command doubles ^carets, which will then be halved in the %ECHO%
setlocal EnableDelayedExpansion
if not defined NAME set "NAME="""
call :Prep2ExpandVars NAME &:# Make sure %ECHO% displays tricky characters correctly
%ECHO% !NAME!
endlocal
%RETURN% 0

:#----------------------------------------------------------------------------#

:# Display a value. %1=Parent key pathname. %2=Optional RETVAR
:type
:cat
:get
:read
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1"
set "RETVAR=%~2"
if defined RETVAR %UPVAR% %RETVAR%
:# Split the registry path and name. Uses a fake drive @ to prevent prepending the current disk drive path to the registry path.
for %%K in (@:"!KEY!") do (
  set "KEY=%%~dpK"
  set "NAME=%%~nxK"
)
:# Remove the head "@:\ and tail \ to the KEY, and tail " to the NAME
set "KEY=%KEY:~4,-1%"
set "NAME=%NAME:~0,-1%"
%ECHOVARS.D% KEY NAME
if "%NAME%"=="" (
  set CMD=reg query "!KEY!" /ve
) else (
  set CMD=reg query "!KEY!" /v "%NAME%"
)
:# For each line in CMD output...
set "ERROR="
set "^ERRORLEVEL=" &:# Make sure the first parsing phase does not expand this in the for 'command' below
%FOREACHLINE% %%l in ('%XEXEC@% CMD ^& call echo. ERRORLEVEL %%^^ERRORLEVEL%%') do (
  setlocal DisableDelayedExpansion
  set "LINE=%%l" &:# Must be executed with expansion disabled, else ! are lost
  call :Prep2ExpandVars LINE
  setlocal EnableDelayedExpansion
  for /f "delims=" %%m in ("!LINE!") do endlocal & endlocal & set "LINE=%%m"
  %ECHOVARS.D% LINE
  :# Special case of the exit code, appended to the data as a uniquely formatted text line " ERRORLEVEL N"
  if "!LINE:~0,12!"==" ERRORLEVEL " %_DO.D% set "ERROR=!LINE:~12!"
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
      setlocal DisableDelayedExpansion
      set "NAME=%%j"
      set "TYPE=%%k"
      set "VALUE=%%l"
      call :Prep2ExpandVars NAME TYPE VALUE
      setlocal EnableDelayedExpansion
      for /f "delims=" %%n in ("!NAME!") do (
	for /f "delims=" %%t in ("!TYPE!") do (
	  for /f "delims=" %%v in ("-!VALUE!") do ( :# Prepend a - to make sure the VALUE is never empty
	    endlocal & endlocal
	    set "NAME=%%n"
	    set "TYPE=%%t"
	    set "VALUE=%%v"
	  )
	)
      )
      set "VALUE=!VALUE:~1!" &:# Remove the - inserted above, possibly leaving an empty value
      %ECHOVARS.D% NAME TYPE VALUE
    )
    set BEFORE=
    if defined RETVAR (
      set "%RETVAR%=!VALUE!"
    ) else (
      %IF_VERBOSE% set "BEFORE=!KEY!\!NAME! = "
      set "OUTPUT=!BEFORE!!VALUE!"
      call :Prep2ExpandVars OUTPUT &:# Make sure %ECHO% displays tricky characters correctly
      %ECHO% !OUTPUT!
    )
  )
)
%RETURN% %ERROR%

:#----------------------------------------------------------------------------#

:# Non-recursive show. %1=Parent key pathname.
:show
:showdir
:dump
:dumpdir
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1" & shift
%ECHOVARS.D% KEY
set "EXEC=!EXEC! -V"		&:# Do not display the commands in verbose mode
set "ECHO.XVD=%ECHO.XD%"	&:# Do not display the commands in verbose mode
:# Then call subroutines to get subkeys, then values.
call :dirs -cb :show.dirCB "!KEY:%%=%%%%!" %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 %RETURN%
set "ATTRS= "
call :files -cb :show.fileCB "!KEY:%%=%%%%!" %1 %2 %3 %4 %5 %6 %7 %8 %9
if errorlevel 1 %RETURN%
%RETURN% 0

:show.dirCB DIRNAME
%FUNCTION% DisableDelayedExpansion
set "SUBKEY\=%~1\"
setlocal EnableDelayedExpansion
call :CondQuote SUBKEY\
call :Prep2ExpandVars SUBKEY &:# Make sure %ECHO% displays tricky characters correctly
%ECHO% !SUBKEY! {}
endlocal
%RETURN% 0

:show.fileCB NAME TYPE VALUEVAR
%FUNCTION% DisableDelayedExpansion
set "NAME=%~1"
set "TYPE=%~2"
setlocal EnableDelayedExpansion
set ^"VALUE=!%~3!^"
call :CondQuote NAME
if ^"!VALUE:"=!"=="!VALUE!" (
  call :CondQuote VALUE
) else ( :# TO DO: A fully SML-compatible conversion of the string into an {SML block}
  set ^"VALUE={!VALUE!}^"
)
set "ATTRS= "
%IF_VERBOSE% SET "ATTRS= type=!TYPE! "
set "OUTPUT=!NAME!!ATTRS!!VALUE!"
call :Prep2ExpandVars OUTPUT &:# Make sure %ECHO% displays tricky characters correctly
%ECHO% !OUTPUT!
endlocal
%RETURN% 0

:#----------------------------------------------------------------------------#

:# Recursive show. %1=Parent key pathname.
:tree
:showtree
:dumptree
%FUNCTION% EnableExtensions EnableDelayedExpansion
:# Do not quote this set command, else ^carets would be doubled
set KEY=%~1
%ECHOVARS.D% KEY
set "EXEC=!EXEC! -V"		&:# Do not display the commands in verbose mode
set "ECHO.XVD=%ECHO.XD%"	&:# Do not display the commands in verbose mode
:# Then call subroutines to get subkeys, then values.
call :dirs -cb :tree.dirCB "!KEY:%%=%%%%!"
if errorlevel 1 %RETURN%
call :files -cb :show.fileCB "!KEY:%%=%%%%!"
if errorlevel 1 %RETURN%
%RETURN% 0

:tree.dirCB DIRNAME
%FUNCTION% DisableDelayedExpansion
:# Do not quote these set commands, else ^carets would be doubled
set SUBKEY=%~1
set SUBKEY\=%~1\
setlocal EnableDelayedExpansion
call :CondQuote SUBKEY\
call :Prep2ExpandVars SUBKEY\ &:# Make sure %ECHO% displays tricky characters correctly
%ECHO% !SUBKEY\! {
  if "%DEBUG%"=="0" set "INDENT=%INDENT%  "
  set "SUBKEY=!KEY!\!SUBKEY!"
  call :tree "!SUBKEY:%%=%%%%!"
  if "%DEBUG%"=="0" set "INDENT=%INDENT:~2%"
%ECHO% }
endlocal
%RETURN% 0

:#----------------------------------------------------------------------------#

:# Delete a key value. %1=Parent key pathname.
:del
:rm
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1"
set "OPTS="
if "%~2"=="-f" set "OPTS=/f"	&:# Force writing without confirmation
:# Split the registry path and name. Uses a fake drive @ to prevent prepending the current disk drive path to the registry path.
for %%K in (@:"!KEY!") do (
  set "KEY=%%~dpK"
  set "NAME=%%~nxK"
)
:# Remove the head "@:\ and tail \ to the KEY, and tail " to the NAME
set "KEY=%KEY:~4,-1%"
set "NAME=%NAME:~0,-1%"
%ECHOVARS.D% KEY NAME
if "%NAME%"=="" (
  set CMD=reg delete "!KEY!" /ve
) else (
  set CMD=reg delete "!KEY!" /v "%NAME%"
)
if defined OPTS set "CMD=!CMD! !OPTS!"
%EXEC% %CMD%
%RETURN%

:#----------------------------------------------------------------------------#

:# Delete a key and all its contents. %1=Parent key pathname.
:rd
:rmdir
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1"
set "OPTS="
if "%~2"=="-f" set "OPTS=/f"	&:# Force writing without confirmation
set CMD=reg delete "!KEY!"
if defined OPTS set "CMD=!CMD! !OPTS!"
%EXEC% %CMD%
%RETURN%

:#----------------------------------------------------------------------------#

:# Set a key value. %1=Key\value. %2=Data
:set
:write
:add
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1"
set "DATA=%2"	&:# Include the argument quotes if present.
:# Split the registry path and name. Uses a fake drive @ to prevent prepending the current disk drive path to the registry path.
for %%K in (@:"!KEY!") do (
  set "KEY=%%~dpK"
  set "NAME=%%~nxK"
)
:# Remove the head "@:\ and tail \ to the KEY, and tail " to the NAME
set "KEY=%KEY:~4,-1%"
set "NAME=%NAME:~0,-1%"
%ECHOVARS.D% KEY NAME
if "%NAME%"=="" (
  set CMD=reg add "!KEY!" /ve 
) else (
  set CMD=reg add "!KEY!" /v "%NAME%"
)
set "CMD=!CMD! /d !DATA! /f"
%EXEC% %CMD%
%RETURN%

:#----------------------------------------------------------------------------#

:# Open RegEdit at a given key. %1=Key pathname.
:open
:start
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1"
:# Make sure the key pathname uses for full-length hive name
set "HIVENAME[HKCR]=HKEY_CLASSES_ROOT"
set "HIVENAME[HKCU]=HKEY_CURRENT_USER"
set "HIVENAME[HKLM]=HKEY_LOCAL_MACHINE"
set "HIVENAME[HKU]=HKEY_USERS"
set "HIVENAME[HKCC]=HKEY_CURRENT_CONFIG"
for /f "tokens=1* delims=\" %%h in ("!KEY!") do (
  set "HIVE=!HIVENAME[%%h]!"
  if not defined HIVE set "HIVE=%%h" & rem :# Assume the user entered the full name already
  set "KEY=!HIVE!\%%i"
)
:# Get the local system name. Ex: "My Computer" or "Computer"
setlocal & %IF_NOEXEC% call :Exec.on &:# Make sure the :get is always executed
call :get "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Internet Settings\Zones\0\DisplayName" COMPUTER
endlocal & set "COMPUTER=%COMPUTER%" &:# Restore the initial NOEXEC mode
:# Change RegEdit's last used key, so that next time it opens that key "again".
call :set "HKCU\Software\Microsoft\Windows\CurrentVersion\Applets\Regedit\LastKey" "%COMPUTER%\!KEY!" >NUL
%EXEC% start /b regedit
%RETURN%

:#----------------------------------------------------------------------------#

:# Test a number of tricky cases
:test
call :CondQuote ARG0
echo -----------------------------------------------------------
echo A key with subkeys containing the %% character:
%EXEC% call %ARG0% dirs HKCU\Console
echo.
echo -----------------------------------------------------------
echo A key with data containing the %% character:
%EXEC% call %ARG0% show HKCU\Environment
echo.
echo -----------------------------------------------------------
echo A key with subkeys containing the ^^ character:
%EXEC% call %ARG0% dirs "HKCU\Control Panel\Desktop\PerMonitorSettings"
echo.
echo -----------------------------------------------------------
echo A key with data containing the ^^ character:
%EXEC% call %ARG0% show "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Extensions"
echo.
echo -----------------------------------------------------------
echo A key with subkeys containing the ^^^! character:
%EXEC% call %ARG0% dir HKLM\SOFTWARE\Microsoft\DirectDraw\Compatibility
echo.
echo -----------------------------------------------------------
echo A key with subkeys containing the ^^^! character:
%EXEC% call %ARG0% show "HKCR\Acrobat.aaui\shell\Open\command"
exit /b

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
echo   -F               List the full pathname of keys and values
echo   -v               Verbose output
echo   -V               Display this script version and exit
echo   -X               Display the reg.exe commands to run, but don't run them
echo.
echo Commands:
echo   del PATH\NAME        Delete a key value
echo   dir PATH             List subkeys/ and values names (1)
echo   keys PATH            List subkeys names
echo   open PATH            Open regedit.exe at the given key
echo   rd PATH              Delete a key, and all sub-keys and value
echo   set PATH\NAME DATA   Set the key value content 
echo   show PATH            Display all values contents (1) (2)
echo   tree PATH            Display the whole tree key values contents (1) (2)
echo   type PATH\NAME       Display the key value content 
echo   values PATH          List values names (1)
echo.
echo PATH\NAME: Concatenation of the key and value names. Ex: HKCU\Environment\TEMP
echo            Use PATH\ to refer to the default value in a key. Ex: HKCR\.txt\
echo.
echo dir, keys, values options:
echo   -c               Case-sensitive search
echo   -p PATTERN       Search for a specific pattern. Default: *
echo.
echo del, rd options:
echo   -f               Force deletion without confirmation
echo.
echo set options:
echo   -t TYPE          Value type. See (reg add /?) for details. Default: REG_SZ
echo.
echo (1): The unnamed "(Default)" value is displayed as name "".
echo (2): In verbose mode, displays attributes with the value type.
echo.
goto :eof

:main
set "ACTION="
set "KEY="
set "OPTS="
set ">DEBUGOUT=>&2"	&:# Send debug output to stderr, so that it does not interfere with subroutines output capture
if not defined FULLPATH set "FULLPATH=0"

:next_arg
if not defined ARGS set "ARG=" & goto :go
%POPARG%
if "!ARG!"=="-?" goto :Help
if "!ARG!"=="/?" goto :Help
if "!ARG!"=="-c" set "OPTS=!OPTS! -c" & goto :next_arg		&:# Case-sensitive option
if "!ARG!"=="-d" call :Debug.on & goto :next_arg
if "!ARG!"=="-f" set "OPTS=!OPTS! -f" & goto :next_arg		&:# Force option
if "!ARG!"=="-F" set "FULLPATH=1" & goto :next_arg
if "!ARG!"=="-p" %POPARG% & set "OPTS=!OPTS! -p !"ARG"!" & goto :next_arg	&:# Pattern
if "!ARG!"=="-t" %POPARG% & set "OPTS=!OPTS! -t !ARG!" & goto :next_arg		&:# Key type
if "!ARG!"=="-v" call :Verbose.on & goto :next_arg
if "!ARG!"=="-V" (echo.%VERSION%) & goto :eof
if "!ARG!"=="-X" call :Exec.off & goto :next_arg
if "!ACTION!"=="" set "ACTION=!ARG!" & goto next_arg
if "!KEY!"=="" set "KEY=!ARG!" & goto next_arg
if not "!ARG:~0,1!!ARG:~0,1!"=="--" set "OPTS=!OPTS! !"ARG"!" & goto next_arg	&:# Some commands take additional arguments 
2>&1 echo Unexpected option, ignored: %"ARG"%
goto next_arg

:# Execute the requested command
:go
call :%ACTION% "!KEY!" !OPTS!
