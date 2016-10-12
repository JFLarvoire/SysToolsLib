@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    make.bat						      *
:#                                                                            *
:#  Description:    Build DOS and Windows targets			      *
:#                                                                            *
:#  Notes:	    Proxy script for include\make.bat.			      *
:#                                                                            *
:#                  If any change is needed, put it in include\make.bat.      *
:#                                                                            *
:#  History:                                                                  *
:#   2016-10-10 JFL jf.larvoire@hpe.com created this file.		      *
:#                                                                            *
:#        © Copyright 2016 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

if not exist include\make.bat (
  >&2 echo Error: Cannot find SysToolsLib's include directory.
  exit /b 1
)

include\make.bat %*
