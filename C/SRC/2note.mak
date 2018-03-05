###############################################################################
#                                                                             #
#   File name:      2note.mak                                                 #
#                                                                             #
#   Description:    Specific rules for building 2note.exe.                    #
#                                                                             #
#   Notes:          2note is a Windows program only.                          #
#                                                                             #
#   History:                                                                  #
#    2018-02-08 JFL jf.larvoire@hp.com created this file.                     #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2018 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=2note.c
OBJECTS=$(O)\2note.obj
EXENAME=2note.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF

