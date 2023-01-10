###############################################################################
#									      #
#   File name	    Files.mak						      #
#									      #
#   Description     C source files list					      #
#									      #
#   Notes	    							      #
#									      #
#   History								      #
#    2014-12-03 JFL Initial version                                           #
#    2022-10-19 JFL Added dependencies on mainutil.h.                         #
#                   							      #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# List of programs to rebuild for distribution in Tools.zip
PROGRAMS = \
  1clip.exe       \
  2clip.exe       \
  2note.exe       \
  backnum.exe     \
  chars.exe       \
  codepage.exe    \
  conv.exe        \
  cpuid.exe       \
  deffeed.exe     \
  detab.exe       \
  dirc.exe        \
  dirsize.exe     \
  driver.exe      \
  dump.exe        \
  encoding.exe    \
  font.exe        \
  gpt.exe         \
  in.exe          \
  inicomp.exe     \
  junction.exe    \
  md.exe          \
  msgbox.exe      \
  rd.exe          \
  redo.exe        \
  remplace.exe    \
  sector.exe      \
  ShareInfo.exe   \
  smbios.exe      \
  tee.exe         \
  trim.exe        \
  truename.exe    \
  update.exe      \
  uuid.exe	  \
  which.exe       \
  whichinc.exe    \
  with.exe        \
  zap.exe         \

# All programs above depend on MsvcLibX.
# If they depend on another SysToolLib library, define it in a program-specific include file.
REQS = MsvcLibX_library						# Check requirement using this dummy target
# LIBRARIES = $(MSVCLIBX)\$(OUTDIR)\lib\MsvcLibX$(LSX).lib	# Add a dependency on this library

UNIX_PROGRAMS = \
  backnum         \
  chars           \
  deffeed         \
  detab           \
  dirc            \
  dirsize         \
  dump            \
  In              \
  inicomp         \
  md              \
  rd              \
  redo            \
  remplace        \
  trim            \
  update          \
  Which		  \
  zap             \

UNIX_REQS = MsvcLibX_debug_macros

# List of sources to include in ToolsSRC.zip
ZIPFILE = ToolsSRC.zip
ZIPSOURCES =		 \
  1clip.c                \
  1clip.mak              \
  2clip.c                \
  2clip.mak              \
  2note.c                \
  2note.mak              \
  backnum.c              \
  chars.c                \
  clean.bat              \
  codepage.c             \
  codepage.mak           \
  configure              \
  configure.bat          \
  configure.msvclibx.bat \
  configure.syslib.bat   \
  conv.c                 \
  conv.mak               \
  cpuid.c                \
  cpuid.mak              \
  deffeed.c              \
  detab.c                \
  dirc.c                 \
  dirc.mak               \
  dirsize.c              \
  driver.c               \
  driver.mak             \
  dump.c                 \
  encoding.c             \
  encoding.mak           \
  exe                    \
  exe.bat                \
  Files.mak              \
  font.c		 \
  font.mak		 \
  gpt.cpp		 \
  gpt.mak		 \
  In.c                   \
  inicomp.c              \
  inicomp.mak            \
  junction.c             \
  junction.mak           \
  macros.cpp             \
  make.bat               \
  Makefile               \
  md.c                   \
  msgbox.c               \
  msgbox.mak             \
  msgnames.h             \
  NMakefile              \
  rd.c                   \
  README.txt             \
  redo.c                 \
  redo.mak               \
  remplace.c             \
  sector.cpp		 \
  ShareInfo.c            \
  ShareInfo.mak          \
  smbios.c               \
  smbios.mak             \
  smbios_defs.c          \
  smbios_lib.c           \
  tee.c                  \
  Tools.lst              \
  trim.c                 \
  truename.c             \
  truename.mak           \
  update.c               \
  update.mak             \
  update.manifest        \
  update.rc              \
  uuid.c		 \
  Which.c                \
  whichinc.c             \
  with.c                 \
  zap.c                  \

#-----------------------------------------------------------------------------#
#			Include files dependencies			      #
#-----------------------------------------------------------------------------#

SI=$(STINCLUDE)
SL=$(SYSLIB)
SX=$(MSVCLIBX)/include

$(S)/1clip.c: footnote.h $(SL)/mainutil.h

$(S)/2clip.c: footnote.h $(SL)/mainutil.h

$(S)/2note.c: footnote.h $(SL)/mainutil.h

$(S)/backnum.c: footnote.h $(SL)/mainutil.h

$(S)/chars.c: footnote.h $(SL)/mainutil.h

$(S)/codepage.c: footnote.h $(SL)/mainutil.h

$(S)/conv.c: footnote.h $(SL)/mainutil.h

$(S)/cpuid.c: footnote.h $(SL)/mainutil.h

$(S)/deffeed.c: footnote.h $(SL)/mainutil.h

$(S)/detab.c: footnote.h $(SL)/mainutil.h

$(S)/dirc.c: footnote.h $(SL)/mainutil.h

$(S)/dirsize.c: footnote.h $(SL)/mainutil.h

$(S)/driver.c: footnote.h $(SL)/mainutil.h

$(S)/dump.c: footnote.h $(SL)/mainutil.h

$(S)/encoding.c: footnote.h $(SL)/mainutil.h

$(S)/font.c: footnote.h $(SL)/mainutil.h

$(S)/gpt.cpp: footnote.h

$(S)/In.c: footnote.h $(SL)/mainutil.h

$(S)/inicomp.c: footnote.h $(SL)/mainutil.h

$(S)/junction.c: footnote.h $(SL)/mainutil.h

$(S)/macros.cpp: footnote.h

$(S)/md.c: footnote.h $(SL)/mainutil.h

$(S)/msgbox.c: footnote.h $(SL)/mainutil.h

$(S)/rd.c: footnote.h $(SL)/mainutil.h

$(S)/redo.c: footnote.h

$(S)/remplace.c: footnote.h $(SL)/mainutil.h

$(S)/sector.cpp: footnote.h

$(S)/ShareInfo.c: footnote.h

$(S)/smbios.c: footnote.h $(SL)/mainutil.h

$(S)/tee.c: footnote.h $(SL)/mainutil.h

$(S)/trim.c: footnote.h $(SL)/mainutil.h

$(S)/truename.c: footnote.h $(SL)/mainutil.h

$(S)/update.c: footnote.h $(SL)/mainutil.h

$(S)/uuid.c: footnote.h

$(S)/Which.c: footnote.h $(SL)/mainutil.h

$(S)/whichinc.c: footnote.h $(SL)/mainutil.h

$(S)/with.c: footnote.h $(SL)/mainutil.h

$(S)/zap.c: footnote.h $(SL)/mainutil.h

