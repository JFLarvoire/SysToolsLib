###############################################################################
#                                                                             #
#   File name:      junction.mak                                              #
#                                                                             #
#   Description:    Specific rules for building junction.exe.                 #
#                                                                             #
#   Notes:          junction is a Windows program only.                       #
#                                                                             #
#   History:                                                                  #
#    2021-11-28 JFL Created this file.                                        #
#                                                                             #
#         © Copyright 2021 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS"
SKIP_THIS=There's no DOS version of this program.
!ENDIF
