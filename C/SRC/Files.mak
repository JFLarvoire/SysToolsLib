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
  backnum.exe     \
  chars.exe       \
  conv.exe        \
  cpuid.exe       \
  deffeed.exe     \
  detab.exe       \
  dirc.exe        \
  dirsize.exe     \
  driver.exe      \
  dump.exe        \
  lessive.exe     \
  redo.exe        \
  remplace.exe    \
  tee.exe         \
  truename.exe    \
  update.exe      \
  which.exe       \
  whichinc.exe    \

REQS = MsvcLibX_library

UNIX_PROGRAMS = \
  backnum         \
  chars           \
  deffeed         \
  detab           \
  dirc            \
  dirsize         \
  dump            \
  lessive         \
  redo            \
  remplace        \
  update          \

UNIX_REQS = MsvcLibX_debug_macros

# List of sources to include in ToolsSRC.zip
ZIPFILE = ToolsSRC.zip
ZIPSOURCES = \
  1clip.c	    \
  1clip.mak         \
  2clip.c           \
  2clip.mak         \
  All.mak           \
  arm.mak           \
  backnum.c         \
  bios.mak          \
  chars.c           \
  clean.bat         \
  configure.bat     \
  conv.c            \
  conv.mak          \
  cpuid.c           \
  cpuid.mak         \
  deffeed.c         \
  detab.c           \
  dict.h            \
  dirc.c            \
  dirc.mak          \
  dirsize.c         \
  dos.mak           \
  driver.c          \
  driver.mak        \
  dump.c            \
  exe               \
  exe.bat           \
  Files.mak         \
  lessive.c         \
  macros.cpp        \
  make.bat          \
  Makefile          \
  redo.c            \
  redo.mak          \
  remplace.c        \
  src2objs.bat      \
  tee.c             \
  Tools.lst         \
  tree.h            \
  truename.c        \
  truename.mak      \
  update.c          \
  update.mak        \
  update.manifest   \
  update.rc         \
  which.c           \
  whichinc.c        \
  win32.mak         \
  win64.mak         \
  win95.mak         \

