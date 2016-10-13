@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.SysLib.bat				      *
:#                                                                            *
:#  Description:    Define SysLib-specific configuration settings	      *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2015-10-20 JFL Created this file.					      *
:#   2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
:#   2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
:#   2016-07-12 JFL Removed dependencies on BIOSLIB and LODOSLIB, now replaced*
:#                  by that on MSVCLIBX, even for building the BIOS version.  *
:#   2016-09-27 JFL Correct the final SYSLIB if there's a different OUTDIR.   *
:#   2016-10-07 JFL Avoid useless warnings if the necessary compilers are miss.
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Get the full pathname of the SysLib library work directory
set "SYSLIB=%CD%"

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% MSVCLIBX
%USE_SDK% GNUEFI
if defined VC95.CC %USE_SDK% 98DDK &:# We only need it for building for Windows95
if defined VC16.CC %USE_SDK% LMPTK &:# We only need it for building for DOS
%USE_SDK% SYSLIB &:# Triggers the emission of a %CONFIG% record for SYSLIB itself
%END_SDK_DEFS%

:# Define where other dependant libraries should look for SYSLIB
set "BUILT_SYSLIB=%SYSLIB%"    &:# By default, use the local instance
if defined OUTDIR (		:# But if there's a different OUTDIR, use it instead 
  if not exist "%OUTDIR%" md "%OUTDIR%"
  pushd "%OUTDIR%"
  set "BUILT_SYSLIB=!CD!"
  popd
)

:# Set the local environment variable just before make exits, so that future commands in this CMD window have it.
%ADD_POST_MAKE_ACTION% set "SYSLIB=%BUILT_SYSLIB%"

:# Set the system environment variable, so that other CMD windows opened later on inherit it
%ADD_POST_MAKE_ACTION% setx SYSLIB "%BUILT_SYSLIB%" ^>NUL
