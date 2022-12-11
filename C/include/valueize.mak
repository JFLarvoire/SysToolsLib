###############################################################################
#                                                                             #
#   File name       valueize.mak                                              #
#                                                                             #
#   Description     Convert a macro to its current value                      #
#                                                                             #
#   Notes           Works around the absence of simply expanded macro         #
#                   definitions in nmake.exe.                                 #
#                                                                             #
#                   Gnu make has two types of macro definitions:              #
#                   NAME=VALUE      Recursively expanded macro definition     #
#                   NAME:=VALUE     Simply expanded macro definition          #
#                   Microsoft nmake.exe only has the first kind of definition.#
#                                                                             #
#                   nmake.exe records the successive definitions of macros,   #
#		    but never their current value. This makes it very         #
#		    difficult to detect changes to a macro value. Ex:         #
#                   X=1                                                       #
#                   X0=$(X)                                                   #
#                   X=2                                                       #
#                   !IF "$(X)"!="$(X0)"                                       #
#                   # X change is NOT detected, because X0 is also 2 now!     #
#                                                                             #
#                   To detect such a change, use valueize.mak this way:       #
#                   X=1                                                       #
#                   VALUEIZE=X0=$(X)                                          #
#                   !INCLUDE valueize.mak                                     #
#                   X=2                                                       #
#                   !IF "$(X)"!="$(X0)"                                       #
#                   # X change IS detected, because X0 still is 1 now!        #
#                                                                             #
#   History                                                                   #
#    2022-12-10 JFL jf.larvoire@free.fr created this file.                    #
#                                                                             #
#                  (C) Copyright 2022 Jean-Francois Larvoire                  #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Use the shell PID to generate a unique pathname, to avoid conflicts in case of // builds.
VIZEMAK=$(TMP)\VALUEIZE-$(PID).mak
!IF DEFINED(MESSAGES)
!  MESSAGE Valueizing $(VALUEIZE) using $(VIZEMAK)
!ENDIF

!IF 0

# Earlier code working in more cases (?), but more complex to use.
# Invoke with two argument variables: VIZEVAR=name  VIZEVAL=value
!IF ![echo $(VIZEVAR)=$(VIZEVAL) >"$(VIZEMAK)" 2>NUL]
!  UNDEF $(VIZEVAR)
!  INCLUDE "$(VIZEMAK)"
!ELSE
!  ERROR Failed to write to "$(VIZEMAK)"
!ENDIF

!ELSE

# Final code, easier to use, with one argument variable: VALUEIZE=name=value
!IF ![for /f "delims== tokens=1" %a in ("$(VALUEIZE)") do @(echo !UNDEF %a && echo $(VALUEIZE)) >"$(VIZEMAK)" 2>NUL]
!  INCLUDE "$(VIZEMAK)"
!ELSE
!  ERROR Failed to write to "$(VIZEMAK)"
!ENDIF

!ENDIF

!UNDEF VIZEMAK
