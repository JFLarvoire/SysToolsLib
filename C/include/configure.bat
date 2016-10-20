@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.bat					      *
:#                                                                            *
:#  Description:    Detect system-specific settings and create config.bat     *
:#                                                                            *
:#  Notes:	    Named by analogy with Unix' ./configure scripts.	      *
:#                                                                            *
:#                  This file is intended to contain only general rules for   *
:#                  locating Microsoft development tools, and a limited set   *
:#                  of common software development kits (SDKs) and libraries. *
:#                  If other tasks are needed, either for project-specific    *
:#                  settings, or for user-specific settings, add additional   *
:#                  configure.XXX.bat scripts. This script will include them  *
:#                  all, after locating Microsoft tools, and before writing   *
:#                  Microsoft tools locations to config.%HOSTNAME%.bat.       *
:#                                                                            *
:#                  Having %HOSTNAME% as part of the config.bat file name     *
:#                  allows testing builds on the same source tree, accessed   *
:#                  from multiple VMs.                                        *
:#                                                                            *
:#                  Invoked automatically by make.bat if config.%HOSTNAME%.bat*
:#		    is missing.                                               *
:#                                                                            *
:#                  Invoke manually if anything changes in the tools config,  *
:#                  such as installing a Visual Studio update, or updating    *
:#                  a configure.XXX.bat script.                               *
:#                                                                            *
:#                  In each project directory, there should be a file called  *
:#                  configure.%PROJECT%.bat, that contains project-specific   *
:#                  definitions.                                              *
:#                  In that file, use the macro %CONFIG% to add files to      *
:#                  config.%HOSTNAME%.bat. Example:                           *
:#                  %CONFIG% set "MAKEFILE=project.mak"                       *
:#                  To add a blank line, use:		                      *
:#		    %CONFIG%.				                      *
:#                                                                            *
:#                  Will also add include paths and library paths if these    *
:#                  variables are declared using a %USE_SDK% macro:           *
:#                  MSVCLIBX	        JFL's Microsoft C library extensions  *
:#                  SYSLIB 	        JFL's System library                  *
:#                  98DDK               Windows 98 DDK                        *
:#                  BOOST               Boost C++ library   www.boost.org     *
:#                  PTHREADS            www.sourceware.org/pthreads-win32     *
:#                  LMPTK               Lan Manager 2.1 Programmer's ToolKit  *
:#                  GNUEFI              gnu-efi.sourceforge.net w. JFL patches*
:#                  Example:                                                  *
:#                  %BEGIN_SDK_DEFS%                                          *
:#                  %USE_SDK% MSVCLIBX                                        *
:#                  %USE_SDK% PTHREADS                                        *
:#                  %END_SDK_DEFS%                                            *
:#                                                                            *
:#                  Will also search these libraries, but won't update	      *
:#		    include paths and library paths:			      *
:#                  BIOSLIB	        JFL's BIOSLIB library                 *
:#                  LODOSLIB	        JFL's LODOSLIB library                *
:#                  PMODE	        JFL's PMODE library                   *
:#                  Example:                                                  *
:#                  %BEGIN_SDK_DEFS%                                          *
:#                  %USE_SDK% MSVCLIBX                                        *
:#                  %USE_SDK% BIOSLIB                                        *
:#                  %USE_SDK% LODOSLIB                                        *
:#                  %END_SDK_DEFS%                                            *
:#                                                                            *
:#                  It's possible to override the default location for these  *
:#                  SDKs by defining your own path in a configure.USER.bat.   *
:#                  Example:                                                  *
:#                  set "MSVCLIBX=D:\My\Tools\MSVCLIBX"                       *
:#                  set "PTHREADS=S:\Shared\Libs\Pthreads2"                   *
:#                                                                            *
:#                  Finally configure.bat will search all SDKs in a directory *
:#                  called %MY_SDKS%. You may set that variable in your       *
:#                  configure.USER.bat file as well.                          *
:#                                                                            *
:#                  A macro called %ADD_POST_CONFIG_ACTION% allows defining   *
:#                  commands that run _after_ all configure.*.bat files are   *
:#                  processed, and just before the variables16/32/64 are      *
:#                  written to config.%HOSTNAME%.bat.                         *
:#                  This allows to correct of undo something that's not       *
:#                  acceptable in the default settings. Example:              *
:#                  %ADD_POST_CONFIG_ACTION% set "PATH16=%PATH16%" ^&:# Undo  *
:#                  %ADD_POST_CONFIG_ACTION% set "PATH32=%PATH32%" ^&:# Undo  *
:#                                                                            *
:#                  A macro called %ADD_POST_MAKE_ACTION% allows defining     *
:#                  commands that run _after_ make.bat ends.                  *
:#                  This allows to set global variables, etc. Example:        *
:#                  %ADD_POST_MAKE_ACTION% set "MYLIB=%MYLIB%"		      *
:#                  %ADD_POST_MAKE_ACTION% setx MYLIB %MYLIB% ^>NUL	      *
:#                                                                            *
:#                  To do: Redesign to use the actual vcvars.bat scripts for  *
:#                  the various compilers, and capture their output.          *
:#                  * This would allow getting all exact paths needed.        *
:#                  * This would work even with newer Visual Studio versions. *
:#                  * This would allow supporting alternate compilers, like   *
:#                    the IA64 compiler, or the x86_* cross-compilers.        *
:#                                                                            *
:#  History:                                                                  *
:#   2014-04-21 JFL Split off the make.bat script.			      *
:#   2014-05-19 JFL Make sure configure.*.bat override scripts are called     *
:#                  last, just before generating config.bat. This ensures     *
:#                  that anything done in configure.bat can be overridden.    *
:#   2015-01-13 JFL Fixed support for the old Visual Studio 6.0.	      *
:#                  Added options -vc and -vs to manually override base PATHs.*
:#   2015-01-14 JFL Rename config.bat as config.%COMPUTERNAME%.bat.	      *
:#   2015-01-21 JFL Redesigned the 2014-05-29 change to give a chance to      *
:#                  change things both before and after the %98DDK% to        *
:#                  %MSVCLIBX% variables handling.                            *
:#   2015-10-15 JFL Do not create config.bat if one of the child scripts fails.
:#   2015-10-16 JFL Improved the MASM and MSVC search and configuration.      *
:#   2015-10-19 JFL Added my standard macro and debugging frameworks.         *
:#		    Define an SDK_LIST, and only define variables for them.   *
:#   2015-10-30 JFL Generalized the variables names as %OS%.VARIABLE.         *
:#                  Added methods and hooks for searching tools for building  *
:#                  other OS targets, like WIN95, WINXP, IA64, ARM.           *
:#   2015-11-12 JFL Output new-style variables for all OS types.              *
:#   2015-11-13 JFL Removed most old-style variables definitions.             *
:#   2015-12-04 JFL We can use long names for MsvcLibX-specific paths.        *
:#   2016-01-11 JFL Bugfix for VS community 2015: Improved search for UCRT.   *
:#   2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
:#   2016-04-13 JFL Display libraries found in the output summary.            *
:#                  Generate a list of HAS_<lib> flags for the C compiler.    *
:#   2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
:#   2016-07-12 JFL Added option -o to set the %OUTDIR% path.		      *
:#   2016-08-24 JFL Added CONV variable definition for the conv.exe tool.     *
:#   2016-09-01 JFL Merged in the latest batch debugging library updates.     *
:#   2016-09-05 JFL Updated macro %USE_SDK% to avoid recording duplicates.    *
:#		    Fixed macros ADD_POST_MAKE_ACTION & ADD_POST_MAKE_ACTION. *
:#                  Fixed a warning when running on Chinese Windows.          *
:#   2016-09-15 JFL For each VC version, record the WinSDK include directory. *
:#   2016-09-27 JFL Make sure the configure.*.bat scripts are invoked in a    *
:#		    predictable order: The alphabetic order.		      *
:#                  Work around bug in old nmake test of "Program Files (x86)".
:#   2016-09-28 JFL Also search for configure.*.bat in %windir% and %HOME%.   *
:#   2016-10-04 JFL Clarified a warning message.			      *
:#   2016-10-06 JFL Bug fix: Avoid failure if no SDK flags are defined.       *
:#   2016-10-07 JFL Bug fix: Avoid double \ in Visual Studio base path.       *
:#                  Bug fix: When invoked from recursive nmake files,         *
:#                  variables %ProgramFiles% and %PROCESSOR_ARCHITECTURE% are *
:#                  reset to their x86 values.                                *
:#                  Improvement: Also look for library paths in the master env.
:#   2016-10-11 JFL Adapted for use in SysToolsLib global C include dir.      *
:#   2016-10-19 JFL Bug fix: Make sure the exit code is 0 when successful.    *
:#   2016-10-20 JFL Added a workaround for an XP/64 bug causing build failure.*
:#                                                                            *
:#        © Copyright 2016 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal enableextensions enabledelayedexpansion
set "VERSION=2016-10-20"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
set  ARGS=%*
:# FOREACHLINE macro. (Change the delimiter to none to catch the whole lines.)
set FOREACHLINE=for /f "delims="

set "POPARG=call :PopArg"
call :Macro.Init
call :Debug.Init
call :Exec.Init
if not defined HOME set "HOME=%HOMEDRIVE%%HOMEPATH%"
goto Main

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

set "MACRO=for %%$ in (1 2) do if %%$==2"			&:# Prolog code of a macro
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
:#                  %LOGFILE%       Log file name. Inherited. Default=NUL.    #
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
:#  Variables       %LOGFILE%	Log file name.                                #
:#                  %NOEXEC%	Exec mode. 0=Execute commands; 1=Don't. Use   #
:#                              functions Exec.Off and Exec.On to change it.  #
:#                              Inherited from the caller. Default=On.	      #
:#                  %NOREDIR%   0=Log command output to the log file; 1=Don't #
:#                              Default: 0                                    #
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
if not .%NOREDIR%.==.1. set "NOREDIR=0"
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

:# Execute a command, logging its output.
:# Use for informative commands that should always be run, even in NOEXEC mode. 
:Do
setlocal EnableExtensions DisableDelayedExpansion
set NOEXEC=0
set "IF_NOEXEC=if .%NOEXEC%.==.1."
goto :Exec.Start

:# Execute critical operations that should not be run in NOEXEC mode.
:# Version supporting input and output redirections, and pipes.
:# Redirection operators MUST be surrounded by quotes. Ex: "<" or ">" or "2>>"
:Exec
setlocal EnableExtensions DisableDelayedExpansion
:Exec.Start
:# Save the initial errorlevel: Build a command for restoring it later
if errorlevel 2 ( :# For complex errors, use a sub-shell. Drawback: This is slow.
  set "Exec.RestoreErr=%COMSPEC% /c exit %ERRORLEVEL%"
) else if errorlevel 1 ( :# For the common error 1, use the quick %FALSE.EXE% trick
  set "Exec.RestoreErr=%FALSE.EXE%"
) else ( :# For no error, use the quick %TRUE% trick
  set "Exec.RestoreErr=%TRUE.EXE%"
)
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
  if .%Exec.HaveTee%.==.1. set "Exec.Redir= 2>&1 | tee.exe -a %LOGFILE%"
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
  %Exec.RestoreErr% &:# Restore the errorlevel we had on :Exec entrance
  %Exec.Cmd%%Exec.Redir%
  call :Exec.ShowExitCode
)
goto :eof

:Exec.ShowExitCode
%IF_DEBUG% %>DEBUGOUT% echo.%INDENT%  exit %ERRORLEVEL%
if defined LOGFILE %>>LOGFILE% echo.%INDENT%  exit %ERRORLEVEL%
exit /b %ERRORLEVEL%

:Exec.End

:#----------------------------------------------------------------------------#
:#                        End of the debugging library                        #
:#----------------------------------------------------------------------------#

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Reg.Xxxxx						      #
:#                                                                            #
:#  Description     Registry management functions                             #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Get a registry value content. Args: KEY NAME [VALUEVAR [TYPEVAR]]
:Reg.GetValue
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
%FOREACHLINE% %%i in ('%CMD% 2^>NUL') do (
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
:#  Function        list						      #
:#                                                                            #
:#  Description     List management routines                                  #
:#                                                                            #
:#  Arguments                                                                 #
:#                                                                            #
:#  Notes 	    Inspired from Tcl list management routines                #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Append an element to a list
:# %1 = Input/Output variable name
:# %2 = element value
:lappend
:# %FUNCTION0%
if defined %~1 call set "%~1=%%%~1%% "
call set "%~1=%%%~1%%"%~2""
:# %ECHOVARS.D% "%~1"
:# %RETURN0%
goto :eof

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Find16, Find32, Find64                                    #
:#                                                                            #
:#  Description     Find Microsoft development tools                          #
:#                                                                            #
:#  Notes                                                                     #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:# Convert a long path name to its corresponding short path name
:long2short %1=LongNameVar %2=ShortNameVar
for %%n in ("!%~1!") do set "%~2=%%~sn"
goto :eof

:paths2short %1=LongNameVar %2=ShortNameVar
setlocal EnableDelayedExpansion
set "SHORT="
if defined "%~1" (
  set LONG=!%~1!
  for /f "delims=" %%f in ('"echo.!LONG:;=&echo.!"') do SET "SHORT=!SHORT!|%%~sf|"
  set "SHORT=%SHORT:||=;%"
  set "SHORT=%SHORT:|=%"
)
endlocal & set "%~2=%SHORT%" & goto :eof

:#----------------------------------------------------------------------------#
:# Find Microsoft's 16-bits development tools

:find16
%FUNCTION0%
:# Microsoft assembler
:# If specified on the command line, and looking reasonably valid, then use it.
if not "%MASM%" == "" if exist "%MASM%\BIN\ML.EXE" goto :find16.found_masm
:# Else try the default MASM installation path, and a few likely alternatives
for /d %%p in (C:\MASM* \MASM* "%PF32%\MASM*" "%PF64%\MASM*") do (
  if exist "%%~p\BIN\ML.EXE" pushd "%%~p" & set "MASM=!CD!" & popd & goto :find16.found_masm
)
set "MASM=" & set "VC16.AS=" & goto :find16.done_masm
:find16.found_masm
set VC16.AS="%MASM%\BIN\ML.EXE"
echo DOS	AS	x86	!VC16.AS!
:find16.done_masm

:# 16-bits Microsoft Visual C++ tools
:# If specified on the command line, and looking reasonably valid, then use it.
if not "%MSVC%" == "" if exist "%MSVC%\BIN\CL.EXE" set "VC16=%MSVC%" & goto :find16.found_msvc
:# Else try the default MSVC 1.x installation path, and a few likely alternatives
for /d %%p in (C:\MSVC* \MSVC* "%PF32%\MSVC*" "%PF64%\MSVC*") do (
  if exist "%%~p\BIN\CL.EXE" pushd "%%~p" & set "VC16=!CD!" & popd & goto :find16.found_msvc
)
set "VC16=" & set "VC16.CC=" & goto :find16.done_msvc
:find16.found_msvc
call :long2short VC16 VC16~S
SET "VC16.BIN=%VC16%\BIN"
SET "VC16.PATH=%VC16%\BIN"
SET VC16.CC="%VC16%\BIN\CL.EXE"
SET "VC16.INCPATH=%VC16%\INCLUDE"
SET "VC16.VCINC=%VC16%\INCLUDE" &:# Path of MSVC compiler include files, for use by MsvcLibX
SET "VC16.CRTINC=%VC16%\INCLUDE" &:# Path of MSVC CRT library include files, for use by MsvcLibX
SET VC16.LK="%VC16%\BIN\LINK.EXE"
SET "VC16.LIBPATH=%VC16%\LIB"
SET VC16.LB="%VC16%\BIN\LIB.EXE"
SET VC16.RC="%VC16%\BIN\RC.EXE"
SET VC16.MS="%VC16%\BIN\MAPSYM.EXE"
:# Note: The WinICE SDK had an improved version of mapsym, called msym.exe. (Supporting symbols in 32-bits segments?)
:# SET VC16.MS=C:\SDK\WINICE\MSYM.EXE
:# Note: Mapsym is an MS-DOS only tool, that cannot be used on a WIN64 host.
:# So if we're running on a WIN64 host, redefine it as rem (=no op).
if not "%PROGRAMFILES(x86)%"=="" SET VC16.MS=rem
:find16.done_msvc

if defined VC16.CC %ECHO% DOS	CC	x86	%VC16.CC%
%RETURN#% "VC16.CC=%VC16.CC%"

:#-----------------------------------------------------------------------------
:# Find Microsoft Visual Studio

:# Find the latest Visual Studio version supporting the specified OS & architecture
:findvs %1=OS %2=Proc (x86|x64|amd|...) %3=VSNAME %4=VCNAME
%FUNCTION0%
set "VS=%~3"
set "VC=%~4"
if /i "%~2"=="x86" (
  set SEARCH_IN=call :FindVsIn %~2 "bin"
) else if /i "%~2"=="AMD64" (
  if /i %ARCH%==AMD64 ( :# Do not test %PROCESSOR_ARCHITECTURE%, which is reset in recursive nmake runs
    set SEARCH_IN=call :FindVsIn %~2 "bin\amd64 bin\x86_amd64"
  ) else (
    set SEARCH_IN=call :FindVsIn %~2 "bin\x86_amd64"
  )
) else (
  if /i %ARCH%==AMD64 ( :# Do not test %PROCESSOR_ARCHITECTURE%, which is reset in recursive nmake runs
    set SEARCH_IN=call :FindVsIn %~2 "bin\amd64_%~2 bin\x86_%~2"
  ) else (
    set SEARCH_IN=call :FindVsIn %~2 "bin\x86_%~2"
  )
)
set "NEEDSHORTPATH=0"
:# If specified on the command line, and looking reasonably valid, then use it.
if defined VSTUDIO call :FindVsIn "%VSTUDIO%" && goto :foundvs
:# If VS' vcvars*.bat has already been run manually, then use it.
if defined VSINSTALLDIR call :FindVsIn "%VSINSTALLDIR%" && goto :foundvs
:# Else scan all Visual Studio versions, starting from the newest ones.
goto :lastvs &:# Scan all Visual Studio versions

:findvs95	&:# Find the latest Visual Studio version supporting Win95 development
%FUNCTION0%
set "VS=VS95"
set "VC=VC95"
set "SEARCH_IN=call :FindVsIn x86 bin"
goto :lastvs95	&:# Skip all Visual Studio versions that don't support Win95 development anymore

:findvsXP	&:# Find the latest Visual Studio version supporting WinXP development
%FUNCTION0%
set "VS=VSXP"
set "VC=VSXP"
set "SEARCH_IN=call :FindVsIn x86 bin"
goto :lastvsXP	&:# Skip all Visual Studio versions that don't support WinXP development anymore

:FindVsIn %1=Proc %2="SUBDIRS" %3=DIRNAME	&:# Test if Visual Studio is present in the proposed directory
%ECHO.D% Searching the %1 compiler in %3
set "%VS%=" & set "%VC%=" & set "%VC%.BIN="
:# The Visual C++ subdirectory can be named VC, VC98, or VC7 depending on the VS version.
for %%s in (%~2) do (
  for /d %%d in ("%PF64%\%~3\VC*" "%PF32%\%~3\VC*") do if exist "%%d\%%s\cl.exe" (
    set "%VS%=%%~dpd" &:# Remove the VC* subdir name
    set "%VS%=!%VS%:~0,-1!" &:# Remove the trailing '\'.
    set "%VC%=%%d"
    set "%VC%.BIN=%%d\%%s"
    set "%VC%.CC="%%d\%%s\cl.exe""
    exit /b 0
  )
)
exit /b 1

:lastvs								    &:#		Min SUBSYSVER	Default SUBSYSVER
%SEARCH_IN% "Microsoft Visual Studio 16.0"	&& goto :foundvs    &:# VS 16
%SEARCH_IN% "Microsoft Visual Studio 15.0"	&& goto :foundvs    &:# VS 15
%SEARCH_IN% "Microsoft Visual Studio 14.0"	&& goto :foundvs    &:# VS 2015
:# %SEARCH_IN% "Microsoft Visual Studio 13.0"	&& goto :foundvs    &:# N/A
:lastvsXP
%SEARCH_IN% "Microsoft Visual Studio 12.0"	&& goto :foundvs    &:# VS 2013	5.01		6.00
%SEARCH_IN% "Microsoft Visual Studio 11.0"	&& goto :foundvs    &:# VS 2012	5.00		5.00
%SEARCH_IN% "Microsoft Visual Studio 10.0"	&& goto :foundvs    &:# VS 2010	5.00		5.00
%SEARCH_IN% "Microsoft Visual Studio 9.0"	&& goto :foundvs    &:# VS 2008	5.00		5.00
:lastvs95
%SEARCH_IN% "Microsoft Visual Studio 8"		&& goto :foundvs    &:# VS 2005	4.00		5.00
%SEARCH_IN% "Microsoft Visual Studio .NET 2003"	&& goto :foundoldvs &:# VS 7.1	4.00		4.00 
%SEARCH_IN% "Microsoft Visual Studio .NET"	&& goto :foundoldvs &:# VS 7.0
%SEARCH_IN% "Microsoft Visual Studio"		&& goto :foundoldvs &:# VS 6
SET "%VS%="
%RETURN0%

:foundoldvs
set "NEEDSHORTPATH=1" &:# Work around bug in old nmake handling of !IF EXIST("Program Files (x86)")
:foundvs
:# Find Visual Studio Common Files, which can have one of two names.
:# The common files subdirectory can be named Common or Common7 depending on the VS version.
SET "%VS%.COMMON="
if exist "!%VS%!\Common*" for %%d in (Common7 Common) do if exist "!%VS%!\%%d\Tools" set "%VS%.COMMON=!%VS%!\%%d"
if not defined %VS%.COMMON %RETURN0%

:# Find Visual Studio IDE Files, which can have one of two names.
set "%VS%.IDE=!%VS%.COMMON!\msdev98\BIN" &:# The location for Visual Studio 6.0
if not exist "!%VS%.IDE!" SET "%VS%.IDE=!%VS%.COMMON!\IDE" &:# The location for all subsequent versions. 
:# Important note: Subdirectory IDE _does_ exist for VS6.0, but is mostly empty.

:# Find Visual Studio Tools, which can have 1 or 2 locations, the 2nd of which having several possible names.
set "%VS%.TOOLS=!%VS%.COMMON!\Tools"
for %%b in (Bin Bin\WinNT WinNT) do ( :# *WinNT = Names for old VS versions with alternate versions for Win95
  if exist "!%VS%.COMMON!\Tools\%%b" SET "%VS%.TOOLS=!%VS%.COMMON!\Tools\%%b;!%VS%.TOOLS!"
)

%RETURN#% "%VS%=!%VS%!"

:#-----------------------------------------------------------------------------
:# Find the Windows SDK
:# Early SDKs were called PlatformSDK, and included in Visual Studio.
:# Then they were called Windows SDKs, and optionally included libraries for multiple platforms=processors
:# Then they were called Windows Kits, with a different tree structure.
:findsdk 
%FUNCTION0%
SET WINSDK=
set "WINSDKMIN=NONE"
set "WINSDKMAX="
set "WINSDKPROC=x86"
set ARGS=%*
:next_winsdk_arg
%POPARG%
if not defined ARG goto :done_sdk_arg
if /i "!ARG!"=="-min" %POPARG% & set "WINSDKMIN=!ARG!" & goto :next_winsdk_arg
if /i "!ARG!"=="-max" %POPARG% & set "WINSDKMAX=!ARG!" & goto :next_winsdk_arg
if /i "!ARG!"=="-platform" %POPARG% & set "WINSDKPROC=!ARG!" & goto :next_winsdk_arg
:done_sdk_arg
:# Search new Windows Kits.
for %%k in (10 8.1) do (
  if defined WINSDKMAX if %%k==!WINSDKMAX! set "WINSDKMAX="
  if not defined WINSDKMAX if defined WINSDKMIN for %%p in (%PF64AND32%) do (
    if not defined WINSDK call :FindWkIn "%%~p\Windows Kits\%%k"
  )
  if defined WINSDKMIN if %%k==!WINSDKMIN! set "WINSDKMIN="
)
:# Search older Microsoft Windows SDKs
if not defined WINSDK for %%k in (v8.1A v8.1 v8.0A v8.0 v7.1A v7.1 v7.0A v7.0 v6.1 v6.0A v6.0 v5.2 v5.1 v5.0) do (
  if defined WINSDKMAX if %%k==!WINSDKMAX! set "WINSDKMAX="
  if not defined WINSDKMAX if defined WINSDKMIN for %%p in (%PF64AND32%) do (
    if not defined WINSDK call :FindSdkIn "%%~p\Microsoft SDKs\Windows\%%k"
  )
  if defined WINSDKMIN if %%k==!WINSDKMIN! set "WINSDKMIN="
)
:# Search even older SDKs
if not defined WINSDK (
  call :FindSdkIn "%PF32%\Microsoft SDK" &rem :# Location for the 2001-08 SDK (Notice it's SDK without an s)
)
if not defined WINSDK (
  call :FindSdkIn "!%VC%!\PlatformSDK" &rem :# Else try using the oldest ones, coming with Visual Studio
)
if not defined WINSDK (
  %ECHO.D% :# Not Found
) else (
  :# Record the SDK version
  set "WINSDKVER="
  for %%p in ("%WINSDK%") do set "WINSDKVER=%%~nxp"
)

if defined WINSDK if "%NEEDSHORTPATH%"=="1" (
  :# Work around bug in old nmake handling of !IF EXIST("Program Files (x86)")
  call :long2short WINSDK_INCDIR WINSDK_INCDIR
  %ECHOVARS.D% WINSDK_INCDIR
)

if defined WINSDK echo %TOS%	WinSDK	%WINSDKPROC%	"%WINSDK%"
%RETURN#% "WINSDK=%WINSDK%"

:# Find old-style Windows SDKs
:FindSdkIn %1=Base directory to search in
%ECHO.D% Searching in "%~1"
if /i "!WINSDKPROC!"=="x86" (
  set "SUBDIR=%~1\lib"
) else (
  set "SUBDIR=%~1\lib\!WINSDKPROC!"
)
if exist "!SUBDIR!\kernel32.lib" (
  %ECHO.D% :# Found
  set "WINSDK=%~1"
  set "WINSDK_BIN=!SUBDIR:\Lib=\Bin!"
  set "WINSDK_INCDIR=!WINSDK!\Include"
  set "WINSDK_INCLUDE=!WINSDK_INCDIR!"
  set "WINSDK_LIBDIR=!SUBDIR!"
  set "WINSDK_LIB=!WINSDK_LIBDIR!"
  %ECHOVARS.D% WINSDK WINSDK_INCDIR WINSDK_INCLUDE WINSDK_LIB
)
goto :eof

:# Find New-style Windows Kits
:FindWkIn %1=Base directory to search in
%ECHO.D% Searching in "%~1"
for /d %%l in ("%~1\Lib" "%~1\Lib\*") do if exist "%%~l\um\!WINSDKPROC!\kernel32.lib" (
  %ECHO.D% :# Found
  set "WINSDK=%~1"
  set "WINSDK_LIBDIR=%%~l\um\!WINSDKPROC!"
  set "WINSDK_BIN=!WINSDK!\Bin\!WINSDKPROC!"
  set "WINSDK_INCDIR="
  set "WINSDK_INCLUDE="
  for /d %%d in ("!WINSDK!\Include" "!WINSDK!\Include\*") do ( :# Pre-release kits have an additional subdir level
    if exist "%%~d\um\windows.h" (
      set "WINSDK_INCDIR=%%~d"
      for %%s in (ucrt shared um winrt) do (
	if exist "!WINSDK_INCDIR!\%%~s" set "WINSDK_INCLUDE=!WINSDK_INCLUDE!;!WINSDK_INCDIR!\%%s"
      )
      set "WINSDK_INCLUDE=!WINSDK_INCLUDE:~1!" &rem :# Remove the initial ; inserted above
    )
  set "WINSDK_LIB=!WINSDK_LIBDIR!"
  )
  %ECHOVARS.D% WINSDK WINSDK_SUBDIR WINSDK_INCDIR WINSDK_INCLUDE WINSDK_LIB
)
goto :eof

:#-----------------------------------------------------------------------------
:# Find 32 or 64-bits Microsoft Visual C++

:findvc32 %1=OS %2=Proc %3=VSVAR %4=VCVAR
%FUNCTION0%
set "TOS=%~1" & if not defined TOS set "TOS=WIN32"
set "PROC=%~2" & if not defined PROC set "PROC=x86"
set "VS=%~3" & if not defined VS set "VS=VS32"
set "VC=%~4" & if not defined VC set "VC=VC32"
call :findvs %TOS% %PROC% %VS% %VC%
:findvc32.common
if not defined %VS% set "%VC%="
if not defined %VC% %RETURN0%
%ECHO% %TOS%	CC	%PROC%	!%VC%.CC!
%RETURN#% "%VC%=!%VC%!"

:findvc95
%FUNCTION0%
set "TOS=WIN95"
set "PROC=x86"
set "VS=VS95"
set "VC=VC95"
call :findvs95 WIN95 %PROC% %VS% %VC%
goto :findvc32.common

:#-----------------------------------------------------------------------------
:# Find Microsoft 32 or 64-bits development tools

:find32
%FUNCTION0%
call :findvc32 WIN32 x86 VS32 VC32
call :findsdk -Platform %PROC%
if not defined VC32 %RETURN0%
goto :find32.common

:find95
%FUNCTION0%
call :findvc95 WIN95 x86 VS95 VC95
call :findsdk -Platform %PROC% -Max 7.1a
if not defined VC95 %RETURN0%
goto :find32.common

:findia64
%FUNCTION0%
call :findvc32 IA64 IA64 VSIA64 VCIA64
call :findsdk -Platform %PROC%
if not defined VCIA64 %RETURN0%
goto :find32.common

:find64
%FUNCTION0%
call :findvc32 WIN64 amd64 VS64 VC64
call :findsdk -Platform x64 &:# Unfortunately this one is named differently in VC and WinSDK
if not defined VC64 %RETURN0%
goto :find32.common

:findarm
%FUNCTION0%
call :findvc32 ARM arm VSARM VCARM
call :findsdk -Platform %PROC%
if not defined VCARM %RETURN0%
goto :find32.common

:findarm64
%FUNCTION0%
call :findvc32 ARM64 arm64 VSARM64 VCARM64
call :findsdk -Platform %PROC%
if not defined VCARM64 %RETURN0%
goto :find32.common

:FindUCRT
%FUNCTION0%
set "UCRT="
set "UCRT.VER="
:# Search new Windows Kits.
for %%p in (%PF64AND32%) do ( if not defined UCRT (
  for %%k in (10 8.1) do ( if not defined UCRT (
    for /f %%d in ('dir /b /on "%%~p\Windows Kits\%%k\Include" 2^>NUL') do ( :# Do not test UCRT here, as we want to use the last build
      %ECHO.D% Searching in "%%~p\Windows Kits\%%k\Include\%%~d"
      if exist "%%~p\Windows Kits\%%k\Include\%%~d\ucrt\stdio.h" (
      	set "UCRT=%%~p\Windows Kits\%%k"
      	set "UCRT.VER=%%~d"
      )
    )
  ))
))
%RETURN#% "UCRT=!UCRT!  UCRT.VER=!UCRT.VER!"

:find32.common
:# MSVC 32-bits compiler, linker, and librarian
if not defined %VC%.BIN set "%VC%.BIN=!%VC%!\BIN"
set "%VC%.PATH=!%VC%.BIN!"
for %%d in ("!%VC%.BIN!") do ( :# For BIN dirs like amd64_arm
  for /f "tokens=1,2 delims=_" %%a in ('echo %%~nxd') do ( :# Split the 2 halves
    :# Append the first half [amd64 in the above example] to the path
    if not "%%b"=="" set "%VC%.PATH=!%VC%.PATH!;%%~dpd%%a"
  )
)
set "%VC%.PATH=!%VC%.PATH!;!%VS%.IDE!;!%VS%.TOOLS!"
set %VC%.CC="!%VC%.BIN!\CL.EXE"
set %VC%.LK="!%VC%.BIN!\LINK.EXE"
set %VC%.LB="!%VC%.BIN!\LIB.EXE"
set "%VC%.AS=" &:# The assembler name varies, depending on the processor
for %%a in (ML ML64 ARMASM IA64ASM) do if not defined %VC%.AS (
  if exist "!%VC%.BIN!\%%a.EXE" set %VC%.AS="!%VC%.BIN!\%%a.EXE"
)

SET "%VC%.INCPATH=!%VC%!\include"
set "%VC%.VCINC=!%VC%.INCPATH!" &:# Path of MSVC compiler include files, for use by MsvcLibX
set "%VC%.CRTINC=" &:# Path of MSVC CRT library include files, for use by MsvcLibX
if exist "!%VC%.INCPATH!\stdio.h" set "%VC%.CRTINC=!%VC%.VCINC!" &:# Up to VS2013 it's in VC, then it's in Windows Kit UCRT
SET "%VC%.LIBPATH=!%VC%!\lib\%PROC%"
if /i %PROC%==x86 if not exist "!%VC%.LIBPATH!\kernel32.lib" SET "%VC%.LIBPATH=!%VC%!\lib"

:# Windows SDK tools
set TRYDIRS="!%VS%.IDE!" &:# Tools location for Visual Studio 6
if defined WINSDK (
  set "%VC%.WINSDK=!WINSDK!"
  set "%VC%.WINSDKINC=!WINSDK_INCDIR!"
  set "%VC%.INCPATH=!%VC%.INCPATH!;!WINSDK_INCLUDE!"
  set "%VC%.LIBPATH=!%VC%.LIBPATH!;!WINSDK_LIB!"
  set TRYDIRS=!TRYDIRS! "%WINSDK_BIN%"
  if /i not %PROC%==x86 (
    for %%b in ("%WINSDK_BIN%") do set TRYDIRS=!TRYDIRS! "%%~dpbx86"
    set TRYDIRS=!TRYDIRS! "%WINSDK%\Bin%"
  )
  set TRYDIRS=!TRYDIRS! "!%VS%.TOOLS:;=" "!" &rem :# Old PlatformSDKs have SDK tools there
  if not defined %VC%.CRTINC for %%d in ("!WINSDK_INCLUDE:;=" "!") do if not "%%~d"=="" (
    if exist "%%~d\stdio.h" set "%VC%.CRTINC=%%~d" %# Path of MSVC CRT library include files, for use by MsvcLibX #%
    :# Also check for the corresponding CRT libraries
    for /d %%l in ("!WINSDK!\Lib" "!WINSDK!\Lib\*") do if exist "%%~l\ucrt\!WINSDKPROC!\*crt*.lib" (
      set "%VC%.LIBPATH=!%VC%.LIBPATH!;%%~l\ucrt\!WINSDKPROC!"
    )
  )
)
%ECHOVARS.D% TRYDIRS

:# The Universal CRT may be in a different kit from that of the Windows SDK
if not defined %VC%.CRTINC ( :# VS version >= 2015 and UCRT not in WINSDK 
  call :FindUCRT
  if defined UCRT (
    set "%VC%.UCRT=!UCRT!\include\!UCRT.VER!\ucrt"
    if exist "!UCRT!\include\!UCRT.VER!\ucrt\stdio.h" (
      set "%VC%.CRTINC=!UCRT!\include\!UCRT.VER!\ucrt"
      set "%VC%.INCPATH=!%VC%.INCPATH!;!%VC%.CRTINC!"
    )
    if exist "!UCRT!\lib\!UCRT.VER!\ucrt\%WINSDKPROC%\*crt*.lib" set "%VC%.LIBPATH=!%VC%.LIBPATH!;!UCRT!\lib\!UCRT.VER!\ucrt\%WINSDKPROC%"
  )
)
if not "!%VC%.CRTINC!"=="!%VC%.VCINC!" ( :# VS version >= 2015
  if defined %VC%.CRTINC ( :# OK
    echo %TOS%	UCRT	%PROC%	"!%VC%.CRTINC!"
  ) else ( :# We did not find the required UCRT
    echo %TOS%	UCRT	%PROC%	"*** ERROR: UCRT NOT FOUND ***"
  )
)

:# Resource Compiler
set "%VC%.RC="
for %%p in (%TRYDIRS%) do (
  if not defined %VC%.RC if exist "%%~p\RC.EXE" set %VC%.RC="%%~p\RC.EXE"
)
:# Manifest Tool
SET "%VC%.MT="
for %%p in (%TRYDIRS%) do (
  if not defined %VC%.MT if exist "%%~p\MT.EXE" set %VC%.MT="%%~p\MT.EXE"
)

%RETURN#% "%VC%.CC=!%VC%.CC!"

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        Main                                                      #
:#                                                                            #
:#  Description     Process command line arguments, and main routine body     #
:#                                                                            #
:#  Arguments       %*	    Command line arguments                            #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#                                                                            #
:#----------------------------------------------------------------------------#

:help
echo %SCRIPT% - Create the %CONFIG.BAT% script used by make.bat
echo.
echo Usage: %SCRIPT% [options]
echo.
echo Options:
echo   -?^|-h         This help
echo   -c CONFIG     Name the output file config.CONFIG.bat
echo   -d            Debug mode. Display internal variables and function calls
echo   -masm PATH    Path to MASM install dir. Default: C:\MASM
echo   -msvc PATH    Path to MSVC 16-bits tools install dir. Default: C:\MSVC
echo   -o OUTDIR     Output base directory. Default: .
echo   -v            Verbose mode. Display what this script does
echo   -vs PATH      Path to Visual Studio install dir. Default: Latest avail.
echo   -V            Display %SCRIPT% version
echo.
exit /b 0

:#-----------------------------------------------------------------------------

:main
set "CONFIG.BAT=config.%COMPUTERNAME%.bat"
set "CONFIG=>>%CONFIG.BAT% echo"
set "MASM="
set "MSVC="
set "VSTUDIO="

:next_arg
%POPARG%
if "!ARG!"=="" goto go
if "!ARG!"=="-?" goto help
if "!ARG!"=="/?" goto help
if "!ARG!"=="-c" %POPARG% & set "CONFIG.BAT=config.!ARG!.bat" & goto next_arg
if "!ARG!"=="-d" call :Debug.On & call :Verbose.On & goto next_arg
if "!ARG!"=="-h" goto help
if "!ARG!"=="-masm" %POPARG% & set "MASM=!ARG!" & goto next_arg
if "!ARG!"=="-msvc" %POPARG% & set "MSVC=!ARG!" & goto next_arg
if "!ARG!"=="-o" %POPARG% & set "OUTDIR=!ARG!" & goto next_arg
if "!ARG!"=="-v" call :Verbose.On & goto next_arg
if "!ARG!"=="-vs" %POPARG% & set "VSTUDIO=!ARG!" & goto next_arg
if "!ARG!"=="-V" (echo %VERSION%) & goto :eof
>&2 echo Unexpected argument ignored: !"ARG"!
goto next_arg

:go
:# Delete %CONFIG.BAT% before rebuilding it
if exist %CONFIG.BAT% del %CONFIG.BAT%
%CONFIG% :# %CONFIG.BAT% generated by %SCRIPT% on %DATE% %TIME%
%CONFIG% :#
%CONFIG% :# If changes are needeed, do not edit this file, but instead create a new script
%CONFIG% :# called configure.YOURCHOICE.bat. This new script will be invoked automatically
%CONFIG% :# by configure.bat while creating this file. Then your script can write extra
%CONFIG% :# definitions, or change some of the variables before configure.bat writes them.
%CONFIG% :#
%CONFIG% :# Invoke configure.bat manually if anything changes in the tools config, such as
%CONFIG% :# installing a Visual Studio update, or updating a configure.XXX.bat script.

:# Find Program Files directories
:# Gotcha: When invoked recursively by nmake, both %ProgramFiles% and %ProgramFiles(x86)%
:# point at the x86 version. This breaks the 64-bits programs detection. The workaround
:# is to rely on the fact that the PF32 and PF64 variables defined here are left unchanged.
:# First checking for symlinks C:\Pgm32 and C:\Pgm64, which I had on some of my systems.
if not defined PF32   set "PF32=C:\Pgm32"
if not exist "%PF32%" set "PF32=%ProgramFiles(x86)%"
if not exist "%PF32%" set "PF32=%ProgramFiles%"

if not defined PF64   set "PF64=C:\Pgm64"
if not exist "%PF64%" set "PF64=%ProgramFiles%"

if "%PF32%"=="%PF64%" (
  set PF64AND32="%PF32%"
) else (
  set PF64AND32="%PF64%" "%PF32%"
)

:# Identify the native PROCESSOR_ARCHITECTURE
:# Gotcha: When invoked recursively by nmake, both %PROCESSOR_ARCHITECTURE% is reset to x86.
if not defined ARCH set "ARCH=%PROCESSOR_ARCHITECTURE%"

:# Find Microsoft development tools directories
echo OS	Tool	Proc	Path
call :find16	&:# Find 16-bits development tools
call :find95	&:# Find Windows 95/NT4 development tools
call :find32	&:# Find 32-bits development tools
call :find64	&:# Find 64-bits development tools
:# call :findXP	&:# Find Windows XP 32-bits development tools
:# call :findMips  &:# Find Mips development tools
:# call :findAlpha  &:# Find Alpha development tools
:# call :findPower  &:# Find PowerPC development tools
call :findIA64  &:# Find IA-64 development tools
call :findArm	&:# Find ARM development tools
call :findArm64	&:# Find ARM64 development tools

:# Manage a list of known SDKs, that we'll include further down in the build variables
set "SDK_LIST=STINCLUDE" &:# List of variable names, defining the SDK install directories.
:# Macro, for use in configure.*.bat scripts, to easily add variables to %SDK_LIST%
set USE_SDK=%MACRO% ( %\n%
  for %%a in (%!%MACRO.ARGS%!%) do ( %\n%
    set "USE_SDK_TMP_LIST= %'!%SDK_LIST%'!% " %\n%
    if "%'!%USE_SDK_TMP_LIST: %%a =%'!%"=="%'!%USE_SDK_TMP_LIST%'!%" ( %\n%
      if defined SDK_LIST set "SDK_LIST=%'!%SDK_LIST%'!% " %\n%
      set "SDK_LIST=%'!%SDK_LIST%'!%%%a" %\n%
    ) %\n%
  ) %\n%
  %ON_MACRO_EXIT% set "SDK_LIST=%'!%SDK_LIST%'!%" %/ON_MACRO_EXIT% %\n%
) %/MACRO%
%IF_DEBUG% set USE_SDK &:# Display the macro, for debugging changes

:# Define a series of commands, separated by &, to run further down before generating the main sections of config.h
set "¡¡=¡¡¡¡" &:# Use %¡¡% (Two inverted exclamation marks - Required for DBCS languages) instead of ! for declaring delayed expansion variables
set "POST_CONFIG_ACTIONS="
:# Macro, for use in configure.*.bat scripts, to easily add commands to %POST_CONFIG_ACTIONS%
set ADD_POST_CONFIG_ACTION=%MACRO% ( %\n%
  if defined POST_CONFIG_ACTIONS set "POST_CONFIG_ACTIONS=%'!%POST_CONFIG_ACTIONS%'!% %&% " %\n%
  %ON_MACRO_EXIT% set "POST_CONFIG_ACTIONS=%'!%POST_CONFIG_ACTIONS%'!%%'!%MACRO.ARGS:~1%'!%" %/ON_MACRO_EXIT% %\n%
) %/MACRO%
%IF_DEBUG% set ADD_POST_CONFIG_ACTION &:# Display the macro, for debugging changes

:# Define a series of commands, separated by &, to run after make exits
set "POST_MAKE_ACTIONS="
:# Macro, for use in configure.*.bat scripts, to easily add commands to %POST_MAKE_ACTIONS%
set ADD_POST_MAKE_ACTION=%MACRO% ( %\n%
  if defined POST_MAKE_ACTIONS set "POST_MAKE_ACTIONS=%'!%POST_MAKE_ACTIONS%'!% %&% " %\n%
  %ON_MACRO_EXIT% set "POST_MAKE_ACTIONS=%'!%POST_MAKE_ACTIONS%'!%%'!%MACRO.ARGS:~1%'!%" %/ON_MACRO_EXIT% %\n%
) %/MACRO%
%IF_DEBUG% set ADD_POST_MAKE_ACTION &:# Display the macro, for debugging changes

:# Call other local and project-specific configure scripts, possibly overriding all the above
:# Must be placed before the following commands, to allow defining %MSVCLIBX%, %SYSLIB%, %98DDK%, %BOOST%, %PTHREADS%
:# Make sure the files are invoked in a predictable order: The alphabetic order.
for %%d in ("%windir%" "%HOME%" ".") do (
  %FOREACHLINE% %%f in ('dir /b /o "%%~d\configure.*.bat" 2^>NUL') do (
    %TRUE.EXE% &:# Clear the errorlevel in the likely case that the batch does not do it
    %DO% call "%%~d\%%~f"
    if errorlevel 1 (
      set "ERROR=!ERRORLEVEL!"
      del %CONFIG.BAT%
      exit /b !ERROR!
    )
  )
)
%ECHOVARS.D% SDK_LIST

:# Known SDKs:
set "SDK.STINCLUDE.NAME=System Tools global C includes"
set "SDK.STINCLUDE.DIR=INCLUDE"
set "SDK.STINCLUDE.FILE=debugm.h"

set "SDK.BIOSLIB.NAME=BIOS Library"
set "SDK.BIOSLIB.FILE=clibdef.h"

set "SDK.LODOSLIB.NAME=Low DOS Library"
set "SDK.LODOSLIB.FILE=dosdrv.h"

set "SDK.PMODE.NAME=x86 Protected Mode library"
set "SDK.PMODE.FILE=pmode.h"

set "SDK.SYSLIB.NAME=System Library"
set "SDK.SYSLIB.FILE=oprintf.h"

set "SDK.MSVCLIBX.NAME=MSVC Library eXtensions library"
set "SDK.MSVCLIBX.FILE=include\msvclibx.h"

set "SDK.LMPTK.NAME=LanManager Programmer's ToolKit"
set "SDK.LMPTK.FILE=DOS\NETSRC\H\lan.h"

set "SDK.98DDK.NAME=Windows 98 Device Driver Kit"
set "SDK.98DDK.FILE=INC\Win98\vmm.h"

set "SDK.GNUEFI.NAME=gnu-efi sources"
set "SDK.GNUEFI.DIR=gnu-efi"
set "SDK.GNUEFI.FILE=inc\efi.h"

set "SDK.PTHREADS2.NAME=Posix Threads for Windows"
set "SDK.PTHREADS2.FILE=include\pthread.h"

set "SDK.BOOST.NAME=Boost C++ libraries"
set "SDK.BOOST.FILE=boost\preprocessor.hpp"

:# Search for the requested SDKs in the specified dir, then the default install dir, then in other likely places
%CONFIG%.
set "HAS_SDK_FLAGS="
if defined SDK_LIST for %%v in (%SDK_LIST%) do (
  set "DIR=%%v"
  if defined SDK.%%v.DIR set "DIR=!SDK.%%v.DIR!"
  %ECHO.V% Searching %%v
  set "INDENT=!INDENT!  "
  set "PATH_LIST="
  :# When doing automated builds, the previously built libraries bases are set in the master environment,
  :# but not available in parent nmake environment.
  if not defined %%v call :Reg.GetValue HKCU\Environment %%v %%v :# Get value from the master environment in the registry
  if defined %%v call :lappend PATH_LIST "!%%v!"
  if defined MY_SDKS for %%s in (%MY_SDKS%) do call :lappend PATH_LIST "%%~s\!DIR!"
  call :lappend PATH_LIST ..\!DIR!
  call :lappend PATH_LIST "%PF64%\!DIR!"
  if not "%PF32%"=="%PF64%" call :lappend PATH_LIST "%PF32%\!DIR!"
  set "%%v=" &:# Delete the output variable.
  if defined SDK.%%v.FILE ( :# Search for that reference file
    for %%d in (!PATH_LIST!) do if not defined %%v (
      %ECHO.V%   Searching in %%d
      if exist "%%~d\!SDK.%%v.FILE!" pushd "%%~d" & set "%%v=!CD!" & popd
    )
    if defined %%v (
      %ECHO.V%   Found
      %CONFIG% set "HAS_%%v=1" ^&:# Found the !SDK.%%v.NAME!
      set "HAS_SDK_FLAGS=!HAS_SDK_FLAGS! /DHAS_%%v=1"
      set "TAB=	" &:# A tabulation
      set "V=%%v"
      if not "!V:~7!"=="" set "TAB= " &rem A space
      echo C	%%v!TAB!	"!%%v!"
    ) else (
      %ECHO% Warning: Can't find the !SDK.%%v.NAME! ^(%%v^). Dependant modules won't be built.
      %CONFIG% set "HAS_%%v=" ^&:# Did not find the !SDK.%%v.NAME!
    )
    %CONFIG% set "%%v=!%%v!" ^&:# !SDK.%%v.NAME!
  ) else (
    %ECHO% Warning: Unknown SDK or library %%v. Please update variables manually.
    %CONFIG% set "%%v="
  )
  set "INDENT=!INDENT:~0,-2!"
)
if defined HAS_SDK_FLAGS set "HAS_SDK_FLAGS=%HAS_SDK_FLAGS:~1%"
%CONFIG% set "HAS_SDK_FLAGS=%HAS_SDK_FLAGS%" ^&:# SDK detection flags for the C compiler

:# Update 16-bits include and library paths for well-known libraries
for %%v in (VC16) do if defined %%v (
  for %%k in (%SDK_LIST%) do if defined %%k (
    :# Do not configure BIOSLIB, LODOSLIB, PMODE variables at this stage, as they'll be needed for BIOS builds only
    if "%%k"=="SYSLIB" ( :# System library
      SET "%%v.INCPATH=!%%v.INCPATH!;%SYSLIB%"
      set "%%v.LIBPATH=!%%v.LIBPATH!;%SYSLIB%\$(BR)"
    )
    if "%%k"=="MSVCLIBX" ( :# MSVC Library eXtensions library
      SET "%%v.INCPATH=%MSVCLIBX%\include;!%%v.INCPATH!" &:# Include MsvcLibX's _before_ MSVC's own include files
      SET "%%v.LIBPATH=%MSVCLIBX%\lib;!%%v.LIBPATH!"
    )
    if "%%k"=="LMPTK" ( :# LanManager 2.1 Programmer's ToolKit
      SET "%%v.INCPATH=!%%v.INCPATH!;%LMPTK%\DOS\NETSRC\H"
    )
    if "%%k"=="98DDK" ( :# Windows 98 DDK
      :# Gotcha: Do not use %98DDK% here, as it's not expanded correctly in set xxxx16 commands
      SET "PATH16=!PATH16!;!98DDK!\bin"
      SET "%%v.INCPATH=!%%v.INCPATH!;!98DDK!\inc;!98DDK!\inc\win98"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;!98DDK!\lib"
    )
    if "%%k"=="GNUEFI" ( :# gnu-efi sources
      SET "%%v.INCPATH=!%%v.INCPATH!;%GNUEFI%\inc"
    )
    if "%%k"=="BIOSLIB" ( :# BIOS library
      SET "%%v.INCPATH=!%%v.INCPATH!;%BIOSLIB%"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;%BIOSLIB%"
    )
    if "%%k"=="LODOSLIB" ( :# Low DOS library
      SET "%%v.INCPATH=!%%v.INCPATH!;%LODOSLIB%"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;%LODOSLIB%"
    )
    if "%%k"=="PMODE" ( :# Protected Mode library
      SET "%%v.INCPATH=!%%v.INCPATH!;%PMODE%"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;%PMODE%"
    )
  )
)

:# Update x86 include and library paths for well-known libraries
for %%v in (VC95 VC32) do if defined %%v (
  for %%k in (%SDK_LIST%) do if defined %%k (
    if "%%k"=="SYSLIB" ( :# System library
      SET "%%v.INCPATH=!%%v.INCPATH!;%SYSLIB%"
      set "%%v.LIBPATH=!%%v.LIBPATH!;%SYSLIB%\$(BR)"
    )
    if "%%k"=="MSVCLIBX" ( :# MSVC Library eXtensions library
      SET "%%v.INCPATH=%MSVCLIBX%\include;!%%v.INCPATH!" &:# Include MsvcLibX's _before_ MSVC's own include files
      SET "%%v.LIBPATH=%MSVCLIBX%\lib;!%%v.LIBPATH!"
    )
    if "%%k"=="98DDK" ( :# Windows 98 DDK
      :# Gotcha: Do not use %98DDK% here, as it's not expanded correctly in set xxxx32 commands
      set "%%v.PATH=!%%v.PATH!;!98DDK!\bin"
      set "%%v.INCPATH=!%%v.INCPATH!;!98DDK!\inc;!98DDK!\inc\win98"
      set "%%v.LIBPATH=!%%v.LIBPATH!;!98DDK!\lib"
    )
    if "%%k"=="GNUEFI" ( :# gnu-efi sources
      SET "%%v.INCPATH=!%%v.INCPATH!;%GNUEFI%\inc"
    )
    if "%%k"=="PTHREADS" ( :# Posix Threads for Windows
      SET "%%v.INCPATH=!%%v.INCPATH!;%PTHREADS%\include"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;%PTHREADS%\lib\x86"
    )
    if "%%k"=="BOOST" ( :# Boost C++ libraries
      SET "%%v.INCPATH=!%%v.INCPATH!;%BOOST%"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;%BOOST%\stage\lib"
    )
  )
)

:# Update other processors include and library paths for well-known libraries
for %%v in (VCIA64 VC64 VCARM VCARM64) do if defined %%v (
  for %%k in (%SDK_LIST%) do if defined %%k (
    if "%%k"=="SYSLIB" ( :# System library
      SET "%%v.INCPATH=!%%v.INCPATH!;%SYSLIB%"
      set "%%v.LIBPATH=!%%v.LIBPATH!;%SYSLIB%\$(BR)"
    )
    if "%%k"=="MSVCLIBX" ( :# MSVC Library eXtensions library
      SET "%%v.INCPATH=%MSVCLIBX%\include;!%%v.INCPATH!" &:# Include MsvcLibX's _before_ MSVC's own include files
      SET "%%v.LIBPATH=%MSVCLIBX%\lib;!%%v.LIBPATH!"
    )
    if "%%k"=="GNUEFI" ( :# gnu-efi sources
      SET "%%v.INCPATH=!%%v.INCPATH!;%GNUEFI%\inc"
    )
    if "%%k"=="PTHREADS" ( :# Posix Threads for Windows
      SET "%%v.INCPATH=!%%v.INCPATH!;%PTHREADS%\include"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;%PTHREADS%\lib\x86"
    )
    if "%%k"=="BOOST" ( :# Boost C++ libraries
      SET "%%v.INCPATH=!%%v.INCPATH!;%BOOST%"
      SET "%%v.LIBPATH=!%%v.LIBPATH!;%BOOST%\stage\lib"
    )
  )
)

:# Define legacy variables until we change all make files to use the generalized names above
set "MSVC=%VC16%" &:# Microsoft Visual C++ 16-bits base path
set "MAPSYM=%VC16.MS%" &:# 16-bits debugging symbols generator 

set VS=VS32
set VC=VC32
set "VSTUDIOLONG=!%VS%!" &:# Microsoft Visual Studio (Long path)
call :long2short VSTUDIOLONG VSTUDIO &:# Microsoft Visual Studio (Short path)
set "VSCOMMONLONG=!%VS%.COMMON!" &:# Microsoft Visual Studio Common Files (Long path)
call :long2short VSCOMMONLONG VSCOMMON &:# Microsoft Visual Studio Common Files (Short path)
set "VSIDELONG=!%VS%.IDE!" &:# Microsoft Visual Studio IDE Files (Long path)
call :long2short VSIDELONG VSIDE &:# Microsoft Visual Studio IDE Files (Short path)
set "VSTOOLSLONG=!%VS%.TOOLS!" &:# Microsoft Visual Studio Tools (Long paths)
call :long2short VSTOOLSLONG VSTOOLS &:# Microsoft Visual Studio Tools (Short paths)
set "MSVC32LONG=!%VC%!" &:# Microsoft Visual C++ 32/64 bits (Long path)
call :long2short MSVC32LONG MSVC32 &:# Microsoft Visual C++ 32/64 bits (Short path)

set "WINSDK=!%VS%.WINSDK!" &:# Microsoft Windows SDK

:# Set the path
:# No need to change that path, as Visual Studio installation sets it, right?
:# Actually it does, but other tools such as MSDN can break it!
:# 2004-12-15 JFL Workaround for the DOS build problem:
:# This build fails if the path is longer than 128 characters. (A bug in VC 1.52's cl.exe) 
:# So use the strict minimum number of paths.
:# 2010-03-29 JFL Moved the path setting inside the make file, as it's OS-dependant.
set PATH0=%WINDIR%\System32;%WINDIR%
set PATH16=%MSVC%\bin;%PATH0%
set PATH32=%MSVC32%\bin;%VSIDE%;%VSTOOLS%;%PATH0%
set PATH64=%MSVC32%\bin\amd64;%VSIDE%\amd64;%VSTOOLS%\amd64;%PATH0%

:# Find the conv.exe tool, that we use for some builds
:# set "CONV="
:# for /f "delims=" %%p in ('where conv.exe') do @if not defined CONV set "CONV=%%p"
:# for %%p in (WIN32\conv.exe WIN64\conv.exe) do @if not defined CONV set "CONV=%%p"

:# Find the chcp.com tool, which we use just below.
:# Work around for an XP/64 bug: The 32-bits version of chcp.com is missing
if exist "%windir%\SysWow64" if not exist "%windir%\SysWow64\chcp.com" (
  echo Note: Replacing the missing 32-bits %windir%\SysWow64\chcp.com by its 64-bits cousin 
  copy /y "%windir%\System32\chcp.com" "%windir%\SysWow64\" >NUL
)

:# Get the Windows system Code Page
call :Reg.GetValue HKLM\SYSTEM\CurrentControlSet\Control\Nls\CodePage ACP WIN.CP
:# Get the corresponding Character Set
call :Reg.GetValue HKLM\SOFTWARE\Classes\MIME\Database\Codepage\%WIN.CP% BodyCharset WIN.CS
if not defined WIN.CS set "WIN.CS=cp%WIN.CP%"

:# Get the DOS cmd.exe Code Page
for /f "tokens=2 delims=:" %%n in ('chcp') do set "DOS.CP=%%n"
set "DOS.CP=%DOS.CP: =%" &:# Trim spaces
:# Get the corresponding Character Set
call :Reg.GetValue HKLM\SOFTWARE\Classes\MIME\Database\Codepage\%DOS.CP% BodyCharset DOS.CS
if not defined DOS.CS set "DOS.CS=cp%DOS.CP%"

:# Optional chance to undo the above, or prepend something _ahead_ of them.
if defined POST_CONFIG_ACTIONS set "POST_CONFIG_ACTIONS=%POST_CONFIG_ACTIONS:¡¡¡¡=^!%" &:# Prepare delayed variables expansion
%IF_DEBUG% if defined POST_CONFIG_ACTIONS (set POST_CONFIG_ACTIONS) else (echo :# No POST_CONFIG_ACTIONS)
%POST_CONFIG_ACTIONS%

:# Generate the %CONFIG.BAT% file, defining variables for use by nmake files.
:# Note: nmake variable names cannot contain dots. Use _ instead.
%CONFIG%.
%CONFIG% SET "PF32=%PF32%" ^&:# 32-bits Program Files
%CONFIG% SET "PF64=%PF64%" ^&:# 64-bits Program Files
%CONFIG% SET "ARCH=%ARCH%" ^&:# PROCESSOR_ARCHITECTURE
%CONFIG%.
%CONFIG% SET "MASM=%MASM%" ^&:# Microsoft 16-bits Assembler base path
%CONFIG% SET "MSVC=%MSVC%" ^&:# Microsoft 16-bits Visual C++ base path
%CONFIG% SET "MAPSYM=%MAPSYM%" ^&:# 16-bits debugging symbols generator 
%CONFIG%.
%CONFIG% SET "VSTUDIOLONG=%VSTUDIOLONG%" ^&:# Microsoft Visual Studio (Long path)
%CONFIG% SET "VSTUDIO=%VSTUDIO%" ^&:# Microsoft Visual Studio (Short path)
%CONFIG% SET "VSCOMMONLONG=%VSCOMMONLONG%" ^&:# Microsoft Visual Studio Common Files (Long path)
%CONFIG% SET "VSCOMMON=%VSCOMMON%" ^&:# Microsoft Visual Studio Common Files (Short path)
%CONFIG% SET "VSIDELONG=%VSIDELONG%" ^&:# Microsoft Visual Studio IDE Files (Long path)
%CONFIG% SET "VSIDE=%VSIDE%" ^&:# Microsoft Visual Studio IDE Files (Short path)
%CONFIG% SET "VSTOOLSLONG=%VSTOOLSLONG%" ^&:# Microsoft Visual Studio Tools (Long paths)
%CONFIG% SET "VSTOOLS=%VSTOOLS%" ^&:# Microsoft Visual Studio Tools (Short paths)
%CONFIG% SET "MSVC32LONG=%MSVC32LONG%" ^&:# Microsoft Visual C++ 32/64 bits (Long path)
%CONFIG% SET "MSVC32=%MSVC32%" ^&:# Microsoft Visual C++ 32/64 bits (Short path)
:# %CONFIG%.
:# %CONFIG% SET "CONV=%CONV%" ^&:# SysToolsLib's code page conversion tool

:# Environment variables
%CONFIG%.
%CONFIG% SET "WIN_CP=%WIN.CP%" ^&:# Windows Code Page
%CONFIG% SET "WIN_CS=%WIN.CS%" ^&:# Windows Character Set
%CONFIG% SET "DOS_CP=%DOS.CP%" ^&:# DOS Code Page
%CONFIG% SET "DOS_CS=%DOS.CS%" ^&:# DOS Character Set

:# Clear Target-OS-specific variables that will be selected in OS-specific make files
%CONFIG%.
%CONFIG% SET "AS=" ^&:# Assembler
%CONFIG% SET "CC=" ^&:# C compiler
%CONFIG% SET "INCLUDE=%STINCLUDE%" ^&:# Include paths. Define USER_INCLUDE if needed.
%CONFIG% SET "LK=" ^&:# Linker
%CONFIG% SET "LIB=" ^&:# Libraries paths. Define USER_LIBS if needed.
%CONFIG% SET "LB=" ^&:# Library manager
%CONFIG% SET "RC=" ^&:# Resource compiler
%CONFIG% SET "MT=" ^&:# Manifest tool

:# New generic variables
for %%s in (
  "DOS    VC16    16-bits"
  "WIN95  VC95    32-bits"
  "WIN32  VC32    32-bits"
  "IA64   VCIA64  IA64"
  "WIN64  VC64    64-bits"
  "ARM    VCARM   ARM"
  "ARM64  VCARM64 ARM64"
) do for /f "tokens=1,2,3" %%k in (%%s) do (
  %CONFIG%.
  %CONFIG% SET "%%k_CC=!%%l.CC!" ^&:# Microsoft Visual C++ %%m compiler
  %CONFIG% SET "%%k_AS=!%%l.AS!" ^&:# Microsoft %%m assembler
  %CONFIG% SET "%%k_LK=!%%l.LK!" ^&:# Microsoft %%m linker
  %CONFIG% SET "%%k_LB=!%%l.LB!" ^&:# Microsoft %%m librarian
  %CONFIG% SET "%%k_RC=!%%l.RC!" ^&:# Microsoft %%m resource compiler
  %CONFIG% SET "%%k_MT=!%%l.MT!" ^&:# Microsoft %%m manifest tool
  %CONFIG% SET "%%k_PATH=!%%l.PATH!;%PATH0%" ^&:# All tools paths for %%m compilation
  %CONFIG% SET "%%k_VCINC=!%%l.VCINC!" ^&:# Visual C++ %%m compiler include dir for MsvcLibX include_next
  %CONFIG% SET "%%k_CRTINC=!%%l.CRTINC!" ^&:# Visual C++ %%m CRT library include dir for MsvcLibX include_next
  %CONFIG% SET "%%k_INCPATH=!%%l.INCPATH!" ^&:# Include paths for %%m compilation
  %CONFIG% SET "%%k_LIBPATH=!%%l.LIBPATH!" ^&:# Libraries paths for %%m linking
  %CONFIG% SET "%%k_WINSDK=!%%l.WINSDK!" ^&:# Microsoft Windows %%m SDK
  %CONFIG% SET "%%k_WINSDKINC=!%%l.WINSDKINC!" ^&:# Microsoft Windows %%m SDK Include directory
)

:# Output base directory
if defined OUTDIR (
  %CONFIG%.
  %CONFIG% SET "OUTDIR=%OUTDIR%" ^&:# Output base directory
)

if defined POST_MAKE_ACTIONS (
  %CONFIG%.
  %CONFIG% :# List of commands to run when make.bat exits
  %CONFIG% SET "POST_MAKE_ACTIONS=%POST_MAKE_ACTIONS%"
)

:# The config.bat script must explicitly return 0, as in XP some set commands do set errorlevel 1!
%CONFIG%.
%CONFIG% exit /b 0 ^&:# Configuration done successfully

:# Configuration file created successfully
exit /b 0
