###############################################################################
#                                                                             #
#   File name:      font.mak                                                  #
#                                                                             #
#   Description:    Specific rules for building font.exe.                     #
#                                                                             #
#   Notes:          font is a Windows program only.                           #
#                                                                             #
#   History:                                                                  #
#    2018-01-25 JFL Created this file.                                        #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2018 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=font.c $(O)\font.rc
OBJECTS=$(O)\font.obj $(O)\font.res
EXENAME=font.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF

!IF "$(T)"=="WIN95"
SKIP_THIS=There's no WIN95 version of this program.
!ENDIF
