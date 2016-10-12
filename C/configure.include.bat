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
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Get the full pathname of the include subdirectory
for /f "delims=" %%d in ('"pushd include & cd & popd"') do SET "STINCLUDE=%%d"

:# Set the system environment variable, so that other CMD windows opened later on inherit it
setx STINCLUDE "%STINCLUDE%" >NUL
