###############################################################################
#                                                                             #
#   File name:      ShareInfo.mak                                             #
#                                                                             #
#   Description:    Specific rules for building ShareInfo.exe.                #
#                                                                             #
#   Notes:          ShareInfo is a Windows program only.                      #
#                                                                             #
#   History:                                                                  #
#    2021-11-24 JFL Created this file.                                        #
#                                                                             #
#         © Copyright 2021 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=ShareInfo.c $(O)\ShareInfo.rc
OBJECTS=$(O)\ShareInfo.obj $(O)\ShareInfo.res
EXENAME=ShareInfo.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF

