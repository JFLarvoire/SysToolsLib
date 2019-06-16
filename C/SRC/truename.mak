###############################################################################
#                                                                             #
#   File name:      truename.mak                                              #
#                                                                             #
#   Description:    Specific rules for building truename.exe.                 #
#                                                                             #
#   Notes:          truename is a Windows program only.                       #
#                                                                             #
#   History:                                                                  #
#    2014-02-07 JFL jf.larvoire@hp.com created this file.                     #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=truename.c $(O)\truename.rc
OBJECTS=$(O)\truename.obj $(O)\truename.res
EXENAME=truename.exe

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF

