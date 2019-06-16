###############################################################################
#									      #
#   File name:	    redo.mak						      #
#									      #
#   Description:    Makefile with redo.exe-specific build rules.	      #
#									      #
#   Notes:	    Use with make.bat and DosWin32.mak.			      #
#		    Usage: make redo.exe				      #
#									      #
#  PVCS Info:	    $Revision:$
#		    ----$Date:$
#		    --$Author: larvoire $
#		    -$Archive:$
#									      #
#  History:								      #
#									      #
#    2003-06-16 JFL Created this file.					      #
#    2014-03-05 JFL Changed the memory model name to match 2010's dos.mak.    #
#									      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Memory model for 16-bit C compilation and misc macros
MEM=L			# Memory model for C compilation

!IF "$(T)"=="DOS"
SOURCES=redo.c
!ELSE # WIN95, WIN32, WIN64
SOURCES=redo.c $(O)\redo.rc
!IF EXIST("redo.rc")	# WIN95, WIN32, WIN64
SOURCES=redo.c redo.rc
!ENDIF
!ENDIF

