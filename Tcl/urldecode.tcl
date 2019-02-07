#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   File name	    UrlDecode.tcl                                             #
#                                                                             #
#   Description     Decode URLs that have been %XX encoded                    #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2016-07-14 JFL Created this script.                                      #
#                                                                             #
#        (C) Copyright 2016 Hewlett Packard Enterprise Development LP         #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Set defaults
set version "2016-07-14"

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    Main                                        	      #
#                                                                             #
#   Description     Process command line arguments.                           #
#                                                                             #
#   Parameters      argc               The number of arguments                #
#                   argv               List of arguments                      #
#                                                                             #
#   Returns 	    0 if success                                              #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2005/01/15 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Usage string
set usage [subst -nobackslashes -nocommands {
Usage: $argv0 [options] [encoded_url_string]

Options:
  -h, --help, -?        Display this help screen
  -V, --version         Display this script version
}]

set initial_argv $argv          ; # Keep the initial command line for debug.
set url ""

# Scan all arguments.
set args $argv
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-h" - "--help" - "-?" {	# Display a help screen and exit.
      puts $usage
      exit 0
    }
    "-V" - "--version" {	# Display this library version
      puts $version
      exit 0
    }
    default {
      if {"$url"==""} {		; # If the url is not set...
	set url $arg   
      } else {                          ; # Anything else is an error.
        Puts stderr "Unrecognized argument: $arg"
	puts stderr $usage
	exit 1
      }
    }
  }
}

# Default if no URL is passed as an argument: Read it from stdin
if {$url == ""} {
  set url [read stdin]
}

package require uri
package require uri::urn
puts [uri::urn::unquote $url]	; # Decode and output to stdout
