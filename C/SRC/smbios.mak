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
#    2022-01-01 JFL Rebuild smbios.exe if one of its dependencies changes.    #
#									      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SMBIOS_DEPS=smbios_defs.c smbios_lib.c	# smbios.c dependencies

!IF EXIST("smbios_hp.c")
CFLAGS=$(CFLAGS) /DHAS_SMBIOS_HP=1
SMBIOS_DEPS=$(SMBIOS_DEPS) smbios_hp.c
!ENDIF

smbios.c: $(SMBIOS_DEPS)
