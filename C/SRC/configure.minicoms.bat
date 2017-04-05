@echo off
:#*****************************************************************************
:#                                                                            *
:#  Filename:	    configure.minicoms.bat				      *
:#                                                                            *
:#  Description:    Define MiniComs-specific configuration settings           *
:#                                                                            *
:#  Notes:	                                                              *
:#                                                                            *
:#  History:                                                                  *
:#   2015-10-27 JFL Created this file.					      *
:#   2016-04-11 JFL Renamed NODOSLIB as BIOSLIB.                              *
:#   2016-04-12 JFL Added a dependency on the MULTIOS library.                *
:#                                                                            *
:#*****************************************************************************

:# Declare the SDKs and libraries we need
%BEGIN_SDK_DEFS%
%USE_SDK% BIOSLIB
%USE_SDK% LODOSLIB
%USE_SDK% PMODELIB
%END_SDK_DEFS%
