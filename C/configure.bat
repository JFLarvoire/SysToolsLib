@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.bat					      *
:#                                                                            *
:#  Description:    Detect system-specific settings and create config.*.bat   *
:#                                                                            *
:#  Notes:	    Proxy script for include\configure.bat.		      *
:#                                                                            *
:#                  If any change is needed, put it in include\configure.bat. *
:#                                                                            *
:#  History:                                                                  *
:#   2016-10-10 JFL jf.larvoire@hpe.com created this file.		      *
:#                                                                            *
:#        © Copyright 2016 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

if not exist include\configure.bat (
  >&2 echo Error: Cannot find SysToolsLib's include directory.
  exit /b 1
)

include\configure.bat %*
