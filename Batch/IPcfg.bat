@echo off
:######################### :encoding=UTF-8:tabSize=8: #########################
:#                                                                            #
:#  Filename        ipcfg.bat                                                 #
:#                                                                            #
:#  Description     Front-end to Window's ipconfig.exe, filtering its output  #
:#                                                                            #
:#  Notes 	    Displays only physical adapters settings by default.      #
:#                  There are many options available to select adapters (by   #
:#                  name or by category) and properties (one or all).         #
:#                  Use option -? to display a help screen.                   #
:#                                                                            #
:#                  ipconfig.exe output is localized.                         #
:#                  The filtering must be adapted on a per-language basis.    #
:#		    See comments at the head of the main routine, then before #
:#                  the big language switch statement, for further details.   #
:#                                                                            #
:#                  The filtering _may_ also need to be adapted depending on  #
:#		    Windows version. As of march 2012, the script has been    #
:#		    tested in 8 localized versions of Windows 7, and also in  #
:#		    the English versions of Windows XP and Windows Vista.     #
:#                                                                            #
:#                  TO DO: The support for PPP adapters has been tested in    #
:#                  English only, and will require adaptation in other lang.  #
:#                                                                            #
:#  History                                                                   #
:#   2012-01-26 JFL Created this script. (jf.larvoire@hp.com)                 #
:#   2012-01-27 JFL Added the ability to display a single property.           #
:#   2012-02-28 JFL Added support for Spanish, Chinese, Japanese.	      #
:#   2012-03-15 JFL Added support for Italian, Russian.          	      #
:#   2012-04-10 JFL Fixed a bug if a property name contains a dot.	      #
:#                  Changed the property matching from equal_name to contains_#
:#                  string. Simplifies search for IPv4 address, where the name#
:#                  may sometimes contain suffixes like "(Autoconfigured)".   #
:#                  Made -v a synonym for /all.                               #
:#                  Added options -u and -U to handle virtual devices (VMWare #
:#		    and VirtualBox).					      #
:#   2012-06-06 JFL Added support PPP adapters. Added -p and -P options.      #
:#   2012-07-19 JFL Updated the debugging framework.                          #
:#                  Added ability to select and display multi-line properties.#
:#                  Changed the adapter name matching from equal_name to      #
:#                  contains_string.                                          #
:#                  Added options -n/-N to control the display of prop names. #
:#   2012-09-12 JFL Default to displaying names if the adapter is "*".        #
:#                  Fixed the -d option, broken in the previous change.       #
:#   2012-09-27 JFL Added options -g/-G to select Windows' global config.     #
:#                  The first use of a type selector clears all types default.#
:#                  Passed all "complex" ipconfig options through unchanged.  #
:#   2013-01-15 JFL Added options -i to display a given interface number.     #
:#   2013-06-26 JFL Restructured to sort interfaces in alphabetic order.      #
:#   2015-02-24 JFL Fixed the -i option for VPN interfaces without a MAC@.    #
:#                  Updated -i to display all interfaces by default.          #
:#   2020-09-03 JFL Fixed the virtual adapter detection for English.          #
:#   2020-10-13 JFL Also search the adapter name in the description field.    #
:#                  Added a special case for the Pulse Secure VPN.            #
:#   2020-10-15 JFL Generalized the ability to define alias names and types.  #
:#   2023-03-28 JFL Fixed the language detection to find the current display  #
:#                  language, instead of the install language as before.      #
:#   2023-03-29 JFL Fixed the Wi-Fi adapters detection in French.	      #
:#   2023-03-31 JFL More generally, fixed all non-ASCII signatures detections.#
:#                  Done by converting this script encoding from 1252 to UTF8,#
:#                  and by using the actual UTF8 string signatures herein.    #
:#                  Previously the signatures had to stored here in their     #
:#		    various native DOS code page. This was done by manually   #
:#                  converting them this way in a terminal window...	      #
:#                  Example for French, using CP 850 for its DOS code page:   #
:#                  chcp 1252						      #
:#                  echo "Rés" | conv . 850				      #
:#                  ... Then pasting the output in this script.               #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal enableextensions enabledelayedexpansion
set "VERSION=2023-03-31"
set "SCRIPT=%~nx0"
set "ARG0=%~f0"

:# User-defined alias(es) for complex adapter names
set "ALIAS.NAMES=VPN" &:# Space-delimited list of alias names, each one used as index below
:# Define sets of variables ALIAS[name].NAME, ALIAS[name].TYPE, ALIAS[name].HELP
set "ALIAS[VPN].NAME=Juniper"	&:# The first word(s) in the adapter name
set "ALIAS[VPN].TYPE=Vir"	&:# One of: Eth Ppp Tun Vir Wir
set "ALIAS[VPN].HELP=The Juniper Networks Pulse Secure VPN virtual adapter"

:# FOREACHLINE macro. (Change the delimiter to none to catch the whole lines.)
set FOREACHLINE=for /f "delims="

call :Debug.Init
goto main

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
:#                  PutVars	    Display the values of a set of variables  #
:#                  PutArgs	    Display the values of all arguments       #
:#                                                                            #
:#  Macros          %FUNCTION%	    Define and trace the entry in a function. #
:#                  %RETURN%        Return from a function and trace it       #
:#                  %RETVAL%        Default return value                      #
:#                  %RETVAR%        Name of the return value. Default=RETVAL  #
:#                                                                            #
:#                  Always match uses of %FUNCTION% and %RETURN%. That is     #
:#                  never use %RETURN% if there was no %FUNCTION% before it.  #
:#                                                                            #
:#                  :# Example of a factorial routine using this framework    #
:#                  :Fact                                                     #
:#                  %FUNCTION% Fact %*                                        #
:#                  setlocal enableextensions enabledelayedexpansion          #
:#                  set N=%1                                                  #
:#                  if .%N%.==.0. (                                           #
:#                    set RETVAL=1                                            #
:#                  ) else (                                                  #
:#                    set /A M=N-1                                            #
:#                    call :Fact !M!                                          #
:#                    set /A RETVAL=N*RETVAL                                  #
:#                  )                                                         #
:#                  endlocal & set "RETVAL=%RETVAL%" & %RETURN%               #
:#                                                                            #
:#                  %ECHO%	    Echo and log a string, indented           #
:#                  %ECHO.V%	    Idem, but display it in verbose mode only #
:#                  %ECHO.D%	    Idem, but display it in debug mode only   #
:#                                                                            #
:#                  %PUTVARS%	    Indent, echo and log variables values     #
:#                  %PUTVARS.V%	    Idem, but display them in verb. mode only #
:#                  %PUTVARS.D%	    Idem, but display them in debug mode only #
:#                                                                            #
:#  Variables       %LOGFILE%       Log file name. Inherited. Default=NUL.    #
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
:#                  Added the three flavors of the Echo and PutVars routines. #
:#   2012-07-19 JFL Added optimizations to improve performance in non-debug   #
:#                  and non-verbose mode. Added routine Debug.SetLog.         #
:#                                                                            #
:#----------------------------------------------------------------------------#

call :Debug.Init
goto :Debug.End

:Debug.Init
set "RETVAR=RETVAL"
set "ECHO=call :Echo"
set "PUTVARS=call :PutVars"
:Debug.Init.2
set "ECHO.V=call :Echo.Verbose"
set "ECHO.D=call :Echo.Debug"
set "PUTVARS.V=call :PutVars.Verbose"
set "PUTVARS.D=call :PutVars.Debug"
:# Variables inherited from the caller...
:# Preserve INDENT if it contains just spaces, else clear it.
for /f %%s in ('echo.%INDENT%') do set "INDENT="
:# Preserve the log file name, else by default use NUL.
if .%LOGFILE%.==.. set "LOGFILE=NUL"
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
set "FUNCTION=rem"
set "RETURN=goto :eof"
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -d=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Optimization to speed things up in non-debug mode
if .%LOGFILE%.==.NUL. set "ECHO.D=rem"
if .%LOGFILE%.==.NUL. set "PUTVARS.D=rem"
goto :eof

:Debug.On
:Debug.1
set "DEBUG=1"
set "FUNCTION=call :Debug.Entry"
set "RETURN=call :Debug.Return & goto :eof"
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -d=% -d"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Reverse the above optimization
set "ECHO.D=call :Echo.Debug"
set "PUTVARS.D=call :PutVars.Debug"
goto :eof

:Debug.Entry
>&2         echo %INDENT%call :%*
>>%LOGFILE% echo %INDENT%call :%*
set "INDENT=%INDENT%  "
goto :eof

:Debug.Return
>&2         echo %INDENT%return !RETVAL!
>>%LOGFILE% echo %INDENT%return !RETVAL!
set "INDENT=%INDENT:~0,-2%"
goto :eof

:# Routine to set the VERBOSE mode, in response to the -v argument.
:Verbose.Off
:Verbose.0
set "VERBOSE=0"
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -v=%"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
:# Optimization to speed things up in non-verbose mode
if .%LOGFILE%.==.NUL. set "ECHO.V=rem"
if .%LOGFILE%.==.NUL. set "PUTVARS.V=rem"
goto :eof

:Verbose.On
:Verbose.1
set "VERBOSE=1"
set "EXEC.ARGS= %EXEC.ARGS%"
set "EXEC.ARGS=%EXEC.ARGS: -v=% -v"
set "EXEC.ARGS=%EXEC.ARGS:~1%"
goto :eof

:# Echo and log a string, indented at the same level as the debug output.
:Echo
echo.%INDENT%%*
:Echo.Log
>>%LOGFILE% 2>&1 echo.%INDENT%%*
goto :eof

:Echo.Verbose
if .%VERBOSE%.==.1. goto :Echo
goto :Echo.Log

:Echo.Debug
if .%DEBUG%.==.1. >&2 echo.%INDENT%%*
goto :Echo.Log

:# Echo and log variable values, indented at the same level as the debug output.
:PutVars
if .%~1.==.. goto :eof
>&2         echo %INDENT%set "%~1=!%~1!"
>>%LOGFILE% echo %INDENT%set "%~1=!%~1!"
shift
goto PutVars

:PutVars.Verbose
if .%VERBOSE%.==.1. (
  call :PutVars %*
) else (
  call :PutVars %* >NUL 2>NUL
)
goto :eof

:PutVars.Debug
if .%DEBUG%.==.1. (
  call :PutVars %*
) else (
  call :PutVars %* >NUL 2>NUL
)
goto :eof

:# Echo a list of arguments.
:PutArgs
setlocal
set N=0
:PutArgs.loop
if .%1.==.. endlocal & goto :eof
set /a N=N+1
>&2 echo %INDENT%set "ARG%N%=%1"
shift
goto PutArgs.loop

:Debug.End

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        trimleft						      #
:#                                                                            #
:#  Description     Trim spaces from the left end of a string		      #
:#                                                                            #
:#  Arguments       %1	    Variable name                                     #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2023-03-31 JFL Rewritten a simplified version for maximum speed.         #
:#                                                                            #
:#----------------------------------------------------------------------------#

:trimleft
for /f "tokens=*" %%a in ("!%~1!") do set "%~1=%%a"
exit /b

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        ConvertSig2CP                                             #
:#                                                                            #
:#  Description     Convert a non-ASCII signature to the current console CP.  #
:#                                                                            #
:#  Arguments       %1 = variable name					      #
:#                                                                            #
:#  Returns         Nothing                                                   #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2023-03-29 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:ConvertSig2CP
for /f %%a in ('echo !%~1!^| conv 8 .') do set "%~1=%%a"
set "%~1=!%~1:~0,3!" &:# Truncate the signature after 3 bytes
exit /b

:Convert2CP
for /f %%a in ('echo !%~1!^| conv 8 .') do set "%~1=%%a"
exit /b

:#----------------------------------------------------------------------------#
:#                                                                            #
:#  Function        UnsetVars                                                 #
:#                                                                            #
:#  Description     Delete all member variables beginning at a given root     #
:#                                                                            #
:#  Arguments       %1 = root name	                                      #
:#                                                                            #
:#  Returns         Nothing                                                   #
:#                                                                            #
:#  Notes 	                                                              #
:#                                                                            #
:#  History                                                                   #
:#   2013-06-26 JFL Created this routine.                                     #
:#                                                                            #
:#----------------------------------------------------------------------------#

:UnsetVars %1=VAR_ROOT
set "%~1.NUL=NUL" &:# Dummy definition to avoid having an empty list to delete
for /f "tokens=1 delims==" %%v in ('set "%~1"') do set "%%v="
goto :eof

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
:#  Function        GetCP	                                              #
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
echo.
echo %SCRIPT% - A front end to ipconfig.exe, filtering its output
echo.
echo Usage: %SCRIPT% [OPTIONS] [ADAPTER [PROPERTY]]
echo.
echo Options:
echo   -?      Display this help and exit
echo   -a      Show all devices types
echo   -A      Show no device type (Useful to then enable just one type below)
echo   -d      Output debug information
echo   -e      Show ethernet devices (Default)
echo   -E      Hide ethernet devices
echo   -g      Show Windows global IP configuration (Default)
echo   -G      Hide Windows global IP configuration
echo   -i [ADAPTER]   Display the interface number (Useful for changing routes)
echo   -l      List network adapters
echo   -n      Display adapter and property names even when they're specified
echo   -N      Do not display specified adapter and property names (Default)
echo   -p      Show PPP devices (Default)
echo   -P      Hide PPP devices
echo   -t      Show tunnel devices
echo   -T      Hide tunnel devices (Default)
echo   -u      Show virtual adapters on Virtual LANs to Virtual Machines
echo   -U      Hide virtual adapters on VLANs to VMs (Default)
echo   -v      Verbose mode. Run ipconfig /all.
echo   -V      Display this script version and exit
echo   -w      Show wireless devices (Default)
echo   -W      Hide wireless devices
echo.
echo Adapter:  Name of a target adapter (Needs quotes if it contains a space)
echo           Default: All adapter types selected with options above
echo           Use option -l to list adapter names
echo           * = All adapters
for %%a in (!ALIAS.NAMES!) do echo           %%a = !ALIAS[%%a].HELP!
echo.
echo Property: Display this property only. Ex: "ipv4 address". Default: all
echo           Can be a partial name. Ex: "ipv4". Make sure to avoid duplicates.
echo           Use option -n in case of doubt about multiple values.
echo           Use adapter name * to display a given property for all adapters.
%RETURN%

:main

:# Interface types (Respectively for Windows, Ethernet, Wireless, Tunnel,Virtual)
set "types=Win Eth Wir Ppp Tun Vir"
:# Show Windows global settings. 1=Yes, 0=No.
set "show.Win=1"
:# Show Ethernet adapters. 1=Yes, 0=No.
set "show.Eth=1"
:# Show Wireless adapters. 1=Yes, 0=No.
set "show.Wir=1"
:# Show PPP adapters. 1=Yes, 0=No.
set "show.Ppp=1"
:# Show Tunnel adapters. 1=Yes, 0=No.
set "show.Tun=0"
:# Show Virtual adapters. 1=Yes, 0=No.
set "show.Vir=0"
:# Interface type offset on title lines
set "type_at=0"
:# Interface types string identifier. (Virtual is a special case, identified differently)
set "type.Win=Win"
set "type.Eth=Eth"
set "type.Wir=Wir"
set "type.Ppp=PPP"
set "type.Tun=Tun"
set "type.Vir=Vir"
:# Number of characters to remove from the title line, to get the adapter name
set "remove.Win=0"
set "remove.Eth=17"
set "remove.Wir=21"
set "remove.Ppp=12"
set "remove.Tun=15"
set "remove.Vir=0"
:# Property names that we'll need to process
set "description=Description"

:# Find the localization, and adjust locale-specific settings if needed.
:# Note that each language uses a specific code page in cmd boxes. Some SBCS and some DBCS.
:# The language-specific strings to search for cannot be visualized correctly in a Windows editor.
:# Type this batch at the localized cmd prompt, to see the actual strings in that code page.

:# Most comments on the Internet about how to get the OS localization are wrong.
:# They explain how to get the install language, not the display language. 
:# I initially got it wrong too, and tested the install language only.
:# The correct method for getting the display language is documented there:
:# https://stackoverflow.com/a/35704694
set "CULTURE="	&:# The display language, aka. culture. Ex: en-US or fr-FR
set "LCID="	&:# The Culture ID. Ex: 0x409 or 0x40C
set "LANG="	&:# The install language. Ex: 0409 or 040C
:# Get the user's preferred display language
for /f "tokens=3" %%l in ('reg query "HKCU\Control Panel\Desktop" /v PreferredUILanguages 2^>NUL ^| findstr /r REG.*SZ') do set "CULTURE=%%l"
:# Convert the Display Language to the corresponding Culture ID
if defined CULTURE for /f "tokens=3" %%l in ('reg query "HKLM\System\CurrentControlSet\Control\MUI\UILanguages\%CULTURE%" /v LCID 2^>NUL ^| findstr REG_DWORD') do set "LCID=%%l"
:# Convert the hexadecimal LCID (Ex: 0x409) to a 4-digit install language (Ex: 0409)
if defined LCID (
  set "LANG=000%LCID:0x=0%"
  set "LANG=!LANG:~-4!"
)
%PUTVARS.D% CULTURE LCID LANG &:# This is called before cmd-line options are processed, so set "DEBUG=1" in the shell to see these vvariables
:# If the above failed, revert to getting the install language
if not defined LANG for /f "tokens=3" %%l in ('reg query "HKLM\system\currentcontrolset\control\nls\language" /v Installlanguage ^| findstr REG_SZ') do set LANG=%%l
if %LANG%==0407 (
  rem :# 0407 = German:
  set "type_at=0"
  set "type.Win=Win"
  set "type.Eth=Eth"
  set "type.Wir=Dra"
  set "type.Tun=Tun"
  set "remove.Eth=17"
  set "remove.Wir=21"
  set "remove.Tun=14"
) else if %LANG%==0409 (
  rem :# 0409 = English. Strings encoded in code page 437.
  rem :# Already OK.
) else if /i %LANG%==040C (
  rem :# 040C = French. Strings encoded in code page 850.
  set "type_at=6"
  set "type.Win=ura"
  set "type.Eth=Eth"
  set "type.Wir=R‚s"
  set "type.Wir=Rés" &rem # "Réseau sans fil"
  call :ConvertSig2CP type.Wir
  set "type.Tun=Tun"
  set "remove.Eth=15"
  set "remove.Wir=22"
  set "remove.Tun=13"
) else if %LANG%==0410 (
  rem :# 0410 = Italian. Strings encoded in code page 850.
  set "type_at=7"
  set "type.Win=raz"
  set "type.Eth=Eth"
  set "type.Wir=LAN"
  set "type.Tun=Tun"
  set "remove.Eth=16"
  set "remove.Wir=20"
  set "remove.Tun=14"
) else if %LANG%==0411 (
  rem :# 0411 = Japanese. Strings encoded in code page 932. (DBCS)
  rem :# Start parsing adapter types at index 2, because the Japanese for "Ethernet Adapter"
  rem :# has a character \x81\x5b at index 1, and \x81 is invalid in code page 1252 here.
  set "type_at=2"
  set "type.Win=ndo"
  set "type.Eth=ƒTƒlƒb"
  set "type.Eth=サネッ"
  call :ConvertSig2CP type.Eth
  set "type.Wir=rel"
  set "type.Tun=nne"
  set "remove.Eth=13"
  set "remove.Wir=21"
  set "remove.Tun=15"
) else if %LANG%==0419 (
  rem :# 0419 = Russian. Strings encoded in code page 866. (SBCS)
  rem :# Skipping the first letter, capitalized, in the 0x8X range, causing problems in code page 1252.
  set "type_at=1"
  set "type.Win= áâ"
  set "type.Win=аст"
  call :ConvertSig2CP type.Win
  set "type.Eth=the"
  set "type.Wir=¤ ¯"
  set "type.Wir=дап"
  call :ConvertSig2CP type.Wir
  set "type.Tun=ã­­"
  set "type.Tun=у­­"
  call :ConvertSig2CP type.Tun
  set "remove.Eth=17"
  set "remove.Wir=36"
  set "remove.Tun=19"
) else if %LANG%==0804 (
  rem :# 0804 = Chinese. Strings encoded in code page 936. (DBCS)
  set "type_at=0"
  set "type.Win=Win"
  set "type.Eth=ÒÔÌ«Íø"
  set "type.Eth=以太网"
  call :ConvertSig2CP type.Eth
  set "type.Wir=ÎÞÏß¾Ö"
  set "type.Wir=无线局"
  call :ConvertSig2CP type.Wir
  set "type.Tun=ËíµÀÊÊ"
  set "type.Tun=隧道适"
  call :ConvertSig2CP type.Tun
  set "remove.Eth=7"
  set "remove.Wir=9"
  set "remove.Tun=6"
) else if /i %LANG%==0C0A (
  rem :# 0C0A = Spanish. Strings encoded in code page 850.
  set "type_at=13"
  set "type.Win= IP"
  set "type.Eth=Eth"
  rem :# The next item is for "Adaptador LAN inalámbrico"
  set "type.Wir= in"
  rem :# The next item is "tún" for "Adaptador de túnel". ú = \xA3 in code page 850.
  set "type.Tun=t£n"
  set "type.Tun=tún"
  call :ConvertSig2CP type.Tun
  set "remove.Eth=22"
  set "remove.Wir=26"
  set "remove.Tun=19"
) else (
  >&2 echo Warning: Unsupported language %LANG%. Filtering results may not be correct.
)

:# Target adapter. Default = all those of the %types% selected below.
set adapter=
:# Action. One of: ipconfig list
set "action=ipconfig"
:# Property to display. Default=all
set "property="
:# Interface type
set "type=none"
:# ipconfig options
set "opts="
:# Whether to display names
set "names.display=1"
:# Whether to force displaying names, when searching for a particular one
set "names.force=0"
:# Flag allowing to identify the first adapter type selector
set "FIRST_SELECT=1"

goto get_opts
:next_opt
shift
:get_opts
if .%1.==.. goto start
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-a. call :select_all & goto next_opt
if .%1.==.-A. call :select_none & goto next_opt
if .%1.==.-d. call :Debug.On & goto next_opt
if .%1.==.-e. call :select Eth 1 & goto next_opt
if .%1.==.-E. call :select Eth 0 & goto next_opt
if .%1.==.-g. call :select Win 1 & set "opts=%opts% /all" & goto next_opt
if .%1.==.-G. call :select Win 0 & goto next_opt
if .%1.==.-i. set "action=get_interface" & goto next_opt
if .%1.==.-l. set "action=list" & goto next_opt
if .%1.==.-n. set "names.force=1" & goto next_opt
if .%1.==.-N. set "names.force=0" & goto next_opt
if .%1.==.-p. call :select Ppp 1 & goto next_opt
if .%1.==.-P. call :select Ppp 0 & goto next_opt
if .%1.==.-t. call :select Tun 1 & goto next_opt
if .%1.==.-T. call :select Tun 0 & goto next_opt
if .%1.==.-u. call :select Vir 1 & goto next_opt
if .%1.==.-U. call :select Vir 0 & goto next_opt
if .%1.==.-v. call :Verbose.On & set "opts=%opts% /all" & goto next_opt
if .%1.==.-V. echo %VERSION% & goto :eof
if .%1.==.-w. call :select Wir 1 & goto next_opt
if .%1.==.-W. call :select Wir 0 & goto next_opt
if .%1.==.-all. set "opts=%opts% /all" & goto next_opt
if .%1.==./all. set "opts=%opts% %1" & goto next_opt
:# If the user passed a "complex" ipconfig option, pass it through unchanged to ipconfig.
for %%o in (allcompartments release release6 renew renew6 flushdns registerdns displaydns showclassid setclassid) do (
  if .%1.==./%%o. ipconfig %1 %2 %3 %4 %5 & goto :eof
)
if "%adapter%"=="" set "adapter=%~1" & goto update_names.display
if "%property%"=="" set "property=%~1" & set "property=!property:.=!" & goto update_names.display
>&2 echo Warning: Unexpected argument %1 ignored
goto next_opt

:select_all
for %%t in (%types%) do set "show.%%t=1"
set "FIRST_SELECT=0"
goto :eof

:select_none
for %%t in (%types%) do set "show.%%t=0"
set "FIRST_SELECT=0"
goto :eof

:select %1=type %2=value
if "%FIRST_SELECT%"=="1" call :select_none
set "FIRST_SELECT=0"
set "show.%1=%2"
goto :eof

:update_names.display
set "names.display=0"
if "!adapter!"=="*" set "names.display=1"
goto next_opt

:get_interface
if "%adapter%"=="" (
  for /f "tokens=*" %%i in ('%SCRIPT% -l') do (
    for /f %%n in ('%SCRIPT% -i "%%i"') do (
      echo %%n	%%i
    )
  )
  goto :eof
)
:# Get the interface MAC address
set "MAC="
for /f "tokens=*" %%m in ('%SCRIPT% -v "%adapter%" "Physical Address" ^| remplace -- - " "') do set MAC=%%m
:# But some adapters (ex: VPNs) do not have a MAC address
if "%MAC%"=="" set "MAC=%adapter%"
%PUTVARS.D% MAC
for /f "tokens=1 delims=." %%i in ('for /f "tokens=*" %%m in ^("%MAC%"^) do @route print ^| findstr /i /c:"...%%m"') do @for %%j in (%%i) do @echo %%j
goto :eof

:start
:# Get the current console code page
call :GetCP OLDCP

:# Tricky character that is code-page-dependent
set "HARDSPACE= " &:# Unicode U+00A0
call :Convert2CP HARDSPACE

:# If an alias is defined for that name, use it.
for %%a in ("%adapter%") do ( :# Use a %%a variable as we begin by updating %adapter%
  if defined ALIAS[%%~a].NAME set "adapter=!ALIAS[%%~a].NAME!"
  if defined ALIAS[%%~a].TYPE call :select !ALIAS[%%~a].TYPE! 1
)
                               
if "%action%"=="get_interface" goto :get_interface
%PUTVARS.D% LANG type_at types adapter names.display
for %%t in (%types%) do %PUTVARS.D% type.%%t show.%%t remove.%%t

if %names.force%==1 set "names.display=1" 

:# Parse windows version (Later used to test for XP)
call :WinVer

:# Run ipconfig, and record its output
set name=unknown
set NADAPTERS=0
call :UnsetVars ADAPTER. &:# Erase all variables beginning with ADAPTER.
call :UnsetVars ADAPTER[ &:# Erase all variables beginning with ADAPTER[
:# The sub-shell and/or pipe used for capturing the ipconfig.exe output
:# outputs lines encoded in the DOS code page, whatever the current code page.
%FOREACHLINE% %%l in ('ipconfig!opts! ^| conv d %OLDCP%') do (
  set "line=%%l"
  :# Work around for XP ipconfig.exe bug: Remove the trailing \x0D
  if %WINMAJOR%==5 set line=!line:~0,-1!
  if not "!line!"=="" (
    set "line=!line:%HARDSPACE%= !" &rem :# Convert hard spaces to normal spaces
    set "head=!line:~0,3!"
    :# Some lines begin with tab-space-space... Treat them as space-space-space.
    if "!head!"=="	  " set "head=   "
    if not "!head!"=="   " (
      :# This is an interface title line starting at column 0.
      set "signature=!line:~%type_at%,3!"
      for %%t in (%types%) do if /i "!signature!"=="!type.%%t!" set "type=%%t"
      :# Extract the adapter name.
      set name=!line!
      for %%t in (!type!) do set "n=!remove.%%t!"
      for %%n in (!n!) do set "name=!name:~%%n!"
      if "!name:~-1!"==":" set "name=!name:~0,-1!"
      if "!name:~-1!"==" " set "name=!name:~0,-1!"
      :# Record the adapter name and type.
      set "NADAPTER=!NADAPTERS!"
      set /a NADAPTERS=NADAPTERS+1
      %PUTVARS.D% line signature type n name NADAPTER NADAPTERS
      :# Except for adapter 0, which is actually Windows properties.
      if not "!NADAPTER!"=="0" set "ADAPTER.NAMES.!name!=!NADAPTER!" 
      set "ADAPTER[!NADAPTER!].LINE=!line!"
      set "ADAPTER[!NADAPTER!].NAME=!name!"
      set "ADAPTER[!NADAPTER!].TYPE=!type!"
      set "NPROPS=0"
    ) else (
      :# This is an interface property line, shifted to the right.
      :# Record them in any case: We need the description, even for the action "list".
      set "NPROP=!NPROPS!"
      set /a NPROPS=NPROPS+1
      set "NPROP2=0!NPROP!"
      set "NPROP2=!NPROP2:~-2!"
      set "ADAPTER[!NADAPTER!].LINES.!NPROP2!=!line!"
      %PUTVARS.D% ADAPTER[!NADAPTER!].LINES.!NPROP2!
      :# Look for the properties with additional information we need
      if not "!line:: =!"=="!line!" ( :# If this is a "property . . . : value" definition line
	for /f "tokens=1,* delims=:" %%p in ("!line:~3!") do set "prop=%%p" & set "val=%%q"
	:# Remove all trailing . . . . behind the property name
	:# Note: Don't use library.bat :trimright for performance reasons.
	set "prop=!prop: .=!"
	if "!prop:~-1!"==" " set "prop=!prop:~0,-1!"
	if "!prop:~-1!"=="." set "prop=!prop:~0,-1!"
	call :trimleft prop
	call :trimleft val
	%PUTVARS.D% prop val
	:# Now check if the description identifies this as a virtual interface
	if "!prop!"=="!description!" (
	  set "ADAPTER[!NADAPTER!].DESC=!val!"
	  if not "!val:Virtual=!"=="!val!" (
	    %ECHO.D% It's a virtual adapter!
	    set "ADAPTER[!NADAPTER!].TYPE=%type.Vir%"
	  )
	)
      )
    )
  )
)

:# If we didn't get all properties, repeat with them all, because we at least need the description
set NADAPTER=-1
if not "!opts: /all=!"=="!opt!" %FOREACHLINE% %%l in ('ipconfig!opts! /all ^|conv d %OLDCP%') do (
  set "line=%%l"
  :# Work around for XP ipconfig.exe bug: Remove the trailing \x0D
  if %WINMAJOR%==5 set line=!line:~0,-1!
  if not "!line!"=="" (
    set "line=!line:%HARDSPACE%= !" &rem :# Convert hard spaces to normal spaces
    set "head=!line:~0,3!"
    :# Some lines begin with tab-space-space... Treat them as space-space-space.
    if "!head!"=="	  " set "head=   "
    if not "!head!"=="   " (	:# This is an interface title line
      :# Assume the adapters are in the same order as the first time
      set /a NADAPTER=NADAPTER+1
    ) else (			:# This is an interface property line, shifted to the right.
      :# Look for the properties with additional information we need
      if not "!line:: =!"=="!line!" ( :# If this is a "property . . . : value" definition line
	for /f "tokens=1,* delims=:" %%p in ("!line:~3!") do set "prop=%%p" & set "val=%%q"
	:# Remove all trailing . . . . behind the property name
	:# Note: Don't use library.bat :trimright for performance reasons.
	set "prop=!prop: .=!"
	if "!prop:~-1!"==" " set "prop=!prop:~0,-1!"
	if "!prop:~-1!"=="." set "prop=!prop:~0,-1!"
	call :trimleft prop
	call :trimleft val
	%PUTVARS.D% prop val
	:# Now check if the description identifies this as a virtual interface
	if "!prop!"=="!description!" (
	  set "ADAPTER[!NADAPTER!].DESC=!val!"
	  if not "!val:Virtual=!"=="!val!" (
	    %ECHO.D% It's a virtual adapter!
	    set "ADAPTER[!NADAPTER!].TYPE=%type.Vir%"
	  )
	)
      )
    )
  )
)

:# Display the list of selected interfaces
if "%action%"=="list" (
  for /f "tokens=1,2 delims==" %%a in ('set "ADAPTER.NAMES." ^| sort') do (
    set "name=%%a"
    set "num=%%b"
    set "name=!name:ADAPTER.NAMES.=!"
    set "type=!ADAPTER[%%b].TYPE!"
    set "line=!ADAPTER[%%b].LINE!"
    for %%t in (!type!) do set "show=!show.%%t!"
    %PUTVARS.D% line type name num show
    if "!show!"=="1" (
      if %VERBOSE%==0 (
	echo !name!
      ) else (
	set "line=!line: :=!
	echo [!type!]  !line::=!
      )
    )
  )
  exit /b 0
)

:# Display the selected interfaces, and the selected properties for each of them
:# List global Windows properties first, then interfaces by alphabetic order.
for /f "tokens=1,2 delims==" %%a in ('echo ADAPTER.NAMES.!ADAPTER[0].NAME!^=0^& set "ADAPTER.NAMES." ^| sort') do (
  set "name=%%a"
  set "NADAPTER=%%b"
  set "name=!name:ADAPTER.NAMES.=!"
  set "line=!ADAPTER[%%b].LINE!"
  set "type=!ADAPTER[%%b].TYPE!"
  set "desc=!ADAPTER[%%b].DESC!"
  set "show=0"
  for %%t in (!type!) do set "show=!show.%%t!"
  if not "%adapter%"=="" ( :# If a name is specified, and if it does _Not_ match, then don't show it.
    if /I "!name:%adapter%=!"=="!name!" if /I "!desc:%adapter%=!"=="!desc!" set "show=0"
  )
  %PUTVARS.D% line type name NADAPTER show
  if "!show!"=="1" (
    if !names.display!==1 (
      echo.
      echo !line!
      echo.
    )
    for /f "tokens=1,* delims==" %%p in ('set "ADAPTER[!NADAPTER!].LINES." 2^>NUL') do (
      set line=%%q
      if "%property%"=="" (
	if !names.display!==0 (
	  echo.!line:~3!
	) else (
	  echo.!line!
	)
      ) else (
	if not "!line: : =!"=="!line!" (
	  :# If the line contains a :
	  for /f "tokens=1,* delims=:" %%p in ("!line:~3!") do set "prop=%%p" & set "val=%%q"
	  :# Remove all trailing . . . . behind the property name
	  :# Note: Don't use library.bat :trimright for performance reasons.
	  set "prop=!prop: .=!"
	  if "!prop:~-1!"==" " set "prop=!prop:~0,-1!"
	  if "!prop:~-1!"=="." set "prop=!prop:~0,-1!"
	  call :trimleft prop
	  call :trimleft val
	) else (
	  :# Else it's a continuation line for the same property
	  for /f "tokens=*" %%p in ("!line:~3!") do set "val=%%p"
	)
	:# Remove the space before the value name
	if "!val:~0,1!"==" " set "val=!val:~1!"
	%PUTVARS.D% property prop val
	:# if /I "!prop!"=="%property%" echo !val:~1!
	:# If !prop! _contains_ %property% then echo it.
	if /I not "!prop:%property%=!"=="!prop!" (
	  if !names.display!==0 (
	    echo.!val!
	  ) else (
	    echo.!line!
	  )
	)
      )
    )
  )
)
echo.
