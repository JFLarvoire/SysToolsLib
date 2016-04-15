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
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\truename.obj $(B)\truename.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

