###############################################################################
#                                                                             #
#   File name:      2clip.mak                                                 #
#                                                                             #
#   Description:    Specific rules for building 2clip.exe.                    #
#                                                                             #
#   Notes:          2clip is a Windows program only.                          #
#                                                                             #
#   History:                                                                  #
#    2012-03-01 JFL jf.larvoire@hp.com created this file.                     #
#                                                                             #
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\2clip.obj $(B)\2clip.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

