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
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\MsgBox.obj $(B)\MsgBox.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF
