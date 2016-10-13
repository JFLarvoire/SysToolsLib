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
#                                                                             #
#         � Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Default rule.
.PHONY: default
default: all

# Place holder for environment dependancies rules
.PHONY: check
check:
	true

# How to reluild all targets
DIRS=SysLib SRC
.PHONY: all
all: $(DIRS)
	@echo "All done."

STINCLUDE=$(CURDIR)/include
SYSLIB=$(CURDIR)/SysLib
MAKEDEFS="STINCLUDE=$(STINCLUDE)" "SYSLIB=$(SYSLIB)"

$(DIRS): force
	$(MAKE) -$(MAKEFLAGS) -C $@ $(MAKEDEFS)

# Force building dependent targets
force: ; 

# How to install all programs built above
.PHONY: install
install:
	$(MAKE) -$(MAKEFLAGS) -C SRC install

# Cleanup all
.PHONY: clean
clean:
	for dir in $(DIRS) ; do $(MAKE) -$(MAKEFLAGS) -C $$dir clean ; done
	-$(RM) *.log    >/dev/null 2>&1

define HELP
Usage: make [MAKEOPTS] [MAKEDEFS] [TARGETS]

MakeDefs:
  STINCLUDE=PATH    SysToolsLib global include dir. Default: $(STINCLUDE)
  SYSLIB=PATH       SysLib library base dir. Default: $(SYSLIB)

Targets:
  all       Build all SysToolsLib libraries and programs. Default.
  clean     Delete all files generated by this Makefile
  help      Display this help message
  install   Copy the programs built to /bin

endef

export HELP
help:
	echo "$$HELP"
