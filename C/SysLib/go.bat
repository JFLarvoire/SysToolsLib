@echo off
:******************************************************************************
:*                                                                            *
:*  Filename:	    go.bat						      *
:*                                                                            *
:*  Description:    Build SysLib libraries				      *
:*                                                                            *
:*  Arguments:	    %1 ...	Optional arguments passed to nmake	      *
:*                                                                            *
:*  Notes:								      *
:*									      *
:*  History:                                                                  *
:*   2001-12-20 JFL Created this file.					      *
:*   2008-03-20 JFL Updated for building with Visual Studio 2005.             *
:*   2010-10-07 JFL Use make.bat, for Visual Studio 2008 and WIN64 support.   *
:*   2012-01-16 JFL Removed the setting of NODOSLIB/LODOSLIB/EFIINC/LM21PTK,  *
:*                  as they're now all set in make.bat.                       *
:*   2015-11-03 JFL Build every buildable version using make.bat.	      *
:*   2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
:*   2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      *
:*                                                                            *
:******************************************************************************

setlocal

if defined BIOSLIB call make.bat bios\syslib.lib bios\debug\syslib.lib
make syslib.lib debug\syslib.lib
