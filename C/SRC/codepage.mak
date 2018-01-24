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
#                                                                             #
#         © Copyright 2017 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\codepage.obj $(B)\codepage.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

!IF "$(T)"=="WIN95"
complain:
	@echo>con There's no Windows 95 version of this program.

dirs $(O)\codepage.obj $(B)\codepage.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF
