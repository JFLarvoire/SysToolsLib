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
###############################################################################

# Memory model for 16-bit C compilation and misc macros
MEM=L			# Memory model for C compilation
