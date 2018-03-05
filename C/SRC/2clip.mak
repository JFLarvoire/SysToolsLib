###############################################################################
#                                                                             #
#   File name:      2clip.mak                                                 #
#                                                                             #
#   Description:    Specific rules for building 2clip.exe.                    #
#                                                                             #
#   Notes:          2clip is a Windows program only.                          #
#                                                                             #
#   History:                                                                  #
#    2012-03-01 JFL jf.larvoire@hp.com created this file.                     #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=2clip.c
OBJECTS=$(O)\2clip.obj
EXENAME=2clip.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF

