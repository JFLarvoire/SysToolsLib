###############################################################################
#									      #
#   File name:	    driver.mak						      #
#									      #
#   Description:    Specific rules for building driver.exe.		      #
#									      #
#   Notes:	    							      #
#									      #
#   History:								      #
#    2015-12-09 JFL Created this file.					      #
#									      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\driver.obj $(B)\driver.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

