@echo off
:# Display all command line arguments.
:# This is not as easy as it seems, as the last argument may contain mismatched " quotes.
:# Such mismatched quotes break the syntax of many commands.

:# Enable the syntax extensions, to get the "if defined" and "set /a" constructs.
:# But disable delayed expansion, to avoid issues with '!' characters.
setlocal enableextensions disableDelayedExpansion

:# Process command line arguments.
set N=1
goto get_args
:next_arg
shift
:get_args
set ARG=%1
:# If there's no ARG, we're done.
:# Note: Testing it with "if .%1.==.. ()" breaks in case of mismatched " quotes.
if not defined ARG goto :eof
echo ARG%N% = %1
set /a N=N+1
goto next_arg
