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
  inicomp.exe     \
  md.exe          \
  msgbox.exe      \
  rd.exe          \
  redo.exe        \
  remplace.exe    \
  sector.exe      \
  smbios.exe      \
  tee.exe         \
  trim.exe        \
  truename.exe    \
  update.exe      \
  uuid.exe	  \
  which.exe       \
  whichinc.exe    \
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
  dict.h                 \
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
  inicomp.c              \
  inicomp.mak            \
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
  smbios.c               \
  smbios.mak             \
  smbios_defs.c          \
  smbios_lib.c           \
  tee.c                  \
  Tools.lst              \
  tree.h                 \
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
  zap.c                  \

