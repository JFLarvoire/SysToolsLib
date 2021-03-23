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
#    2020-04-15 JFL Updated scripts enumeration for compatibility w. MacOS.   #
#    2021-01-17 JFL Install files from Shell/profile.d/ into /etc/profile.d/. #
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

# Enumerate the scripts to be installed
# Avoid using `sed -r` because this option does not exist in FreeBSD's sed.
SHELL_SCRIPTS = $(shell cat Scripts.lst | tr -d '\r' | grep '^Shell\\' | sed 's/^Shell.//' | grep -v '^profile.d\\')
PROFILE_SCRIPTS = $(shell cat Scripts.lst | tr -d '\r' | grep '^Shell\\profile.d\\' | sed 's/.*profile.d.//')
TCL_SCRIPTS = $(shell cat Scripts.lst | tr -d '\r' | grep '^Tcl\\.*tcl$$' | sed 's/^Tcl.\(.*\).tcl$$/\1/')

# How to install all SysToolsLib scripts and programs
$(DESTDIR)$(bindir)/: # Create the $(bindir) directory if it does not yet exist
	install -d $(DESTDIR)$(bindir)/

# How to install Shell scripts
$(DESTDIR)$(bindir)/%: Shell/%
	install -p $< $@

comment_shell_scripts:
	$(info # Install Shell scripts)

install_shell_scripts: comment_shell_scripts $(addprefix $(DESTDIR)$(bindir)/,$(SHELL_SCRIPTS))

# How to install Tcl scripts
$(DESTDIR)$(bindir)/%: Tcl/%.tcl
	install -p $< $@

comment_tcl_scripts:
	$(info # Install Tcl scripts)

install_tcl_scripts: comment_tcl_scripts $(addprefix $(DESTDIR)$(bindir)/,$(TCL_SCRIPTS))

# How to install Shell profile scripts
/etc/profile.d/%: Shell/profile.d/%
	install -p -m 0644 $< $@

comment_profile_scripts:
	$(info # Install Shell Profile scripts)
	
# FreeBSD and MacOSX do not have a /etc/profile.d directory
/etc/profile.d:
	mkdir $@

.PHONY: /etc/profile
/etc/profile: # Add the /etc/profile.d processing in /etc/profile if it's not there already
	@grep /etc/profile.d /etc/profile >/dev/null 2>&1 || (		       \
	  echo "# Add the /etc/profile.d processing in /etc/profile"          ;\
	  chmod +w /etc/profile                                               ;\
	  echo >>/etc/profile ''                                              ;\
	  echo >>/etc/profile 'for PROFILE_SCRIPT in /etc/profile.d/*.sh ; do';\
	  echo >>/etc/profile '  . $$PROFILE_SCRIPT'                          ;\
	  echo >>/etc/profile 'done'                                          ;\
	  echo >>/etc/profile 'unset PROFILE_SCRIPT'                          ;\
	)

install_profile_scripts: /etc/profile.d /etc/profile comment_profile_scripts $(addprefix /etc/profile.d/,$(PROFILE_SCRIPTS))

# How to install all SysToolsLib scripts and programs
.PHONY: install # Do not use `make -s` to get info about the directory change
install: $(DESTDIR)$(bindir)/ install_shell_scripts install_profile_scripts install_tcl_scripts
	$(info # Install C programs)
	@$(MAKE) -C C $(MFLAGS) PWD="$(PWD)/C" install

# How to uninstall all SysToolsLib scripts and programs
dummy_uninstall_dir/%: Shell/%
	rm -f $(bindir)/$(@F)

dummy_uninstall_dir/%: Tcl/%.tcl
	rm -f $(bindir)/$(@F)

dummy_profile_dir/%: Shell/profile.d/%
	rm -f /etc/profile.d/$(@F)

# List of scripts installed
INSTALLED = $(wildcard $(addprefix $(bindir)/,$(SHELL_SCRIPTS) $(TCL_SCRIPTS)))
PROFILED = $(wildcard $(addprefix /etc/profile.d/,$(PROFILE_SCRIPTS)))
# But pretend they are in a dummy_uninstall_dir, to avoid having them first reinstalled if they're out of date.
.PHONY: uninstall
uninstall: $(subst $(bindir)/,dummy_uninstall_dir/,$(INSTALLED)) $(subst /etc/profile.d/,dummy_profile_dir/,$(PROFILED))
	@$(MAKE) -s -C C $(MFLAGS) PWD="$(PWD)/C" uninstall

# Pass everything else on to the make file in the C directory
OTHERGOALS = $(filter-out all default install uninstall,$(MAKECMDGOALS))
$(OTHERGOALS) all:
	@$(MAKE) -s -C C $(MFLAGS) PWD="$(PWD)/C" $(MAKECMDGOALS)
