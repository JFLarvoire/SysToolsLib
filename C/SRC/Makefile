###############################################################################
#                                                                             #
#  File name        Makefile                                                  #
#                                                                             #
#  Description      A GNU make (gmake) makefile to build a program.           #
#                                                                             #
#  Notes            Source files and their dependencies are defined in sub-   #
#                   makefile Files.mak or $(PROGRAM).mak.                     #
#                   The same Files.mak is also used for the same purpose by   #
#                   the DOS/Windows makefile.                                 #
#                                                                             #
#                   MUST BE EXECUTED BY GMAKE (GNU Make), NOT UNIX MAKE.      #
#                   Else the conditional directives won't work.               #
#                                                                             #
#  History                                                                    #
#    2004-12-14 JFL jf.larvoire@hp.com created this file.                     #
#    2014-12-05 JFL Finished the job. Yes, 10 years later!                    #
#    2016-01-07 JFL Added compilation option -Wall.                           #
#    2016-10-12 JFL Search includes in $(STINCLUDE) and $(SYSLIB).	      #
#    2016-10-14 JFL Fixed rules for building C++ programs.        	      #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Standard installation directories.
# NOTE: This directory must exist when you start the install.
prefix = /usr/local
datarootdir = $(prefix)/share
datadir = $(datarootdir)
exec_prefix = $(prefix)
# Where to put the executable for the command `gcc'.
bindir = $(exec_prefix)/bin
# Where to put the directories used by the compiler.
libexecdir = $(exec_prefix)/libexec
# Where to put the Info files.
infodir = $(datarootdir)/info

# Identify the OS and processor, and generate an output base directory name from that
ifeq "$(OS)" ""    # If $(OS) is undefined or empty
  OS := $(shell uname -s)
  PROC := $(shell uname -p)
  MACHINE := $(shell uname -m)
  ifeq "$(OS)" "OSF1"
    ifeq "$(MACHINE)" "alpha"
      OS := Tru64
    endif
  endif
  ifeq "$(OS)" "WindowsNT"
    OS := WIN32
  endif
  # Define the output base directory
  OSP := $(OS).$(PROC)
  # Now handle the special case of Unix-compatible shells for Windows
  ifneq "$(findstring MINGW32, $(OS))" "" # Ex: "MINGW32_NT-6.1"
    # MigGW shell if NOT case sensitive, so use a well readable camelcase spelling
    OSP := MinGW32
    # 2013-12-16 Actually, the 64-bits tool chain also reports MINGW32_NT-6.1
    # So distinguish the two by whether /mingw is mounted on C:\MinGW or C:\MinGW64
    ifneq "$(shell mount | grep -i /mingw64)" ""
      # MigGW shell if NOT case sensitive, so use a well readable camelcase spelling
      OSP := MinGW64
    endif
  endif
  ifneq "$(findstring MINGW64,$(OS))" "" # Ex: ?
    OSP := MinGW64
  endif
  ifneq "$(findstring CYGWIN,$(OS))" "" # Ex: "CYGWIN_NT-6.1-WOW64"
    # Cygwin shell if case sensitive, so use lower case
    OSP := cygwin
  endif
endif

# Distinguish the output directory bases for normal and debug output
# Normal output base directory
OSPN := $(OSP)
# Debug output base directory
OSPD := $(OSP)/debug

# Finally define the output directories for the current debug mode
ifdef _DEBUG
OSP := $(OSPD)
else
OSP := $(OSPN)
endif
# Sources path
SP = .
# Objects path
OP = $(OSP)/OBJ
OPN = $(OSPN)/OBJ
OPD = $(OSPD)/OBJ
# Listings path
LP = $(OSP)/LIST
LPN = $(OSPN)/LIST
LPD = $(OSPD)/LIST
# Executables path
XP = $(OSP)
XPN = $(OSPN)
XPD = $(OSPD)

# Define compiler and linker flags
CFLAGS = -std=c99 -pthread -Wall
CXXFLAGS =        -pthread -Wall
LFLAGS = 
LDLIBS = -ltermcap

# Add the path to SysToolsLib global include files
ifdef STINCLUDE
  ifeq '$(shell ls -1 "$(STINCLUDE)/debugm.h" 2>/dev/null)' ''
    $(error "Can't find SysToolsLib global include files in $(STINCLUDE)")
  endif
endif
ifndef STINCLUDE
  ifneq '$(shell ls -1 "../include/debugm.h" 2>/dev/null)' ''
    STINCLUDE := ../include
  endif
endif
ifdef STINCLUDE
CFLAGS := $(CFLAGS) -I $(STINCLUDE)
CXXFLAGS := $(CXXFLAGS) -I $(STINCLUDE)
endif

# Add the path to SysLib library files
ifdef SYSLIB
  ifeq '$(shell ls -1 "$(SYSLIB)/SysLib.h" 2>/dev/null)' ''
    $(error "Can't find the SysLib library files in $(SYSLIB)")
  endif
endif
ifndef SYSLIB
  ifneq '$(shell ls -1 "../SysLib/SysLib.h" 2>/dev/null)' ''
    SYSLIB := ../SysLib
  endif
endif
ifdef SYSLIB
CFLAGS := $(CFLAGS) -I $(SYSLIB)
CXXFLAGS := $(CXXFLAGS) -I $(SYSLIB)
NLDLIBS := $(NLDLIBS) -L $(SYSLIB)/$(OSPN) -l SysLib # Normal build linker flags
DLDLIBS := $(DLDLIBS) -L $(SYSLIB)/$(OSPD) -l SysLib # Debug build linker flags
endif

# Make file messages control
TRACE_MSGS = $(or $(filter-out 0, $(VERBOSE)), $(filter-out 0, $(DEBUG)))
REPORT_FAILURE = (ERR=$$? ; echo " ... FAILED" ; exit $$ERR)

# Pattern rules for compiling any standalone C or C++ source.
$(OSPN)/%: %.c
	$(MAKE) -$(MAKEFLAGS) dirs
	echo "Building $@ ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -U_DEBUG -o $@ $< $(NLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

$(OSPD)/%: %.c
	$(MAKE) -$(MAKEFLAGS) ddirs
	echo "Building $@ ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_DEBUG -o $@ $< $(DLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

$(OSPN)/%: %.cpp
	$(MAKE) -$(MAKEFLAGS) dirs
	echo "Building $@ ..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -U_DEBUG -o $@ $< $(NLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

$(OSPD)/%: %.cpp
	$(MAKE) -$(MAKEFLAGS) ddirs
	echo "Building $@ ..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -D_DEBUG -o $@ $< $(DLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

QMAKEFLAGS := $(MAKEFLAGS) --no-print-directory

%: %.c
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPN)/$@
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPD)/$@

%.exe: %.c
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPN)/$@
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPD)/$@

%: %.cpp
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPN)/$@
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPD)/$@

%.exe: %.cpp
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPN)/$@
	$(MAKE) -$(QMAKEFLAGS) PROGRAM=$* $(OSPD)/$@

.SILENT:

# Default rule.
.PHONY: default
default: all

# Internal rule for testing the makefile itself
.PHONY: testmak
testmak:
	echo "OS = $(OS)"
	echo "PROC = $(PROC)"
	echo "MACHINE = $(MACHINE)"
	echo "OSP = $(OSP)"
	echo "PROGRAM = $(PROGRAM)"
	echo "MAKEFLAGS = $(MAKEFLAGS)"
	echo "CURDIR = $(CURDIR)"

# Load $(PROGRAM).mak or Files.mak if possible 
ifdef PROGRAM
  program_mak := $(shell find . -maxdepth 1 -type f -iname $(PROGRAM).mak)
  ifneq "$(program_mak)" ""
    # $(info Including $(program_mak))
    # Source files and dependancies for this program. Defines macros SOURCES and PROGRAM.
    # include $(program_mak) # Do not include for now, due to make/nmake incompatibibities
  else
    # $(info No $(PROGRAM).mak available) 
  endif
else
  files_mak := $(shell find . -maxdepth 1 -type f -iname files.mak)
  ifneq "$(files_mak)" ""
    $(if $(TRACE_MSGS), $(info Including $(files_mak)))
    include $(files_mak)
    # The PROGRAMS list contains .exe suffixes for DOS/Windows. Remove it.
    # (But do leave it in Unix-compatible shells in Windows, like MinGW or CygWin.)
    ifneq "$(or $(findstring Linux,$(OS)), $(findstring True64,$(OS)))" ""
      PROGRAMS := $(subst .exe,, $(PROGRAMS))
    endif
    ifneq "$(UNIX_PROGRAMS)" ""
      PROGRAMS := $(UNIX_PROGRAMS)
    endif
  else
    # $(info No files.mak available) 
  endif
endif

# Create output directories
.PHONY: dirs ddirs
dirs: $(XPN) $(OPN) $(LPN)

ddirs: $(XPD) $(OPD) $(LPD)

$(XPN) $(OPN) $(LPN) $(XPD) $(OPD) $(LPD):
	$(info Creating directory $@)
	mkdir -p $@

# Place holder for build results selt test
.PHONY: check
check:
	true

# Check the build environment. Ex: global include files location
.PHONY: checkenv
checkenv:
	if [ ! -r "$(STINCLUDE)/debugm.h" ] ; then \
	    >&2 echo "C/Makefile: Error: Can't find SysToolsLib global include files. Please export their location in STINCLUDE." ; \
	    false ; \
	fi
	if [ ! -r "$(SYSLIB)/SysLib.h" ] ; then \
	    >&2 echo "C/Makefile: Error: Can't find SysLib library files. Please export their location in SYSLIB." ; \
	    false ; \
	fi

# How to reluild all targets
.PHONY: all
all: $(PROGRAMS)
	echo "All C programs built."

# How to install all programs built above
.PHONY: install
install:
	for exe in $(PROGRAMS) ; do cp -p $(XP)/$$exe $(bindir) ; done

# Cleanup all
.PHONY: clean
clean:
	-$(RM) $(OPD)/* >/dev/null 2>&1
	-rmdir $(OPD)   >/dev/null 2>&1
	-$(RM) $(LPD)/* >/dev/null 2>&1
	-rmdir $(LPD)   >/dev/null 2>&1
	-$(RM) $(XPD)/* >/dev/null 2>&1
	-rmdir $(XPD)   >/dev/null 2>&1
	-$(RM) $(OPN)/* >/dev/null 2>&1
	-rmdir $(OPN)   >/dev/null 2>&1
	-$(RM) $(LPN)/* >/dev/null 2>&1
	-rmdir $(LPN)   >/dev/null 2>&1
	-$(RM) $(XPN)/* >/dev/null 2>&1
	-rmdir $(XPN)   >/dev/null 2>&1
	-$(RM) *.log    >/dev/null 2>&1

$(BP)/$(PROGRAM): dirs $(SOURCES)
	$(CC) -o $@ -pthread -ltermcap $(CFLAGS) $(SOURCES)
	@echo "$(BP)/$(PROGRAM) done."

define HELP
Usage: make [MAKEOPTS] [MAKEDEFS] [TARGETS]

MakeDefs:
  STINCLUDE=PATH    SysToolsLib global include dir. Default: $(STINCLUDE)
  SYSLIB=PATH       SysLib library base dir. Default: $(SYSLIB)

Targets:
  PROGRAM   Build the normal and debug versions of PROGRAM.c or .cpp
  all       Build all programs defined in Files.mak. Default.
  checkenv  Check if all necessary definitions are set for the build to succeed
  clean     Delete all files generated by this Makefile
  help      Display this help message
  install   Copy the programs built to $$bindir. Default: $(bindir)

endef

export HELP
help:
	echo "$$HELP"
