@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    CheckEOL.bat					      *
:#                                                                            *
:#  Description:    Check the line ending type for a set of files	      *
:#                                                                            *
:#  Notes:	    TODO: Rewrite this in JScript instead of VBScript.        *
:#                        (But keep this as a Batch+VBScript example)         *
:#                                                                            *
:#  History:                                                                  *
:#   2017-11-14 JFL Created this script.				      *
:#   2017-11-15 JFL Merged the bat and vbs scripts into a single file.	      *
:#                  Added the -r recurse option.                              *
:#                                                                            *
:#        © Copyright 2018 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license: www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2017-11-15"
set "SCRIPT=%~nx0"
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"
set "ARG0=%~f0"
:# Never quote the set ARGS command, else some complex quoted strings break
set ARGS=%*

set "POPARG=call :PopArg"
set "RETURN=exit /b"
goto :Main

:#----------------------------------------------------------------------------#

:PopArg :# Will corrupt result if expansion is on and ARG contains ^ or ! characters.
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

:echoVars
setlocal EnableExtensions EnableDelayedExpansion
for %%v in (%*) do echo set "%%v=!%%v!"
endlocal & %RETURN%

:#----------------------------------------------------------------------------#

:is_dir PATHNAME	:# Return ERRORLEVEL 0 if a pathname refers to an existing directory
for /f "tokens=1,2 delims=d" %%a in ("-%~a1") do if not "%%~b"=="" %RETURN% 0
%RETURN% 1

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
endlocal & set "%~2=%DIR%" & %RETURN%

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
endlocal & set "%~2=%NAME%" & %RETURN%

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
endlocal & %RETURN% %RESULT%

:#----------------------------------------------------------------------------#

:Help
echo.
echo %SCRIPT% version %VERSION% -  Check the line ending type for a set of files
echo.
echo Usage: %SCRIPT% [OPTIONS] PATHNAME [PATHNAME [...]]
echo.
echo Options:
echo   -?      This help
echo   -r      Recursive mode
echo.
echo Pathnames may include wildcards
echo For each file in pathnames, output one of: Windows, Unix, Mac, None
%RETURN%

:#----------------------------------------------------------------------------#

:Main
set "BAT=%TEMP%\%~n0.bat"	&:# Copy of this batch file in the %TEMP% directory
set "VBS=%TEMP%\%~n0.vbs"	&:# The VBS file that we extract from this file
set "RECURSE=0"
set "LIST="
set "DEBUG.ECHO=rem"
set "DEBUG.ECHOVARS=rem"

:next_arg
%POPARG%
if "%ARG%"=="" goto :Start
if "%ARG%"=="/?" goto :Help
if "%ARG%"=="-?" goto :Help
if "%ARG%"=="-d" set "DEBUG.ECHO=echo" & set "DEBUG.ECHOVARS=call :echoVars" & goto :next_arg
if "%ARG%"=="-r" set "RECURSE=1" & goto :next_arg
set LIST=%LIST% %"ARG"%
goto :next_arg

:Start
if not defined LIST goto Help
%DEBUG.ECHOVARS% LIST
set ARGS=%LIST%

:# Update the VBS script if needed
if not exist "%BAT%" copy /y "%~f0" "%BAT%" >NUL & goto :create_vbs
if not exist "%VBS%" copy /y "%~f0" "%BAT%" >NUL & goto :create_vbs
:# OK, there's already an instance of the VBS script. Check if this batch is newer.
for %%f in ("%BAT%") do set "BATDATE=%%~tf" &:# Date of the previous instance in %TEMP%
xcopy /d /y "%~f0" "%BAT%" >NUL	&:# Update the %BAT% if this one is newer
for %%f in ("%BAT%") do set "NEWDATE=%%~tf" &:# Date of the (possibly) updated file
if "%NEWDATE%"=="%BATDATE%" goto :up-to-date
:create_vbs
findstr /E "'VBS" <"%~f0" >"%VBS%"
:up-to-date

:# Run the VBS script
setlocal EnableExtensions EnableDelayedExpansion
:next_item
%POPARG%
if "!ARG!"=="." set "ARG=*" & set ^""ARG"=*^" &:# Workaround to avoid getting abs paths is this case
:# Append \* to the pathname if it's a directory, to force searching files inside
call :filename %"ARG"% NAME
%DEBUG.ECHOVARS% NAME
call :has_wildcards "%NAME%"
if errorlevel 1 ( :# If the PATHNAME does NOT yet have wildcards
  call :is_dir %"ARG"%
  if not errorlevel 1 ( :# If it's a directory, then search all files in that dir
    set "ARG=!ARG!\*"
    set ^""ARG"="!ARG!\*"^"
  )
)
%DEBUG.ECHOVARS% ARG
if %RECURSE%==0 (	:# Simple case when not recursive
  call :CheckEOL "%ARG%"
) else (		:# Complex case when recursive
  :# 'for /r' outputs absolute paths. But we want relative paths in the output
  :# if that's what we provided in the argument.
  :# So compute the absolute path that must be removed before the relative path.
  for %%f in ("%ARG%") do set "ABSDIR=%%~dpf"
  %DEBUG.ECHOVARS% ABSDIR
  call :dirname "%ARG%" ARGDIR &:# %ARGDIR% = The path part of %ARG%
  %DEBUG.ECHOVARS% ARGDIR
  :# Now remove %ARGDIR% from the right end of %ABSDIR%
  if defined ARGDIR (
    set "ABSDIR=!ABSDIR!>" &:# Anchor the replacement to the end of string
    for %%d in ("!ARGDIR!>") do set "ABSDIR=!ABSDIR:%%~d=!"
    %DEBUG.ECHOVARS% ABSDIR
  )
  call :filename "%ARG%" FILE
  if defined ARGDIR pushd "!ARGDIR!"
  for /r %%d in (.) do (
    set "DIR=%%~d"
    set "DIR=!DIR:~0,-1!" &:# Remove the trailing .
    if defined ABSDIR for %%a in ("!ABSDIR!") do set "DIR=!DIR:%%~a=!"
    %DEBUG.ECHOVARS% DIR
    pushd "%CD%"
    call :CheckEOL "!DIR!!FILE!"
    popd
  )
  popd
)
if defined ARGS goto :next_item
%RETURN%

:# Expand wildcards, if any, and call the VBScript to check line endings
:CheckEOL FILE
%DEBUG.ECHO% call :CheckEOL %1
if not exist %1 %RETURN% 1
set "FILES="
for %%f in ("%~1") do set FILES=!FILES! "%%~f"
%DEBUG.ECHOVARS% FILES
cscript //nologo "%VBS%" !FILES!
%RETURN%

:# VBScript adapted from sample in https://www.computing.net/answers/programming/batch-to-detect-unix-and-windows-line-endings/24948.html

Set fso = CreateObject("Scripting.FileSystemObject")		'VBS
Set rxWin = New RegExp : rxWin.Pattern = "\r\n"                 'VBS
Set rxUnix = New RegExp : rxUnix.Pattern = "\n"	                'VBS
Set rxMac = New RegExp : rxMac.Pattern = "\r"	                'VBS
                                                                'VBS
For Each arg In WScript.Arguments                               'VBS
  file = fso.OpenTextFile(arg).ReadAll                          'VBS
  If rxWin.Test(file) Then                                      'VBS
    EOL = "Windows"                                             'VBS
  ElseIf rxUnix.Test(file) Then                                 'VBS
    EOL = "Unix"                                                'VBS
  ElseIf rxMac.Test(file) Then                                  'VBS
    EOL = "Mac"                                                 'VBS
  Else                                                          'VBS
    EOL = "None"                                                'VBS
  End If                                                        'VBS
  WScript.Echo EOL & "	" & arg ' fso.GetFileName(arg)          'VBS
Next 'arg                                                       'VBS
