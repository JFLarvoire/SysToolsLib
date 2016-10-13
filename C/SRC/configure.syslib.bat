@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.syslib.bat				      *
:#                                                                            *
:#  Description:    Define SysLib-specific configuration settings             *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2015-10-30 JFL Created this file.					      *
:#   2016-10-07 JFL Added the dependency on the 98DDK, for Windows 95 builds. *
:#   2016-10-11 JFL Added the dependency on gnu-efi.			      *
:#   2016-10-07 JFL Removed the dependency on the 98DDK. (Fixed in sector.cpp)*
:#                                                                            *
:#         © Copyright 2016 Hewlett Packard Enterprise Development LP         *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% SYSLIB
%USE_SDK% GNUEFI
%END_SDK_DEFS%
