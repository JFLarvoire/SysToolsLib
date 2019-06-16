###############################################################################
#                                                                             #
#   File name:      1clip.mak                                                 #
#                                                                             #
#   Description:    Specific rules for building 1clip.exe.                    #
#                                                                             #
#   Notes:          1clip is a Windows program only.                          #
#                                                                             #
#   History:                                                                  #
#    2010-09-21 JFL Created this file.                                        #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=1clip.c $(O)\1clip.rc
OBJECTS=$(O)\1clip.obj $(O)\1clip.res
EXENAME=1clip.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF

