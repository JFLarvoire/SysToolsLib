###############################################################################
#                                                                             #
#   File name:      cpuid.mak                                                 #
#                                                                             #
#   Description:    Specific rules for building cpuid.exe.                    #
#                                                                             #
#   Notes:          cpuid is a DOS/WIN32 program only.                        #
#                                                                             #
#   History:                                                                  #
#    2012-10-18 JFL jf.larvoire@hp.com created this file.                     #
#    2016-04-17 JFL Avoid building the DOS version if missing required libs.  #
#    2017-05-17 JFL Prevent a link warning U4004: too many rules for target.  #
#    2018-03-02 JFL Use new variable SKIP_THIS to prevent builds.             #
#    2019-02-10 JFL Added (non)support for ARM, ARM64 and IA64.		      #
#    2022-11-09 JFL But DO support WIN64 now.                   	      #
#    2022-11-27 JFL Force the DOS app to actually be a LODOS app.	      #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="BIOS" || "$(T)"=="LODOS" || "$(T)"=="DOS"
SOURCES=cpuid.c
OBJECTS=$(O)\cpuid.obj
!IF "$(T)"=="DOS"
# T=LODOS	# Actually build a LODOS application in the DOS directory
!ENDIF
!ELSE # WIN95, WIN32, WIN64
SOURCES=cpuid.c $(O)\cpuid.rc
OBJECTS=$(O)\cpuid.obj $(O)\cpuid.res
!ENDIF

EXENAME=cpuid.exe

!IF "$(T)"=="DOS" && ("$(HAS_BIOSLIB)"!="1" || "$(HAS_LODOSLIB)"!="1" || "$(HAS_PMODELIB)"!="1")
SKIP_THIS=The DOS version of this program requires the BIOSLIB, LODOSLIB, and PMODE libraries.
!ENDIF

!IF "$(T)"!="BIOS" && "$(T)"!="LODOS" && "$(T)"!="DOS" && "$(T)"!="WIN95" && "$(T)"!="WINXP" && "$(T)"!="WIN32" && "$(T)"!="WIN64" # Ex: ARM, ARM64, IA64, etc
SKIP_THIS=There's no $(T) version of this program yet.
!ENDIF

