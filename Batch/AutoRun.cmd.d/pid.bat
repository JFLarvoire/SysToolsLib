@echo off
:##############################################################################
:#                                                                            #
:#  Filename        pid.bat                                                   #
:#                                                                            #
:#  Description     Define a %PID% variable in cmd.exe, like $$ in bash.      #
:#                                                                            #
:#  Notes 	    This script is not intended to be used directly.          #
:#                  It's an AutoRun script, intended to be run by AutoRun.cmd.#
:#                  For details, limitations, and how to install it, see:     #
:#		    https://github.com/JFLarvoire/SysToolsLib/tree/master/Batch/AutoRun.cmd.d
:#                                                                            #
:#                  Once installed, every instance of cmd.exe has a %PID%     #
:#                  variable defined, containing the current Process ID.      #
:#                  This is similar to the $$ variable in Unix shells.        #
:#                                                                            #
:#                  It uses the :getPID routine published on dostips.com:     #
:#                  https://www.dostips.com/forum/viewtopic.php?p=38870#p38870
:#                                                                            #
:#  History                                                                   #
:#   2019-02-05 JFL Created this script.                                      #
:#		                                                              #
:##############################################################################

call :getPID PID
exit /b

:getPID  [RtnVar]
::
:: Store the Process ID (PID) of the currently running script in environment variable RtnVar.
:: If called without any argument, then simply write the PID to stdout.
::
setlocal disableDelayedExpansion
:getLock
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
  ) do for %%B in (%%A) do set "PID=%%B"
  (call )
))||goto :getLock
del "%lock%" 2>nul
endlocal & if "%~1" equ "" (echo(%PID%) else set "%~1=%PID%"
exit /b
