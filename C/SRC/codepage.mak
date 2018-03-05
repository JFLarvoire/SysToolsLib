###############################################################################
#                                                                             #
#   File name:      codepage.mak                                              #
#                                                                             #
#   Description:    Specific rules for building codepage.exe.                 #
#                                                                             #
#   Notes:          codepage is a Windows program only.                       #
#                                                                             #
#   History:                                                                  #
#    2017-03-16 JFL Created this file.                                        #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2017 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=codepage.c
OBJECTS=$(O)\codepage.obj
EXENAME=codepage.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF
