@echo off
:##############################################################################
:#                                                                            #
:#  Filename        search.bat                                                #
:#                                                                            #
:#  Description     Run a Windows search using the Search Service	      #
:#                                                                            #
:#  Notes 	    See Windows Search documentation:			      #
:#		    https://docs.microsoft.com/en-us/windows/win32/search/windows-search
:#		    https://docs.microsoft.com/en-us/windows/win32/search/getting-started-with-parameter-value-arguments
:#		    https://docs.microsoft.com/en-us/windows/win32/search/-search-3x-wds-qryidx-searchms
:#		    https://docs.microsoft.com/en-us/windows/win32/search/-search-3x-advancedquerysyntax
:#		    https://docs.microsoft.com/en-us/windows/win32/lwef/-search-2x-wds-perceivedtype
:#		    https://docs.microsoft.com/en-us/windows/win32/lwef/-search-2x-wds-aqsreference
:#                                                                            #
:#  History                                                                   #
:#   2016-01-12 JFL jf.larvoire@free.fr created this script.                  #
:#   2021-11-30 JFL Added option -c to open the search Control Panel.         #
:#   2021-12-01 JFL Show the search time in the output window. This makes     #
:#                  sure the window is not reused by subsequent searches.     #
:#                  Changed the default to search in all indexed locations.   #
:#                  Added option -i to specify the location to search in.     #
:#   2021-12-05 JFL Encode = signs with the pure Batch :ReplaceEquals functn. #
:#   2021-12-06 JFL Encode all characters that need to be URL-encoded.        #
:#                                                                            #
:##############################################################################

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2021-12-06"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0"
set "SPATH=%SPATH:~0,-1%"
set "ARG0=%~f0"
set "ARGS=%*"
setlocal EnableExtensions EnableDelayedExpansion

call :Library.Init
goto Main

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
:# call :Echo.Color.Init

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
:#                  Known limitation: Special character ^ is preserved within #
:#                  "quoted" arguments, but not within unquoted arguments.    #
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

:Sub.Init # Create a SUB variable containing a SUB (Ctrl-Z) character
>NUL copy /y NUL + NUL /a "%TEMP%\1A.chr" /a
for /f %%c in (%TEMP%\1A.chr) do set "SUB=%%c"
exit /b

:Call.Init
if not defined LCALL set "LCALL=call"	&:# Macro to call functions in this library
set "POPARG=%LCALL% :PopArg"
set "POPSARG=%LCALL% :PopSimpleArg"

:# Mechanism for calling subroutines in a second external instance of the top script.
set ^"XCALL=call "!SFULL!" -call^"	&:# This is the full path to the top script's (or this lib's if called directly) ARG0
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
:# Use the ASCII control character name, or the HTML entity name.
:# Warning: The excl and hat characters need different quoting depending on context.
set  "@percnt=%%"	&:# One percent sign
set  "@excl=^!"		&:# One exclamation mark
set  "@hat=^"		&:# One caret, aka. circumflex accent, or hat sign
set ^"@quot=""		&:# One double quote
set  "@apos='"		&:# One apostrophe
set  "@amp=&"		&:# One ampersand
set  "@vert=|"		&:# One vertical bar
set  "@gt=>"		&:# One greater than sign
set  "@lt=<"		&:# One less than sign
set  "@lpar=("		&:# One left parenthesis
set  "@rpar=)"		&:# One right parenthesis
set  "@lbrack=["	&:# One left bracket
set  "@rbrack=]"	&:# One right bracket
set  "@sp= "		&:# One space
set  "@tab=	"	&:# One tabulation
set  "@quest=?"		&:# One question mark
set  "@ast=*"		&:# One asterisk
set  "@cr=!CR!"		&:# One carrier return
set  "@lf=!LF!"		&:# One line feed
set  "@bs=!BS!"		&:# One backspace
set  "@ff=!FF!"		&:# One form feed
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

:PopArg.Eon
setlocal DisableDelayedExpansion
call :PopArg.Eoff
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

:# Prepare one variable, in a local scope with !expansion! either on or off, for %expansion% in another scope with !expansion! on
:Prep2ExpandVar     INVAR [OUTVAR]
if "!!"=="" (	:# The local scope has expansion on
  :# Prepare one variable, in a local scope with !expansion! on, for %expansion% in another scope with !expansion! on
  :Prep2ExpandVar.Eon INVAR [OUTVAR]
  if not "%~2"=="" set "%~2=!%~1!" & shift
  if defined %1 (
    for %%e in (sp tab cr lf quot amp vert lt gt hat percnt) do ( :# Encode named character entities
      for %%c in ("!@%%e!") do (
	set "%~1=!%~1:%%~c= @%%e !"
      )
    )
    call set "%~1=%%%~1:^!= @excl %%" 	& rem :# Encode exclamation points                          
    call set "%~1=%%%~1: =^!%%"		& rem :# Encode final expandable entities
  )
  exit /b
) else (	:# The local scope has expansion off
  :# Prepare one variable, in a local scope with !expansion! off, for %expansion% in another scope with !expansion! on
  :Prep2ExpandVar.Eoff INVAR [OUTVAR]
  setlocal EnableDelayedExpansion
  set "VALUE=!%~1!"
  call :Prep2ExpandVar.Eon VALUE
  if not "%~2"=="" shift
  endlocal & set "%~1=%VALUE%"
  exit /b
)

:# Prepare variables, in a local scope with !expansion! either on or off, for %expansion% in another scope with !expansion! on
:Prep2ExpandVars VAR [VAR ...]
if "!!"=="" (	:# The local scope has expansion on
  :# Prepare variables, in a local scope with !expansion! on, for %expansion% in another scope with !expansion! on
  :Prep2ExpandVars.Eon VAR [VAR ...]
  for %%v in (%*) do call :Prep2ExpandVar.Eon %%v
  exit /b
) else (	:# The local scope has expansion off
  :# Prepare variables, in a local scope with !expansion! off, for %expansion% in another scope with !expansion! on
  :Prep2ExpandVars.Eoff
  for %%v in (%*) do call :Prep2ExpandVar.Eoff %%v
  exit /b
)

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
:# Define a ! protected by an exponential number of hats
set "^^1^!=^^^!"					&:# %^1!% expands to (2^1)-1 hats before the !
set "^^2^!=^^^^^^^!"					&:# %^2!% expands to (2^2)-1 hats before the !
set "^^3^!=^^^^^^^^^^^^^^^!"				&:# %^3!% expands to (2^3)-1 hats before the !
set "^^4^!=^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^!"		&:# %^4!% expands to (2^4)-1 hats before the !
set "^^5^!=^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^!" &:# Etc...
set "^^6^!=^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^!"

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
:#                                                                            #
:#                  Echo	    Echo and log strings, indented            #
:#                  EchoVars	    Display a set of variables name=value     #
:#                  EchoStringVars  Display a string, then a set of variables #
:#                  EchoArgs	    Display all arguments name=value          #
:#                  EchoVals	    Display the value of multiple variables   #
:#		    All functions in that series have two other derivatives,  #
:#                  with the .debug and .verbose suffix. Ex: Echo.Debug       #
:#                  These display only in debug and verbose mode respectively,#
:#                  but always log the string (if a log file is defined).     #
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
:#                  %ECHOSVARS%	    Echo ARG1 before each variable.           #
:#                  %ECHOSVARS.V%   Idem, but display them in verb. mode only #
:#                  %ECHOSVARS.D%   Idem, but display them in debug mode only #
:#                                                                            #
:#                  %ECHOVALS%      Echo the value of multiple variables      #
:#                  %ECHOVALS.V%    Idem, but display them in verb. mode only #
:#                  %ECHOVALS.D%    Idem, but display them in debug mode only #
:#                                                                            #
:#                  %ECHOSTRINGS%   Echo the value of multiple quoted strings #
:#                  %ECHOSTRINGS.V% Idem, but display them in verb. mode only #
:#                  %ECHOSTRINGS.D% Idem, but display them in debug mode only #
:#                                                                            #
:#                  %IF_DEBUG%      Execute a command in debug mode only      #
:#                  %IF_VERBOSE%    Execute a command in verbose mode only    #
:#                                                                            #
:#                  %FUNCTION0%	    Weak functions with no local variables.   #
:#                  %RETURN0%       Return from a %FUNCTION0% and trace it    #
:#                  %RETURN#%       Idem, with comments after the return      #
:#                                                                            #
:#                  %+INDENT%       Manually increase the debug INDENT        #
:#                  %-INDENT%       Manually decrease the debug INDENT        #
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
:#                  The debug output is sent stdout or stderr, depending on   #
:#                  variable %>DEBUGOUT%.				      # 
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
set "ECHOSVARS=%LCALL% :EchoStringVars"
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
	set "DEBUG.VALUE=%'!%DEBUG.VALUE:%%=%%@percnt%%%'!%"	%# Encode percent #% %\n%
	for %%e in (sp tab cr lf quot amp vert lt gt) do for %%c in ("%'!%@%%e%'!%") do ( %# Encode named character entities #% %\n%
	  set "DEBUG.VALUE=%'!%DEBUG.VALUE:%%~c=%%@%%e%%%'!%" %\n%
	) %\n%
	set "DEBUG.VALUE=%'!%DEBUG.VALUE:^^=%%@hat%%%'!%"	%# Encode carets #% %\n%
	call set "DEBUG.VALUE=%%DEBUG.VALUE:%!%=^^^^%%" 		%# Encode exclamation points #% %\n%
	set "DEBUG.VALUE=%'!%DEBUG.VALUE:^^^^=%%@excl%%%'!%"	%# Encode exclamation points #% %\n%
      ) %\n%
      set DEBUG.SETARGS=%!%DEBUG.SETARGS%!% "%%v=%'!%DEBUG.VALUE%'!%"%\n%
    ) %\n%
    if %!%DEBUG%!%==1 ( %# Build the debug message and display it #% %\n%
      set "DEBUG.MSG=return %'!%DEBUG.EXITCODE%'!%" %\n%
      for /f "delims=" %%v in ("%'!%DEBUG.SETARGS: =%%~l%'!%") do if not %%v=="" ( %# for /f avoids issues with ? and * #% %\n%
	set "DEBUG.MSG=%'!%DEBUG.MSG%'!% %%@amp%% set %%v" %!% %\n%
      ) %\n%
      call set "DEBUG.MSG=%'!%DEBUG.MSG:%%=%%@excl%%%'!%" %# Change all percent to ! #%  %\n%
      if defined ^^%>%DEBUGOUT ( %# If we use a debugging stream distinct from stdout #% %\n%
	%LCALL% :Echo.Eval2DebugOut DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
      ) else ( %# Output directly here, which is faster #% %\n%
	for /f "delims=" %%c in ("%'!%INDENT%'!%%'!%DEBUG.MSG%'!%") do echo %%c%# Use a for loop to do a double !variable! expansion #%%\n%
      ) %\n%
      if defined LOGFILE ( %# If we have to send a copy to a log file #% %\n%
	%LCALL% :Echo.Eval2LogFile DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
      ) %\n%
    ) %\n%
    for %%r in (%!%DEBUG.EXITCODE%!%) do ( %# Carry the return values through the endlocal barriers #% %\n%
      for /f "delims=" %%a in ("%'!%DEBUG.SETARGS%'!%") do ( %\n%
	endlocal %&% endlocal %&% endlocal %# Exit the RETURN and FUNCTION local scopes #% %\n%
	set "DEBUG.SETARGS=%%a" %\n%
	if "%'!%%'!%"=="" ( %# Delayed expansion is ON #% %\n%
	  call set "DEBUG.SETARGS=%'!%DEBUG.SETARGS:%%=%%@excl%%%'!%" %# Change all percent to ! #%  %\n%
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
set "ECHOVARS.XD=rem" &:# Keep old debug directives, but don't output them anymore
set "ECHOSVARS.V=%LCALL% :EchoStringVars.Verbose"
set "ECHOSVARS.D=%LCALL% :EchoStringVars.Debug"
set "ECHOVALS=%LCALL% :EchoVals"
set "ECHOVALS.V=%LCALL% :EchoVals.Verbose"
set "ECHOVALS.D=%LCALL% :EchoVals.Debug"
set "ECHOSTRINGS=%LCALL% :EchoStrings"
set "ECHOSTRINGS.V=%LCALL% :EchoStrings.Verbose"
set "ECHOSTRINGS.D=%LCALL% :EchoStrings.Debug"
set "+INDENT=%LCALL% :Debug.IncIndent"
set "-INDENT=%LCALL% :Debug.DecIndent"
set ">MSGOUT.V[0]=rem"
set ">MSGOUT.V[1]="
set ">MSGOUT.D[0]=rem"
set ">MSGOUT.D[1]=%>DEBUGOUT%"
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
  call set ARGS=%%*%# Do not quote this, to keep string/non string alternance #%%\n%
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
if not defined LOGFILE set "ECHO.D=echo >NUL"
if .%LOGFILE%.==.NUL. set "ECHO.D=echo >NUL"
if not defined LOGFILE set "ECHOVARS.D=echo >NUL"
if .%LOGFILE%.==.NUL. set "ECHOVARS.D=echo >NUL"
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
      %LCALL% :Echo.2DebugOut DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
    ) else ( %# Output directly here, which is faster #% %\n%
      echo%!%INDENT%!% %!%DEBUG.MSG%!%%\n%
    ) %\n%
    if defined LOGFILE ( %# If we have to send a copy to a log file #% %\n%
      %LCALL% :Echo.2LogFile DEBUG.MSG %# Use a helper routine, as delayed redirection does not work #% %\n%
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
:Debug.IncIndent
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
:Debug.DecIndent
if defined INDENT set "INDENT=%INDENT:~2%"
exit /b %1

:Debug.Return# :# %RETURN.ERR% %MACRO.ARGS%
setlocal DisableDelayedExpansion
%>DEBUGOUT% echo %INDENT%return %RETURN.ERR% ^&:#%MACRO.ARGS%
if defined LOGFILE %>>LOGFILE% echo %INDENT%return %RETURN.ERR% ^&:#%MACRO.ARGS%
endlocal
goto :eof &:# %RETURN.ERR% will be processed in the %DEBUG#% macro.

:# A lightweight alternative for the %RETURN% macro.
:# Only traces the %ERRORLEVEL%, but not the variables returned.
:# Trace the return from a subroutine, and do the actual return, in a single call
:Return
:# gotcha: setlocal sometimes clears %ERRORLEVEL%, so the reading must be on same line
setlocal & set "ERR=%~1" & if not defined ERR set "ERR=%ERRORLEVEL%"
%IF_DEBUG% %>DEBUGOUT% echo   exit /b %ERR%
:# An explicit endlocal isn't required, as (goto) does it automatically.
2>NUL (goto) & exit /b %ERR% &:# Endlocal and pop one call stack, then return to the upper level

:# Routine to set the VERBOSE mode, in response to the -v argument.
:Verbose.Off
:Verbose.0
set "VERBOSE=0"
set "IF_VERBOSE=if .%VERBOSE%.==.1."
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -v=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Optimization to speed things up in non-verbose mode
if not defined LOGFILE set "ECHO.V=echo >NUL"
if .%LOGFILE%.==.NUL. set "ECHO.V=echo >NUL"
if not defined LOGFILE set "ECHOVARS.V=echo >NUL"
if .%LOGFILE%.==.NUL. set "ECHOVARS.V=echo >NUL"
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
:EchoStringVars %1=string %2=VARNAME %3=VARNAME ...
setlocal EnableExtensions EnableDelayedExpansion
set "INDENT=%INDENT%%~1 "
shift
goto :EchoVars.loop
:EchoVars	%1=VARNAME %2=VARNAME %3=VARNAME ...
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

:EchoStringVars.Verbose
%IF_VERBOSE% (
  call :EchoStringVars %*
) else ( :# Make sure the variables are logged
  call :EchoStringVars %* >NUL 2>NUL
)
goto :eof

:EchoStringVars.Debug
%IF_DEBUG% (
  call :EchoStringVars %*
) else ( :# Make sure the variables are logged
  call :EchoStringVars %* >NUL 2>NUL
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

:# Echo the value of multiple variables on the same line
:EchoVals	%1=VARNAME, %2=VARNAME, ...
setlocal EnableDelayedExpansion
set ">MSGOUT="
:EchoVals.1
set "EchoVals.LINE=" &:# Use a qualified name, in case the caller passes a variable called LINE
for %%v in (%*) do set "EchoVals.LINE=!EchoVals.LINE! !%%v!"
if not defined EchoVals.LINE set "EchoVals.LINE= " &:# Make sure there's a head space even if the variable list was empty
%>MSGOUT% echo.%INDENT%!EchoVals.LINE:~1!
if defined LOGFILE %>>LOGFILE% echo.%INDENT%!EchoVals.LINE:~1!
endlocal & exit /b

:EchoVals.Verbose
setlocal EnableDelayedExpansion
set ">MSGOUT=!>MSGOUT.V[%VERBOSE%]!"
goto :EchoVals.1

:EchoVals.Debug
setlocal EnableDelayedExpansion
set ">MSGOUT=!>MSGOUT.D[%DEBUG%]!"
goto :EchoVals.1

:# Echo the value of multiple strings on the same line. They must not contain double quotes.
:EchoStrings	%1=Quoted_String, %2=Quoted_String, ...
setlocal DisableDelayedExpansion
set ">MSGOUT="
:EchoStrings.1
set "LINE=" &:# No need for a qualified name, since we don't use caller variables
for %%v in (%*) do set "LINE=%LINE% %%~v"
if not defined LINE set "LINE= " &:# Make sure there's a head space even if the string list was empty
%>MSGOUT% echo.%INDENT%%LINE:~1%
if defined LOGFILE %>>LOGFILE% echo.%INDENT%%LINE:~1%
endlocal & exit /b

:EchoStrings.Verbose
setlocal DisableDelayedExpansion
%IF_VERBOSE% (
  set ">MSGOUT="
) else ( :# Make sure the variables are logged
  set ">MSGOUT=rem"
)
goto :EchoStrings.1

:EchoString1.Debug
setlocal DisableDelayedExpansion
%IF_DEBUG% (
  set ">MSGOUT=%>DEBUGOUT%"
) else ( :# Make sure the variables are logged
  set ">MSGOUT=rem"
)
goto :EchoStrings.1

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
:#		    -f		Force executing the command, even in NOEXEC m.#
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
:#   2017-01-13 JFL Added option -f to routine :Exec.                         #
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
set "FALSE0.EXE=(call)"	&:# Macro to silently set ERRORLEVEL to 1
set "FALSE.EXE=((for /f %%i in () do .)||rem.)" &:# Faster macro to silently set ERRORLEVEL to 1
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
set "Exec.IF_EXEC=%IF_EXEC%"			&:# IF_EXEC macro
set "Exec.IF_NOEXEC=%IF_NOEXEC%"		&:# IF_NOEXEC macro
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
if "%~1"=="-f" set "Exec.IF_EXEC=if 1==1" & set "Exec.IF_NOEXEC=if 0==1" & goto :Exec.NextArg :# Always execute the command
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
%Exec.IF_NOEXEC% set "Exec.Echo=echo"
%IF_DEBUG% set "Exec.Echo=echo"
%Exec.IF_VERBOSE% set "Exec.Echo=echo"
%>DEBUGOUT% %Exec.Echo%.%INDENT%%Exec.toEcho%
if defined LOGFILE %>>LOGFILE% echo.%INDENT%%Exec.toEcho%
:# Constraints at this stage:
:# The command exit code must make it through, back to the caller.
:# The local variables must disappear before return.
:# But the new variables created by the command must make it through.
:# This should work whether :Exec is called with delayed expansion on or off.
endlocal & %Exec.IF_EXEC% (
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
:#                  I've removed "!" as quoting does NOT prevent expansions.  #
:#                                                                            #
:#  History                                                                   #
:#   2010-12-19 JFL Created this routine                                      #
:#   2011-12-12 JFL Rewrote using findstr. (Executes much faster.)	      #
:#		    Added support for empty pathnames.                        #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Quote file pathnames that require it. %1=Input variable. %2=Opt. output variable.
:condquote
%FUNCTION% EnableExtensions EnableDelayedExpansion
set RETVAR=%~2
if "%RETVAR%"=="" set RETVAR=%~1
set "P=!%~1!"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Remove double quotes inside P. (Fails if P is empty)
set "P=%P:"=%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Look for any special character that needs quoting
echo."%P%"|findstr /C:" " /C:"&" /C:"(" /C:")" /C:"@" /C:"," /C:";" /C:"[" /C:"]" /C:"{" /C:"}" /C:"=" /C:"'" /C:"+" /C:"`" /C:"~" >NUL
if not errorlevel 1 set P="%P%"
:condquote_ret
%UPVAR% %RETVAR%
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Now                                                       #
:#                                                                            #
:#  Description     Locale-independant routine to parse the current date/time #
:#                                                                            #
:#  Returns         Environment variables YEAR MONTH DAY HOUR MINUTE SECOND MS#
:#                                                                            #
:#  Notes 	    This routine is a pure-batch attempt at parsing the date  #
:#                  and time in a way compatible with any language and locale.#
:#                  Forces the output variables widths to fixed widths,       #
:#		    suitable for use in ISO 8601 date/time format strings.    #
:#                  Note that it would have been much easier to cheat and     #
:#                  do all this by invoking a PowerShell command!             #
:#                                                                            #
:#                  The major difficulty is that the cmd.exe date and time    #
:#                  are localized, and the year/month/day order and separator #
:#                  vary a lot between countries and languages.               #
:#                  Workaround: Use the short date format from the registry   #
:#                  as a template to analyse the date and time strings.       #
:#                  Tested in English, French, German, Spanish, Simplified    #
:#		    Chinese, Japanese.                                        #
:#                                                                            #
:#                  Uses %TIME% and not "TIME /T" because %TIME% gives more:  #
:#                  %TIME% returns [H]H:MM:SS.hh			      #
:#		    "TIME /T" returns MM:SS only.                             #
:#                                                                            #
:#                  Set DEBUG_NOW=1 before calling this routine, to display   #
:#                  the values of intermediate results.                       #
:#                                                                            #
:#  History                                                                   #
:#   2012-02-14 JFL Created this routine.                                     #
:#   2015-10-18 JFL Bug fix: The output date was incorrect if loop variables  #
:#                  %%a, %%b, or %%c existed already.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Get an ISO 8601 date/time stamp
:now
setlocal EnableExtensions EnableDelayedExpansion
:# First get the short date format from the Control Panel data in the registry
for /f "tokens=3" %%a in ('reg query "HKCU\Control Panel\International" /v sShortDate 2^>NUL ^| findstr "REG_SZ"') do set "SDFTOKS=%%a"
:# Now simplify this (ex: "yyyy/MM/dd") to a "YEAR MONTH DAY" format
for %%a in ("yyyy=y" "yy=y" "y=YEAR" "MMM=M" "MM=M" "M=MONTH" "dd=d" "d=DAY" "/=-" ".=-" "-= ") do set "SDFTOKS=!SDFTOKS:%%~a!"
:# From the actual order, generate the token parsing instructions
set "%%=%%" &:# Define a % variable that will generate a % _after_ the initial %LoopVariable parsing phase
for /f "tokens=1,2,3" %%t in ("!SDFTOKS!") do set "SDFTOKS=set %%t=!%%!a&set %%u=!%%!b&set %%v=!%%!c"
:# Then get the current date and time. (Try minimizing the risk that they get off by 1 day around midnight!)
set "D=%DATE%" & set "T=%TIME%"
:# Remove the day-of-week that appears in some languages (US English, Chinese...)
for /f %%d in ('for %%a in ^(%D%^) do @^(echo %%a ^| findstr /r [0-9]^)') do set "D=%%d"
:# Extract the year/month/day components, using the token indexes set in %SDFTOKS%
for /f "tokens=1,2,3 delims=/-." %%a in ("%D%") do (%SDFTOKS%)
:# Make sure the century is specified, and the month and day have 2 digits.
set "YEAR=20!YEAR!"  & set "YEAR=!YEAR:~-4!"
set "MONTH=0!MONTH!" & set "MONTH=!MONTH:~-2!"
set "DAY=0!DAY!"     & set "DAY=!DAY:~-2!"
:# Remove the leading space that appears for time in some cases. (Spanish...)
set "T=%T: =%"
:# Split seconds and milliseconds
for /f "tokens=1,2 delims=,." %%a in ("%T%") do (set "T=%%a" & set "MS=%%b")
:# Split hours, minutes and seconds. Make sure they all have 2 digits.
for /f "tokens=1,2,3 delims=:" %%a in ("%T%") do (
  set "HOUR=0%%a"   & set "HOUR=!HOUR:~-2!"
  set "MINUTE=0%%b" & set "MINUTE=!MINUTE:~-2!"
  set "SECOND=0%%c" & set "SECOND=!SECOND:~-2!"
  set "MS=!MS!000"  & set "MS=!MS:~0,3!"
)
if .%DEBUG%.==.1. echo set "YEAR=%YEAR%" ^& set "MONTH=%MONTH%" ^& set "DAY=%DAY%" ^& set "HOUR=%HOUR%" ^& set "MINUTE=%MINUTE%" ^& set "SECOND=%SECOND%" ^& set "MS=%MS%"
endlocal & set "YEAR=%YEAR%" & set "MONTH=%MONTH%" & set "DAY=%DAY%" & set "HOUR=%HOUR%" & set "MINUTE=%MINUTE%" & set "SECOND=%SECOND%" & set "MS=%MS%" & goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        UrlEncode                                                 #
:#                                                                            #
:#  Description     Encode reserved characters in a URL                       #
:#                                                                            #
:#  Arguments       %1	    variable name                                     #
:#                                                                            #
:#  Notes 	    Reference for the list of characters to encode:           #
:#		    https://secure.n-able.com/webhelp/nc_9-1-0_so_en/content/sa_docs/api_level_integration/api_integration_urlencoding.html
:#                                                                            #
:#  History                                                                   #
:#   2016-01-12 JFL Created this routine                                      #
:#   2021-12-05 JFL Encode = signs with the pure Batch :ReplaceEquals functn. #
:#   2021-12-06 JFL Encode all characters that need to be encoded.            #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Replace = characters in a string
:# Limitations:
:# - Max 256 = characters
:# - The string must not contain ! or LF characters
:ReplaceEquals %1=STRING_VARNAME %2=REPLACEMENT_VARNAME or "=REPLACEMENT_VALUE"
if not defined %1 exit /b &:# Avoid issues with empty strings
setlocal EnableDelayedExpansion
for /F "delims==" %%v in ('set $_ 2^>NUL') do set "%%v=" &:# Clear existing $_XXX variables
:# $_=input  $f=Termination flag  $v=output value  $r=replacement var
set "$_=!%~1!|" & set "$f=1" & set "$v=" & set "$r=%~2"
if /i "!$_:%$_%=%$_%!" equ "!$_!" endlocal & exit /b 0	&:# No = sign in $_. Return now to save time
if defined $r if not "!$r:~0,1!"=="=" (set "$r=!%~2!") else set "$r=!$r:~1!" &:# $r=replacement value
for /L %%i in (0,1,256) do if defined $f (
  for /F "delims==" %%a in ('set $_') do (
    set "$a=%%a" & set "$b=!%%a!" &:# $a=$_variable name  $b=its value=all that followed the first =
    set "%%a=" & set "$_!$b!" 2>NUL || set "$f="
    if %%i gtr 0 set "$v=!$v!!$a:~2!!$r!"
  )
)
set "$v=!$v!!$b:~0,-1!" &:# The complete result, with the tail | removed in the end
endlocal & set "%~1=%$v%" & exit /b

:# ------------------------------------------------------------------------

:UrlEncode %1=VARNAME
setlocal EnableDelayedExpansion
set "STRING=!%~1:%%=%%25!"
set "STRING=!STRING: =%%20!"
set ^"STRING=!STRING:"=%%22!"
for %%x in ("# 23" "$ 24" "& 26" "+ 2B" ", 2C" "/ 2F" ": 3A"
            "; 3B" "< 3C" "> 3E" "? 3F" "@ 40" "\ 5C" "^ 5E"
           ) do for /f "tokens=1,2" %%a in (%%x) do (
             set "STRING=!STRING:%%a=%%%%b!"
           )
:# Special case for the ! character, which can only be replaced in a set %STRING%
set "STRING=%STRING:!= %" & set "STRING=!STRING: =%%21!" &:# Prerequisite: No more " and ^
:# Special case for the = character, which cannot be replaced by any set command
set "REPL=%%3D" & call :ReplaceEquals STRING REPL &:# Prerequisite: No more !
endlocal & set "%~1=%STRING%" & exit /b

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        GetRegValue                                               #
:#                                                                            #
:#  Description     Get a registry value                                      #
:#                                                                            #
:#  Arguments       KEY [VALUENAME] [RETVAR]                                  #
:#                                                                            #
:#  Notes 	    Default RETVAR output variable name: %VALUENAME%          #
:#                  Default when reading the default unnamed value: VALUE     #
:#                                                                            #
:#                  Also outputs the value type into %RETVAR%.TYPE            #
:#                  Ex: REG_SZ                                                #
:#  History                                                                   #
:#   2016-01-13 JFL Created this routine                                      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:GetRegValue KEY [VALUENAME] [RETVAR] => variables TYPE and VALUE
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "KEY=%~1"
set "NAME=%~2"
set "RETVAR=%~3"
if not defined RETVAR set "RETVAR=%NAME%"
if not defined RETVAR set "RETVAR=VALUE"
%ECHOVARS.XD% KEY NAME RETVAR
if "%NAME%"=="" (
  set CMD=reg query "%KEY%" /ve
) else (
  set CMD=reg query "%KEY%" /v "%NAME%"
)
%ECHOVARS.XD% %CMD%
:# For each line in CMD output...
%FOREACHLINE% %%i in ('%CMD%') do (
  set "LINE=%%i"
  %ECHOVARS.XD% LINE
  :# Values are indented by 4 spaces.
  set "HEAD=!LINE:~0,4!"
  set "LINE=!LINE:~4!"
  :# But extra lines of multi-lined values are indented by >20 spaces.
  set "HEAD2=!LINE:~0,4!"
  if "!HEAD!"=="    " if not "!HEAD2!"=="    " (
    :# Some versions of reg.exe use 4 spaces as field separator; others use a TAB. 
    :# Change the 4-spaces around the REG_XX type word to a TAB.
    set "TOKENS=!LINE:    =	!"
    %ECHOVARS.XD% TOKENS
    :# Extract the value name as the first item before the first TAB.
    :# Names can contain spaces, but assume they don't contain TABs.
    for /f "tokens=1,2* delims=	" %%j in ("!TOKENS!") do (
      set "NAME=%%j"
      set "TYPE=%%k"
      set "VALUE=%%l"
      :# TO DO: Fix multi-line output value, by concatenating values.
      %ECHOVARS.XD% NAME TYPE VALUE
    )
  )
)
%UPVAR% %RETVAR% %RETVAR%.TYPE
set "%RETVAR%=!VALUE!"
set "%RETVAR%.TYPE=!TYPE!"
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        GetIndexedPaths                                           #
:#                                                                            #
:#  Description     Get a list Windows Search indexed paths                   #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2016-01-13 JFL Created this routine                                      #
:#   2021-12-01 JFL Rewrote this routine, making it much faster.              #
:#                                                                            #
:#----------------------------------------------------------------------------#

:GetIndexedPaths [RETVAR]
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "RETVAR=%~1"
if not defined RETVAR set "RETVAR=PATHS"
set "SPACER="
set "KEY=HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Search\Gather\Windows\SystemIndex\Sites\LocalHost\Paths"
%FOREACHLINE% %%l in ('reg query "%KEY%" /v Included /s') do (
  set "LINE=%%l"
  if "!LINE:~0,4!"=="HKEY" (
    set "SUBKEY=%%l"
  ) else (
    for /f "tokens=3" %%v in ("%%l") do set "VALUE=%%v"
    if "!VALUE!"=="0x1" (
      call :GetRegValue "!SUBKEY!" Path P
      set "P=!P:file:///=!"
      if "!P:~1,3!"==":\[" set "P=!P:~0,2!!P:*]=!"
      call :CondQuote P
      set "%RETVAR%=!%RETVAR%! !P!"
    )
  )
)
%UPVAR% %RETVAR%
%RETURN%

:#----------------------------------------------------------------------------#

:# List indexed paths
:ListPaths
echo on
call :GetIndexedPaths 
for %%p in (%PATHS%) do echo %%~p
goto :eof

:#----------------------------------------------------------------------------#

:# Run the search Control Panel
:Configure
setlocal EnableExtensions EnableDelayedExpansion
set "START=start"
%IF_NOEXEC% set "START=echo"
%START% control.exe srchadmin.dll
endlocal
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
echo %SCRIPT% version %VERSION% - Query the Windows Search service
echo.
echo Usage: %SCRIPT% [OPTIONS] QUERY ...
echo.
echo Options:
echo   -?       Display this help and exit
echo   -c       Run the search Control Panel
echo   -i DIR   Search in this directory. Default: %USERPROFILE%
echo   -l       List indexed paths. Searching outside of them will be slow!
echo   -V       Display the script version and exit
echo   -X       Show the command to execute, but don't run it
echo.
echo Search: An ms-search "Advanced Query Syntax" query, as specified in:
echo   https://docs.microsoft.com/en-us/windows/win32/search/-search-3x-wds-qryidx-searchms
echo Ex:
echo   moon             Files containing the string "moon"
echo   moon sun         Files containing the strings "moon" and "sun"
echo   moon AND sun     Files containing the strings "moon" and "sun"
echo   moon OR sun      Files containing the strings "moon" or "sun"
echo   "moon and sun"   Files containing the exact string "moon and sun"
echo   moon NOT sun     Files containing the string "moon", but not "sun"
echo.
goto :eof

:#----------------------------------------------------------------------------#
:# Main routine

:Main
set "SEARCH="		&:# The search query
set "PATHS="		&:# List of paths where to search in

:next_arg
%POPARG%
if "!ARG!"=="" goto :Start
if "!ARG!"=="-?" goto :Help
if "!ARG!"=="/?" goto :Help
if "!ARG!"=="-c" call :Configure & goto :eof
if "!ARG!"=="-d" call :Debug.On & goto next_arg
if "!ARG!"=="-i" %POPARG% & set PATHS=!PATHS! !"ARG"! & goto next_arg
if "!ARG!"=="-l" call :ListPaths & goto :eof
if "!ARG!"=="-v" call :Verbose.On & goto next_arg
if "!ARG!"=="-V" (echo.%VERSION%) & goto :eof
if "!ARG!"=="-X" call :Exec.Off & goto next_arg
if "!ARG:~0,1!"=="-" (
  >&2 %ECHO% Warning: Unexpected option ignored: !ARG!
  goto :next_arg
)
set "SEARCH=!SEARCH! !"ARG"!"
goto :next_arg

:#----------------------------------------------------------------------------#
:# Start the real work

:Start
if not defined SEARCH (
  >&2 echo Error: No search criteria provided
  exit /b 1
)

call :Now
set "NOW=%YEAR%-%MONTH%-%DAY% %HOUR%:%MINUTE%:%SECOND%"

:# Set the default paths, if none specified on the command line
:# if not defined PATHS call :GetIndexedPaths &:# Takes several seconds, and some returned paths cause search failure
if not defined PATHS set "PATHS=%USERPROFILE%"

set "SEARCH=!SEARCH:~1!" &:# Remove the initial space inserted above
set "URL=search-ms:"

set "DISPLAYNAME=!NOW! Search Results" &:# The timestamp makes sure the Explorer window is not reused by subsequent searches
call :UrlEncode DISPLAYNAME
set "URL=!URL!displayname=!DISPLAYNAME!"

:#      crumb=filename:!SEARCH! OR System.Generic.String:!SEARCH!
:# <==> query=!SEARCH!
%IF_DEBUG% set SEARCH
call :UrlEncode SEARCH
set "URL=!URL!&query=!SEARCH!"
%IF_DEBUG% set URL

for %%n in (%PATHS%) do (
  set "LOCATION=%%~n"
  call :UrlEncode LOCATION
  set "URL=!URL!&crumb=location:!LOCATION!"
)
%IF_DEBUG% set URL
:# Explorer command line: https://support.microsoft.com/en-us/kb/152457
%EXEC% %windir%\explorer.exe /root,"!URL:%%=%%%%!"
