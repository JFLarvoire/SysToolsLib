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
:#   2016-11-03 JFL Removed the side effect creating %OUTDIR%.		      *
:#   2016-11-07 JFL Removed the dependency on OUTDIR.			      *
:#                  Immediately set the system environment.		      *
:#   2016-11-08 JFL Removed the dependency on OUTDIR.			      *
:#   2016-11-16 JFL Allow using a predefined alias for this lib base path.    *
:#   2016-12-16 JFL Only use setx if requested by user, with PERSISTENT_VARS. *
:#   2017-03-12 JFL Don't automatically set the SYSLIB variable.              *
:#   2023-01-14 JFL Added library dependencies for BIOS & LODOS builds.       *
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Get the full pathname of the SysLib library work directory
if defined SYSLIB if not exist SysLib.h set "SYSLIB=" &:# Allow overriding with another alias name, but ignore invalid overrides
if not defined SYSLIB set "SYSLIB=%CD%" &:# Default: Use the current directory

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% BIOSLIB &:# Used for building OS=BIOS
%USE_SDK% LODOSLIB &:# Used for building OS=LODOS
%USE_SDK% MSVCLIBX &:# Used for building OS=DOS, and all Windows flavors
%USE_SDK% GNUEFI
if defined VC95.CC %USE_SDK% 98DDK &:# We only need it for building for Windows95
if defined VC16.CC %USE_SDK% LMPTK &:# We only need it for building for DOS
%USE_SDK% SYSLIB &:# Triggers the emission of a %CONFIG% record for SYSLIB itself
%END_SDK_DEFS%

:# Set the local environment variable just before make exits, so that future commands in this CMD window have it.
if defined PERSISTENT_VARS %ADD_POST_MAKE_ACTION% set "SYSLIB=%SYSLIB%"

:# Set the system environment variable, so that other CMD windows opened later on inherit it
if defined PERSISTENT_VARS setx SYSLIB "%SYSLIB%" >NUL
