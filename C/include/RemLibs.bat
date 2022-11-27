@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename	    RemLibs.bat						      *
:#                                                                            *
:#  Description	    Remove libraries from the list of argument files	      *
:#                                                                            *
:#  Notes	    Useful to force relinking an executable if a dependent    *
:#		    library was updated, by putting these libraries on the    *
:#		    list of dependencies for the executable.		      *
:#		    But the MSVC 1.5 linker for DOS requires only having      *
:#		    the list of object files on the first line of its	      *
:#		    input file. Libraries are entered on a subsequent line.   *
:#		    In make file inference rules, this script allows	      *
:#		    generating the first line of the linker input file,	      *
:#		    by passing it the $** macro as argument.		      *
:#		    							      *
:#  History	                                                              *
:#   2022-11-25 JFL Created this script                                       *
:#		    							      *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

setlocal EnableExtensions EnableDelayedExpansion
set "VERSION=2022-11-25"
set ARG0=%0
goto :main

:main
set "LIST="
for %%f in (%*) do (
  set "F=%%~f"
  if /i not "!F:~-4!"==".lib" (
    if defined LIST set "LIST=!LIST! "
    set "LIST=!LIST!%%f" & rem include %%f quotes in the list
  )
)
if defined LIST echo !LIST!
