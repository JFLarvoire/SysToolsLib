###############################################################################
#                                                                             #
#   File name:      MsgBox.mak                                                #
#                                                                             #
#   Description:    Specific rules for building MsgBox.exe.                   #
#                                                                             #
#   Notes:          MsgBox is a Windows program only.                         #
#                                                                             #
#   History:                                                                  #
#    2016-12-31 JFL Created this file.                                        #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=MsgBox.c
OBJECTS=$(O)\MsgBox.obj
EXENAME=MsgBox.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF
