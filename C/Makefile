###############################################################################
#                                                                             #
#  File name        Makefile                                                  #
#                                                                             #
#  Description      A GNU make makefile to build all SysToolsLib  program.    #
#                                                                             #
#  Notes            MUST BE EXECUTED BY GMAKE (GNU Make), NOT UNIX MAKE.      #
#                   Else the conditional directives won't work.               #
#                                                                             #
#  History                                                                    #
#    2016-10-14 JFL jf.larvoire@hpe.com created this file.                    #
#    2017-08-29 JFL `make install` now verifies that $bindir is in the PATH.  #
#                   `make check` now checks if $bindir is in the PATH.        #
#    2017-08-30 JFL Make sure the installed scripts are executable.           #
#    2020-01-06 JFL Install Shell scripts.                                    #
#                   Do not actually install files if NOEXEC=1.                #
#    2020-01-09 JFL Move -C DIR before -$(MAKEFLAGS), because the latter also #
#                   includes macro definitions, and the -C coming afterwards  #
#                   is interpreted as a target.                               #
#                   Use PWD instead of CURDIR, unless PWD missing or wrong.   #
#    2020-03-19 JFL Install to /usr/bin if /usr/local/bin is not in the PATH. #
#                   Fixed recursion with command-line VAR=VALUE definitions.  #
#    2020-04-13 JFL Moved the scripts installation to ../Makefile.	      #
#                   Added an uninstall target rule.			      #
#    2023-04-23 JFL Create the link bin -> ../bin.			      #
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
needed_before_using_truename := $(shell chmod +x ../Shell/truename)
ifeq "$(PWD)" ""  # But some old shells don't define PWD
  PWD := $(CURDIR)# If it's not defined, fall back to using CURDIR
else ifneq ($(shell ../Shell/truename -q "$(PWD)"), $(shell ../Shell/truename -q "$(CURDIR)"))
  PWD := $(CURDIR)# If it's ill defined, fall back to using CURDIR
endif

# Default rule.
.PHONY: default
default: all

# Place holder for environment dependancies rules
.PHONY: check
check:
	@if ! ( echo ":$(PATH):" | grep -q ":$(bindir):" ) ; then \
	  >&2 echo ERROR: $(bindir) not in PATH. Please add it for the installed programs to work. ; \
	  false ; \
	fi
	@echo Success
	@true

# How to rebuild all targets
DIRS=SysLib SRC
.PHONY: all
all: ../bin bin $(DIRS)
	@echo "All done."

STINCLUDE := $(PWD)/include
SYSLIB := $(PWD)/SysLib
MAKEDEFS="STINCLUDE=$(STINCLUDE)" "SYSLIB=$(SYSLIB)"

../bin:
	$(info Creating directory $@)
	mkdir -p $@

bin:
	$(info Creating link $@ to directory ../$@)
	ln -s "../$@" "$@"

$(DIRS): force
	$(MAKE) -C $@ $(MFLAGS) $(MAKEDEFS)

# Force building dependent targets
force: ; 

# How to install all C programs
.PHONY: install
install: # $(DIRS) # Do not use `make -s` to get info about the directory change
	@$(MAKE) -C SRC $(MFLAGS) $(MAKEDEFS) PWD="$(PWD)/SRC" install

# How to uninstall all C programs
.PHONY: uninstall
uninstall:	 # Do use `make -s` as we don't care about teh directory change
	@$(MAKE) -s -C SRC $(MFLAGS) $(MAKEDEFS) PWD="$(PWD)/SRC" uninstall

# Cleanup all
.PHONY: clean
clean:
	for dir in $(DIRS) ; do $(MAKE) -C $$dir $(MFLAGS) $(MAKEDEFS) clean ; done
	-$(RM) *.log    >/dev/null 2>&1

define HELP
Usage: make [MAKEOPTS] [MAKEDEFS] [TARGETS]

MakeDefs:
  bindir=PATH       With install: Where to install files. Default: $(bindir)
  STINCLUDE=PATH    SysToolsLib global include dir. Default: $(STINCLUDE)
  SYSLIB=PATH       SysLib library base dir. Default: $(SYSLIB)

Targets:
  all       Build all SysToolsLib libraries and programs. Default.
  clean     Delete all files generated by this Makefile
  help      Display this help message
  install   Install the programs built to $$bindir. (Use make -n to dry-run it)
  uninstall Uninstall the programs from $$bindir

endef

export HELP
help:
	@echo "$$HELP"

