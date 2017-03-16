###############################################################################
#									      #
#   File name	    Files.mak						      #
#									      #
#   Description     Declare the subdirectories to build recursively	      #
#									      #
#   Notes	    							      #
#									      #
#   History								      #
#    2017-03-02 JFL Created this file.                                        #
#									      #
#         © Copyright 2017 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Check which subdirectories exist, and rebuild each in the correct order
!IF !DEFINED(DIRS)
DIRS=::			# Initialize it with an invalid pathname, that we'll remove below
!IF EXIST("BiosLib") && DEFINED(DOS_CC)
DIRS=$(DIRS) BiosLib 
!ENDIF
!IF EXIST("LoDosLib") && DEFINED(DOS_CC)
DIRS=$(DIRS) LoDosLib 
!ENDIF
!IF EXIST("PModeLib") && DEFINED(DOS_CC)
DIRS=$(DIRS) PModeLib 
!ENDIF
!IF EXIST("MsvcLibX")
DIRS=$(DIRS) MsvcLibX 
!ENDIF
!IF EXIST("SysLib")
DIRS=$(DIRS) SysLib 
!ENDIF
!IF EXIST("SRC")
DIRS=$(DIRS) SRC
!ENDIF
DIRS=$(DIRS::: =)	# Remove the initial invalid pathname AND the first space appended to it
DIRS=$(DIRS:::=)	# Remove the initial invalid pathname in the unlikely case that nothing was appended
!ENDIF # !DEFINED(DIRS)

# Default log file name, etc
MODULE=SysToolsLib
