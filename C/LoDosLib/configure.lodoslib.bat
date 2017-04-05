@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.lodoslib.bat				      *
:#                                                                            *
:#  Description:    Define LoDosLib-specific configuration settings           *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2015-10-16 JFL Created this file.					      *
:#   2016-11-16 JFL Allow using a predefined alias for this lib base path.    *
:#   2016-12-16 JFL Only use setx if requested by user, with PERSISTENT_VARS. *
:#                                                                            *
:#     (c) Copyright 2015-2017 Hewlett Packard Enterprise Development LP      *
:# Licensed under the Apache 2.0 license: www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Get the full pathname of the lodos.lib library work directory
if defined LODOSLIB if not exist lodos.h set "LODOSLIB=" &:# Allow overriding with another alias name, but ignore invalid overrides
if not defined LODOSLIB set "LODOSLIB=%CD%" &:# Default: Use the current directory

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% BIOSLIB
%USE_SDK% LODOSLIB
%END_SDK_DEFS%

:# Set the local environment variable just before make exits, so that future commands in this CMD window have it.
%ADD_POST_MAKE_ACTION% set "LODOSLIB=%LODOSLIB%"

:# Set the system environment variable, so that other CMD windows opened later on inherit it
if defined PERSISTENT_VARS setx LODOSLIB "%LODOSLIB%" >NUL

