@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.MsvcLibX.bat				      *
:#                                                                            *
:#  Description:    Special make actions for rebuilding the MsvcLibX library  *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2015-11-06 JFL Created this script.				      *
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *

:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *

:#*****************************************************************************

:# Get the full pathname of the MsvcLibX library base directory
for /f "delims=" %%d in ('"pushd .. & cd & popd"') do SET MSVCLIBX=%%d

:# Set the environment variable in config.h for use in the make file
%CONFIG% set "MSVCLIBX=%MSVCLIBX%"

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% MSVCLIBX
%END_SDK_DEFS%

:# Set the local environment variable just before make exits, so that future commands in this CMD window have it.
%CONFIG% set POSTMAKEACTION=set "MSVCLIBX=%MSVCLIBX%"

:# Set the system environment variable, so that other CMD windows opened later on inherit it
setx MSVCLIBX %MSVCLIBX% >NUL

