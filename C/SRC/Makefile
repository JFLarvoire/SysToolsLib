############################## :encoding=UTF-8: ###############################
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
#		    Use variable QMAKEFLAGS instead of MAKEFLAGS.	      #
#    2016-10-14 JFL Fixed rules for building C++ programs.        	      #
#    2017-08-29 JFL `make install` now verifies that $bindir is in the PATH.  #
#                   `make check` now checks if $bindir is in the PATH.        #
#    2017-10-26 JFL Changed the default OUTDIR to bin.			      #
#    2018-03-23 JFL Install which as Which, to avoid conflicts with Unix's.   #
#    2020-01-06 JFL Do not actually install files if NOEXEC=1.                #
#    2020-01-09 JFL Use PWD instead of CURDIR, unless PWD missing or wrong.   #
#    2020-03-18 JFL Fixed PROC for the Raspberry Pi, and define EXE_PROC_NAME.#
#                   Install to /usr/bin if /usr/local/bin is not in the PATH. #
#    2020-04-12 JFL Rewrote `make install` using an inference rule.           #
#                   Use `make -n` instead of NOEXEC=1 for testing it.         #
#    2020-04-13 JFL Added an uninstall target rule, also using inferences.    #
#    2020-11-18 JFL Disable diagnostics carets for gcc and clang that have it.#
#    2020-11-21 JFL Avoid displaying entering/leaving directory for same dir. #
#    2021-11-08 JFL Define C macro HAS_SYSLIB when SYSLIB is present.         #
#    2023-04-23 JFL Create the link bin -> ../bin.			      #
#                   Fixed a failure to link with the termcap library.         #
#    2024-01-07 JFL Define both NMINCLUDE and STINCLUDE.		      #
#    2024-10-17 JFL Added a configuration step generating a config.h file.    #
#                   Added goals mostlyclean & veryclean.                      #
#    2025-04-07 JFL Fixed the use of MAKEFLAGS. No need for a QMAKEFLAGS var. #
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
needed_before_using_truename := $(shell chmod +x ../../Shell/truename)
ifeq "$(PWD)" ""  # But some old shells don't define PWD
  PWD := $(CURDIR)# If it's not defined, fall back to using CURDIR
else ifneq ($(shell ../../Shell/truename -q "$(PWD)"), $(shell ../../Shell/truename -q "$(CURDIR)"))
  PWD := $(CURDIR)# If it's ill defined, fall back to using CURDIR
endif

# Identify the OS and processor, and generate an output base directory name from that
needed_before_using_distrib := $(shell chmod +x ../../Shell/distrib)
ifeq "$(OS)" ""    # If $(OS) is undefined or empty
  OS := $(shell uname -s)
  PROC := $(shell ../../Shell/distrib processor)# Don't just use uname -p, which is unreliable
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
IP = $(OSPN)/INC
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
NLDLIBS := $(LDLIBS) # Normal build linker flags
DLDLIBS := $(LDLIBS) # Debug build linker flags
CPPFLAGS += -DEXE_PROC_NAME='"$(PROC)"'

# The C compiler diagnostic caret is useful, but annoying when outputing information messages.
# Find the C-compiler-specific option for removing it.
# Note: All this should be better done using autoconf/automake!
ifndef CC_NOCARET_OPT
  CC_VERSION_LINE := $(shell cc -v 2>&1 | grep -i ' version ')
  CC_NAME := $(shell echo "$(CC_VERSION_LINE)" | sed -e 's/ *[Vv]ersion .*//' -e 's/.* //')# The compiler name is the last word before ' version '
  # $(info CC_NAME=$(CC_NAME))
  CC_VERSION := $(shell echo "$(CC_VERSION_LINE)" | sed -e 's/.* [Vv]ersion *//' -e 's/ .*//')# The version is the first word after ' version '
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
    $(warning Unidentified compiler: "$(CC_NAME)")
    CC_NOCARET_OPT := 
  endif
endif
export CC_NOCARET_OPT
ifneq "$(CC_NOCARET_OPT)" ""
  CFLAGS   += $(CC_NOCARET_OPT)
  CXXFLAGS += $(CC_NOCARET_OPT)
endif

# Add the path to the system-specific config.h
CFLAGS := $(CFLAGS) -I $(IP)
CXXFLAGS := $(CXXFLAGS) -I $(IP)

# Add the path to NMaker include files
ifdef NMINCLUDE
  ifeq '$(shell ls -1 "$(NMINCLUDE)/debugm.h" 2>/dev/null)' ''
    $(error "Can't find NMaker include files in $(NMINCLUDE)")
  endif
endif
ifndef NMINCLUDE
  ifneq '$(shell ls -1 "../include/debugm.h" 2>/dev/null)' ''
    NMINCLUDE := ../include
  endif
endif
ifdef NMINCLUDE
CFLAGS := $(CFLAGS) -I $(NMINCLUDE)
CXXFLAGS := $(CXXFLAGS) -I $(NMINCLUDE)
endif

# Add the path to SysToolsLib global include files
ifdef STINCLUDE
  ifeq '$(shell ls -1 "$(STINCLUDE)/stversion.h" 2>/dev/null)' ''
    $(error "Can't find SysToolsLib global include files in $(STINCLUDE)")
  endif
endif
ifndef STINCLUDE
  ifneq '$(shell ls -1 "../include/stversion.h" 2>/dev/null)' ''
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
CFLAGS := $(CFLAGS) -I $(SYSLIB) -DHAS_SYSLIB=1
CXXFLAGS := $(CXXFLAGS) -I $(SYSLIB) -DHAS_SYSLIB=1
# Important: Place SysLib ahead of termcap, else there's a link failure
NLDLIBS := -L $(SYSLIB)/$(OSPN) -l SysLib $(NLDLIBS) # Normal build linker flags
DLDLIBS := -L $(SYSLIB)/$(OSPD) -l SysLib $(DLDLIBS) # Debug build linker flags
endif

# Make file messages control
TRACE_MSGS = $(or $(filter-out 0, $(VERBOSE)), $(filter-out 0, $(DEBUG)))
REPORT_FAILURE = (ERR=$$? ; echo " ... FAILED" ; exit $$ERR)
MAKEFLAGS += --no-print-directory # Don't print entering/leaving even if -w is in the inherited flags

# Pattern rules for compiling any standalone C or C++ source.
$(OSPN)/%: %.c
	$(MAKE) dirs
	echo "Building $@ ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -U_DEBUG -o $@ $< $(NLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

$(OSPD)/%: %.c
	$(MAKE) ddirs
	echo "Building $@ ..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -D_DEBUG -o $@ $< $(DLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

$(OSPN)/%: %.cpp
	$(MAKE) dirs
	echo "Building $@ ..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -U_DEBUG -o $@ $< $(NLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

$(OSPD)/%: %.cpp
	$(MAKE) ddirs
	echo "Building $@ ..."
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -D_DEBUG -o $@ $< $(DLDLIBS) || $(REPORT_FAILURE)
	echo " ... done"

# Pattern rules for Building the normal and debug versions of a C or C++ program.
%: %.c
	$(MAKE) PROGRAM=$* $(OSPN)/$@
	$(MAKE) PROGRAM=$* $(OSPD)/$@

%.exe: %.c
	$(MAKE) PROGRAM=$* $(OSPN)/$@
	$(MAKE) PROGRAM=$* $(OSPD)/$@

%: %.cpp
	$(MAKE) PROGRAM=$* $(OSPN)/$@
	$(MAKE) PROGRAM=$* $(OSPD)/$@

%.exe: %.cpp
	$(MAKE) PROGRAM=$* $(OSPN)/$@
	$(MAKE) PROGRAM=$* $(OSPD)/$@

# Make sure all commands are silent by default
.SILENT:

# Define a non_silent macro, which reverts .SILENT for specific commands passed in $(1)
# Define a silent macro, which is really silent in -n mode for commands passed in $(1)
ifneq (,$(findstring n,$(MAKEFLAGS)))	# If invoked with the -n option
  non_silent = $(1)
  silent =
else					# Else _not_ invoked with the -n option
  non_silent = $(info $(1))$(1)
  silent = $(1)
endif

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
  else
    # $(info No files.mak available) 
  endif
endif

# Create output directories
.PHONY: dirs ddirs files
dirs: bin $(XPN) $(OPN) $(LPN) files

ddirs: $(XPD) $(OPD) $(LPD) files

bin:
	$(info Creating link $@ to directory ../$@)
	ln -s "../$@" "$@"

$(XPN) $(OPN) $(LPN) $(XPD) $(OPD) $(LPD):
	$(info Creating directory $@)
	mkdir -p $@

files: $(IP)/config.h

$(IP)/config.h: Makefile configure
	echo "Configuring ..."
	$(MAKE) mostlyclean # Force rebuilding everything thereafter
	chmod +x configure # Sources are installed without execution rights
	./configure -q
	echo " ... done"

# Manage the dialog with the ./configure script
.PHONY: config_dir
config_dir: # Tell it where to store config.h
	mkdir -p $(IP) # Silently create it if not done already
	echo $(IP)

# Place holder for build results self test
.PHONY: check
check:
	@if ! ( echo ":$(PATH):" | grep -q ":$(bindir):" ) ; then \
	  >&2 echo ERROR: $(bindir) not in PATH. Please add it for the installed programs to work. ; \
	  false ; \
	fi
	@echo Success
	@true

# Check the build environment. Ex: global include files location
.PHONY: checkenv
checkenv:
	if [ ! -r "$(NMINCLUDE)/debugm.h" ] ; then \
	    >&2 echo "C/Makefile: Error: Can't find NMaker include files. Please export their location in NMINCLUDE." ; \
	    false ; \
	fi
	if [ ! -r "$(STINCLUDE)/stversion.h" ] ; then \
	    >&2 echo "C/Makefile: Error: Can't find SysToolsLib global include files. Please export their location in STINCLUDE." ; \
	    false ; \
	fi
	if [ ! -r "$(SYSLIB)/SysLib.h" ] ; then \
	    >&2 echo "C/Makefile: Error: Can't find SysLib library files. Please export their location in SYSLIB." ; \
	    false ; \
	fi

# How to rebuild all targets
.PHONY: all
all: $(PROGRAMS)
	echo "All C programs built."

# How to install all programs built above
$(DESTDIR)$(bindir)/: # Create the $(bindir) directory if it does not yet exist
	$(call non_silent,install -d $(DESTDIR)$(bindir)/)

$(DESTDIR)$(bindir)/%: $(FROM)$(OSPN)/%
	$(call non_silent,install -p $< $@)

.PHONY: install
install: $(DESTDIR)$(bindir)/ $(addprefix $(DESTDIR)$(bindir)/,$(PROGRAMS))
	$(call silent,@if ! ( echo ":$(PATH):" | grep -q ":$(bindir):" ) ; then \
	  >&2 echo "WARNING: $(bindir) not in PATH. Please add it for the installed programs to work." ; \
	fi)

# How to uninstall all programs built above
dummy_uninstall_dir/%: %.c
	$(call non_silent,rm -f $(bindir)/$(@F))

# List of programs installed
INSTALLED = $(wildcard $(addprefix $(bindir)/,$(PROGRAMS)))
# But pretend they are in a dummy_uninstall_dir, to avoid having them rebuilt if they're out of date.
.PHONY: uninstall
uninstall: $(subst $(bindir)/,dummy_uninstall_dir/,$(INSTALLED))

# Cleanup all
.PHONY: clean mostlyclean veryclean
mostlyclean:
	-$(RM) $(OPD)/* >/dev/null 2>&1
	-$(RM) $(LPD)/* >/dev/null 2>&1
	-$(RM) $(XPD)/* >/dev/null 2>&1
	-$(RM) $(OPN)/* >/dev/null 2>&1
	-$(RM) $(LPN)/* >/dev/null 2>&1
	-$(RM) $(XPN)/* >/dev/null 2>&1

clean: mostlyclean
	-rmdir $(OPD)   >/dev/null 2>&1
	-rmdir $(LPD)   >/dev/null 2>&1
	-rmdir $(XPD)   >/dev/null 2>&1
	-rmdir $(OPN)   >/dev/null 2>&1
	-rmdir $(LPN)   >/dev/null 2>&1
	-rmdir $(XPN)   >/dev/null 2>&1
	-$(RM) *.log    >/dev/null 2>&1

veryclean: clean	# Also clean the files created by the configuration
	-$(RM) $(IP)/*  >/dev/null 2>&1
	-rmdir $(IP)    >/dev/null 2>&1
	-rmdir $(OSPN)  >/dev/null 2>&1

$(BP)/$(PROGRAM): dirs $(SOURCES)
	$(CC) -o $@ -pthread -l termcap $(CFLAGS) $(SOURCES)
	@echo "$(BP)/$(PROGRAM) done."

define HELP
Usage: make [MAKEOPTS] [MAKEDEFS] [TARGETS]

MakeDefs:
  bindir=PATH       With install: Where to install files. Default: $(bindir)
  NMINCLUDE=PATH    NMaker include directory. Default: $(NMINCLUDE)
  STINCLUDE=PATH    SysToolsLib global include dir. Default: $(STINCLUDE)
  SYSLIB=PATH       SysLib library base dir. Default: $(SYSLIB)

Targets:
  PROGRAM   Build the normal and debug versions of PROGRAM.c or .cpp
  all       Build all programs defined in Files.mak. Default.
  checkenv  Check if all necessary definitions are set for the build to succeed
  clean     Delete all files generated by this Makefile
  help      Display this help message
  install   Install the programs built to $$bindir. (Use make -n to dry-run it)
  uninstall Uninstall the programs from $$bindir

endef

export HELP
help:
	@echo "$$HELP"
