@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    go.bat						      *
:#                                                                            *
:#  Description:    Build the pmode.lib library				      *
:#                                                                            *
:#  Notes:	    Old command for buiding the library in DOS and Windows.   *
:#                  Now superseded by the configure.bat/make.bat system.      *
:#                                                                            *
:#                  This is a batch file for DOS and Windows 95.              *
:#                  Do not use cmd.exe extensions to command.com syntax.      *
:#                                                                            *
:#                  The paths in the beginning must be adapted to your config.*
:#                                                                            *
:#  History:                                                                  *
:#   1999-08-31 JFL Use VC++ 6.0 nmake instead of VC++ 5.                     *
:#   2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
:#                                                                            *
:#     (c) Copyright 1995-2017 Hewlett Packard Enterprise Development LP      *
:# Licensed under the Apache 2.0 license: www.apache.org/licenses/LICENSE-2.0 *
:#*****************************************************************************

SET AS=C:\MASM\BIN\ML
SET CC=C:\MSVC\BIN\CL
SET INCLUDE=
SET LIB=
SET LIBEXE=C:\MSVC\BIN\LIB
SET LNK=C:\MSVC\BIN\LINK
:# Note: The nmake from VC++ 1.52 fails to detect errorlevels!
:#       Use that of VC++ 5.0 or 6.0 instead.
SET NMAKE=C:\PROGRA~2\MICROS~1.0\VC\BIN\nmake
SET BIOSLIB=C:\JFL\LIBS\BIOSLIB
SET LODOSLIB=C:\JFL\LIBS\LODOSLIB
SET PMODELIB=C:\JFL\LIBS\PMODE
echo.
%NMAKE% %1 %2 %3 %4 %5 /f pmode.mak /x - >pmode.log
echo.
type pmode.log
