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
:#   2016-09-27 JFL Correct the final MSVCLIBX if there's a different OUTDIR. *
:#   2016-11-03 JFL Removed the side effect creating %OUTDIR%.		      *
:#   2016-11-07 JFL Removed the dependency on OUTDIR.			      *
:#                  Immediately set the system environment.		      *
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Get the full pathname of the MsvcLibX library base directory
for /f "delims=" %%d in ('"pushd .. & cd & popd"') do SET MSVCLIBX=%%d

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% MSVCLIBX &:# Triggers the emission of a %CONFIG% record for MSVCLIBX
%END_SDK_DEFS%

:# Set the local environment variable just before make exits, so that future commands in this CMD window have it.
%ADD_POST_MAKE_ACTION% set "MSVCLIBX=%MSVCLIBX%"

:# Set the system environment variable, so that other CMD windows opened later on inherit it
setx MSVCLIBX "%MSVCLIBX%" >NUL
