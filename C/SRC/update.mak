###############################################################################
#									      #
#   File name:	    update.mak						      #
#									      #
#   Description:    Specific rules for building update.exe.		      #
#									      #
#   Notes:	    Under Vista and Windows 7, update.exe, setup.exe, and     #
#                   install.exe are reserved name prefixes.                   #
#                   Windows automatically attempts to switch the machine to   #
#                   Administrator mode when a program named this way is run.  #
#									      #
#		    To prevent this, a signed application manifest must be    #
#                   included in the .exe, specifiying that such a privilege   #
#                   escalation is not necessary.                              #
#									      #
#   History:								      #
#    2010-03-15 JFL Created this file.					      #
#    2017-05-12 JFL No need to add /MANIFEST to $(LFLAGS), as it just outputs #
#		     a useless copy next to the exe.			      #
#									      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS"
SOURCES=update.c
!ENDIF

!IF "$(T)"=="WIN32"
SOURCES=update.c update.rc
# LFLAGS=$(LFLAGS) /MANIFEST
!ENDIF

!IF "$(T)"=="WIN64"
SOURCES=update.c update.rc
# LFLAGS=$(LFLAGS) /MANIFEST
!ENDIF

