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
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\truename.obj $(B)\truename.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

