@if (@Language == @Batch) @then // JScript preprocessor block protecting the batch section
@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    DumpLink.bat					      *
:#                                                                            *
:#  Description:    Display the contents of .lnk program shortcuts	      *
:#                                                                            *
:#  Notes:	    							      *
:#                                                                            *
:#  History:                                                                  *
:#   2019-02-03 JFL Created this script.				      *
:#                                                                            *
:#        © Copyright 2019 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license: www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************
setlocal EnableExtensions DisableDelayedExpansion
set "VERSION=2019-02-03"
set "SCRIPT=%~nx0"				&:# Script name
set "SPATH=%~dp0" & set "SPATH=!SPATH:~0,-1!"	&:# Script path, without the trailing \
set "SFULL=%~f0"				&:# Script full pathname
set ^"ARG0=%0^"					&:# Script invokation name
set ^"ARGS=%*^"					&:# Argument line
set ARGS=%*

goto :Main

:#----------------------------------------------------------------------------#

:Help
echo.
echo %SCRIPT% - Display the contents of .lnk program shortcuts
echo.
echo Usage: %SCRIPT% [OPTIONS] [PATHNAME [...]]
echo.
echo Options:
echo   -?    Display this help
echo   -V    Display the script version
echo.
echo Pathname: The pathname of an existing *.lnk shortcut. Wildcards accepted.
exit /b

:#----------------------------------------------------------------------------#

:Main
set "BAT=%TEMP%\%~n0.bat"	&:# Copy of this batch file in the %TEMP% directory
set "VBS=%TEMP%\%~n0.vbs"	&:# The VBS file that we extract from this file
set "LIST="

goto :get_arg
:next_arg
shift
:get_arg
set "ARG=%~1"
if "%ARG%"=="" goto :Start
if "%ARG%"=="/?" goto :Help
if "%ARG%"=="-?" goto :Help
if "%ARG%"=="-V" (echo.%VERSION%) & exit /b
set LIST=%LIST% %1
goto :next_arg

:Start
if not defined LIST goto Help

:# Run the VBS script for each pathname in the LIST
for %%f in (%LIST%) do if exist "%%~f" (
  echo.
  echo %%~f
  cscript //nologo //E:JScript "%SFULL%" "%%~f"
)
exit /b

:# End of the batch section, and beginning of the JScript section
@end

// Extract data from the link
var WshShell = WScript.CreateObject("WScript.Shell")
var Lnk = WshShell.Createshortcut(WScript.Arguments(0))
if (Lnk.WorkingDirectory != "") { // Not all links define a working directory
  WScript.Stdout.WriteLine("cd \"" + Lnk.WorkingDirectory + "\"")
}
if (Lnk.TargetPath != "") {	  // Not all links define a target path
  WScript.Stdout.WriteLine("\"" + Lnk.TargetPath + "\" " + Lnk.Arguments)
}
