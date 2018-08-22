@echo off
:##############################################################################
:#                                                                            #
:#  Filename        hosts.bat                                                 #
:#                                                                            #
:#  Description     Open Notepad to edit the system's /etc/hosts file         #
:#                                                                            #
:#  Notes 	    The hosts file can be seen by all users,                  #
:#                  but it can only be updated by administrators.             #
:#                  This script attemps to switch to administrator mode,      #
:#                  to avoid getting stuck with changes that can't be saved.  #
:#                                                                            #
:#  History                                                                   #
:#   2013-09-10 JFL jf.larvoire@hpe.com created this script.                  #
:#   2018-06-28 JFL Use elevate.exe if it is available, else do without.      #
:#		    Added option -X to test what command is executed.         #
:#		                                                              #
:#      © Copyright 2016-2018 Hewlett Packard Enterprise Development LP       #
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 #
:##############################################################################

setlocal EnableExtensions EnableDelayedExpansion
goto :main

:# runas /user:Administrator "%windir%\System32\notepad.exe" "%windir%\System32\Drivers\etc\hosts"
:# runas /user:%USERDOMAIN%\%USERNAME% "%windir%\System32\notepad.exe" "%windir%\System32\Drivers\etc\hosts"

:# Display arguments, skipping spaces ahead
:echo
echo %*
exit /b

:# Quote file pathnames that require it. %1=Input/output variable.
:condquote
:# Assumes EnableExtensions EnableDelayedExpansion
:# If the value is empty, don't go any further.
if not defined %1 set "%1=""" & exit /b
:# Remove double quotes inside %1. (Fails if %1 is empty, which we excluded above)
set ^"%1=!%1:"=!"
:# Look for any special character that needs "quoting". See list from (cmd /?).
:# Added "@" that needs quoting ahead of commands.
:# Added "|&<>" that are not valid in file names, but that do need quoting if used in an argument string.
echo."!%1!"|findstr /C:" " /C:"&" /C:"(" /C:")" /C:"[" /C:"]" /C:"{" /C:"}" /C:"^^" /C:"=" /C:";" /C:"!" /C:"'" /C:"+" /C:"," /C:"`" /C:"~" /C:"@" /C:"|" /C:"&" /C:"<" /C:">" >NUL
if not errorlevel 1 set %1="!%1!"
exit /b

:# Test if the user has administrator rights
:IsAdmin
>NUL 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
goto :eof

:# Main routine
:main
set "ELEVATE="
set "EXEC=start"
set "NOTEPAD=%windir%\System32\notepad.exe"
set "HOSTS=%windir%\System32\Drivers\etc\hosts"

if [%1]==[-X] set "EXEC=call :echo"	&:# Display the command, but don't run it

:# Check if we're running as administrator already
call :IsAdmin
if not errorlevel 1 goto :go &:# We are. Go run the notepad command directly.

:# We're not running as administrator. Find a way to switch to administrator mode.

:# The elevate tool from http://www.winability.com/elevate is the most convenient,
:# as it avoids the need for prompting the user with the admin password.
where elevate >NUL 2>&1 &:# Check if elevate.exe is available
if not errorlevel 1 set "ELEVATE=elevate" & goto :go

echo Warning: This shell is not running as Administrator, so you won't be allowed to save any change.

:go
call :condquote NOTEPAD
call :condquote HOSTS
%EXEC% %ELEVATE% %NOTEPAD% %HOSTS%
