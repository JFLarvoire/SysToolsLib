@echo off
:##############################################################################
:#                                                                            #
:#  Filename:       clean.bat                                                 #
:#                                                                            #
:#  Description:    Delete all compilation output files for a given source    #
:#                                                                            #
:#  Notes:	    Remarks begin with :# instead of rem, to avoid having     #
:#		    them displayed while single-stepping though the batch.    #
:#		    This also makes the batch more readable.                  #
:#									      #
:#  History:                                                                  #
:#   2012-01-17 JFL Created this file.                                        #
:#   2012-10-21 JFL Fixed bug if one of the target dirs does not exist.       #
:#   2015-10-27 JFL Added the BIOS and WIN95 target dirs.                     #
:#   2015-12-09 JFL Added support for a config.bat file defining an %OUTDIR%. #
:#   2017-10-26 JFL The default OUTDIR is now bin\.			      #
:#                                                                            #
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2017-10-26"
set "ARG0=%~f0"
set "SCRIPT=%~nx0"
set "TOOLS=%~dp0"

goto main

:help
echo Delete all compilation output files for a given source
echo.
echo Usage: clean [OPTIONS] {BASENAME}
echo.
echo Options:
echo   -X	Display the files to delete, but don't delete them
goto :eof

:main
set "CONFIG.BAT=config.%COMPUTERNAME%.bat"
set "CMD=zap.bat"

goto :get_arg
:next_arg
shift
:get_arg
if .%1.==.. goto help
if .%1.==.-?. goto help
if .%1.==./?. goto help
if .%1.==.-X. set "CMD=%CMD% -X" & goto :next_arg

if exist %CONFIG.BAT% call %CONFIG.BAT% &:# Possibly defines %OUTDIR%

:# Select the binary output path prefix
if not defined OUTDIR (
  set "BP=bin\"
) else if "%OUTDIR%"=="." (
  set "BP="
) else (
  set "BP=%OUTDIR%\"
)

for %%d in (ARM BIOS DOS WIN95 IA64 WINXP WIN32 WIN64) do (
  set "DIR=%BP%%%d"
  if exist !DIR! (
    pushd !DIR!
    call %CMD% -p !DIR!\ -r %1.*
    popd
  )
)
goto :eof

