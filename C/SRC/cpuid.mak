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
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

SOURCES=cpuid.c
OBJECTS=$(O)\cpuid.obj
EXENAME=cpuid.exe

!IF "$(T)"=="DOS" && ("$(HAS_BIOSLIB)"!="1" || "$(HAS_LODOSLIB)"!="1" || "$(HAS_PMODELIB)"!="1")
SKIP_THIS=The DOS version of this program requires the BIOSLIB, LODOSLIB, and PMODE libraries.
!ENDIF

!IF "$(T)"=="WIN64" || "$(T)"=="ARM" || "$(T)"=="ARM64" || "$(T)"=="IA64"
SKIP_THIS=There's no $(T) version of this program yet.
!ENDIF

