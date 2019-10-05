@echo off
:##############################################################################
:#                                                                            #
:#  Filename        library.cmd                                               #
:#                                                                            #
:#  Description     A library of useful batch routines for Windows NT cmd.exe #
:#                                                                            #
:#  Notes 	    Use this file as a template for new batch files:          #
:#                  Copy this whole file into the new batch file.             #
:#                  Remove all unused code (possibly everything) from the end #
:#                  of the debugging library to the header of the main routine#
:#                  (Always keep the whole debugging library at the beginning,#
:#                  even if you have no immediate need for it. The first time #
:#		    you'll have a bug, it'll be priceless!)                   # 
:#                  Update the header and the main routine.                   #
:#                                                                            #
:#                  Microsoft reference page on batch files:		      #
:#		    http://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/batch.mspx
:#                  Excellent sources of tips and examples:                   #
:#		    http://www.dostips.com/				      #
:#                  http://www.robvanderwoude.com/batchfiles.php              #
:#                                                                            #
:#                  Reserved characters that affect batch files:              #
:#                  Command sequencing (escaped by ^ and ""): & | ( ) < > ^   #
:#                  Echo control (escaped by enclosing command in ""): @      #
:#                  Argument delim. (escaped by enclosing in ""): , ; = blanks#
:#		    Environment variables (escaped by %): %                   #
:#		    Delayed variables (escaped by ^): !			      #
:#                  Wildcards: * ?                                            #
:#                  Some internal commands also use: [ ] { } = ' + ` ~        #
:#                                                                            #
:#                  Filenames cannot contain characters: \ / : * ? " < > |    #
:#                  But they can contain: & ( ) ^ @ , ; % ! [ ] { } = ' + ` ~ #
:#                  Conclusion: Always put "quotes" around file names.        #
:#                  Warning: Even "quotes" do not resolve issues with: ^ % !  #
:#                  Files containing these 3 characters will not be processed #
:#                  correctly, except in a for loop with delayed expansion off.
:#                                                                            #
:#                  When cmd parses a line, it does the following steps:      #
:#                  1) Replace %N arguments.                                  #
:#                  2) Replace %VARIABLES%.                                   #
:#		    3) Tokenization. Separate command sequencing tokens,      #
:#			using "" and ^ as escape characters. (See above)      #
:#		    4) Replace for %%V variables                              #
:#		    5) Replace !VARIABLES!.                                   #
:#			If any ! is present in the command, remove another    #
:#			set of ^ escape characters.                           #
:#		    6) If tokenization finds a pipe, and if one of the        #
:#			commands is an internal command (ex: echo), a         #
:#			subshell is executed and passed the processed tokens. #
:#			This subshell repeats steps 1 to 4 or 5 while parsing #
:#			its own arguments. (Depending on /V:OFF or /V:ON)     #
:#			Note: Most internals command, like cd or set, have no #
:#			effect in this case on the original shell.            #
:#                  For a complete description of the cmd line parser, see:   #
:#                  https://stackoverflow.com/a/4095133		              #
:#                                                                            #
:#                  Steps 4 & 5 are not done for the call command.            #
:#                  Step 3 is done, but the redirections are ignored.         #
:#                                                                            #
:#                  The following four instructions are equivalent:           #
:#                  echo !%VAR_NAME%!                                         #
:#                  call echo %%%VAR_NAME%%%                                  #
:#                  echo %%%VAR_NAME%%% | more                                #
:#                  cmd /c echo %%%VAR_NAME%%%                                #
:#                                                                            #
:#                  During the tokenization step, the analyser switches       #
:#                  between a normal mode and a string mode after every ".    #
:#                  In normal mode, the command sequencing characters (see    #
:#                  above) may be escaped using the ^ character.              #
:#                  In string mode, they are stored in the string token       #
:#                  without escaping. The ^ itself is stored without escaping.#
:#                  The " character itself can be escaped, to avoid switching #
:#                  mode. Ex:                                                 #
:#                  echo "^^"   outputs "^^"  ;  set "A=^^"   stores ^^ in A  #
:#                  echo ^"^^"  outputs "^"   ;  set ^"A=^^"  stores ^  in A  #
:#                                                                            #
:#                  Good practice:                                            #
:#                  * Use :# for comments instead of rem.                     #
:#                    + The # sign is the standard comment marker for most    #
:#                       other scripting languages.                           #
:#                    + This stands out better than the :: used by many.      #
:#                    + This avoids echoing the comment in echo on mode.      #
:#                    + Gotcha: A :# comment cannot be at the last line of    #
:#                       a ( block of code ). Use (rem :# comment) instead.   #
:#                  * Always enquote args sent, and dequote args received.    #
:#                    + Best strategy for preserving reserved chars across.   #
:#                  * Always enclose the set command in quotes: set "VAR=val" #
:#                    + Best strategy for preserving reserved chars in val.   #
:#                  * Always use echo.%STRING% instead of echo %STRING%       #
:#                    + This will work even for empty strings.                #
:#                  * Do not worry about strings with unbalanced quotes.      #
:#                    + File names cannot contain quotes.                     #
:#                    + This is not a general purpose language anyway.        #
:#                  * Do worry about arguments with unbalanced quotes.        #
:#                    + The last argument can contain unbalanced quotes.      #
:#                  * Always surround routines by init call and protection    #
:#                    jump. This allows using it by just cutting and pasting  #
:#                    it. Example:                                            #
:#                      call :MyFunc.Init                                     #
:#                      goto :MyFunc.End                                      #
:#                      :MyFunc.Init                                          #
:#                      ...                                                   #
:#                      goto :eof                                             #
:#                      :MyFunc                                               #
:#                      ...                                                   #
:#                      goto :eof                                             #
:#                      :MyFunc.End                                           #
:#                                                                            #
:#                  Gotcha:                                                   #
:#                  * It is not possible a call a subroutine from inside a    #
:#                    for /f ('command pipeline'). This is because this       #
:#                     command pipeline is executed in a sub-shell, and has   #
:#                     no access to the rest of the batch file.               #
:#                                                                            #
:#  Author          Jean-François Larvoire, jf.larvoire@hpe.com               #
:#                                                                            #
:#  History                                                                   #
:#   2012-07-10 JFL Updated the debugging framework.                          #
:#                  Added routine get_IP_address.                             #
:#                  Added a factorial routine, to test the tracing framework  #
:#   2012-07-11 JFL Added options -c and -C to respectively test the command  #
:#                  tail as one, or as N separate, commands.                  #
:#   2012-07-19 JFL Added debug optimizations.				      #
:#   2012-10-02 JFL Added options -a and -b.                                  #
:#                  Added the Echo.Color functions.                           #
:#   2013-12-05 JFL Improved the :Exec routine.                               #
:#                  Added :Firewall.GetRules                                  #
:#   2014-05-13 JFL Added EnableExpansion and EnableExpansion.Test routines.  #
:#                  Fixed the self-test mode.				      #
:#   2014-09-30 JFL Added macro system from dostips.com forum topics 5374,5411.
:#                  Added tee routine from dostips.com forum topic #32615.    #
:#   2014-11-19 JFL Added routine PopArg, and use it in the main routine.     #
:#   2015-03-02 JFL Added routine GetServerAddress.			      #
:#   2015-03-18 JFL Rewrote PopArg, which did not process quotes properly.    #
:#   2015-04-16 JFL Added my own version of macro management macros, working  #
:#                  with DelayedExpansion enabled.                            #
:#   2015-10-18 JFL Bug fix: Function :now output date was incorrect if loop  #
:#                  variables %%a, %%b, or %%c existed already.               #
:#                  Renamed macros SET_ERR_0,SET_ERR_1 as TRUE.EXE,FALSE.EXE. #
:#   2015-10-29 JFL Added macro %RETURN#% to return with a comment.           #
:#   2015-11-19 JFL %FUNCTION% now automatically generates its name & %* args.#
:#                  Removed args for all %FUNCTION% invokations.              #
:#                  Added an %UPVAR% macro allowing to define the list of     #
:#                  variables that need to make it back to the caller.        #
:#                  Updated all functions that return such variables.         #
:#   2015-11-23 JFL Added routines :extensions.* to detect the extension      #
:#                  and expansion modes, and option -qe to display it.        #
:#   2015-11-25 JFL Changed the default for %LOGFILE% from NUL to undefined.  #
:#                  Rewrote the %FUNCTION% and %RETURN% macros to manage      #
:#                  most common cases without calling a subroutine.           #
:#   2015-11-27 JFL Added a macro debugging capability.                       #
:#                  Redesigned the problematic character return mechanism     #
:#                  using a table of predefined generic entities. Includes    #
:#                  support for returning strings with CR & LF. Also use this #
:#                  to expand input entities in -a, -b, and -c commands.      #
:#   2015-11-29 JFL Made the RETURN macro better and simpler.                 #
:#                  Added a backspace entity.                                 #
:#   2015-12-01 JFL Rewrote :extensions.get and :extensions.show.             #
:#                  Fixed a bug in the %FUNCTION% macro.                      #
:#   2016-09-01 JFL Bug fix: %RETURN% incorrectly returned empty variables.   #
:#                  Added registry access routines.			      #
:#   2016-10-19 JFL Bug fix: Routine :Exec now preserves initial errorlevel.  #
:#   2016-11-02 JFL Bug fix: Avoid log file redirection failures in recursive #
:#                  scripts.                                                  #
:#   2016-11-05 JFL Fixed :Exec bug in XP/64.				      #
:#                  Indent sub-scripts output in debug mode.                  #
:#   2016-11-06 JFL Updated the 10/19 errorlevel fix to work for DO and EXEC. #
:#   2016-11-09 JFL Bug fix: %RETURN% failed if an UPVAR contained a '?'.     #
:#                  Added routine :lsort.                                     #
:#                  Fixed :condquote, which was severely broken :-(	      #
:#   2016-11-10 JFL Added another implementation for routine :now.            #
:#   2016-11-11 JFL Simplified routine :strlen, now 5% faster.                #
:#                  Added routine :strlen.q. No tracing but twice as fast.    #
:#   2016-11-12 JFL Added routines :ReplaceChars, :ReplaceDelims, etc.        #
:#   2016-11-13 JFL Bug fix: Correctly return special characters & | < > ? *  #
:#   2016-11-17 JFL Fixed tracing %EXEC% command exit code when exp. disabled.#
:#		    Several other :Exec bug fixes and perf. improvements.     #
:#   2016-11-21 JFL Fixed ! quoting and added |&<> quoting to :CondQuote.     #
:#   2016-11-24 JFL Added %XCALL% for calling functions in a 2nd script inst. #
:#                  Fixed %EXEC% with commands containing a ^ character.      #
:#                  Fixed tracing %FUNCTION% arguments with ^ and % chars.    #
:#                  Updated %POPARG% to work with trick characters ! and ^ in #
:#                  delayed expansion mode. The old and faster version is now #
:#		    called %POPSARG%.                                         #
:#		    Moved character entity definitions from the :Debug section#
:#		    to the :PopArg section, where they're now needed first.   #
:#		    Added routine :Prep2ExpandVars allowing to pass any       #
:#		    tricky string across call or endlocal barriers.           #
:#   2016-12-07 JFL Added a source command for using debug functions in this  #
:#		    library from an outside script.                           #
:#   2016-12-08 JFL Moved the most common initializations into :Library.Init. #
:#   2016-12-12 JFL Fixed function call/return logging when sourcing this lib.#
:#   2016-12-14 JFL Fixed macros RETURN0 and RETURN#.                         #
:#   2016-12-16 JFL Changed %EXEC% to not capture commands output by default. #
:#   2017-01-13 JFL Added option -f to routine :Exec.                         #
:#   2017-01-16 JFL Use bright colors for [Success]/[Warning]/[Failure] in    #
:#                  :Echo.Color, and added an optional suffix and end of line.#
:#   2017-01-17 JFL Renamed %ARG0% as %SFULL%, and added a new %ARG0% with    #
:#		    the actual invokation pathname, possibly relative.        #
:#   2017-05-11 JFL Removed rscp which was trivial and useless.               #
:#                  Changed chcp into GetCP, returning the current code page. #
:#                  Added GetACP, GetOEMCP.				      #
:#   2018-03-01 JFL New faster version of the FALSE.EXE macro.		      #
:#		    Simpler and faster versions of function is_dir.	      #
:#		    Added functions dirname, filename, has_wildcards.	      #
:#   2019-10-02 JFL Disable delayed expansion when capturing cmd-line args.   #
:#   2019-10-03 JFL Added macros ECHOSVARS, ECHOSVARS.V, ECHOSVARS.D.         #
:#		    Fixed the passing of ^ ! arguments in options -c, -C, -M. #
:#   2019-10-04 JFL Added routines :EscapeCmdString & :TestEscapeCmdString.   #
:#		    Rewrote routine :convert_entities as :ConvertEntities.    #
:#   2019-10-05 JFL Finalized :TestEscapeCmdString & :TestConvertEntities.    #
:#		                                                              #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

:# Check Windows version: minimum requirement Windows
:# 2000, but useful only for Windows XP SP2 and later
if not "%OS%"=="Windows_NT"     goto Err9X
ver | find "Windows NT" >NUL && goto ErrNT

:# Mechanism for calling subroutines in this library, either locally or from another script.
:# Called by (%LCALL% :label [arguments]), with LCALL defined in the Call module below.
if '%1'=='call' %*& exit /b

setlocal EnableExtensions DisableDelayedExpansion &:# Make sure ! characters are preserved
set "VERSION=2019-10-05"
set "SCRIPT=%~nx0"		&:# Script name
set "SNAME=%~n0"		&:# Script name, without its extension
set "SPATH=%~dp0"		&:# Script path
set "SPATH=%SPATH:~0,-1%"	&:# Script path, without the trailing \
set "SFULL=%~f0"		&:# Script full pathname
set ^"ARG0=%0^"			&:# Script invokation name
set ^"ARGS=%*^"			&:# Argument line
setlocal EnableExtensions EnableDelayedExpansion &:# Use the ! expansion now on

:# Mechanism for calling subroutines in a second instance of a script, from its main instance.
:# Done by (%XCALL% :label [arguments]), with XCALL defined in the Call module below.
if '%1'=='-call' !ARGS:~1!& exit /b

:# Mechanism for "sourcing" this library from another script.
if '%1'=='source' (
  endlocal &:# Define everything in the context of the caller script
  if not "!!"=="" >&2 echo "%SFULL%" %1 Error: Must be called with DelayedExpansion ENABLED. & exit /b 1
  set ^"LCALL=call "%SFULL%" call^"	&rem :# This is the full path of this library's ARG0
) &:# Now initialize the library modules, then return to the parent script.

:# Initialize the most commonly used library components.
call :Library.Init

if '%1'=='source' exit /b 0	&:# If we're sourcing this lib, we're done. 

:# Go process command-line arguments
goto Main

:Err9X
echo Error: Does not work on Windows 9x
goto eof

:ErrNT
>&2 echo Error: Works only on Windows 2000 and later
goto :eof

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
:#                  %ECHOSVARS%	    Echo ARG1 before each variable.           #
:#                  %ECHOSVARS.V%   Idem, but display them in verb. mode only #
:#                  %ECHOSVARS.D%   Idem, but display them in debug mode only #
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
set "ECHOSVARS.V=%LCALL% :EchoStringVars.Verbose"
set "ECHOSVARS.D=%LCALL% :EchoStringVars.Debug"
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
:#  Variables containing control characters
:#----------------------------------------------------------------------------#

:# Define a CR variable containing a Carriage Return ('\x0D')
for /f %%a in ('copy /Z "%~dpf0" nul') do set "CR=%%a"

:# Define a LF variable containing a Line Feed ('\x0A')
:# The two blank lines below are necessary.
set LF=^


:# End of define Line Feed. The two blank lines above are necessary.

:# Define a BS variable containing a BackSpace ('\x08')
:# Use prompt to store a  backspace+space+backspace into a DEL variable.
for /F "tokens=1 delims=#" %%a in ('"prompt #$H# & echo on & for %%b in (1) do rem"') do set "DEL=%%a"
:# Then extract the first backspace
set "BS=%DEL:~0,1%"

:# Define a FF variable containing a Form Feed ('\x0C')
for /f %%A in ('cls') do set "FF=%%A"

:#----------------------------------------------------------------------------#
:#  Useful tricks
:#----------------------------------------------------------------------------#

:# The following commands end up with a loop that works identically both
:# inside a batch file, and at the cmd.exe prompt.
set "@=%"	&:# Ends up as % at cmd prompt, or undefined in a batch
if not defined @ set "@=%%"	&:# If undefined (in a batch) redefine as %
for /l %@%N in (1,1,3) do @echo Loop %@%N   &:# Display the loop number

:#----------------------------------------------------------------------------#
:#  Test macros
:#----------------------------------------------------------------------------#

:Macro.test
:# Sample macro showing macro features, and how to use them
set $reflect=%MACRO% ( %\n%
  echo $reflect %!%MACRO.ARGS%!% %\n%
  :# Make sure ARG is undefined if there's no argument %\n%
  set "ARG=" %\n%
  :# Scan all arguments, and show what they are %\n%
  for %%v in (%!%MACRO.ARGS%!%) do ( %\n%
    set "ARG=%%~v" %\n%
    echo Hello %!%ARG%!% %\n%
  ) %\n%
  :# Change another variable, to show the change is local only %\n%
  set "LOCALVAR=CHANGED" %\n%
  :# Return the last argument %\n%
  set RETVAL=%!%ARG%!%%\n%
  :# Work around the inability to use either %RETVAL% or !RETVAL! in macros after endlocal %\n%
  echo return "RETVAL=%'!%RETVAL%'!%"%\n%
  %ON_MACRO_EXIT% set "RETVAL=%'!%RETVAL%'!%" %/ON_MACRO_EXIT% %\n%
) %/MACRO%

set $reflect

echo.
set LOCALVAR=BEFORE
echo set "RETVAL=%RETVAL%"
echo set "LOCALVAR=%LOCALVAR%"
%$reflect% inline functions
echo set "RETVAL=%RETVAL%"
echo set "LOCALVAR=%LOCALVAR%"

echo.
%$reflect%
echo set "RETVAL=%RETVAL%"
echo set "LOCALVAR=%LOCALVAR%"

echo.
%$reflect% more "inline functions"
echo set "RETVAL=%RETVAL%"
echo set "LOCALVAR=%LOCALVAR%"
goto :eof

:Return#.Test1
%FUNCTION0%
if %1==0 %TRUE.EXE% & %RETURN#% ErrorLevel Zero
%FALSE.EXE% & %RETURN#% ErrorLevel One

:Return#.Test
call :Return#.Test1 0
%ECHO% :# ERRORLEVEL=%ERRORLEVEL% Expected 0
call :Return#.Test1 1
%ECHO% :# ERRORLEVEL=%ERRORLEVEL% Expected 1
call :Return#.Test1 0
%ECHO% :# ERRORLEVEL=%ERRORLEVEL% Expected 0
goto :eof

:#----------------------------------------------------------------------------#
:# From: http://www.dostips.com/forum/viewtopic.php?f=3&t=5411
:# This is posted mostly for (my) reference, since I don't always remember,
:# and never seem to be able to find everything in one place.
:# Below is a set of LF-related macros, with !LF! and %\n% following common
:# usage around here, %/n% being proposed for a linefeed without continuation,
:# and multi-slashed %\\n% %//n% indicating the target depth of expansion.
:# Code:
@echo off & setlocal disableDelayedExpansion

@rem single linefeed char 0x0A (two blank lines required below)
set LF=^


@rem linefeed macros
set ^"/n=^^^%LF%%LF%^%LF%%LF%"
set ^"//n=^^^^^^%/n%%/n%^^%/n%%/n%"
set ^"///n=^^^^^^^^^^^^%//n%%//n%^^^^%//n%%//n%"
set ^"////n=^^^^^^^^^^^^^^^^^^^^^^^^%///n%%///n%^^^^^^^^%///n%%///n%"
:: set ^"//n=^^^^^^^%LF%%LF%^%LF%%LF%^^^%LF%%LF%^%LF%%LF%"

@rem newline macros (linefeed + line continuation)
set ^"\n=%//n%^^"
set ^"\\n=%///n%^^"
set ^"\\\n=%////n%^^"

setlocal enableDelayedExpansion

@rem check inline expansion
echo(
set ^"NL=^%LF%%LF%"
if '!LF!'=='!NL!' echo '^^!LF^^!'    == '^^^^%%LF%%%%LF%%'

@rem check linefeed macros
echo(
set "ddx=!/n!" & set "edx=!LF!"
call :check && (echo '%%/n%%'    == '^^!LF^^!') || (echo ???)
set "ddx=!//n!" & set "edx=!/n!"
call :check && (echo '%%//n%%'   == '^^!/n^^!') || (echo ???)
set "ddx=!///n!" & set "edx=!//n!"
call :check && (echo '%%///n%%'  == '^^!//n^^!') || (echo ???)
set "ddx=!////n!" & set "edx=!///n!"
call :check && (echo '%%////n%%' == '^^!///n^^!') || (echo ???)

@rem check newline macros
echo(
set "ddx=!\n!" & set "edx=!LF!^"
call :check && (echo '%%\n%%'    == '^^!LF^^!^^^^') || (echo ???)
set "ddx=!\\n!" & set "edx=!/n!^"
call :check && (echo '%%\\n%%'   == '^^!/n^^!^^^^') || (echo ???)
set "ddx=!\\\n!" & set "edx=!//n!^"
call :check && (echo '%%\\\n%%'  == '^^!//n^^!^^^^') || (echo ???)

endlocal & endlocal & goto :eof

:check
set ^"dvar='%ddx%'"
set ^"evar='!edx!'"
if "!dvar!" equ "!evar!" (call;) else (call) & goto :eof

:#----------------------------------------------------------------------------#
:#batchTee.bat  OutputFile  [+]
:#
:#  Write each line of stdin to both stdout and outputFile.
:#  The default behavior is to overwrite any existing outputFile.
:#  If the 2nd argument is + then the content is appended to any existing
:#  outputFile.
:#
:#  Limitations:
:#
:#  1) Lines are limited to ~1000 bytes. The exact maximum line length varies
:#     depending on the line number. The SET /P command is limited to reading
:#     1021 bytes per line, and each line is prefixed with the line number when
:#     it is read.
:#
:#  2) Trailing control characters are stripped from each line.
:#
:#  3) Lines will not appear on the console until a newline is issued, or
:#     when the input is exhaused. This can be a problem if the left side of
:#     the pipe issues a prompt and then waits for user input on the same line.
:#     The prompt will not appear until after the input is provided.
:#
:# From http://www.dostips.com/forum/viewtopic.php?p=32615#p32615
:#----------------------------------------------------------------------------#

@echo off
setlocal enableDelayedExpansion
if "%~1" equ ":tee" goto :tee

:lock
set "teeTemp=%temp%\tee%time::=_%"
2>nul (
  9>"%teeTemp%.lock" (
    for %%F in ("%teeTemp%.test") do (
      set "yes="
      pushd "%temp%"
      copy /y nul "%%~nxF" >nul
      for /f "tokens=2 delims=(/" %%A in (
        '^<nul copy /-y nul "%%~nxF"'
      ) do if not defined yes set "yes=%%A"
      popd
    )
    for /f %%A in ("!yes!") do (
        find /n /v ""
         echo :END
         echo %%A
      ) >"%teeTemp%.tmp" | <"%teeTemp%.tmp" "%~f0" :tee %* 7>&1 >nul
    (call )
  ) || goto :lock
)
del "%teeTemp%.lock" "%teeTemp%.tmp" "%teeTemp%.test"
exit /b

:tee
set "redirect=>"
if "%~3" equ "+" set "redirect=>>"
8%redirect% %2 (call :tee2)
set "redirect="
(echo ERROR: %~nx0 unable to open %2)>&7

:tee2
for /l %%. in () do (
  set "ln="
  set /p "ln="
  if defined ln (
    if "!ln:~0,4!" equ ":END" exit
    set "ln=!ln:*]=!"
    (echo(!ln!)>&7
    if defined redirect (echo(!ln!)>&8
  )
)

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

:# Test proving that the :EnableExpansion routine does not have lasting effects.
:EnableExpansion.Test
setlocal EnableExtensions DisableDelayedExpansion
echo :# First attempt with variable expansion disabled
set "X=Initial value"
set "X=Modified value" & echo X=!X!
call :EnableExpansion
echo :# Second attempt after call :EnableExpansion
set "X=Initial value"
set "X=Modified value" & echo X=!X!
endlocal
setlocal EnableExtensions EnableDelayedExpansion
echo :# Third attempt after setlocal EnableDelayedExpansion
set "X=Initial value"
set "X=Modified value" & echo X=!X!
endlocal
goto :eof

:#----------------------------------------------------------------------------#

:# Check that cmd extensions work
:check_exts
verify other 2>nul
setlocal EnableExtensions EnableDelayedExpansion
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
:#                                                                            #
:#  Function        Echo-n                                                    #
:#                                                                            #
:#  Description     Output a string with no newline                           #
:#                                                                            #
:#  Macros          %ECHO-N%    Output a string with no newline.              #
:#                                                                            #
:#  Arguments       %1          String to output.                             #
:#                                                                            #
:#  Notes           Quotes around the string, if any, will be removed.        #
:#                  Leading spaces will NOT be output. (Limitation of set /P) #
:#                                                                            #
:#  History                                                                   #
:#   2010-05-19 JFL Created this routine.                                     #
:#   2012-07-09 JFL Send the output to the log file too.                      #
:#                                                                            #
:#----------------------------------------------------------------------------#

set "ECHO-N=call :Echo-n"
goto :Echo-n.End

:Echo-n
setlocal
if defined LOGFILE %>>LOGFILE% <NUL set /P =%~1
                               <NUL set /P =%~1
endlocal
goto :eof

:Echo-n.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Echo.Color						      #
:#                                                                            #
:#  Description     Echo colored strings                                      #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    Based on the colorPrint sample code in                    #
:#                  http://stackoverflow.com/questions/4339649/how-to-have-multiple-colors-in-a-batch-file
:#                                                                            #
:#                  Requires ending the script with a last line containing    #
:#                  a single dash "-" and no CRLF in the end.                 #
:#                                                                            #
:#                  Known limitations:                                        #
:#                  Backspaces do not work across a line break, so the        #
:#                  technique can have problems if the line wraps.            #
:#                  For example, printing a string with length between 74-79  #
:#                  will not work properly in a 80-columns console.           #
:#                                                                            #
:#  History                                                                   #
:#   2011-03-17 JEB Published the first sample on stackoverflow.com           #
:#   2012-04-30 JEB Added support for strings containing invalid file name    #
:#                  characters, by using the \..\x suffix.                    #
:#   2012-05-02 DB  Added support for strings that contain additional path    #
:#                  levels, like: "a\b\" "a/b/" "\" "/" "." ".." "c:"         #
:#                  Store the temp file on %TEMP%, which is always writable.  #
:#                  Created 2 variants, one takes a string literal, the other #
:#                  the name of a variable containing the string. The variable#
:#                  version is generally less convenient, but it eliminates   #
:#                  some special character escape issues.                     #
:#                  Added the /n option as an optional 3rd parameter to       #
:#                  append a newline at the end of the output.                #
:#   2012-09-26 JFL Renamed routines as object-oriented Echo.Methods.         #
:#                  Added routines Echo.Success, Echo.Warning, Echo.Failure.  #
:#   2012-10-02 JFL Renamed variable DEL as ECHO.DEL to avoid name collisions.#
:#                  Removed the . in the temp file. findstr can search a BS.  #
:#                  Removed a call level to improve performance a bit.        #
:#                  Added comments.                                           #
:#                  New implementation not using a temporary file.            #
:#   2012-10-06 JFL Fixed the problem with displaying "!".                    #
:#   2012-11-13 JFL Copy the string into the log file, if defined.            #
:#   2017-01-16 JFL Use bright colors for [Success]/[Warning]/[Failure],      #
:#                  and added an optional suffix and end of line.             #
:#                                                                            #
:#----------------------------------------------------------------------------#
 
call :Echo.Color.Init
goto Echo.Color.End

:Echo.Color %1=Color %2=Str [%3=/n]
:# Temporarily disable expansion to preserve ! in the input string
setlocal disableDelayedExpansion
set "str=%~2"
:Echo.Color.2
setlocal enableDelayedExpansion
if defined LOGFILE %>>LOGFILE% <NUL set /P =!str!
:# Replace path separators in the string, so that the final path still refers to the current path.
set "str=a%ECHO.DEL%!str:\=a%ECHO.DEL%\..\%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%!"
set "str=!str:/=a%ECHO.DEL%/..\%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%!"
set "str=!str:"=\"!"
:# Go to the script directory and search for the trailing -
pushd "%ECHO.DIR%"
findstr /p /r /a:%~1 "^^-" "!str!\..\!ECHO.FILE!" nul
popd
:# Remove the name of this script from the output. (Dependant on its length.)
for /l %%n in (1,1,24) do if not "!ECHO.FILE:~%%n!"=="" <nul set /p "=%ECHO.DEL%"
:# Remove the other unwanted characters "\..\: -"
<nul set /p "=%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%"
:# Append the optional CRLF
if not "%~3"=="" echo.&if defined LOGFILE %>>LOGFILE% echo.
endlocal & endlocal & goto :eof

:Echo.Color.Var %1=Color %2=StrVar [%3=/n]
if not defined %~2 goto :eof
setlocal enableDelayedExpansion
set "str=!%~2!"
goto :Echo.Color.2

:Echo.Color.Init
set "ECHO.COLOR=call :Echo.Color"
set "ECHO.DIR=%~dp0"
set "ECHO.FILE=%~nx0"
:# Use prompt to store a backspace into a variable. (Actually backspace+space+backspace)
for /F "tokens=1 delims=#" %%a in ('"prompt #$H# & echo on & for %%b in (1) do rem"') do set "ECHO.DEL=%%a"
goto :eof

:#----------------------------------------------------------------------------#

:Echo.Color.Test
setlocal disableDelayedExpansion
%ECHO.COLOR% 0a "a"
%ECHO.COLOR% 0b "b"
set "txt=^" & %ECHO.COLOR%.Var 0c txt
%ECHO.COLOR% 0d "<"
%ECHO.COLOR% 0e ">"
%ECHO.COLOR% 0f "&"
%ECHO.COLOR% 1a "|"
%ECHO.COLOR% 1b " "
%ECHO.COLOR% 1c "%%%%" & rem # Escape the '%' character
%ECHO.COLOR% 1d ^""" & rem # Escape the '"' character
%ECHO.COLOR% 1e "*"
%ECHO.COLOR% 1f "?"
%ECHO.COLOR% 2a "!" & rem # This one does not need escaping in disableDelayedExpansion mode
%ECHO.COLOR% 2b "."
%ECHO.COLOR% 2c ".."
%ECHO.COLOR% 2d "/"
%ECHO.COLOR% 2e "\"
%ECHO.COLOR% 2f "q:" /n
echo.
set complex="c:\hello world!/.\..\\a//^<%%>&|!" /^^^<%%^>^&^|!\
%ECHO.COLOR%.Var 74 complex /n
goto :eof

:# Experimental code that does not work...
:# Check if this script contains a trailing :eof. If not, add one.
set "ECHO.FULL=%ECHO.DIR%%ECHO.FILE%"
findstr /r "^^-" "%ECHO.FULL%" >NUL 2>&1
if errorlevel 1 (
  >&2 echo Notice: Adding the missing - at the end of this script
  >>"%ECHO.FULL%" echo goto :eof
  >>"%ECHO.FULL%" <nul set /p "=-"
) else (
echo three
  for /f "delims=" %%s in ('findstr /r "^-" "%ECHO.FULL%"') do set "ECHO.TMP=%%s"
  %ECHOVARS% ECHO.TMP
)
  if not "%ECHO.TMP:~1%"=="" >&2 echo Error: Please remove all CRLF after the trailing -

:#----------------------------------------------------------------------------#

:# Initial implementation with a temporary file %TEMP%\x containing backspaces

:Echo.Color1 %1=Color %2=Str [%3=/n]
setlocal enableDelayedExpansion
set "str=%~2"
set "strvar=str"
goto :Echo.Color1.2

:Echo.Color1.Var %1=Color %2=StrVar [%3=/n]
if not defined %~2 goto :eof
setlocal enableDelayedExpansion
set "strvar=%~2"
:Echo.Color1.2
:# Replace path separators in the string, so that they still refer to the current path.
set "str=a%ECHO.DEL%!%strvar%:\=a%ECHO.DEL%\..\%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%!"
set "str=!str:/=a%ECHO.DEL%/..\%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%!"
set "str=!str:"=\"!"
pushd "%TEMP%"
findstr /P /L /A:%~1 "%ECHO.BS%" "!str!\..\x" nul
popd
if not "%~3"=="" echo.
endlocal & goto :eof

:Echo.Color1.Init
set "ECHO.COLOR=call :Echo.Color"
:# Use prompt to store a backspace into a variable. (Actually backspace+space+backspace)
for /F "tokens=1 delims=#" %%a in ('"prompt #$H# & echo on & for %%b in (1) do rem"') do set "ECHO.DEL=%%a"
set "ECHO.BS=%ECHO.DEL:~0,1%"
:# Generate a temp file containing backspaces. This content will be used later to post-process the findstr output.
<nul >"%TEMP%\x" set /p "=%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%%ECHO.DEL%"
goto :eof

:Echo.Color1.Cleanup
del "%TEMP%\x"
goto :eof

:#----------------------------------------------------------------------------#

:Echo.Success	[%1=Suffix string] [%2=/n]
%ECHO.COLOR% 0A "[Success]%~1" %2
goto :eof

:Echo.Warning	[%1=Suffix string] [%2=/n]
%ECHO.COLOR% 0E "[Warning]%~1" %2
goto :eof

:Echo.Failure	[%1=Suffix string] [%2=/n]
%ECHO.COLOR% 0C "[Failure]%~1" %2
goto :eof

:Echo.Color.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        EchoF						      #
:#                                                                            #
:#  Description     Echo a string with several formatted fields               #
:#                                                                            #
:#  Arguments       %1	    Format string with columns width and alignment    #
:#                          Ex: "[-10][10][10]"				      #
:#                  %*	    Optional substrings to format                     #
:#                                                                            #
:#  Notes 	    Inspired from C printf routine.                           #
:#                                                                            #
:#                  Prefix all local variable names with a character that     #
:#                  cannot be used in %%N loop variables. The ";" here.       #
:#                  This avoids a bug when this routine is invoked from       #
:#                  within a loop, where that loop variable uses the first    #
:#                  letter of one of our variables.                           #
:#                                                                            #
:#                  Limitations                                               #
:#                  - Can format at most 8 strings.                           #
:#                  - The format string cannot contain | < > &                #
:#                  - Each formatted field can be at most 53 characters long. #
:#                                                                            #
:#  History                                                                   #
:#   2006-01-01     Created Format on http://www.dostips.com                  #
:#   2009-11-30     Changed                                                   #
:#   2012-10-25 JFL Adapted to my usual style and renamed as EchoF.           #
:#                  Fixed bug when invoked in a loop on %%c or %%l, etc...    #
:#                  Added the option to have an unspecified width: []         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:EchoF.Init
set "ECHOF=call :EchoF"
goto :eof

:EchoF fmt str1 str2 ... -- outputs columns of strings right or left aligned
::                       -- fmt [in] - format string specifying column width and alignment. Ex: "[-10] / [10] / []"
:$created 20060101 :$changed 20091130 :$categories Echo
:# Updated 20121026 JFL: Added tons of comments.
:#                       Fixed a bug if invoked in a loop on %%c or %%f or %%l or %%s or %%i.
:#                       Added an unspecified width format []. Useful for the last string.
:$source http://www.dostips.com
setlocal EnableExtensions DisableDelayedExpansion
set ";fmt=%~1" &:# Format string
set ";line="   &:# Output string. Initially empty.
set ";spac=                                                     "
set ";i=1"     &:# Argument index. 1=fmt; 2=str1; ...
:# %ECHOVARS.D% ";fmt" ";line" ";spac" ";i"
:# For each substring in fmt split at each "]"... (So looking like "Fixed text[SIZE]".) 
for /f "tokens=1,2 delims=[" %%a in ('"echo..%;fmt:]=&echo..%"') do ( :# %%a=Fixed text before "["; %%b=size after "["
  call set /a ";i=%%;i%%+1"                            &:# Compute the next str argument index.
  call call set ";subst=%%%%~%%;i%%%;spac%%%%%~%%;i%%" &:# Append that str at both ends of the spacer
:# %ECHOVARS.D% ";i" ";subst"
  if "%%b"=="" (         :# Unspecified width. Use the string as it is.
    call call set ";subst=%%%%~%%;i%%" 
  ) else if %%b0 GEQ 0 ( :# Cut a left-aligned field at the requested size.
    call set ";subst=%%;subst:~0,%%b%%"
  ) else (               :# Cut a right-aligned field at the requested size.
    call set ";subst=%%;subst:~%%b%%"
  )
  call set ";const=%%a" &:# %%a=Fixed text before "[". Includes an extra dot at index 0.
  call set ";line=%%;line%%%%;const:~1%%%%;subst%%" &:# Append the fixed text, and the formated string, to the output line.
:# %ECHOVARS.D% ";subst" ";const" ";line"
)
echo.%;line%
endlocal & exit /b

:# Original Format function from dostips.com

:Format fmt str1 str2 ... -- outputs columns of strings right or left aligned
::                        -- fmt [in] - format string specifying column width and alignment, i.e. "[-10][10][10]"
:$created 20060101 :$changed 20091130 :$categories Echo
:$source http://www.dostips.com
SETLOCAL
set "fmt=%~1"
set "line="
set "spac=                                                     "
set "i=1"
for /f "tokens=1,2 delims=[" %%a in ('"echo..%fmt:]=&echo..%"') do (
    set /a i+=1
    call call set "subst=%%%%~%%i%%%spac%%%%%~%%i%%"
    if %%b0 GEQ 0 (call set "subst=%%subst:~0,%%b%%"
    ) ELSE        (call set "subst=%%subst:~%%b%%")
    call set "const=%%a"
    call set "line=%%line%%%%const:~1%%%%subst%%"
)
echo.%line%
EXIT /b

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        strlen						      #
:#                                                                            #
:#  Description     Measure the length of a string                            #
:#                                                                            #
:#  Arguments       %1	    String variable name                              #
:#                  %2	    Ouput variable name                               #
:#                                                                            #
:#  Notes 	    Inspired from C string management routines                #
:#                                                                            #
:#                  Many thanks to 'sowgtsoi', but also 'jeb' and 'amel27'    #
:#		    dostips forum users helped making this short and efficient#
:#  History                                                                   #
:#   2008-11-22     Created on dostips.com.                                   #
:#   2010-11-16     Changed.                                                  #
:#   2012-10-08 JFL Adapted to my %FUNCTION% library.                         #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#   2016-11-11 JFL Avoid copying the input string. 5% faster.		      #
:#                  Added routine :strlen.q. No tracing but twice as fast.    #
:#   2016-11-16 JFL Test just 1 char at each index. 0.6% faster on long strgs.#
:#                                                                            #
:#----------------------------------------------------------------------------#

:strlen stringVar lenVar                -- returns the length of a string
%FUNCTION% EnableDelayedExpansion
if "%~2"=="" %RETURN% 1 &:# Missing argument
%UPVAR% %~2
set "%~2=0"
if defined %~1 for /l %%b in (12,-1,0) do (
  set /a "i=(%~2|(1<<%%b))-1"
  for %%i in (!i!) do if not "!%~1:~%%i,1!"=="" set /a "%~2=%%i+1"
)
%RETURN%

:strlen.q stringVar lenVar                -- returns the length of a string
setlocal EnableDelayedExpansion
set "len=0"
if defined %~1 for /l %%b in (12,-1,0) do (
  set /a "i=(len|(1<<%%b))-1"
  for %%i in (!i!) do if not "!%~1:~%%i,1!"=="" set /a "len=%%i+1"
)
endlocal & if "%~2" neq "" set "%~2=%len%"
exit /b

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        strcpy						      #
:#                                                                            #
:#  Description     Copy the content of a variable into another one           #
:#                                                                            #
:#  Arguments       %1	    Destination variable name                         #
:#                  %2	    Source variable name                              #
:#                                                                            #
:#  Notes 	    Inspired from C string management routines                #
:#                                                                            #
:#                  Features:						      #
:#                  - Supports empty strings (if %2 is undefined, %1 will too)#
:#                  - Supports strings including balanced quotes	      #
:#                  - Supports strings including special characters & and |   #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Copy the content of a variable into another one
:# %1 = Destination variable name
:# %2 = Source variable name
:strcpy
%FUNCTION%
if not "%~1"=="%~2" call set "%~1=%%%~2%%"
%ECHOVARS.D% "%~1"
%RETURN%

:# Append the content of a variable to another one
:# %1 = Destination variable name
:# %2 = Source variable name
:strcat
%FUNCTION%
call set "%~1=%%%~1%%%%%~2%%"
%ECHOVARS.D% "%~1"
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        strlwr						      #
:#                                                                            #
:#  Description     Convert a variable content to lower case		      #
:#                                                                            #
:#  Arguments       %1	    Variable name                                     #
:#                                                                            #
:#  Notes 	    Inspired from C string management routines                #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Convert a variable content to lower case
:# %1 = Variable name
:strlwr
%FUNCTION%
if not defined %~1 %RETURN%
for %%a in ("A=a" "B=b" "C=c" "D=d" "E=e" "F=f" "G=g" "H=h" "I=i"
            "J=j" "K=k" "L=l" "M=m" "N=n" "O=o" "P=p" "Q=q" "R=r"
            "S=s" "T=t" "U=u" "V=v" "W=w" "X=x" "Y=y" "Z=z" "Ç=ç"
            "À=à" "Â=â" "Ä=ä" "É=é" "È=è" "Ê=ê" "Ë=ë" "Î=î" "Ï=ï"
            "Ô=ô" "Ö=ö" "Ù=ù" "Û=û" "Ü=ü" "Ñ=ñ" "Ø=ø" "Å=å") do (
  call set "%~1=%%%~1:%%~a%%"
)
%ECHOVARS.D% "%~1"
%RETURN%

:# Convert a variable content to upper case
:# %1 = Variable name
:strupr
%FUNCTION%
if not defined %~1 %RETURN%
for %%a in ("a=A" "b=B" "c=C" "d=D" "e=E" "f=F" "g=G" "h=H" "i=I"
            "j=J" "k=K" "l=L" "m=M" "n=N" "o=O" "p=P" "q=Q" "r=R"
            "s=S" "t=T" "u=U" "v=V" "w=W" "x=X" "y=Y" "z=Z" "ç=Ç"
            "à=À" "â=Â" "ä=Ä" "é=É" "è=È" "ê=Ê" "ë=Ë" "î=Î" "ï=Ï"
            "ô=Ô" "ö=Ö" "ù=Ù" "û=Û" "ü=Ü" "ñ=Ñ" "ø=Ø" "å=Å") do (
  call set "%~1=%%%~1:%%~a%%"
)
%ECHOVARS.D% "%~1"
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        trim						      #
:#                                                                            #
:#  Description     Trim spaces (or other chars.) from the ends of a string   #
:#                                                                            #
:#  Arguments       %1	    Variable name                                     #
:#                  %2	    Characters to be trimmed. Default: space and tab  #
:#                                                                            #
:#  Notes 	    Inspired from Tcl string timming routines                 #
:#                                                                            #
:#  History                                                                   #
:#   2012-11-09 JFL  Disable delayed expansion to support strings with !s.    #
:#                   Fixed the debug output for the returned value.           #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Trim spaces (or other characters) from the beginning of a string
:# %1 = String variable to be trimmed
:# %2 = Characters to be trimmed. Default: space and tab
:trimleft
%FUNCTION% EnableExtensions DisableDelayedExpansion
if not defined %~1 %RETURN%
call set "string=%%%~1%%"
set "chars=%~2"
if not defined chars set "chars=	 "
:# %ECHOVARS.D% %~1 chars
for /f "tokens=* delims=%chars%" %%a in ("%string%") do set "string=%%a"
%UPVAR% %~1
set "%~1=%string%"
%RETURN%

:# Trim spaces (or other characters) from the end of a string
:# %1 = String variable to be trimmed
:# %2 = Characters to be trimmed. Default: space and tab
:trimright
%FUNCTION% EnableExtensions DisableDelayedExpansion
if not defined %~1 %RETURN%
call set "string=%%%~1%%"
set "chars=%~2"
if not defined chars set "chars=	 "
:# %ECHOVARS.D% RETVAR %~1 string chars DEBUG.RETVARS
:trimright_loop
if not defined string goto trimright_exit
for /f "delims=%chars%" %%a in ("%string:~-1%") do goto trimright_exit
set "string=%string:~0,-1%"
goto trimright_loop
:trimright_exit
%UPVAR% %~1
set "%~1=%string%"
%RETURN%

:# Trim spaces (or other characters) from both ends of a string
:# %1 = String variable to be trimmed
:# %2 = Characters to be trimmed. Default: space and tab
:trim
%FUNCTION%
if not defined %~1 %RETURN%
call :trimleft "%~1" "%~2"
call :trimright "%~1" "%~2"
%UPVAR% %~1
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        TrimRightSlash                                            #
:#                                                                            #
:#  Description     Remove the trailing \ of a pathname, if any               #
:#                                                                            #
:#  Note                                                                      #
:#                                                                            #
:#  History                                                                   #
:#   2014-06-23 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:TrimRightSlash     %1=VARNAME
%FUNCTION% EnableDelayedExpansion
set "VARNAME=%~1"
%UPVAR% %VARNAME%
set "%VARNAME%=!%VARNAME%!::" &:# Note that :: cannot appear in a pathname
set "%VARNAME%=!%VARNAME%:\::=::!"
set "%VARNAME%=!%VARNAME%:::=!"
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        ReplaceXxxx						      #
:#                                                                            #
:#  Description     Replace tricky characters                                 #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    '*' '=' ':' cannot be replaced by %VAR:c=repl%            #
:#                                                                            #
:#  History                                                                   #
:#   2016-11-13 JFL Added routine ReplaceChars.                               #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Replace characters
:# Advantage: Works with CHAR '=' ':' '*' '~'
:# Advantage: The string can contain LF and '"' characters
:# Drawback: Does not work with '='.
:ReplaceChars STRVAR CHAR REPLACEMENT RETVAR 
setlocal EnableDelayedExpansion
set "STRING=!%~1!"
set "REPL=%~3"
set "RESULT="
if defined STRING (
  call :strlen.q STRING SLEN	&:# SLEN = Full string length
:ReplaceChars.again
  set "TAIL=!STRING:*%~2=!"	&:# Split STRING into HEAD CHAR TAIL
  call :strlen.q TAIL TLEN	&:# TLEN = Tail length
  if !TLEN!==!SLEN! (	:# No more C chars
    set "RESULT=!RESULT!!TAIL!"
  ) else (		:# Reached one char
    set /a "HLEN=SLEN-TLEN-1"	&:# HLEN = Head length
    for %%h in (!HLEN!) do set "RESULT=!RESULT!!STRING:~0,%%h!!REPL!"
    if defined TAIL (	:# Then there might be more chars in the tail
      set "STRING=!TAIL!"	&:# Repeat the same operation for the tail.
      set "SLEN=!TLEN!"
      goto :ReplaceChars.again
    )
  )
)
endlocal & set "%~4=%RESULT%"
exit /b 0

:# Replace delimiter sets.
:# Advantage: Simple and fast; Works with CHAR '=' ':' '*'
:# Drawback: Multiple consecutive CHARs are replaced by a single REPL string.
:# Drawback: Does not work on strings with LF or '!' characters.
:ReplaceDelimSets STRVAR CHAR REPLACEMENT RETVAR
setlocal EnableDelayedExpansion
set "STRING=[!%~1!]"	&:# Make mure the string does not begin or end with delims
set "REPL=%~3"
set "RESULT="
:ReplaceDelimSets.loop
for /f "delims=%~2 tokens=1*" %%s in ("!STRING!") do (
  set "RESULT=!RESULT!%%s"
  set "TAIL=%%t"
  if defined TAIL (	 :# Then there might be more chars to replace in the tail
    set "RESULT=!RESULT!!REPL!"
    set "STRING=!TAIL!"	&:# Repeat the same operation for the tail.
    goto :ReplaceDelimSets.loop
  )
)
endlocal & set "%~4=%RESULT:~1,-1%"
exit /b

:# Replace delimiters.
:# Inspired by npocmaka post: http://www.dostips.com/forum/viewtopic.php?p=29901#p29901
:# Advantage: Works with CHAR '=' ':' '*'
:# Drawback: Does not work on strings with LF or '"' characters.
:ReplaceDelims STRVAR CHAR REPLACEMENT RETVAR
setlocal DisableDelayedExpansion
call set "STRING=[%%%~1%%]"	&:# Make mure the string does not begin or end with delims
set "REPL=%~3"
set "RESULT="
call :strlen.q STRING SLEN	&:# SLEN = Full string length
:ReplaceDelims.loop
for /f "delims=%~2 tokens=1*" %%s in ("%STRING%") do (
  set "HEAD=%%s"
  set "TAIL=%%t"
)
set "RESULT=%RESULT%%HEAD%"
call :strlen.q HEAD HLEN	&:# HLEN = Head length
call :strlen.q TAIL TLEN	&:# TLEN = Tail length
set /a "N=SLEN-HLEN-TLEN"	&:# Number of delimiters in between
setlocal EnableDelayedExpansion
for /l %%n in (1,1,%N%) do set "RESULT=!RESULT!!REPL!"
endlocal & set "RESULT=%RESULT%"
if defined TAIL (	 :# Then there might be more chars to replace in the tail
  set "STRING=%TAIL%"	&:# Repeat the same operation for the tail.
  set "SLEN=%TLEN%"
  goto :ReplaceDelims.loop
)
endlocal & set "%~4=%RESULT:~1,-1%"
exit /b

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

:# Append an element to a list
:# %1 = Input/Output variable name
:# %2 = element value
:lappend
%FUNCTION%
%UPVAR% %~1
if defined %~1 call set "%~1=%%%~1%% "
call set "%~1=%%%~1%%"%~2""
%RETURN%

:# Split a string into a list of quoted substrings
:# %1 = Output variable name
:# %2 = Intput variable name
:# %3 = Characters separating substrings. Default: space and tab
:split
%FUNCTION%
if "%~2"=="" %RETURN%
setlocal
call set "string=%%%~2%%"
set "chars=%~3"
if not defined chars set "chars=	 "
set "list="
if not defined string goto split_exit
:# If the string begins with separator characters, begin the list with an empty substring.
set head_chars=true
for /f "delims=%chars%" %%a in ("%string:~0,1%") do set head_chars=false
if %head_chars%==true (
  call :lappend list ""
  :# Remove the head separators. Necessary to correctly detect the tail separators.
  for /f "tokens=* delims=%chars%" %%a in ("%string%") do set "string=%%a"
)
if not defined string goto split_exit
:# If the string ends with separator characters, prepare to append an empty substring to the list.
set tail_chars=true
for /f "delims=%chars%" %%a in ("%string:~-1%") do set tail_chars=false
:# Main loop splitting substrings and appending them to the list.
:split_loop
for /f "tokens=1* delims=%chars%" %%a in ("%string%") do (
  call :lappend list "%%a"
  set "string=%%b"
  goto split_loop
)
if %tail_chars%==true call :lappend list ""
:split_exit
%UPVAR% %~1
set "%~1=%list%"
%RETURN%

:foreach
%FUNCTION% %1=Loop_var_name %2=Values_list %3=Block of code
call set "foreach_list=%%%2%%"
for %%i in (%foreach_list%) do (set "%1=%%i" & %~3)
%RETURN%

:lsort LIST_NAME [RETVAR]
%FUNCTION% EnableExtensions EnableDelayedExpansion
set "RETVAR=%~2"
if not defined RETVAR set "RETVAR=%~1"
%UPVAR% %RETVAR%
set "SORTED_LIST="
%ECHOVARS.D% %~1
:# No idea why %FOREACHLINE% appends a space to every line
%FOREACHLINE% %%l in ('^(for %%e in ^(!%~1!^) do @echo %%e^) ^| sort') do set "SORTED_LIST=!SORTED_LIST!%%l"
if defined SORTED_LIST set "SORTED_LIST=!SORTED_LIST:~0,-1!"
set "%RETVAR%=!SORTED_LIST!"
%RETURN%

:list2lines LIST_NAME
if defined %~1 for %%e in (!%~1!) do (echo.%%e)
exit /b

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
:#   2018-11-19 JFL Improved routine condquote2.                              #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Quote file pathnames that require it.
:condquote	 %1=Input variable. %2=Opt. output variable.
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

:# Simpler version not using the %FUNCTION%/%RETURN% macros
:condquote2	 %1=Input variable. %2=Opt. output variable.
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
endlocal&set RETVAL=%P%&set %RETVAR%=%P%&%RETURN%

:#----------------------------------------------------------------------------#
:# Older implementation (More complex, but actually just as fast)

:# Quote file pathnames that require it. %1=Input variable. %2=Opt. output variable.
:condquote1
%FUNCTION%
setlocal enableextensions
call set "P=%%%~1%%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
:# Remove double quotes inside P. (Fails if P is empty)
set "P=%P:"=%"
:# If the value is empty, don't go any further.
if not defined P set "P=""" & goto :condquote_ret
set RETVAR=%~2
if "%RETVAR%"=="" set RETVAR=%~1
for %%c in (" " "&" "(" ")" "@" "," ";" "[" "]" "{" "}" "=" "'" "+" "`" "~") do (
  :# Note: Cannot directly nest for loops, due to incorrect handling of /f in the inner loop.
  cmd /c "for /f "tokens=1,* delims=%%~c" %%a in (".%%P%%.") do @if not "%%b"=="" exit 1"
  if errorlevel 1 (
    set P="%P%"
    goto :condquote_ret
  )
)
:condquote_ret
:# Contrary to the general rule, do NOT enclose the set command below in "quotes",
:# because this interferes with the quoting already added above. This would
:# fail if the quoted string contained an & character.
:# But because of this, do not leave any space around & separators.
endlocal&set RETVAL=%P%&set %RETVAR%=%P%&%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        time                                                      #
:#                                                                            #
:#  Description     Functions for manipulating date and time.                 #
:#                                                                            #
:#  Arguments       echotime        Display the current time                  #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Display the current time. Useless, except for a comparison with the next function.
:echotime
%FUNCTION%
echo %TIME%
%RETURN%

:echotime
%FUNCTION%
:# Get the time; Keep only the line with numbers; Display only what follows ": ".
@for /f "tokens=1,* delims=:" %%a in ('echo.^|time^|findstr [0-9]') do @echo%%b
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        sleep                                                     #
:#                                                                            #
:#  Description     Sleep N seconds				              #
:#                                                                            #
:#  Arguments       %1        Number of seconds to wait                       #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2012-07-10 JFL Add 1 to the argument, because the 1st ping delays 0 sec. #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:goto :Sleep.End

:# Sleep N seconds. %1 = Number of seconds to wait.
:Sleep
%FUNCTION%
set /A N=%1+1
ping -n %N% 127.0.0.1 >NUL 2>&1
%RETURN%

:Sleep.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        GetPid                                                    #
:#                                                                            #
:#  Description     Get the PID and title of the current console              #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Function GetProcess: Set PID and TITLE with the current console PID and title string.
:GetProcess
%FUNCTION% EnableExtensions
if not defined SFULL >&2 echo Function GetProcess error: Please set "SFULL=%%~0" in script initialization. & %RETURN% 1
:# Get the list of command prompts titles
for /f "tokens=2,9*" %%a in ('tasklist /v /nh /fi "IMAGENAME eq cmd.exe"') do set TITLE.%%a=%%c
:# Change the current title to a statistically unique value
for /f "tokens=2 delims=.," %%A IN ("%TIME%") DO set RDMTITLE=%%A %RANDOM% %RANDOM% %RANDOM% %RANDOM%
%ECHOVARS.D% RDMTITLE
title %RDMTITLE%
:# Find our PID
set N=3
:GetProcessAgain
:# Note: Do not filter by title, because when running as administrator, there's a prefix: Administrator:
:#       And at any time, there's a temporary suffix: The name of the running script (This very script!) and its arguments.
:# for /f "tokens=2" %%a in ('tasklist /v /nh /fi "WINDOWTITLE eq %RDMTITLE%"') do set PID=%%a
for /f "tokens=2,9*" %%a in ('tasklist /v /nh /fi "IMAGENAME eq cmd.exe" ^| findstr "%RDMTITLE%"') do set "PID=%%a" & set "TITLENOW=%%c"
:# Gotcha: Sometimes the above command returns a wrong TITLENOW, containing "N/A". (What would it be in localized versions of Windows?)
:# Maybe there's a small (variable?) delay before and entry with the new title appears in Windows task list?
:# Maybe it's another instance with the findstr command itself that disrupts the test?
:# Anyway, double check the result, and try again up to 3 times if it's bad. 
echo "%TITLENOW%" | findstr "%RDMTITLE%" >nul
if errorlevel 1 (
  if .%DEBUG%.==.1. (
    >&2 echo Warning: Wrong title: %TITLENOW%
    :# Note: This tasklist has never returned an entry with N/A, but tests with teeing the initial tasklist above have.
    tasklist /v /nh /fi "IMAGENAME eq cmd.exe"
  )
  if not %N%==0 (
    if .%DEBUG%.==.1. >&2 echo Scan cmd.exe windows titles again.
    set /a N=N-1
    goto GetProcessAgain
  )
  >&2 echo Function GetProcess error: Failed to identify the current process title.
  title ""
  %RETURN% 1
)
%ECHOVARS.D% PID TITLENOW
:# Parse the actual title now. (May differ from the one we set, due to an additional Administrator: prefix.)
:# If we find such a prefix, then assume the initial title had that same prefix.
call :trimright TITLENOW
:# Find our initial title
call set TITLE=%%TITLE.%PID%%%
set TITLE=%TITLE:"=''% &:# Remove quotes, which may be unmatched, and confuse the %RETURN% macro
%ECHOVARS.D% TITLE
call :trimright TITLE
:# Find the possible title prefix and suffix
%ECHO.D% call set "PREFIX=%%TITLENOW:%RDMTITLE%=";rem %%
call set "PREFIX=%%TITLENOW:%RDMTITLE%=";rem %%
%ECHOVARS.D% PREFIX
:# Now trim the possible prefix and suffix from the title
:# In the absence of a special char (like ^) to anchor the match string at the beginning,
:# prefix the prefix with our random string, to avoid problems if the prefix string is repeated elsewhere in the string
:# Additional gotcha: Initially there's 1 space between the prefix and title; 
:# but the title command always ends up putting 2 spaces there. So erase all spaces there.
call :trimright PREFIX
set "TITLE=%RDMTITLE% %TITLE%"
call set "TITLE=%%TITLE:%RDMTITLE% %PREFIX%=%%"
%ECHOVARS.D% TITLE
call :trimleft TITLE
:# Remove the suffix from the title. Else if we leave it and restore the title with
:# that suffix, then the suffix will remain after the script exits.
set "SUFFIX=%RDMTITLE% %TITLE%"
call set "TITLE=%%TITLE: - %SFULL%=";rem %%
%ECHOVARS.D% TITLE
call set "SUFFIX=%%SUFFIX:%RDMTITLE% %TITLE%=%%"
%ECHOVARS.D% SUFFIX
call :trimleft SUFFIX
:# Restore the title
title %TITLE%
%UPVAR% PID TITLE PREFIX SUFFIX
%RETURN% 0

:GetPid1
%FUNCTION% enableextensions
:# Get the list of command prompts
for /f "tokens=2,9*" %%a in ('tasklist /v /nh /fi "IMAGENAME eq cmd.exe"') do set TITLE.%%a=%%c
:# Change the current title to a random value
set TITLE=%RANDOM% %RANDOM% %RANDOM% %RANDOM%
title %TITLE%
:# Find our PID
set PID=0
:GetPidAgain
for /f "tokens=2" %%a in ('tasklist /v ^| findstr "%TITLE%"') do set PID=%%a
if %PID%==0 goto GetPidAgain
:# Find our initial title
call set TITLE=%%TITLE.%PID%%%
:# Restore the title
title %TITLE%
%UPVAR% PID TITLE
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        GetPid                                                    #
:#                                                                            #
:#  Description     Get the PID of the current console                        #
:#                                                                            #
:#  Arguments       %1 = Variable name. Default: PID                          #
:#                                                                            #
:#  Notes 	    Uses a lock file to make sure that two scripts running    #
:#                  within 0.01s of each other do not get the same uid string.#
:#                  The instance that fails retries until it succeeds.        #
:#                  No side effect on the window title.                       #
:#                                                                            #
:#  History                                                                   #
:#   2014-12-23 DB  D.Benham published on dostips.com:                        #
:#                  http://www.dostips.com/forum/viewtopic.php?p=38870#p38870 #
:#   2016-09-11 JFL Adapted to my %FUNCTION%/%UPVAR%/%RETURN% mechanism.      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:GetPID [VARNAME]
%FUNCTION% EnableExtensions DisableDelayedExpansion
set "PIDVAR=%~1" & if not defined PIDVAR set "PIDVAR=PID"
%UPVAR% %PIDVAR%
:GetPID.retry
set "lock=%temp%\%~nx0.%time::=.%.lock"
set "uid=%lock:\=:b%"
set "uid=%uid:,=:c%"
set "uid=%uid:'=:q%"
set "uid=%uid:_=:u%"
setlocal enableDelayedExpansion
set "uid=!uid:%%=:p!"
endlocal & set "uid=%uid%"
2>nul ( 9>"%lock%" (
  for /f "skip=1" %%A in (
    'wmic process where "name='cmd.exe' and CommandLine like '%%<%uid%>%%'" get ParentProcessID'
  ) do for %%B in (%%A) do set "%PIDVAR%=%%B"
  (call )
))||goto :GetPID.retry
del "%lock%" 2>nul
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        IsAdmin                                                   #
:#                                                                            #
:#  Description     Test if the user has administration rights                #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    Returns the result in %ERRORLEVEL%: 0=Yes; 5=No           #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:IsAdmin
>NUL 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
goto :eof

:IsAdmin
>NUL 2>&1 net session
goto :eof

:RunAsAdmin
:# adaptation of https://sites.google.com/site/eneerge/home/BatchGotAdmin and http://stackoverflow.com/q/4054937
:# Check for ADMIN Privileges
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
if '%errorlevel%' NEQ '0' (
  REM Get ADMIN Privileges
  echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
  echo UAC.ShellExecute "%~s0", "", "", "runas", 1 >> "%temp%\getadmin.vbs"
  "%temp%\getadmin.vbs"
  del "%temp%\getadmin.vbs"
  exit /B
) else (
  REM Got ADMIN Privileges
  pushd "%cd%"
  cd /d "%~dp0"
)
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        chcp	                                              #
:#                                                                            #
:#  Description     Get code pages		                              #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2017-05-11 JFL Removed rscp which was trivial and useless.               #
:#                  Changed chcp into GetCP, returning the current code page. #
:#                  Added GetACP, GetOEMCP.				      #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Get the current console code page.
:GetCP %1=variable name
for /f "tokens=2 delims=:" %%n in ('chcp') do for %%p in (%%n) do set "%1=%%p"
goto :eof

:# Get the default console code page
:GetOEMCP %1=variable name
set "CP_KEY=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage"
for /f "tokens=3" %%p in ('reg query "%CP_KEY%" /v "OEMCP" ^| findstr REG_SZ') do set "%1=%%p"
goto :eof

:# Get the default system code page
:GetACP %1=variable name
set "CP_KEY=HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Nls\CodePage"
for /f "tokens=3" %%p in ('reg query "%CP_KEY%" /v "ACP" ^| findstr REG_SZ') do set "%1=%%p"
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        FOREACHLINE                                               #
:#                                                                            #
:#  Description     Repeat a block of code for each line in a text file       #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    :# Example d'utilisation                                  #
:#                  %FOREACHLINE% %%l in ('%CMD%') do (                       #
:#                    set "LINE=%%l"                                          #
:#                    echo Notice: !LINE!                                     #
:#                  )                                                         #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# FOREACHLINE macro. (Change the delimiter to none to catch the whole lines.)
set FOREACHLINE=for /f "delims="

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        path_depth                                                #
:#                                                                            #
:#  Description     Compute the depth of a pathname                           #
:#                                                                            #
:#  Arguments       %1	    pathname                                          #
:#                                                                            #
:#  Notes 	    Ex: A\B\C -> 3 ; \A -> 1 ; A\ -> 1                        #
:#                                                                            #
:#  History                                                                   #
:#   2011-12-12 JFL Created this routine                                      #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Compute the depth of a pathname. %1=pathname. Ex: A\B\C -> 3 ; \A -> 1 ; A\ -> 1
:path_depth
%FUNCTION%
if not "%~2"=="" set "RETVAR=%~2"
call :path_depth2 %*
%UPVAR% %RETVAR%
set %RETVAR%=%RETVAL%
%RETURN%

:# Worker routine, with call/return trace disabled, to avoid tracing recursion.
:path_depth2
set "ARGS=%~1"
set ARGS="%ARGS:\=" "%"
set ARGS=%ARGS:""=%
set RETVAL=0
for %%i in (%ARGS%) do @set /a RETVAL=RETVAL+1
goto :eof

:#----------------------------------------------------------------------------#
:# First implementation, 50% slower.

:path_depth1
set RETVAL=0
for /f "tokens=1* delims=\" %%i in ("%~1") do (
  if not "%%j"=="" call :path_depth1 "%%j"
  if not "%%i"=="" set /a RETVAL=RETVAL+1
)
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        is_dir						      #
:#                                                                            #
:#  Description     Check if a pathname refers to an existing directory       #
:#                                                                            #
:#  Arguments       %1	    pathname                                          #
:#                                                                            #
:#  Notes 	    Returns errorlevel 0 if it's a valid directory.           #
:#                                                                            #
:#  History                                                                   #
:#   2013-08-27 JFL Created this routine.                                     #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#   2017-10-23 JFL Simpler and faster versions of is_dir.                    #
:#                                                                            #
:#----------------------------------------------------------------------------#

:get_attrs PATHNAME RETVAR -- Returns string drahscotl-- (Directory/Read Only/Archived/Hidden/System/Compressed/Offline/Temporary/Link)
set "%~2="
for /f "delims=" %%a in ("%~a1") do set "%~2=%%~a"
exit /b

:is_dir pathname       -- Check if a pathname refers to an existing directory
for /f "tokens=1,2 delims=d" %%a in ("-%~a1") do if not "%%~b"=="" exit /b 0
exit /b 1

:is_dir2 pathname       -- Check if a pathname refers to an existing directory
pushd "%~1" 2>NUL && popd
exit /b

:is_dir1 pathname       -- Check if a pathname refers to an existing directory
%FUNCTION%
pushd "%~1" 2>NUL
if errorlevel 1 (
  set "ERROR=1"
) else (
  set "ERROR=0"
  popd
)
%RETURN% %ERROR%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        basename						      #
:#                                                                            #
:#  Description     Get the file name part of a pathname                      #
:#                                                                            #
:#  Arguments       %1	    Input pathname variable name                      #
:#                  %2	    Ouput file name variable name                     #
:#                                                                            #
:#  Notes 	    Inspired from Unix' basename command                      #
:#                                                                            #
:#                  Works even when the base name contains wild cards,        #
:#                  which prevents using commands such as                     #
:#                  for %%f in (%ARG%) do set NAME=%%~nxf                     #
:#                                                                            #
:#  History                                                                   #
:#   2013-08-27 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# TODO: This :basename routine will give wrong results on "D:relative.txt"
:basename pathnameVar filenameVar :# Returns the file name part of the pathname
%FUNCTION% enabledelayedexpansion
set "RETVAR=%~2"
if "%RETVAR%"=="" set "RETVAR=RETVAL"
set "NAME=!%~1!"
:basename.trim_path
set "NAME=%NAME:*\=%"
if not "%NAME%"=="%NAME:\=%" goto :basename.trim_path
%UPVAR% %RETVAR%
set "%RETVAR%=%NAME%"
%RETURN%

:#----------------------------------------------------------------------------#

:dirname PATHNAME DIRVAR	:# Returns the directory part of the pathname
setlocal EnableExtensions EnableDelayedExpansion
set "DIR=%~1"
:dirname.next
if "!DIR:~-1!"=="\" goto :dirname.done
if "!DIR:~-1!"==":" goto :dirname.done
set "DIR=!DIR:~0,-1!"
if defined DIR goto :dirname.next
:dirname.done
endlocal & set "%~2=%DIR%" & exit /b

:#----------------------------------------------------------------------------#

:filename PATHNAME FILEVAR	:# Returns the file name part of the pathname
setlocal EnableExtensions EnableDelayedExpansion
set "PATHNAME=%~1"
set "NAME="
:filename.next
set "C=!PATHNAME:~-1!"
if "!C!"=="\" goto :filename.done
if "!C!"==":" goto :filename.done
set "NAME=!C!!NAME!"
set "PATHNAME=!PATHNAME:~0,-1!"
if defined PATHNAME goto :filename.next
:filename.done
endlocal & set "%~2=%NAME%" & exit /b

:#----------------------------------------------------------------------------#

:has_wildcards NAME	:# Return ERRORLEVEL 0 if a name contains wildcards
setlocal EnableExtensions EnableDelayedExpansion
set "NAME=%~1"
set "RESULT=0"
:has_wildcards.next
set "C=!NAME:~-1!"
if "!C!"=="*" goto :has_wildcards.done
if "!C!"=="?" goto :has_wildcards.done
set "NAME=!NAME:~0,-1!"
if defined NAME goto :has_wildcards.next
set "RESULT=1"
:has_wildcards.done
endlocal & exit /b %RESULT%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        touch						      #
:#                                                                            #
:#  Description     Pure batch implementation of the Unix touch command       #
:#                                                                            #
:#  Arguments       %1	    file name                                         #
:#                                                                            #
:#  Notes 	    Based on sample in http://superuser.com/a/764725          #
:#                                                                            #
:#                  Creates file if it does not exist.                        #
:#                  Just uses cmd built-ins.                                  #
:#                  Works even on read-only files, like touch does.           #
:#                                                                            #
:#  History                                                                   #
:#   2011-02-16     http://superuser.com/users/201155/bobbogo created this.   #
:#   2015-11-02 JFL Wrapped in a %FUNCTION% with local variables.             #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:touch
%FUNCTION%
if not exist "%~1" type NUL >>"%~1"& %RETURN%
set _ATTRIBUTES=%~a1
if "%~a1"=="%_ATTRIBUTES:r=%" (copy "%~1"+,,) else attrib -r "%~1" & copy "%~1"+,, & attrib +r "%~1"
%RETURN%

:# Simpler version without read-only file support
:touch
type nul >>"%~1" & copy "%~1",,
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        extensions.test                                           #
:#                                                                            #
:#  Description     Test if cmd extensions work                               #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    Do not use %FUNCTION% or %RETURN%, as these do require    #
:#                  command extensions to work.                               #
:#                  Only use command.com-compatible syntax!                   #
:#                                                                            #
:#  History                                                                   #
:#   2015-11-23 JFL Renamed, and added :extensions.get and :extensions.show.  #
:#   2015-12-01 JFL Rewrote :extensions.get and :extensions.show as extension #
:#                  and expansion modes are independant of each other. Also   #
:#                  call :extensions.get cannot work if extensions are off.   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Get cmd extensions and delayed expansion settings
:extensions.get returns errorlevel=1 if extensions are disabled
ver >NUL &:# Clear the errorlevel
:# Note: Don't use quotes around set commands in the next two lines, as this will not work if extensions are disabled
set EXTENSIONS=DisableExtensions
set DELAYEDEXPANSION=DisableDelayedExpansion
set "EXTENSIONS=EnableExtensions" 2>NUL &:# Fails if extensions are disabled
if "!!"=="" set DELAYEDEXPANSION=EnableDelayedExpansion
goto %EXTENSIONS.RETURN% :eof 2>NUL &:# goto :eof will work, but report an error if extensions are disabled

:# Display cmd extensions and delayed expansion settings
:extensions.show
setlocal &:# Avoid changing the parent environment
set EXTENSIONS.RETURN=:extensions.show.ret
goto :extensions.get &:# call :extensions.get will not work if extensions are disabled
:extensions.show.ret
%ECHO% SetLocal %EXTENSIONS% %DELAYEDEXPANSION%
endlocal &:# Restore the parent environment
goto :eof 2>NUL &:# This goto will work, but report an error if extensions are disabled

:# Test if cmd extensions work (They don't in Windows 2000 and older)
:extensions.test
verify other 2>nul
setlocal enableextensions enabledelayedexpansion
if errorlevel 1 (
  >&2 echo Error: Unable to enable command extensions.
  >&2 echo This script requires Windows XP or later.
  endlocal & set "RETVAL=1" & goto :eof
)
set VAR=before
if "%VAR%" == "before" (
  set VAR=after
  if not "!VAR!" == "after" (
    >&2 echo Error: Failed to enable delayed environment variable expansion.
    >&2 echo This script requires Windows XP or later.
    endlocal & set "RETVAL=1" & goto :eof
  )
)
endlocal & set "RETVAL=0" & goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        get_IP_address                                            #
:#                                                                            #
:#  Description     Get the current IP address                                #
:#                                                                            #
:#  Arguments       %1	    Ouput variable name. Default name: MYIP           #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2010-04-30 JFL Created this routine.                                     #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Find the current IP address
:get_IP_address %1=Ouput variable name; Default name: MYIP   
%FUNCTION%
set "RETVAR=%~1"
if "%RETVAR%"=="" set "RETVAR=MYIP"
set %RETVAR%=
:# Note: The second for in the command below is used to remove the head space left in %%i after the : delimiter.
for /f "tokens=2 delims=:" %%i in ('ipconfig ^| find "IPv4" ^| find /V " 169.254"') do for %%j in (%%i) do set %RETVAR%=%%j
%UPVAR% %RETVAR%
%RETURN%

:# Other versions experimented
:# for /f %%i in ('ipconfig ^| find "IPv4" ^| find " 10." ^| remplace -q "   IPv4 Address[. ]*: " ""') do set MYIP=%%i
:# for /f "tokens=14" %%i in ('ipconfig ^| find "IPv4" ^| findstr /C:" 16." /C:" 10." /C:" 192."') do set MYIP=%%i

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        IsIPv4Supported                                           #
:#                                                                            #
:#  Description     Is IP v4 supported on this computer                       #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    Result in %ERRORLEVEL%: 0=Supported; 1=NOT supported      #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:IsIPv4Supported
%FUNCTION%
ping 127.0.0.1 | find "TTL=" >NUL 2>&1
%RETURN%

:# Alternative implementation, faster, but the wmic command is only available on XP Pro or later.
:IsIPv4Supported
%FUNCTION%
wmic Path Win32_PingStatus WHERE "Address='127.0.0.1'" Get StatusCode /Format:Value | findstr /X "StatusCode=0" >NUL 2>&1
%RETURN%

:IsIPv6Supported
%FUNCTION%
ping ::1 | findstr /R /C:"::1:[ˆ$]" >NUL 2>&1
%RETURN%

:# Alternative implementation, faster, but the wmic command is only available on XP Pro or later.
:IsIPv6Supported
%FUNCTION%
wmic Path Win32_PingStatus WHERE "Address='::1'" Get StatusCode >NUL 2>&1
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        EnumLocalAdmins                                           #
:#                                                                            #
:#  Description     List all local administrators                             #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    Using only native Windows NT 4+ commands.                 #
:#                                                                            #
:#  History                                                                   #
:#   2015-11-19 JFL Adapted to new %UPVAR% mechanism.                         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:EnumLocalAdmins
%FUNCTION% enableextensions enabledelayedexpansion
for /f "delims=[]" %%a in ('net localgroup Administrators ^| find /n "----"') do set HeaderLines=%%a
for /f "tokens=*"  %%a in ('net localgroup Administrators') do set FooterLine=%%a
net localgroup Administrators | more /E +%HeaderLines% | find /V "%FooterLine%"
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        num_ips                                                   #
:#                                                                            #
:#  Description     Count IP addresses in a range                             #
:#                                                                            #
:#  Arguments       %1	    First address. Ex: 192.168.0.1                    #
:#                  %2	    Last address, not included in the count.          #
:#                                                                            #
:#  Notes 	    Adapted from a sample published by Walid Toumi:           #
:#                  http://walid-toumi.blogspot.com/                          #
:#                                                                            #
:#  History                                                                   #
:#   2011-08-24 WT  Sample published on http://walid-toumi.blogspot.com/.     #
:#   2011-12-20 JFL Renamed, fixed, and simplified.                           #
:#                                                                            #
:#----------------------------------------------------------------------------#

:num_ips
setlocal enableextensions enabledelayedexpansion
for /f "tokens=1-8 delims=." %%a in ("%1.%2") do (
  set /A a=%%e-%%a,b=%%f-%%b,c=%%g-%%c,d=%%h-%%d
  for %%e in (b c d) do set /A a=256*a + !%%e!
)
endlocal & set "RETVAL=%a%" & goto :eof

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

:now
setlocal enableextensions enabledelayedexpansion
:# First get the short date format from the Control Panel data in the registry
for /f "tokens=3" %%a in ('reg query "HKCU\Control Panel\International" /v sShortDate 2^>NUL ^| findstr "REG_SZ"') do set "SDFTOKS=%%a"
if .%DEBUG_NOW%.==.1. echo set "SDFTOKS=!SDFTOKS!"
:# Now simplify this (ex: "yyyy/MM/dd") to a "YEAR MONTH DAY" format
for %%a in ("yyyy=y" "yy=y" "y=YEAR" "MMM=M" "MM=M" "M=MONTH" "dd=d" "d=DAY" "/=-" ".=-" "-= ") do set "SDFTOKS=!SDFTOKS:%%~a!"
if .%DEBUG_NOW%.==.1. echo set "SDFTOKS=!SDFTOKS!"
:# From the actual order, generate the token parsing instructions
set "%%=%%" &:# Define a % variable that will generate a % _after_ the initial %LoopVariable parsing phase
for /f "tokens=1,2,3" %%t in ("!SDFTOKS!") do set "SDFTOKS=set %%t=!%%!a&set %%u=!%%!b&set %%v=!%%!c"
if .%DEBUG_NOW%.==.1. echo set "SDFTOKS=!SDFTOKS!"
:# Then get the current date and time. (Try minimizing the risk that they get off by 1 day around midnight!)
set "D=%DATE%" & set "T=%TIME%"
if .%DEBUG_NOW%.==.1. echo set "D=%D%" & echo set "T=%T%"
:# Remove the day-of-week that appears in some languages (US English, Chinese...)
for /f %%d in ('for %%a in ^(%D%^) do @^(echo %%a ^| findstr /r [0-9]^)') do set "D=%%d"
if .%DEBUG_NOW%.==.1. echo set "D=%D%"
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
if .%DEBUG_NOW%.==.1. echo set "T=%T%" & echo set "MS=%MS%"
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

:# Initial implementation, with a less detailed output, but simple and guarantied to work in all cases.
:now
setlocal enableextensions enabledelayedexpansion
:# Get the time, including seconds. ('TIME /T' returns MM:SS only)
for /f "delims=.," %%t in ("%TIME%") do SET T=%%t
:# Change the optional leading space to a 0. (For countries that use a 12-hours format)
set T=%T: =0%
:# Change HH:MM:SS to HHhMMmSS, as : is invalid in pathnames
for /f "tokens=1-3 delims=:" %%a in ("%T%") do (
  SET HH=%%a
  SET MM=%%b
  SET SS=%%c
)
set T=%HH%h%MM%m%SS%
:# Build the DATE_TIME string
set NOW=%DATE:/=-%_%T%
endlocal & set "RETVAL=%NOW%" & set "NOW=%NOW%" & goto :eof

:#----------------------------------------------------------------------------#

:# Other implementation, independant of the locale, but not of the language, and not relying on the registry.
:# This will work for all languages that output a hint like (mm-dd-yy)
:# This can easily be adapted to other languages: French=(jj-mm-aa) German=(TT-MM-JJ) Spanish=(dd-mm-aa) Japanese ([]-[]-[])
:# But Chinese outputs a string without dashes: ([][][]) so this would be more difficult.
:now
setlocal enableextensions enabledelayedexpansion
set "D="
for /f "tokens=2 delims=:" %%a in ('echo.^|date') do (
  if "!D!"=="" ( set "D=%%a" ) else ( set "O=%%a" )
)
for /f "tokens=1-3 delims=(-) " %%a in ("%O%") DO (
  set "first=%%a" & set "second=%%b" & set "third=%%c"
)
for /f %%d in ('for %%a in ^(%D%^) do @^(echo %%a ^| findstr /r [0-9]^)') do set "D=%%d"
SET %first%=%D:~0,2%
SET %second%=%D:~3,2%
SET %third%=%D:~6,4%
endlocal & SET "YEAR=%yy%" & SET "MONTH=%mm%" & SET "DAY=%dd%" & goto :eof

:#----------------------------------------------------------------------------#

:# Another implementation based on wmic. Not available in early XP versions?
:# Just as fast as the pure batch version using reg for internationalization.

:noww
setlocal enableextensions
for %%i in ("0=Sun" "1=Mon" "2=Tue" "3=Wed" "4=Thu" "5=Fri" "6=Sat") do set "wd%%~i"
for /f %%i in ('WMIC OS GET LocalDateTime /value') do for /f "tokens=2 delims==" %%j in ("%%i") do set "dt=%%j"
:# for /f %%i in ('WMIC PATH Win32_LocalTime GET DayOfWeek /value') do for /f "tokens=2 delims==" %%j in ("%%i") do (
:#   call set "dt=%%wd%%j%% %dt:~,4%-%dt:~4,2%-%dt:~6,2% %dt:~8,2%:%dt:~10,2%:%dt:~12,5%"
:# )
endlocal & set "YEAR=%dt:~,4%" & set "MONTH=%dt:~4,2%" & set "DAY=%dt:~6,2%" & set "HOUR=%dt:~8,2%" & set "MINUTE=%dt:~10,2%" & set "SECOND=%dt:~12,2%" & set "MS=%dt:~15,3%"

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Time.Delta                                                #
:#                                                                            #
:#  Description     Compute the difference between two times                  #
:#                                                                            #
:#  Returns         Environment variables DC DH DM DS DMS                     #
:#                  for carry, hours, minutes, seconds, milliseconds          #
:#                                                                            #
:#  Notes 	    Carry == 0, or -1 if the time flipped over midnight.      #
:#                                                                            #
:#  History                                                                   #
:#   2012-10-08 JFL Created this routine.                                     #
:#   2012-10-12 JFL Renamed variables. Added support for milliseconds.        #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Time.Delta %1=T0 %2=T1 [%3=-f]. Input times in HH:MM:SS[.mmm] format.
setlocal enableextensions enabledelayedexpansion
for /f "tokens=1,2,3,4 delims=:." %%a in ("%~1") do set "H0=%%a" & set "M0=%%b" & set "S0=%%c" & set "MS0=%%d000" & set "MS0=!MS0:~0,3!"
for /f "tokens=1,2,3,4 delims=:." %%a in ("%~2") do set "H1=%%a" & set "M1=%%b" & set "S1=%%c" & set "MS1=%%d000" & set "MS1=!MS1:~0,3!"
:# Remove the initial 0, to avoid having numbers interpreted in octal afterwards. (MS may have 2 leading 0s!)
for %%n in (0 1) do for %%c in (H M S MS MS) do if "!%%c%%n:~0,1!"=="0" set "%%c%%n=!%%c%%n:~1!"
:# Compute differences
for %%c in (H M S MS) do set /a "D%%c=%%c1-%%c0"
set "DC=0" & :# Carry  
:# Report carries if needed
if "%DMS:~0,1%"=="-" set /a "DMS=DMS+1000" & set /a "DS=DS-1"
if "%DS:~0,1%"=="-" set /a "DS=DS+60" & set /a "DM=DM-1"
if "%DM:~0,1%"=="-" set /a "DM=DM+60" & set /a "DH=DH-1"
if "%DH:~0,1%"=="-" set /a "DH=DH+24" & set /a "DC=DC-1"
:# If requested, convert the results back to a 2-digit format.
if "%~3"=="-f" for %%c in (H M S MS) do if "!D%%c:~1!"=="" set "D%%c=0!D%%c!"
if "!DMS:~2!"=="" set "DMS=0!DMS!"
endlocal & set "DC=%DC%" & set "DH=%DH%" & set "DM=%DM%" & set "DS=%DS%" & set "DMS=%DMS%" & goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        WinVer                                                    #
:#                                                                            #
:#  Description     Parse Windows version, extracting major, minor & build #. #
:#                                                                            #
:#  Arguments       None                                                      #
:#                                                                            #
:#  Returns         Environment variables WINVER WINMAJOR WINMINOR WINBUILD   #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2012-02-29 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:WinVer
for /f "tokens=*" %%v in ('ver') do @set WINVER=%%v
for /f "delims=[]" %%v in ('for %%a in ^(%WINVER%^) do @^(echo %%a ^| findstr [0-9]^)') do @set WINVER=%%v
for /f "tokens=1,2,3 delims=." %%v in ("%WINVER%") do @(set "WINMAJOR=%%v" & set "WINMINOR=%%w" & set "WINBUILD=%%x")
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Firewall.GetRules                                         #
:#                                                                            #
:#  Description     Get a list of firewall rules, and their properties        #
:#                                                                            #
:#  Arguments       %1	    Rule(s) name                                      #
:#                                                                            #
:#  Returns         RULE.N                Number of rules found               #
:#                  RULE.LIST             List of rule indexes                #
:#                  RULE[!N!].PROPERTIES  List of properties                  #
:#                  RULE[!N!].!PROPERTY!  Property value                      #
:#                                                                            #
:#  Notes 	    Requires delayed expansion enabled beforehand.            #
:#                                                                            #
:#  History                                                                   #
:#   2013-11-28 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:Firewall.GetRules
%FUNCTION%
set "RULE.N=0"
set "RULE.LIST="
               %ECHO.XVD% netsh advfirewall firewall show rule name^=%1 verbose
for /f "delims=" %%l in ('netsh advfirewall firewall show rule name^=%1 verbose') do (
  for /f "tokens=1,* delims=:" %%a in ('echo.%%l') do (
    set "RULE.NAME=%%a"  &:# Property name
    set "RULE.VALUE=%%b" &:# Property value
    if not "%%b"=="" (
      if "!RULE.NAME!"=="Rule Name" ( :# It's a new rule
      	set "RULE.I=!RULE.N!"
	set "RULE.LIST=!RULE.LIST! !RULE.I!"
      	set /a "RULE.N=!RULE.N!+1"
      ) else ( :# It's a property of the current rule.
      	set "RULE.NAME=!RULE.NAME: =_!"	& rem :# Make sure it does not contain spaces
	call set "RULE[%%RULE.I%%].PROPERTIES=%%RULE[!RULE.I!].PROPERTIES%% !RULE.NAME!"
      	:# %%b is the value, but we need to skip all spaces after the :
      	for /f "tokens=1,*" %%c in ('echo 1 !RULE.VALUE!') do (
      	  set "RULE.VALUE=%%d"
      	)
      	set "RULE[!RULE.I!].!RULE.NAME!=!RULE.VALUE!"
      )
    )
  )
)
%IF_DEBUG% set RULE
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        GetServerAddress                                          #
:#                                                                            #
:#  Description     Use nslookup.exe to resolve an IP address		      #
:#                                                                            #
:#  Arguments       %1	    Server name                                       #
:#                  %2      Name of the return variable. Default: ADDRESS     #
:#                                                                            #
:#  Notes 	    Returns an empty string if it cannot resolve the address. #
:#                                                                            #
:#  History                                                                   #
:#   2015-03-02 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# The nslookup output contains:
:#	0 or more lines with parameters, like:
:#		1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa
:#		        primary name server = 1.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.0.ip6.arpa
:#		        responsible mail addr = (root)
:#		        serial  = 0
:#		        refresh = 28800 (8 hours)
:#		        retry   = 7200 (2 hours)
:#		        expire  = 604800 (7 days)
:#		        default TTL = 86400 (1 day)
:#	Then 2 lines (+1 blank line) with the DNS name and address:
:#		        Server:  UnKnown
:#		        Address:  ::1
:#	Then, if success, 2 lines with a name and a first address
:#		        Name:    katz1.adm.lab.gre.hp.com
:#		        Address:  10.16.131.1
:#	Then, if there are multiple addresses, N lines with just an address
:#		                  10.18.131.1
:#		                  10.17.131.1

:# Use nslookup.exe to resolve an IP address. Return the last one found, or an empty string.

:GetServerAddress %1=Name %2=RetVar
%FUNCTION% enableextensions enabledelayedexpansion
set "NAME=%~1"
set "RETVAR=%~2"
if "%RETVAR%"=="" set "RETVAR=ADDRESS"
set "ADDRESS="
set "NFIELD=0"
for /f "tokens=1,2" %%a in ('nslookup %NAME% 2^>NUL') do (
  set "A=%%a"
  set "B=%%b"
  %ECHOVARS.D% A B
  set "ADDRESS=%%b"			&REM Normally the address is the second token.
  if "%%b"=="" set "ADDRESS=%%a"	&REM But for final addresses it may be the first.
  if not "!A!"=="!A::=!" set /a "NFIELD=NFIELD+1" &REM Count lines with a NAME: header.
  if "!NFIELD!"=="0" set "ADDRESS="	&REM The first two values are for the DNS server, not for the target server.
  if "!NFIELD!"=="1" set "ADDRESS="
  if "!NFIELD!"=="2" set "ADDRESS="
  %ECHOVARS.D% NFIELD ADDRESS
)
%UPVAR% %RETVAR%
set "%RETVAR%=%ADDRESS%"
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        GetKeys						      #
:#                                                                            #
:#  Description     Get sub-keys of a Registry key                            #
:#                                                                            #
:#  Arguments       [-c]	  Case sensitive. Default: Insensitive        #
:#                  [-f PATTERN]  Pattern to search for. Default: *           #
:#                  KEY           Parent key name                             #
:#                  [OUTVAR]      Output list name. Default: KEYS             #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# List registry sub-keys. Args: [-c] [-f PATTERN] KEY [OUTVAR]
:GetKeys
%FUNCTION% enableextensions enabledelayedexpansion
set "PATTERN=*"
set "OPTS=/k"
set "KEY="
set "OUTVAR="
:get_keys_args
if "%~1"=="" goto got_keys_args
if "%~1"=="-c" shift & set "OPTS=%OPTS% /c" & goto get_keys_args
if "%~1"=="-f" shift & set "PATTERN=%~1" & shift & goto get_keys_args
if not defined KEY set "KEY=%~1" & shift & goto get_keys_args
if not defined OUTVAR set "OUTVAR=%~1" & shift & goto get_keys_args
:got_keys_args
if not defined OUTVAR set "OUTVAR=KEYS"
set "%OUTVAR%="
%ECHOVARS.D% KEY OUTVAR
%UPVAR% %OUTVAR%
set "BEFORE="
if "%FULLPATH%"=="1" set "BEFORE=%KEY%\"
:# Use reg.exe to get the key information
set CMD=reg query "%KEY%" /f !PATTERN! !OPTS!
%ECHO.D% %CMD%
:# For each line in CMD output...
set "SEPARATOR="
%FOREACHLINE% %%l in ('%CMD%') do (
  set "LINE=%%l"
  set "HEAD=!LINE:~0,2!"
  if "!HEAD!"=="HK" (
    set "NAME=%%~nxl"
    if "!NAME!"=="(Default)" set "NAME="
    set "NAME=!BEFORE!!NAME!"
    call :CondQuote NAME
    set %OUTVAR%=!%OUTVAR%!!SEPARATOR!!NAME!
    set "SEPARATOR= "
  )
)
%RETURN%

:#----------------------------------------------------------------------------#

:# List registry values. Args: [-/] [-c] [-f PATTERN] KEY [OUTVAR]
:GetValues
%FUNCTION% enableextensions enabledelayedexpansion
set "DETAILS=0"
set "PATTERN=*"
set "OPTS=/v"
set "KEY="
set "OUTVAR="
:get_values_args
if "%~1"=="" goto got_values_args
if "%~1"=="-/" shift & set "DETAILS=1" & goto get_values_args
if "%~1"=="-c" shift & set "OPTS=%OPTS% /c" & goto get_values_args
if "%~1"=="-f" shift & set "PATTERN=%~1" & shift & goto get_values_args
if not defined KEY set "KEY=%~1" & shift & goto get_values_args
if not defined OUTVAR set "OUTVAR=%~1" & shift & goto get_values_args
:got_values_args
if not defined OUTVAR set "OUTVAR=VALUES"
set "%OUTVAR%="
%ECHOVARS.D% KEY OUTVAR
%UPVAR% %OUTVAR%
set BEFORE=
if "%FULLPATH%"=="1" set "BEFORE=%KEY%\"
:# Use reg.exe to get the key information
set CMD=reg query "%KEY%" /f !PATTERN! !OPTS!
%ECHO.D% %CMD%
:# For each line in CMD output... 
set "SEPARATOR="
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
      if "!NAME!"=="(Default)" set "NAME="
      set "TYPE=%%k"
      set "VALUE=%%l"
      %ECHOVARS.D% NAME TYPE VALUE
      if %DETAILS%==0 (
	set "NAME=!BEFORE!!NAME!"
	call :CondQuote NAME
	set %OUTVAR%=!%OUTVAR%!!SEPARATOR!!NAME!
	set "SEPARATOR= "
      ) else (
      	echo !NAME!/!TYPE!/!VALUE!
      )
    )
  )
)
%RETURN%

:#----------------------------------------------------------------------------#

:# Get a registry value content. Args: KEY NAME [VALUEVAR [TYPEVAR]]
:GetValue
%FUNCTION% enableextensions enabledelayedexpansion
set "KEY="
set "NAME="
set "VALUEVAR="
set "TYPEVAR="
:get_value_args
if "%~1"=="" goto got_value_args
if not defined KEY set "KEY=%~1" & shift & goto get_value_args
if not defined NAME set "NAME=%~1" & shift & goto get_value_args
if not defined VALUEVAR set "VALUEVAR=%~1" & shift & goto get_value_args
if not defined TYPEVAR set "TYPEVAR=%~1" & shift & goto get_value_args
:got_value_args
if not defined VALUEVAR set "VALUEVAR=VALUE"
set "%VALUEVAR%="
:# Returning the type is optional. Do not define a default for TYPEVAR.
%ECHOVARS.D% KEY NAME VALUEVAR TYPEVAR
%UPVAR% %VALUEVAR%
if defined TYPEVAR %UPVAR% %TYPEVAR%
if "%NAME%"=="" (
  set CMD=reg query "%KEY%" /ve
) else (
  set CMD=reg query "%KEY%" /v "%NAME%"
)
%ECHO.D% %CMD%
:# For each line in CMD output...
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
  )
)
set %VALUEVAR%=!VALUE!
if defined TYPEVAR set %TYPEVAR%=%TYPE%
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        ReadPassword					      #
:#                                                                            #
:#  Description     Prompt for a password string, displaying only stars       #
:#                                                                            #
:#  Arguments       VAR="Prompt string"					      #
:#                                                                            #
:#  Notes 	    Returns the password string in variable VAR.              #
:#                                                                            #
:#  History                                                                   #
:#   2017-03-16 APA Published at https://www.dostips.com/forum/viewtopic.php?f=3&t=8442&sid=7d459deb904a629c16a11e6f9bd658be#p55984
:#   2017-03-26 JFL Fixed a problem with the '!' character changed to a ')' in the password string.
:#                                                                            #
:#----------------------------------------------------------------------------#

:ReadPassword var="prompt"

rem Read a password
rem Antonio Perez Ayala

rem Initialize variables
setlocal EnableDelayedExpansion
rem Get a CarriageReturn (ASCII 13) character
for /F %%a in ('copy /Z "%~F0" NUL') do set "CR=%%a"
rem Get a BackSpace (ASCII 8) character
for /F %%a in ('echo prompt $H ^| cmd') do set "BS=%%a"

rem Show the prompt and start reading
set /P "=%~2" < NUL
set "input="
set i=0

rem Get the localized xcopy prompt
set "msg="
for /F "delims=" %%a in ('echo.^|xcopy /W "%~F0" "%~F0" 2^>NUL') do if not defined msg set "msg=%%a"

:ReadPassword.nextKey
   set "key="
   for /F "delims=" %%a in ('xcopy /W "%~F0" "%~F0" 2^>NUL') do if not defined key set "key=%%a"

   rem Remove the localized xcopy prompt from the beginning of the string
   set key=!key:%msg%=!
   rem If the key is a question mark, it'll have been lost in the set "key=%%a" above
   if not defined key set "key=^!"

   rem If key is CR: terminate input
   if "!key:~-1!" equ "!CR!" goto :ReadPassword.endRead

   rem If key is BS: delete last char, if any
   set "key=!key:~-1!"
   if "!key!" equ "!BS!" (
      if %i% gtr 0 (
         set /P "=!BS! !BS!" < NUL
         set "input=!input:~0,-1!"
         set /A i-=1
      )
      goto nextKey
   )

   rem Else: show and accept the key
   set /P "=*" < NUL
   set "input=!input!!key!"
   set /A i+=1

goto :ReadPassword.nextKey

:ReadPassword.endRead
echo/
endlocal & set "%~1=%input%"
exit /B

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Test*                                                     #
:#                                                                            #
:#  Description     Misc test routines for testing the debug library itself   #
:#                                                                            #
:#  Arguments       %*	    Vary                                              #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:TempT
echo set "RETURN=!RETURN!"
echo set "RETURN=%RETURN%"
%ECHOVARS.D% RETURN

for %%f in ("A%%A" "BB" "CC") do @echo f=%%f
  
set A=a
set B=b
set V=A
%ECHOVARS% A B V
echo ^^!%%V%%^^!=!%V%!

set V=B
%ECHOVARS% V
echo ^^!%%V%%^^!=!%V%!

set W=^^!V^^!
setlocal disabledelayedexpansion
echo %%W%%=%W%
endlocal
echo %%W%%=%W%
goto :eof

:ExecHello
%EXEC% echo "Hello world^!"
goto :eof

:TestDelayedExpansion
if .%USERNAME%.==.!USERNAME!. (
  echo Delayed Expansion is ON
) else (
  echo Delayed Expansion is OFF
)
goto :eof

:#----------------------------------------------------------------------------#
:# Factorial routine, to test the tracing framework indentation

:Fact
%FUNCTION% enableextensions enabledelayedexpansion
%UPVAR% RETVAL
set N=%1
if .%1.==.. set N=0
if .%N%.==.0. (
  set RETVAL=1
) else (
  set /A M=N-1
  call :Fact !M!
  set /A RETVAL=N*RETVAL
)
%RETURN%

:Fact.test
%FUNCTION%
call :Fact %*
%ECHO% %RETVAL%
%RETURN%

:#----------------------------------------------------------------------------#
:# Test routines to measure the overhead of call/return

:noop
goto :eof

:noop1
%FUNCTION0%
%RETURN0%

:noop2 %1=retcode
%FUNCTION%
%RETURN% %~1

:noop2d %1=retcode
%FUNCTION% DisableDelayedExpansion
%RETURN% %~1

:noop2e %1=retcode
%FUNCTION% EnableDelayedExpansion
%RETURN% %~1

:noop22 %1=retcode
%FUNCTION%
call :noop2 %~1
%RETURN%

:noop3 %1=retcode %2=string to return in RETVAL
%FUNCTION%
call :extensions.show
%UPVAR% RETVAL
:# Do not use parenthesis, in case there are some in the return value
if "!!"=="" set "RETVAL=!ARGS:* =!"
if not "!!"=="" set "RETVAL=%ARGS:* =%"
%RETURN% %~1

:noop3d %1=retcode %2=string to return in RETVAL
%FUNCTION% DisableDelayedExpansion
call :extensions.show
%UPVAR% RETVAL
set "RETVAL=%~2"
%RETURN% %~1

:noop3e %1=retcode %2=string to return in RETVAL
%FUNCTION% EnableDelayedExpansion
call :extensions.show
%UPVAR% RETVAL
set "RETVAL=%~2"
%RETURN% %~1

:noop33 %1=retcode %2=string to return in RETVAL
%FUNCTION%
call :extensions.show
%UPVAR% RETVAL
if "!!"=="" (
  call :noop3 !ARGS!
) else (
  call :noop3 %ARGS%
)
%RETURN%

:noop4 %1=retcode %2=string to return in RETVAL1 %3=string to return in RETVAL2
%FUNCTION%
%UPVAR% RETVAL1 RETVAL2
set "RETVAL1=%2"
set "RETVAL2=%3"
%RETURN% %~1

:noop4i %1=retcode %2=string to return in RETVAL1 %3=string to return in RETVAL2
%FUNCTION%
if "!!"=="" (echo NOOP4 [EnableExpansion]) else echo NOOP4 [DisableExpansion]
set ARGS
%UPVAR% RETVAL1 RETVAL2
set "RETVAL1=%2"
set "RETVAL2=%3"
set RETVAL1 & set RETVAL2
%RETURN% %~1

:noop4d %1=retcode %2=string to return in RETVAL1 %3=string to return in RETVAL2
%FUNCTION% DisableDelayedExpansion
%IF_XDLEVEL% 1 if "!!"=="" (echo NOOP4D [EnableExpansion]) else echo NOOP4D [DisableExpansion]
%IF_XDLEVEL% 1 set ARGS
%UPVAR% RETVAL1 RETVAL2
set "RETVAL1=%~2"
set "RETVAL2=%~3"
%IF_XDLEVEL% 1 set RETVAL1 & set RETVAL2
%RETURN% %~1

:noop4e %1=retcode %2=string to return in RETVAL1 %3=string to return in RETVAL2
%FUNCTION% EnableDelayedExpansion
%IF_XDLEVEL% 1 if "!!"=="" (echo NOOP4E [EnableExpansion]) else echo NOOP4E [DisableExpansion]
%IF_XDLEVEL% 1 set ARGS
%UPVAR% RETVAL1 RETVAL2
set "RETVAL1=%~2"
set "RETVAL2=%~3"
%IF_XDLEVEL% 1 set RETVAL1 & set RETVAL2
%RETURN% %~1

:noop44 %1=retcode %2=string to return in RETVAL1 %3=string to return in RETVAL2
%FUNCTION%
%IF_XDLEVEL% 1 set ARGS
%UPVAR% RETVAL1 RETVAL2
if "!!"=="" (
  call :noop4 !ARGS!
) else (
  call :noop4 %ARGS%
)
%IF_XDLEVEL% 1 set RETVAL1 & set RETVAL2
%RETURN%

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        EscapeCmdString					      #
:#                                                                            #
:#  Description     Prepare a command for passing through multiple parsings   #
:#                                                                            #
:#  Arguments       %1	Name of the variable containing the command string    #
:#                  %2	Output variable name. Default: Same as input variable #
:#                  %3	Number of parsings to go through. Default: 1          #
:#                  %4	# of the above with !expansion. Default: 1 if exp. on #
:#                                                                            #
:#  Notes 	    The cmd parser tokenizer removes levels of ^ escaping.    #
:#                  This routine escapes a command line, or an argument, so   #
:#                  that special characters like ^ & | > < ( ) make it        #
:#		    through intact through one or more tokenizations.	      #
:#                                                                            #
:#                  Known limitation: The LF character is not managed.        #
:#                                                                            #
:#  History                                                                   #
:#   2019-10-03 JFL Initial implementation                                    #
:#                                                                            #
:#----------------------------------------------------------------------------#

:EscapeCmdString %1=CMDVAR [%2=OUTVAR] [%3=# parsings] [%4=# with !expansion]
for /f "tokens=2" %%e in ("!! 0 1") do setlocal EnableDelayedExpansion & set "CallerExp=%%e"
%ECHO.D% :# :EscapeCmdString called with expansion=%CallerExp%
set "H0=^^"		&:# Return a Hat ^ with QUOTE_MODE 0=off
set "H1=^"		&:# Return a Hat ^ with QUOTE_MODE 1=on
if %CallerExp%==1 set "H0=!H0!!H0!" & set "H1=!H1!!H1!" &:# !escape our return value
set "NPESC=1"			  &:# Default number of %expansion escaping to do
if not "%~3"=="" set "NPESC=%~3"  &:# specified # of extra %expansion escaping to do 
set /a "NXESC=%CallerExp%*NPESC"  &:# Default number of !expansion escaping to do
if not "%~4"=="" set "NXESC=%~4"  &:# specified # of extra !expansion escaping to do
for /l %%i in (1,1,%NXESC%) do set "H0=!H0!!H0!" & set "H1=!H1!!H1!"
for /l %%i in (1,1,%NPESC%) do set "H0=!H0!!H0!"
:# Define characters that need escaping outside of quotes
for %%c in ("<" ">" "|" "&" "(" ")") do set ^"EscapeCmdString.NE[%%c]=1^"
set ^"STRING=!%1!^"
%ECHOVARS.D% STRING H0 H1
set "OUTVAR=%2"
if not defined OUTVAR set "OUTVAR=%1"
set "RESULT="
set "QUOTE_MODE=0"	&:# 1=Inside a quoted string
set "ESCAPE=0"		&:# 1=The previous character was a ^ character
set "N=-1"
:EscapeCmdString.loop
  set /a "N+=1"
  set "C=!STRING:~%N%,1!" &:# Get the Nth character in the string
  %ECHOVARS.D% N C
  if not defined C goto :EscapeCmdString.end
  if "!C!!C!"=="""" (
    if !ESCAPE!==0 (
      set /a "QUOTE_MODE=1-QUOTE_MODE"
    ) else ( :# Open " quotes can be escaped, but not close " quotes
      if "!QUOTE_MODE!"=="0" set "RESULT=!RESULT!!H0:~1!"
    )
  ) else if "!C!"=="^" (
    if "!QUOTE_MODE!"=="0" set /a "ESCAPE=1-ESCAPE"
    set "RESULT=!RESULT!!H%QUOTE_MODE%:~1!"
  ) else if "!C!"=="^!" (
    set "RESULT=!RESULT!!H%QUOTE_MODE%:~1!"
  ) else if defined EscapeCmdString.NE["!C!"] ( :# Characters that need escaping outside of quotes
    if "!QUOTE_MODE!"=="0" set "RESULT=!RESULT!!H0:~1!"
  )
  if not "!C!"=="^" set "ESCAPE=0"
  set "RESULT=!RESULT!!C!"
  %ECHOSVARS.D% RESULT
goto :EscapeCmdString.loop
:EscapeCmdString.end
endlocal & set ^"%OUTVAR%=%RESULT%^" ! = &:# The ! forces always having !escaping ^ removal in delayed expansion mode
exit /b

:ParseDelayedExpansion %1=OUTVAR %2=0|1|off|on|Disable|Enable
goto :ParseDelayedExpansion.%~2
:ParseDelayedExpansion.0
:ParseDelayedExpansion.off
:ParseDelayedExpansion.disable
:ParseDelayedExpansion.DisableDelayedExpansion
set "%1=DisableDelayedExpansion"
exit /b
:ParseDelayedExpansion.
:ParseDelayedExpansion.1
:ParseDelayedExpansion.on
:ParseDelayedExpansion.enable
:ParseDelayedExpansion.EnableDelayedExpansion
set "%1=EnableDelayedExpansion"
exit /b

:# Convert the supported html entities to their corresponding character
:# Internal subroutine that does not create a setlocal frame. Only uses !expansion!.
:ConvertEntities.internal %1=INPUTVAR [%2=OUTPUTVAR]
set "OUTVAR=%2"
if not defined OUTVAR set "OUTVAR=%1"
set "ARG=!%1!"
%ECHOSVARS.D% 1 ARG
for %%e in (quot lt gt amp vert rpar lpar rbrack lbrack sp bs cr lf hat) do (
  for %%c in ("!DEBUG.%%e!") do set "ARG=!ARG:[%%e]=%%~c!"
)
%ECHOSVARS.D% 2 ARG
:# Then convert special characters that need special attention
:# The ! character cannot be substituted in a !variable! substitution
:# So use the % character instead
set "ARG=!ARG:%%=[percnt]!" &:# Make sure there are no % characters in ARG
set "ARG2=!ARG:[excl]=%%!"
if not "!ARG2!"=="!ARG!" ( :# If ARG does contain ! characters
  set "ARG="		  &:# Then individually convert each % to an !
  set "N=0"
  :ConvertEntities.loop
    set "C=!ARG2:~%N%,1!"
    if not defined C goto :ConvertEntities.end_loop
    if "!C!"=="%%" set "C=^!"
    set "ARG=!ARG!!C!"
    set /a "N+=1"
    goto :ConvertEntities.loop
  :ConvertEntities.end_loop
  rem
)
set "ARG=!ARG:[percnt]=%%!" &:# Convert % characters back to their real value
%ECHOSVARS.D% 3 ARG
set "%OUTVAR%=!ARG:[lbrack]=[!" &:# Must be converted last
exit /b

:ConvertEntities %1=INPUTVAR [%2=OUTPUTVAR]
for /f "tokens=2" %%e in ("!! 0 1") do setlocal EnableDelayedExpansion & set "CallerExp=%%e"
%ECHO.D% :# :ConvertEntities called with expansion=%CallerExp%
call :ConvertEntities.internal %1 %2
call :EscapeCmdString ARG ARG 1 %CallerExp%
endlocal & set ^"%OUTVAR%=%ARG%^" ! &:# The ! forces always having !escaping ^ removal in delayed expansion mode
goto :eof

:TestConvertEntities %1=VAR [%2=0|1=Disable|Enable Delayed Expansion]
setlocal EnableDelayedExpansion
%POPARG%
set "INPUT=!ARG!"
set INPUT
%POPARG%
call :ParseDelayedExpansion EXP=!ARG! & echo # !EXP! & setlocal !EXP!
call :ConvertEntities INPUT OUTPUT
set OUTPUT
endlocal & endlocal
exit /b

:TestEscapeCmdString %1=VAR [%2=EXPANSION] [%3=# parsings] [%4=# with !expansion]
setlocal EnableDelayedExpansion
%POPARG%
call :ConvertEntities.internal ARG _INITIAL
:# set ^"_INITIAL=!_INITIAL:Q="!^"
:# for %%r in ("H=^" "A=&" "O=|" "G=>" "L=<") do set "_INITIAL=!_INITIAL:%%~r!"
set _INITIAL
%POPARG%
call :ParseDelayedExpansion EXP=!ARG! & echo # !EXP! & setlocal !EXP!
call :EscapeCmdString _INITIAL _ESCAPED !ARGS!
set _ESCAPED
set ^"REPARSED=%_ESCAPED%^" ! = &:# The ! forces always having !escaping ^ removal in delayed expansion mode
set REPARSED
%POPARG%
set "NPARSE=1"
if defined ARG set "NPARSE=%ARG%"
:TestEscapeCmdString.loop
if %NPARSE%==1 goto :TestEscapeCmdString.done
set ^"REPARSED=%REPARSED%^" ! = &:# The ! forces always having !escaping ^ removal in delayed expansion mode
set REPARSED
set /a "NPARSE-=1"
goto :TestEscapeCmdString.loop
:TestEscapeCmdString.done
endlocal & endlocal
exit /b

:#----------------------------------------------------------------------------#
:# Test %EXEC% one command line. Display start/end time if looping.

:exec_cmd_line
%CMD_BEFORE%
set ^"CMDLINE=!ARGS!^"
call :ConvertEntities CMDLINE
if not %NLOOPS%==1 echo Start at %TIME% & set "T0=%TIME%"
for /l %%n in (1,1,%NLOOPS%) do %EXEC% !CMDLINE!
if not %NLOOPS%==1 echo End at %TIME% & set "T1=%TIME%"
if not %NLOOPS%==1 call :Time.Delta %T0% %T1% -f & echo Delta = !DH!:!DM!:!DS!.!DMS:~0,2!
%CMD_AFTER%
goto :eof

:#----------------------------------------------------------------------------#
:# Test call one command line. Display start/end time if looping.
:# Do not add anything to the inner do loop, such as echoing the command, as this
:# would prevent from doing accurate measurements of the duration of the command.

:call_cmd_line
%CMD_BEFORE%
set ^"CMDLINE=!ARGS!^"
call :ConvertEntities CMDLINE
if not %NLOOPS%==1 echo Start at %TIME% & set "T0=%TIME%"
for /l %%n in (1,1,%NLOOPS%) do call !CMDLINE!
if not %NLOOPS%==1 echo End at %TIME% & set "T1=%TIME%"
if not %NLOOPS%==1 call :Time.Delta %T0% %T1% -f & echo Delta = !DH!:!DM!:!DS!.!DMS:~0,2!
%CMD_AFTER%
goto :eof

:call_macro_line
%CMD_BEFORE%
%POPARG%
set ^"CMDLINE=%%%ARG%%% !ARGS!^"
call :ConvertEntities CMDLINE
%ECHOVARS.D% CMDLINE
if not %NLOOPS%==1 echo Start at %TIME% & set "T0=%TIME%"
for /l %%n in (1,1,%NLOOPS%) do call !CMDLINE!
if not %NLOOPS%==1 echo End at %TIME% & set "T1=%TIME%"
if not %NLOOPS%==1 call :Time.Delta %T0% %T1% -f & echo Delta = !DH!:!DM!:!DS!.!DMS:~0,2!
%CMD_AFTER%
goto :eof

:#----------------------------------------------------------------------------#
:# Test call N command lines. Display start/end time if looping.
:# Do not add anything to the inner do loop, such as echoing the command, as this
:# would prevent from doing accurate measurements of the duration of the commands.

:call_all_cmds
%IF_XDLEVEL% 3 set FUNCTION & set UPVAR & set RETURN &:# Dump the structured programming macros
:# Record all commands to run, converting entities to special characters
set NCMDS=0
:call_all_cmds.next_arg
%POPARG%
if not defined "ARG" goto :call_all_cmds.done_args
set /a NCMDS+=1
call :ConvertEntities ARG
%IF_XDLEVEL% 2 set ARG | findstr ARG=
set "CMD[%NCMDS%]=!ARG!"
goto :call_all_cmds.next_arg
:call_all_cmds.done_args
if defined CMD_BEFORE call :ConvertEntities CMD_BEFORE
if defined CMD_AFTER call :ConvertEntities CMD_AFTER
%IF_DEBUG% %>DEBUGOUT% (
  if defined CMD_BEFORE set CMD_BEFORE
  set CMD[
  if defined CMD_AFTER set CMD_AFTER
)
:# Run all commands in a loop, measuring the total duration when looping more than once
if defined CMD_BEFORE !CMD_BEFORE!
if not %NLOOPS%==1 echo Start at %TIME% & set "T0=%TIME%"
for /l %%n in (1,1,%NLOOPS%) do for /l %%c in (1,1,%NCMDS%) do call %%CMD[%%c]%% &:# Don't use !CMD[]! in case one command disables expansion
if not %NLOOPS%==1 echo End at %TIME% & set "T1=%TIME%"
if not %NLOOPS%==1 call :Time.Delta %T0% %T1% -f & echo Delta = !DH!:!DM!:!DS!.!DMS:~0,2!
if defined CMD_AFTER !CMD_AFTER!
goto :eof

:# Short aliases for common before & after commands
:EDE
:EDX
setlocal EnableDelayedExpansion
goto :eof

:DDE
:DDX
setlocal DisableDelayedExpansion
goto :eof

:#----------------------------------------------------------------------------#
:# Test %FUNCTION0% / %RETURN0%

:Func0
%FUNCTION0%
%ECHO% This is function 0
call :Func1
%RETURN0%
%ECHO% Failed to return from :Func0
exit /b

:Func1
%FUNCTION0%
%ECHO% This is function 1
%RETURN0%
%ECHO% Failed to return from :Func1
exit /b

:Func#0
%FUNCTION0%
%ECHO% This is function #0
call :Func#1
%RETURN#% Returning from function #0
%ECHO% Failed to return from :Func#0
exit /b

:Func#1
%FUNCTION0%
%ECHO% This is function #1
%RETURN#% Returning from function #1
%ECHO% Failed to return from :Func#1
exit /b

:#----------------------------------------------------------------------------#
:# Test %EXEC% and errorlevels, on entry and exit

:echoErr
%ECHO% set "ERRORLEVEL=%ERRORLEVEL%"
exit /b

:testErrorLevel
for /l %%n in (0,1,2) do (
  echo.
  call :Exec.SetErrorLevel %%n
  echo :# Calling with ERRORLEVEL=!ERRORLEVEL!
  %EXEC% call :testErrorLevelCallback
  echo :# Returned with ERRORLEVEL=!ERRORLEVEL!
)
exit /b 0

:testErrorLevelCallback
set ERROR=%ERRORLEVEL%
%ECHOVARS% ERROR
exit /b %ERROR%

:#----------------------------------------------------------------------------#
:# Test returning tricky characters

:testR2
%FUNCTION% EnableDelayedExpansion
%UPVAR% V1
%UPVAR% S
%UPVAR% V2
set "V1=%~1"
set "S=!STRING!"
set "V2=%~2"
%ECHO% :# In testR2
%ECHOVARS% V1 S V2
%RETURN%

:testR
%FUNCTION% EnableDelayedExpansion
set "STRING=@||&&(())<<>>^^^^,,;;  %%%%^!^!**??[[]]==~~''""%%CD%%_^!CD^!""
call :testR2 "With EnableDelayedExpansion" "last but not least"
%ECHO% :# In testR
%ECHOVARS% V1 S V2

set "S="
set "V1="
set "V2="

setlocal DisableDelayedExpansion
set  STRING=@^|^|^&^&(())^<^<^>^>^^^^,,;;  %%%%^!^!**??[[]]==~~''""%%CD%%_!CD!"
call :testR2 "With DisableDelayedExpansion" "last but not least"
%ECHO% :# In testR
%ECHOVARS% V1 S V2
endlocal

%RETURN%

:#----------------------------------------------------------------------------#
:# Test passing tricky characters

:testC
%FUNCTION% EnableDelayedExpansion
:# set "STRING=@(())^^^^,,;;  %%%%^!^!**??[[]]==~~''""%%CD%%_^!CD^!"
:# set STRING=!STRING!"@||&&(())<<>>^^,,;;  %%%%^!^!**??[[]]==~~''%%CD%%_^!CD^!"
set "STRING0=@||&&(())<<>>^^^^,,;;  %%%%^!^!**??[[]]==~~''%%CD%%_^!CD^!"
set "STRING1=@(()),,;;  %%%%^!^!**??[[]]==~~''""%%CD%%_^!CD^!" &:# Remove ||&&^^<<>> that cause problems when not quoted
set STRING=!STRING1! "!STRING0!"
set STRING
@echo on
call :testC2 !STRING:%%=%%%%!
%RETURN%

:testC2
@echo off
echo :# First show what a normal call lets go through
%_DO% setlocal DisableDelayedExpansion
@echo on
set ARGLINE=%*
set ARG#1=%1
set ARG#2=%2
set ARG#3=%3
set ARG#4=%4
@echo off
set ARGLINE
set ARG#

%ECHO% :# Idem with initial ^^caret
@echo on
set ^"ARGLINE=%*^"
set ^"ARG#1=%1^" &:# yes
set ^"ARG#2=%2^"
set ^"ARG#3=%3^"
set ^"ARG#4=%4^"
@echo off
set ARGLINE
set ARG#
echo echo ARG#1={%ARG#1%}
echo echo ARG#2={%ARG#2%}
endlocal

%_DO% setlocal EnableDelayedExpansion
@echo on
set ^"ARGLINE=%*^"
set ARG#1=%1
set ARG#2=%2
set ARG#3=%3
set ARG#4=%4
set ARGLINE
@echo off
set ARG#
endlocal

echo :# Now show what this library FUNCTIONS can do!
%FUNCTION% EnableDelayedExpansion
set ARGS
set "STRING0="
set "STRING1="
set "STRING=!ARGS!"
set STRING
%_DO% setlocal DisableDelayedExpansion
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
endlocal

%_DO% setlocal EnableDelayedExpansion
set ARGS=!STRING!
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
%POPARG%
%ECHOVARS% ARG ^""ARG"^"
endlocal
%RETURN%

:# Test preparing tricky arguments for expansion
:testP
%FUNCTION% EnableDelayedExpansion
:# set "STRING=@(())^^^^,,;;  %%%%^!^!**??[[]]==~~''""%%CD%%_^!CD^!"
:# set STRING=!STRING!"@||&&(())<<>>^^,,;;  %%%%^!^!**??[[]]==~~''%%CD%%_^!CD^!"
set "STRING0=@||&&(())<<>>^^^^,,;;  %%%%^!^!**??[[]]==~~''%%CD%%_^!CD^!"
set "STRING1=@(()),,;;  %%%%^!^!**??[[]]==~~''""%%CD%%_^!CD^!" &:# Remove ||&&^^<<>> that cause problems when not quoted
set STRING=!STRING1! "!STRING0!"
set STRING
setlocal DisableDelayedExpansion
call :Prep2ExpandVars STRING
endlocal
%RETURN%

:#----------------------------------------------------------------------------#
:# Test relative performances of two possible ways to do indirect variable expansion

:call_set
call set "X=%%windir%%"
goto :eof

:call_:set
call :set "X=%%windir%%"
goto :eof

:set
set %*
goto :eof

:#----------------------------------------------------------------------------#

:testLogFunc
%FUNCTION%
%ECHO% This is myFunc
%POPARG%
set "RET=!ARG!"
%POPARG%
set "VAR=!ARG!"
if defined VAR %UPVAR% !VAR! & %POPARG% & set "!VAR!=!ARG!"
%RETURN% !RET!

:testLog
call :Debug.SetLog t.log

call :testLogFunc
call :testLogFunc 0
call :testLogFunc 1
call :testLogFunc 0 RESULT Zero
call :testLogFunc 0 TOTO "0 + 0 = La tête à Toto"
set "STRING=@||&&(())<<>>^^^^,,;;  %%%%^!^!**??[[]]==~~''%%CD%%_^!CD^!"
call :testLogFunc 0 RESULT "!STRING:%%=%%%%!"
exit /b 0

:#----------------------------------------------------------------------------#

:test_do

set "VAR=BEFORE"
%ECHOVARS% VAR
%_DO% setlocal EnableExtensions
set "VAR=AFTER"
%ECHOVARS% VAR
%_DO% endlocal
%ECHOVARS% VAR

%ECHO%
%ECHOVARS% CD
%_DO% pushd "%TEMP%"
%ECHOVARS% CD
%_DO% popd
%ECHOVARS% CD

goto :eof

:#----------------------------------------------------------------------------#
:# Test relative performances of various ways to return
:# (And the conclusion is that they're all pretty equivalent)

:gotoeof
goto :eof

:exit
exit /b

:exit/b
exit /b %1

:# Test relative performances of various macros

:test_true
%TRUE.EXE%
exit /b

:test_false
%FALSE.EXE%
exit /b

:test_false0
%FALSE0.EXE%
exit /b

:#----------------------------------------------------------------------------#

:test_pipe0
cmd /c "exit /b"
exit /b

:test_pipe1
cmd /c "cmd /c break 1>&4 4>&6 | cmd /c break 0>&3 3>&6"
exit /b

:test_pipe2
cmd /c "doskey 1>&4 4>&6 | doskey 0>&3 3>&6"
exit /b

:test_pipe3
cmd /c "rundll32 1>&4 4>&6 | rundll32 0>&3 3>&6"
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
echo %SCRIPT% version %VERSION% - JFL cmd.exe Batch Library tests
echo.
echo Usage: %SCRIPT% [OPTIONS]
echo.
echo Options:
echo   -?       Display this help
echo   --       End of wrapper options
echo   -a CMDLINE         Call the command line once after the -c/-C commands (1)
echo   -b CMDLINE         Call the command line once before the -c/-C commands (1)
echo   -c CMDLINE1 ...    Call each following argument as a separate cmd. (1)
echo   -C CMD ARGS        Call the whole command tail as one command line
echo   -d       Debug mode. Trace functions entry and exit
echo   -d2      Send debug output to stderr instead of stdout
echo   -e       Display all arguments and exit
echo   -E CMD ARGS        %%EXEC%% the whole command tail as one command line
echo   -l LOG   Set the log file name
echo   -M MACRO ARGS      Call %%MACRO%% and pass it ARGS
echo   -n N     Run the commands N times and display the start and end times
echo   -qe      Query the current cmd extensions and delayed expansion settings
echo   -r       Test %%EXEC%% with an output redirection to exec.log
echo   -R       Test %%EXEC%% without an output redirection
echo   -te CMDLINE [EXP] [#PARSE] [#w.EXP]  Test escaping a command line. (1)
echo            EXP=0^|1 or off^|on : Delayed ^^!expansion^^!. Default: on
echo            #PARSE : Number of parsings to escape for. Default: 1
echo            #w.EXP : Number of those with delayed ^^!expansion^^!. Default: 0^|#PARSE
echo   -v       Verbose mode. Display commands executed
echo   -V       Display the script version and exit
echo   -X       Display commands to execute, but don't execute them
echo.
echo Notes:
echo 1) The following html entity names, within brackets, will be converted to their
echo    corresponding character:
echo    [percnt]=%% [excl]=^^^! [quot]=" [Hat]=^^ [lt]=< [gt]=> [amp]=& [vert]=| [lpar]=( [rpar]=) [lbrack]=[ [rbrack]=]
goto :eof

:#----------------------------------------------------------------------------#
:# Main routine

:Main
set "NLOOPS=1"
set "CMD_AFTER="
set "CMD_BEFORE="
set "CMDLINE=!ARG0! !ARGS!"
:next_arg
if not defined ARGS set "ARG=" & set ""ARG"=" & goto :Start
%POPARG%
%ECHOVARS.D% ARG ARGS
if "!ARG!"=="-?" goto :Help
if "!ARG!"=="/?" goto :Help
if "!ARG!"=="-a" %POPARG% & set "CMD_AFTER=!ARG!" & goto next_arg
if "!ARG!"=="-b" %POPARG% & set "CMD_BEFORE=!ARG!" & goto next_arg
if "!ARG!"=="-c" goto :call_all_cmds
if "!ARG!"=="-C" goto :call_cmd_line
if "!ARG!"=="-d" call :Debug.On & %ECHOVARS% CMDLINE ARG ARGS & goto next_arg
if "!ARG!"=="-d0" set ">DEBUGOUT=>NUL" & call :Debug.On & goto next_arg	&:# Useful for library performance measurements
if "!ARG!"=="-d1" set ">DEBUGOUT=>&3" & call :Debug.On & goto next_arg	&:# Useful to test debug output routines to 
if "!ARG!"=="-d2" set ">DEBUGOUT=>&2" & call :Debug.On & goto next_arg	&:# Useful to test debug output routines
if "!ARG!"=="-e" goto EchoArgs
if "!ARG!"=="-E" goto :exec_cmd_line
if "!ARG!"=="-l" %POPARG% & call :Debug.SetLog "!ARG!" & goto next_arg
if "!ARG!"=="-M" goto :call_macro_line
if "!ARG!"=="-n" %POPARG% & set "NLOOPS=!ARG!" & goto next_arg
if "!ARG!"=="-qe" endlocal & (set ECHO=echo) & goto :extensions.show
if "!ARG!"=="-r" call :Debug.Setlog test.log & %EXEC% cmd /c %SCRIPT% -? ">"exec.log & goto :eof
if "!ARG!"=="-R" call :Debug.Setlog test.log & %EXEC% cmd /c %SCRIPT% -? & goto :eof
if "!ARG!"=="-tc" goto :TestConvertEntities &:# Test routine :ConvertEntities
if "!ARG!"=="-te" goto :TestEscapeCmdString &:# Test routine :EscapeCmdString
if "!ARG!"=="-tg" %POPARG% & call :GetServerAddress !ARG! & %ECHOVARS% ADDRESS & goto :eof &:# Test routine GetServerAddress
if "!ARG!"=="-v" call :Verbose.On & goto next_arg
if "!ARG!"=="-V" (echo.%VERSION%) & goto :eof
if "!ARG!"=="-X" call :Exec.Off & goto next_arg
if "!ARG!"=="-xd" %POPARG% & set "XDLEVEL=!ARG!" & goto next_arg
if "!ARG:~0,1!"=="-" (
  >&2 %ECHO% Warning: Unexpected option ignored: !ARG!
  goto :next_arg
)
>&2 %ECHO% Warning: Unexpected argument ignored: !"ARG"!
goto :next_arg

:#----------------------------------------------------------------------------#
:# Start the real work

:Start
:# This library does nothing. Display the help screen.
goto :Help

:# The following line, used by :Echo.Color, must be last and not end by a CRLF.
-