###############################################################################
#                                                                             #
#   File name:      junction.mak                                              #
#                                                                             #
#   Description:    Specific rules for building junction.exe.                 #
#                                                                             #
#   Notes:          junction is a Windows program only.                       #
#                                                                             #
#   History:                                                                  #
#    2021-11-28 JFL Created this file.                                        #
#    2022-01-12 JFL There are no junctions in Windows 95 either.              #
#                                                                             #
#         � Copyright 2021 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!IF "$(T)"=="DOS" || "$(T)"=="WIN95"
SKIP_THIS=There's no $(T) version of this program.
!ENDIF

CI=$(STINCLUDE)
MI=$(MSVCLIBX)
SI=$(SYSLIB)

# Include files dependencies
access.c: $(CI)\debugm.h $(CI)\tree.h$(CI)\stversion.h \
          $(MI)\stdio.h $(MI)\string.h $(MI)\errno.h $(MI)\unistd.h $(MI)\sys\stat.h \
          $(SI)\mainutil.h $(SI)\pathnames.h
