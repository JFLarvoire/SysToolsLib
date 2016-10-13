###############################################################################
#									      #
#   File name	    smbios.mak						      #
#									      #
#   Description	    Specific rules for building smbios.exe.		      #
#									      #
#   Notes	    The goal is to allow building smbios.exe with optional    #
#                   OEM extension modules.                                    #
#									      #
#   History								      #
#    2016-07-04 JFL Created this file.					      #
#									      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF EXIST("smbios_hp.c")
CFLAGS=$(CFLAGS) /DHAS_SMBIOS_HP=1
!ENDIF

