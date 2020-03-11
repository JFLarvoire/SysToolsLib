###############################################################################
#									      #
#   File name:	    Files.mak						      #
#									      #
#   Description:    Define files included in SysLib.lib.		      #
#									      #
#   Notes:	    Shared between the DOS/Windows and Unix make files.	      #
#		    Do not use any OS-specific make syntax, such as           #
#		    conditional directives.				      #
#		    							      #
#   History:								      #
#    2015-11-04 JFL Split the old MultiOS.mak into NMakeFile and Files.mak.   #
#    2016-10-11 JFL moved debugm.h to SysToolsLib global C include dir.       #
#    2020-03-11 JFL Added Unix-specific object modules.                       #
#									      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# List of object files to build and include in the SysLib library
# IMPORTANT NOTE: Every time you add an object file in the list here, also
#                 store its specific source file dependancies below.
#		  (Including for MS-specific objects)

# Object files, grouped by category, and sorted alphabetically in each group.

# Common objects usable in all operating systems with a Standard C library
COMMON_OBJECTS = \
    +$(O)/IsMBR.obj		\
    +$(O)/oprintf.obj		\
    +$(O)/oprintf6.obj		\
    +$(O)/oprintf7.obj		\
    +$(O)/oprintf8.obj		\
    +$(O)/PcUuid.obj		\
    +$(O)/PrintUuid.obj		\
    +$(O)/qword.obj		\
    +$(O)/uuid.obj		\
    +$(O)/uuidnull.obj		\

# Microsoft-OS-specific objects are defined conditionally in SysLib.mak
# MS_OBJECTS = \
#     $(COMMON_OBJECTS) \
# !IF DEFINED(...)
#     +$(O)/xxx.obj
#     ...
# !ENDIF

# Unix-specific objects
UNIX_OBJECTS = \
    $(O)/dirx.o			\

# Objects usable in Unix
OBJECTS1 = $(COMMON_OBJECTS:+=)
OBJECTS = $(OBJECTS1:.obj=.o) $(UNIX_OBJECTS)

#-----------------------------------------------------------------------------#
#			Include files dependancies			      #
#-----------------------------------------------------------------------------#

$(S)/Block.cpp: $(S)/Block.h $(S)/File.h $(S)/FloppyDisk.h $(S)/HardDisk.h $(S)/LogDisk.h

$(S)/Block.h: $(S)/SysLib.h $(S)/qword.h

$(S)/crc32.cpp: $(S)/crc32.h \
		$(GNUEFI)/inc/efi.h $(GNUEFI)/inc/efilib.h

$(S)/crc32.h: $(S)/SysLib.h

$(S)/efibind.h: $(S)/qword.h

$(S)/FDisk95.cpp: $(S)/FloppyDisk.h $(S)/int13.h $(S)/VxDCall.h

$(S)/FDiskDOS.cpp: $(S)/FloppyDisk.h $(S)/int13.h

$(S)/FDiskNT.cpp: $(S)/FloppyDisk.h

$(S)/FDiskW32.cpp: $(S)/FloppyDisk.h

$(S)/File.cpp: $(S)/File.h $(S)/FileLibc.cpp $(S)/FileW32.cpp

$(S)/File.h: $(S)/SysLib.h

$(S)/gpt.cpp: $(S)/gpt.h $(S)/qword.h $(S)/harddisk.h $(S)/uuid.h

$(S)/gpt.h: $(S)/SysLib.h $(S)/Block.h $(S)/efibind.h \
	    $(GNUEFI)/inc/efi.h $(GNUEFI)/inc/efigpt.h

# $(S)\HardDisk.cpp:
# Dependencies defined conditionally in SysLib.mak

$(S)/HardDisk.h: $(S)/SysLib.h $(S)/qword.h

$(S)/HDisk95.cpp: $(S)/HardDisk.h $(S)/VxDCall.h    # The old version used $(S)/Ring0.h $(S)/R0Ios.h

$(S)/HDiskDos.cpp: $(S)/HardDisk.h $(S)/int13.h

$(S)/HDiskNT.cpp: $(S)/HardDisk.h

$(S)/HDiskVar.c: $(S)/HardDisk.h

$(S)/HDiskW32.cpp: $(S)/HardDisk.h $(S)/HDisk95.cpp $(S)/HDiskNT.cpp

$(S)/HDiskW64.cpp: $(S)/HardDisk.h $(S)/HDiskNT.cpp

$(S)/Int13.h:

$(S)/IsMBR.c: $(S)/IsMBR.h

$(S)/IsMBR.h: $(S)/SysLib.h

$(S)/LDisk95.cpp: $(S)/LogDisk.h $(S)/Ring0.h $(S)/R0Ios.h

$(S)/LDiskDos.cpp: $(S)/LogDisk.h

$(S)/LDiskNT.cpp: $(S)/LogDisk.h

$(S)/LDiskW32.cpp: $(S)/LogDisk.h $(S)/LDisk95.cpp $(S)/LDiskNT.cpp

# $(S)\LogDisk.cpp:
# Dependencies defined conditionally in SysLib.mak

$(S)/LogDisk.h: $(S)/SysLib.h $(S)/qword.h

$(S)/macaddr.c: $(S)/macaddr.h $(S)/smbios.h $(S)/NetBIOS.h	# The DOS version optionally uses Microsoft LAN Manager Programmer's ToolKit vers. 2.1 (LMPTK)

$(S)/macaddr.h: $(S)/SysLib.h $(S)/qword.h

$(S)/NetBIOS.c: $(S)/NetBIOS.h	# The DOS version requires Microsoft LAN Manager Programmer's ToolKit vers. 2.1 (LMPTK)

$(S)/NetBIOS.h: $(S)/SysLib.h

$(S)/OPrintf.cpp: $(S)/OPrintf.h

$(S)/OPrintf.h: $(S)/SysLib.h

$(S)/OPrintf6.cpp: $(S)/OPrintf.h

$(S)/OPrintf7.cpp: $(S)/OPrintf.h

$(S)/OPrintf8.cpp: $(S)/OPrintf.h

$(S)/PcUuid.c: $(S)/Uuid.h $(S)/smbios.h

$(S)/printerr.c:

$(S)/PrintUuid.c: $(S)/Uuid.h

$(S)/qword.cpp: $(S)/qword.h

$(S)/qword.h: $(S)/SysLib.h $(S)/OPrintf.h

$(S)/R0Ios.c: $(S)/Ring0.h $(S)/R0Ios.h

$(S)/R0Ios.h: $(S)/SysLib.h # $(S)/dcb.h now recommended to be put in 98DDK\inc\win98\.

$(S)/Ring0.c: $(S)/Ring0.h

$(S)/Ring0.h: $(S)/SysLib.h

$(S)/smbios.c: $(S)/smbios.h

$(S)/smbios.h: $(S)/SysLib.h

$(S)/stringx.c: $(S)/stringx.h

$(S)/stringx.h: $(S)/SysLib.h

$(S)/SysLib.h:

$(S)/Uuid.c: $(S)/Uuid.h $(S)/macaddr.h

$(S)/Uuid.h: $(S)/SysLib.h $(S)/qword.h

$(S)/UuidNull.c: $(S)/Uuid.h

$(S)/VxDCall.c: $(S)/VxDCall.h

$(S)/VxDCall.h: $(S)/SysLib.h

