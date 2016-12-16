@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.include.bat				      *
:#                                                                            *
:#  Description:    Special make actions for recording the include dir. path  *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2016-10-10 JFL jf.larvoire@hpe.com created this script.		      *
:#   2016-12-16 JFL Only use setx if requested by user, with PERSISTENT_VARS. *
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Get the full pathname of the include subdirectory
if defined STINCLUDE if not exist "%STINCLUDE%\debugm.h" set "STINCLUDE=" &:# Allow overriding with another alias name, but ignore invalid overrides
if not defined STINCLUDE for /f "delims=" %%d in ('"pushd include & cd & popd"') do SET "STINCLUDE=%%d" &:# Default: Use the current directory

:# Set the system environment variable, so that other CMD windows opened later on inherit it
if defined PERSISTENT_VARS setx STINCLUDE "%STINCLUDE%" >NUL
