###############################################################################
#                                                                             #
#   File name:      1clip.mak                                                 #
#                                                                             #
#   Description:    Specific rules for building 1clip.exe.                    #
#                                                                             #
#   Notes:          1clip is a Windows program only.                          #
#                                                                             #
#   History:                                                                  #
#    2010-09-21 JFL Created this file.                                        #
#                                                                             #
###############################################################################

!IF "$(T)"=="DOS"
complain:
	@echo>con There's no DOS version of this program.

dirs $(O)\1clip.obj $(B)\1clip.exe: complain
	@rem Do nothing as there's nothing to do
!ENDIF

!IF 0

!IF "$(T)"=="WIN32"
SOURCES=1clip.c
LFLAGS=$(LFLAGS) /SUBSYSTEM:WINDOWS
!ENDIF

!IF "$(T)"=="WIN64"
SOURCES=1clip.c
LFLAGS=$(LFLAGS) /DEF:1clip.def /SUBSYSTEM:WINDOWS
!ENDIF

!ENDIF
