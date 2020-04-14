###############################################################################
#                                                                             #
#  File name        Makefile                                                  #
#                                                                             #
#  Description      A GNU make (gmake) makefile to build all SysToolsLib      #
#                                                                             #
#  Notes            MUST BE EXECUTED BY GMAKE (GNU Make), NOT UNIX MAKE.      #
#                   Else the conditional directives won't work.               #
#                                                                             #
#  History                                                                    #
#    2020-03-19 JFL jf.larvoire@hpe.com created this file.                    #
#    2020-04-13 JFL Moved the scripts installation here from C/Makefile.      #
#		    Rewrote it using an inference rule.			      #
#                   Added an uninstall target rule, also using inferences.    #
#                                                                             #
#         © Copyright 2020 Hewlett Packard Enterprise Development LP          #
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

# How to install all SysToolsLib scripts and programs
BASH_SCRIPTS = $(shell cat Scripts.lst | sed "s/\r//" | grep '^Bash' | sed -r 's/^Bash.(.*)/\1/')
TCL_SCRIPTS = $(shell cat Scripts.lst | sed "s/\r//" | grep '^Tcl.*tcl$$' | sed -r 's/^Tcl.(.*).tcl$$/\1/' | sed 's/.tcl//')

$(DESTDIR)$(bindir)/: # Create the $(bindir) directory if it does not yet exist
	install -d $(DESTDIR)$(bindir)/

$(DESTDIR)$(bindir)/%: Bash/%
	install -p $< $@

comment_bash_scripts:
	$(info # Install Bash scripts)

install_bash_scripts: comment_bash_scripts $(addprefix $(DESTDIR)$(bindir)/,$(BASH_SCRIPTS))

$(DESTDIR)$(bindir)/%: Tcl/%.tcl
	install -p $< $@

comment_tcl_scripts:
	$(info # Install Tcl scripts)

install_tcl_scripts: comment_tcl_scripts $(addprefix $(DESTDIR)$(bindir)/,$(TCL_SCRIPTS))

.PHONY: install # Do not use `make -s` to get info about the directory change
install: $(DESTDIR)$(bindir)/ install_bash_scripts install_tcl_scripts
	$(info # Install C programs)
	@$(MAKE) -C C $(MFLAGS) PWD="$(PWD)/C" install

# How to uninstall all SysToolsLib scripts and programs
dummy_uninstall_dir/%: Bash/%
	rm -f $(bindir)/$(@F)

dummy_uninstall_dir/%: Tcl/%.tcl
	rm -f $(bindir)/$(@F)

# List of scripts installed
INSTALLED = $(wildcard $(addprefix $(bindir)/,$(BASH_SCRIPTS) $(TCL_SCRIPTS)))
# But pretend they are in a dummy_uninstall_dir, to avoid having them first reinstalled if they're out of date.
.PHONY: uninstall
uninstall: $(subst $(bindir)/,dummy_uninstall_dir/,$(INSTALLED))
	@$(MAKE) -s -C C $(MFLAGS) PWD="$(PWD)/C" uninstall

# Pass everything else on to the make file in the C directory
OTHERGOALS = $(filter-out all default install uninstall,$(MAKECMDGOALS))
$(OTHERGOALS) all:
	@$(MAKE) -s -C C $(MFLAGS) PWD="$(PWD)/C" $(MAKECMDGOALS)
