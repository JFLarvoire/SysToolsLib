###############################################################################
#                                                                             #
#  File name        Makefile                                                  #
#                                                                             #
#  Description      A GNU make (gmake) makefile to build the SysLib library.  #
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
#    2016-10-10 JFL jf.larvoire@hpe.com created this file.                    #
#    2016-10-12 JFL Search global includes in $(STINCLUDE).                   #
#    2017-10-26 JFL Changed the default OUTDIR to bin.			      #
#    2020-01-06 JFL Do not actually install files if NOEXEC=1.                #
#    2020-01-09 JFL Use PWD instead of CURDIR, unless PWD missing or wrong.   #
#    2020-03-18 JFL Fixed PROC for the Raspberry Pi, and define EXE_PROC_NAME.#
#                   Install to /usr/bin if /usr/local/bin is not in the PATH. #
#    2020-04-19 JFL Corrected the C/C++ "Compiling xxxx.c" message.	      #
#    2020-11-18 JFL Disable diagnostics carets for gcc and clang that have it.#
#    2020-11-21 JFL Avoid displaying entering/leaving directory for same dir. #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Standard installation directory macros, based on
# https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
ifeq "$(prefix)" ""
  ifeq "$(bindir)" ""
    ifneq "$(findstring :/usr/local/bin:,:$(PATH):)" ""
      prefix := /usr/local	# If /usr/local/bin is in the PATH, use it
    else
      prefix := /usr		# Else use /usr/bin
    endif
  else # Extract the prefix from the bindir provided
    prefix := $(dir $(bindir))
  endif
endif
# Remove the trailing / from prefix, if any
prefix := $(patsubst %/,%,$(strip $(prefix)))
datarootdir = $(prefix)/share
datadir = $(datarootdir)
exec_prefix = $(prefix)
# Where to put the executables.
bindir = $(exec_prefix)/bin
# Where to put the libraries.
libdir = $(exec_prefix)/lib

# Prefer using PWD instead of CURDIR, as PWD does not expand links
needed_before_using_Bash_scripts := $(shell for s in truename distrib ; do chmod +x ../../Bash/$$s ; done)
ifeq "$(PWD)" ""  # But some old shells don't define PWD
  PWD := $(CURDIR)# If it's not defined, fall back to using CURDIR
else ifneq ($(shell ../../Bash/truename -q "$(PWD)"), $(shell ../../Bash/truename -q "$(CURDIR)"))
  PWD := $(CURDIR)# If it's ill defined, fall back to using CURDIR
endif

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
  ifeq "$(PROC)" "unknown" # On a Raspberry Pi, it's unknown, and MACHINE = armv7
    PROC := $(MACHINE)
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

# Output in the bin subdirectory, unless overridden by OUTDIR
ifdef OUTDIR
  ifneq "$(OUTDIR)" "."
    OD := $(OUTDIR)/
  else
    OD := 
  endif
else
  OD := bin/
endif

# Distinguish the output directory bases for normal and debug output
# Normal output base directory
OSPN := $(OD)$(OSP)
# Debug output base directory
OSPD := $(OD)$(OSP)/debug

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
O = $(OP)
# Listings path
LP = $(OSP)/LIST
LPN = $(OSPN)/LIST
LPD = $(OSPD)/LIST
L = $(LP)
# Executables path
XP = $(OSP)
XPN = $(OSPN)
XPD = $(OSPD)
B = $(XP)

# Define compiler and linker flags
CFLAGS = -std=c99 -pthread -Wall
CXXFLAGS =        -pthread -Wall
LFLAGS = 
LDLIBS = -ltermcap
CPPFLAGS += -DEXE_PROC_NAME='"$(PROC)"'

# The C compiler diagnostic caret is useful, but annoying when outputing information messages.
# Find the C-compiler-specific option for removing it.
# Note: All this should be better done using autoconf/automake!
ifndef CC_NOCARET_OPT
  CC_VERSION_LINE := $(shell cc -v 2>&1 | grep -i ' version ')
  CC_NAME := $(shell echo "$(CC_VERSION_LINE)" | sed -e 's/\(.* \)\?\([^ ][^ ]*\) version .*/\2/')
  # $(info CC_NAME=$(CC_NAME))
  CC_VERSION := $(shell echo "$(CC_VERSION_LINE)" | sed -e 's/\(.* \)\?version \([0-9][0-9.]*\).*/\2/')
  # $(info CC_VERSION=$(CC_VERSION))
  CC_VER := $(shell echo "$(CC_VERSION)" | sed -e 's/.*/&.0.0/' -e 's/\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/' -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g')
  # $(info CC_VER=$(CC_VER))
  # Now the C-compiler-specific part
  ifeq "$(CC_NAME)" "gcc"
    GCC_MIN_VER := 40800 # 4.8.0
    ifeq "$(shell expr $(CC_VER) \>= $(GCC_MIN_VER))" "1" # If GCC_VER >= GCC_MIN_VER
      # $(info gcc is $(GCC_MIN_VER) or later)
      CC_NOCARET_OPT := -fno-diagnostics-show-caret # -ftrack-macro-expansion=0
    else
      # $(info gcc is older than $(GCC_MIN_VER))
      CC_NOCARET_OPT := 
    endif
  else ifeq "$(CC_NAME)" "clang"
    CLANG_MIN_VER := 40000 # 4.0.0 (Actually it may have been there in earlier versions)
    # $(info clang is fine in any version)
    CC_NOCARET_OPT := -fno-caret-diagnostics # -Wno-\#pragma-messages
  else
    $(warning Unidentified compiler: $(CC_VERSION_LINE))
    CC_NOCARET_OPT := 
  endif
endif
export CC_NOCARET_OPT
ifneq "$(CC_NOCARET_OPT)" ""
  CFLAGS   += $(CC_NOCARET_OPT)
  CXXFLAGS += $(CC_NOCARET_OPT)
endif

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

# Make file messages control
TRACE_MSGS = $(or $(filter-out 0, $(VERBOSE)), $(filter-out 0, $(DEBUG)))
REPORT_FAILURE = (ERR=$$? ; echo " ... FAILED" ; exit $$ERR)
QMAKEFLAGS := $(MAKEFLAGS) --no-print-directory # Don't print entering/leaving even if -w is in the inherited flags

# Pattern rules for compiling a C or C++ source.
$(OPN)/%.o: %.c
	$(MAKE) -$(QMAKEFLAGS) dirs
	echo "Compiling $< ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -U_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

$(OPD)/%.o: %.c
	$(MAKE) -$(QMAKEFLAGS) ddirs
	echo "Compiling $< ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -D_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

$(OPN)/%.o: %.cpp
	$(MAKE) -$(QMAKEFLAGS) dirs
	echo "Compiling $< ..."
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c -U_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

$(OPD)/%.o: %.cpp
	$(MAKE) -$(QMAKEFLAGS) ddirs
	echo "Compiling $< ..."
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -c -D_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

# Pattern rules for compiling and linking a standalone C or C++ program.
$(OSPN)/%: %.c
	$(MAKE) -$(QMAKEFLAGS) dirs
	echo "Building $@ ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -U_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

$(OSPD)/%: %.c
	$(MAKE) -$(QMAKEFLAGS) ddirs
	echo "Building $@ ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

$(OSPN)/%: %.cpp
	$(MAKE) -$(QMAKEFLAGS) dirs
	echo "Building $@ ..."
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -U_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

$(OSPD)/%: %.cpp
	$(MAKE) -$(QMAKEFLAGS) ddirs
	echo "Building $@ ..."
	$(CC) $(CXXFLAGS) $(CPPFLAGS) -D_DEBUG -o $@ $< || $(REPORT_FAILURE)
	echo " ... done"

# Pattern rules for Building the normal and debug versions of a C or C++ program.
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
.PHONY: default all
default all: lib

# Internal rule for testing the makefile itself
.PHONY: testmak
testmak:
	echo "OS = $(OS)"
	echo "PROC = $(PROC)"
	echo "MACHINE = $(MACHINE)"
	echo "OSP = $(OSP)"

	echo "O = $(O)"
	echo "L = $(L)"
	echo "B = $(B)"
	echo "OBJECTS = $(OBJECTS)"
	echo "PROGRAMS = $(PROGRAMS)"
	echo "MAKEFLAGS = $(MAKEFLAGS)"
	echo "CURDIR = $(CURDIR)"
	echo "exec_prefix = $(exec_prefix)"
	echo "bindir = $(bindir)"

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
    ifneq "$(OBJECTS)" ""
      OBJECTS := $(subst +,,$(OBJECTS))
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

# Place holder for build results self test
.PHONY: check
check:
	true

# Check the build environment. Ex: global include files location
.PHONY: checkenv
checkenv:
	if [ ! -r "$(STINCLUDE)/debugm.h" ] ; then \
	    >&2 echo "SysLib/Makefile: Error: Can't find SysToolsLib global include files. Please export their location in STINCLUDE." ; \
	    false ; \
	fi

# How to rebuild all targets
.PHONY: lib libSysLib SysLib libsyslib syslib
_DEBUG :=
lib libSysLib libsyslib SysLib syslib: checkenv
	$(MAKE) -$(QMAKEFLAGS)            $(OSPN)/libSysLib.a
	$(MAKE) -$(QMAKEFLAGS) "_DEBUG=1" $(OSPD)/libSysLib.a
	echo "All SysLib library versions built."

$(OSP)/libSysLib.a: $(OBJECTS)
	$(MAKE) -$(QMAKEFLAGS) dirs
	echo "Building $@ ..."
	$(AR) -cvq $@ $? || $(REPORT_FAILURE)
	echo " ... $@ done"

# How to install all programs built above
.PHONY: install
install:
	@for exe in $(PROGRAMS) ; do \
	  dest=$$exe ; \
	   echo cp -p $$PWD/$(XP)/$$exe $(bindir)/$$dest ; \
	  $(DO) cp -p $$PWD/$(XP)/$$exe $(bindir)/$$dest ; \
	done


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

define HELP
Usage: make [MAKEOPTS] [MAKEDEFS] [TARGETS]

MakeDefs:
  bindir=PATH       With install: Where to install files. Default: $(bindir)
  NOEXEC=1          With install: Show what files would be copied, but don't do it.
  STINCLUDE=PATH    SysToolsLib global include dir. Default: $(STINCLUDE)

Targets:
  all       Build all programs defined in Files.mak. Default.
  checkenv  Check if all necessary definitions are set for the build to succeed
  clean     Delete all files generated by this Makefile
  help      Display this help message
  install   Copy the programs built to $$bindir

endef

export HELP
help:
	@echo "$$HELP"
