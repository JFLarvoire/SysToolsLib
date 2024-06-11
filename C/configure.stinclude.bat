@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.stinclude.bat				      *
:#                                                                            *
:#  Description:    Configure SysToolsLib's INCLUDE directory path            *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2016-10-10 JFL jf.larvoire@hpe.com created this script.		      *
:#   2016-12-16 JFL Only use setx if requested by user, with PERSISTENT_VARS. *
:#   2024-01-07 JFL Changed STINCLUDE test file from debugm.h to stversion.h. *
:#                  Add STINCLUDE to the list of SDKs we depend on.           *
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Get the full pathname of the include subdirectory
if defined STINCLUDE if not exist "%STINCLUDE%\stversion.h" set "STINCLUDE=" &:# Allow overriding with another alias name, but ignore invalid overrides
if not defined STINCLUDE for /f "delims=" %%d in ('"pushd include & cd & popd"') do SET "STINCLUDE=%%d" &:# Default: Use the current directory

:# Set the system environment variable, so that other CMD windows opened later on inherit it
if defined PERSISTENT_VARS setx STINCLUDE "%STINCLUDE%" >NUL

:# Make sure the variable is recorded as an SDK in the config.%COMPUTERNAME%.bat file
%BEGIN_SDK_DEFS%
%USE_SDK% STINCLUDE
%END_SDK_DEFS%
