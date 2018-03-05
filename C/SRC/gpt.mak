###############################################################################
#                                                                             #
#   File name:      gpt.mak                                                   #
#                                                                             #
#   Description:    Specific rules for building gpt.exe.                      #
#                                                                             #
#   Notes:          gpt depends on the gnu-efi library.                       #
#                                                                             #
#   History:                                                                  #
#    2016-09-28 JFL jf.larvoire@hpe.com created this file.                    #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=gpt.c
OBJECTS=$(O)\gpt.obj
EXENAME=gpt.exe

!IF ("$(HAS_GNUEFI)"!="1")
SKIP_THIS=This program requires the GNUEFI library.
!ENDIF
