@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.bat					      *
:#                                                                            *
:#  Description:    Detect system-specific settings and create config.*.bat   *
:#                                                                            *
:#  Notes:	    Proxy script for %STINCLUDE%\configure.bat.	      *
:#                                                                            *
:#                  Make any change needed in %STINCLUDE%\configure.bat.     *
:#                                                                            *
:#  History:                                                                  *
:#   2016-10-10 JFL jf.larvoire@hpe.com created this file.		      *
:#                                                                            *
:#        © Copyright 2016 Hewlett Packard Enterprise Development LP          *
:# Licensed under the Apache 2.0 license  www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

if not defined STINCLUDE ( :# Try getting the copy in the master environment
  for /f "tokens=3" %%v in ('reg query "HKCU\Environment" /v STINCLUDE 2^>NUL') do set "STINCLUDE=%%v"
)

if not exist %STINCLUDE%\configure.bat (
  >&2 echo %0 Error: Cannot find SysToolsLib's global C include directory "%STINCLUDE%".
  exit /b 1
)

"%STINCLUDE%\configure.bat" %*
