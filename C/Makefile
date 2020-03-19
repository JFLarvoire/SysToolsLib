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
#    2020-01-06 JFL Install Bash scripts.                                     #
#                   Do not actually install files if NOEXEC=1.                #
#    2020-01-09 JFL Move -C DIR before -$(MAKEFLAGS), because the latter also #
#                   includes macro definitions, and the -C coming afterwards  #
#                   is interpreted as a target.                               #
#                   Use PWD instead of CURDIR, unless PWD missing or wrong.   #
#    2020-03-19 JFL Install to /usr/bin if /usr/local/bin is not in the PATH. #
#                   Fixed recursion with command-line VAR=VALUE definitions.  #
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
ifeq "$(PWD)" ""  # But some old shells don't define PWD
  PWD := $(CURDIR)# If it's not defined, fall back to using CURDIR
else ifneq ($(shell realpath "$(PWD)"), $(shell realpath "$(CURDIR)"))
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
all: $(DIRS)
	@echo "All done."

STINCLUDE := $(PWD)/include
SYSLIB := $(PWD)/SysLib
MAKEDEFS="STINCLUDE=$(STINCLUDE)" "SYSLIB=$(SYSLIB)"

$(DIRS): force
	$(MAKE) -C $@ $(MFLAGS) $(MAKEDEFS)

# Force building dependent targets
force: ; 

# How to install all programs built above
DO :=
ifeq "$(NOEXEC)" "1"    # Set $(NOEXEC) to 1 to avoid actually running the installation below
  DO := true # The `true` command ignores all arguments
endif
.PHONY: install
install:
	@echo "# Install Bash scripts"
	@cd ../Bash && for f in `cat ../Scripts.lst | sed "s/\r//" | grep '^Bash' | sed -r 's/^Bash.(.*)/\1/'` ; do \
	  if [ ! -x $$f ] ; then chmod +x $$f ; fi ; \
	   echo cp -p $$PWD/$$f $(bindir)/$$f ; \
	  $(DO) cp -p $$PWD/$$f $(bindir)/$$f ; \
	done
	@echo "# Install Tcl scripts, renaming them without the .tcl extension"
	@cd ../Tcl && for f in `cat ../Scripts.lst | sed "s/\r//" | grep '^Tcl.*tcl$$' | sed -r 's/^Tcl.(.*).tcl$$/\1/'` ; do \
	  if [ ! -x $$f.tcl ] ; then chmod +x $$f.tcl ; fi ; \
	   echo cp -p $$PWD/$$f.tcl $(bindir)/$$f ; \
	  $(DO) cp -p $$PWD/$$f.tcl $(bindir)/$$f ; \
	done
	@echo "# Install C executables"
	@$(MAKE) -s -C SRC $(MFLAGS) $(MAKEDEFS) PWD="$(PWD)/SRC" install

# Cleanup all
.PHONY: clean
clean:
	for dir in $(DIRS) ; do $(MAKE) -C $$dir $(MFLAGS) $(MAKEDEFS) clean ; done
	-$(RM) *.log    >/dev/null 2>&1

define HELP
Usage: make [MAKEOPTS] [MAKEDEFS] [TARGETS]

MakeDefs:
  bindir=PATH       With install: Where to install files. Default: $(bindir)
  NOEXEC=1          With install: Show what files would be copied, but don't do it.
  STINCLUDE=PATH    SysToolsLib global include dir. Default: $(STINCLUDE)
  SYSLIB=PATH       SysLib library base dir. Default: $(SYSLIB)

Targets:
  all       Build all SysToolsLib libraries and programs. Default.
  clean     Delete all files generated by this Makefile
  help      Display this help message
  install   Copy bash and tcl scripts, and the programs built, to $$bindir

endef

export HELP
help:
	@echo "$$HELP"

