@echo off
:##############################################################################
:#                                                                            #
:#  Filename        n.bat	                                              #
:#                                                                            #
:#  Description     Start Windows Notepad				      #
:#                                                                            #
:#  Notes 	    Keep compatibility with Windows 95                        #
:#                                                                            #
:#  History                                                                   #
:#   1990s      JFL Created this script.			              #
:#   2019-04-09 JFL Work around trailing spaces issue in Windows 10 v 2019-03.#
:#                                                                            #
:##############################################################################

:# Keep compatibility with Windows 95
if not "%OS%"=="Windows_NT" goto :start_95

:start_nt
:# In Windows 10 version 2019-03, notepad always appends .txt when creating a file if there are trailing spaces
start notepad.exe %*
exit /b

:start_95
:# Do not use %* to remain compatible with Windows 95
start notepad.exe %1 %2 %3 %4
