###############################################################################
#									      #
#   File name:	    SysLib.mak						      #
#									      #
#   Description:    Microsoft-OS-specific files included in SysLib.lib.       #
#									      #
#   Notes:	    							      #
#		    							      #
#   History:								      #
#    2015-11-04 JFL Split the old MultiOS.mak into NMakeFile and Files.mak.   #
#    2016-04-22 JFL Renamed the MULTIOS library as SYSLIB.		      #
#		    Added HardDisk.cpp and LogDisk.cpp rules for WIN95.	      #
#    2016-09-30 JFL Rewrote clean rules for building C files that	      #
#		    conditionally include other C files.		      #
#    2021-03-01 JFL Compile R0Ios.c, Ring0.c, VxDCall.c only for WIN95.       #
#    2022-11-28 JFL Added support for LODOS OS type builds.                   #
#		    							      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

!INCLUDE "Files.mak"	# Include common files and files dependency definitions

!IF "$(T)"=="BIOS" || "$(T)"=="LODOS"
OBJECTS = $(BASE_OBJECTS)
!ELSE
OBJECTS = $(COMMON_OBJECTS)
!ENDIF

OBJECTS = $(OBJECTS:/=\)	\
    +$(O)\Block.obj		\
!IF DEFINED(GNUEFI)
    +$(O)\Crc32.obj		\
!ENDIF
!IF "$(T)"!="BIOS" && "$(T)"!="LODOS"
    +$(O)\File.obj		\
!ENDIF
    +$(O)\FloppyDisk.obj	\
!IF DEFINED(GNUEFI) # GPTs use EFI and UUIDs
    +$(O)\Gpt.obj		\
!ENDIF
    +$(O)\HardDisk.obj		\
    +$(O)\HDiskVar.obj		\
!IF "$(T)"!="BIOS"
    +$(O)\LogDisk.obj		\
!ENDIF
    +$(O)\MacAddr.obj		\
!IF !("$(T)"=="BIOS" || "$(T)"=="LODOS" || ("$(T)"=="DOS" && !DEFINED(LMPTK)))
    +$(O)\NetBios.obj		\
!ENDIF
    +$(O)\SmBios.obj		\
!IF ("$(T)"=="WIN95") && DEFINED(HAS_98DDK)
    +$(O)\R0Ios.obj		\
    +$(O)\Ring0.obj		\
    +$(O)\VxDCall.obj		\
!ENDIF

# Now build rules, that should only be included in the final part of $(T).mak
!IF DEFINED(B)

h2inc: $(SYSLIB)\int13.inc

# Force building C files that conditionally include other C files
$(O)\HardDisk.obj: $(S)\HardDisk.cpp \
!IF "$(T)"=="BIOS" || "$(T)"=="LODOS" || "$(T)"=="DOS"
                   $(S)\HDiskdos.cpp
!ELSEIF "$(T)"=="WIN95"
                   $(S)\HDiskw32.cpp $(S)\HDisk95.cpp
!ELSE # "$(T)"=="WIN32" || "$(T)"=="WIN64" || "$(T)"=="IA64" || etc...
                   $(S)\HDiskw32.cpp
!ENDIF

$(O)\LogDisk.obj: $(S)\LogDisk.cpp \
!IF "$(T)"=="BIOS" || "$(T)"=="LODOS" || "$(T)"=="DOS"
                  $(S)\LDiskdos.cpp
!ELSEIF "$(T)"=="WIN95"
                  $(S)\LDiskw32.cpp $(S)\LDisk95.cpp
!ELSE # "$(T)"=="WIN32" || "$(T)"=="WIN64" || "$(T)"=="IA64" || etc...
                  $(S)\LDiskw32.cpp
!ENDIF

!ENDIF # DEFINED(B)
