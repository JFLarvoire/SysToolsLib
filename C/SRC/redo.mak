###############################################################################
#									      #
#   File name:	    redo.mak						      #
#									      #
#   Description:    Makefile with redo.exe-specific build rules.	      #
#									      #
#   Notes:	    Use with make.bat and DosWin32.mak.			      #
#		    Usage: make redo.exe				      #
#									      #
#		    The DOS version had to be compiled using the large memory #
#		    model, and linked with at least a 16 KB stack. This was   #
#		    due to the sorting of the directory names, which required #
#		    building (possibly) large tables of names.		      #
#		    As of 2025-12, the new redo version based on SysLib's     #
#		    WalkDirTree() uses far less memory than the original      #
#		    version. The DOS version can be built using the small     #
#		    memory model, and seems to work fine on small DOS VMs.    #
#		    Yet, as there's still some sorting done, it's safer	      #
#		    to still use the large memory model.		      #
#		    							      #
#  History:								      #
#    2003-06-16 JFL Created this file.					      #
#    2014-03-05 JFL Changed the memory model name to match 2010's dos.mak.    #
#    2025-12-18 JFL In DOS, this now works fine using the small memory model. #
#    2026-01-01 JFL In DOS, changed back to using the large memory model.     #
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

