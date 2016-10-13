###############################################################################
#                                                                             #
#   File name:      gpt.mak                                                   #
#                                                                             #
#   Description:    Specific rules for building gpt.exe.                      #
#                                                                             #
#   Notes:          gpt depends on the gnu-efi library.                       #
#                                                                             #
#   History:                                                                  #
#    2016-09-28 JFL jf.larvoire@hpe.com created this file.                    #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF ("$(HAS_GNUEFI)"!="1")
complain:
	@echo>con This program requires the GNUEFI library.

dirs $(O)\gpt.obj $(B)\gpt.exe: complain
	@rem Do nothing as we don't have the necessary libraries
!ENDIF
