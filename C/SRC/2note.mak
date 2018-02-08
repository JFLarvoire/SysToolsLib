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
#                                                                             #
#         © Copyright 2018 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\2note.obj $(B)\2note.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

