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
#                                                                             #
###############################################################################

!IF "$(T)"=="WIN64"
complain:
	@echo>con There's no 64-bits version of this program yet.

dirs $(O)\cpuid.obj $(B)\cpuid.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

