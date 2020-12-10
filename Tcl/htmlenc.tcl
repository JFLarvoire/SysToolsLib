#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   File name	    htmlenc.tcl                                               #
#                                                                             #
#   Description     Encode text strings with the 3 basic HTML entities        #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2020-11-24 JFL Created this script.                                      #
#    2020-12-10 JFL Don't append an \n for piped input data.                  #
#                                                                             #
#        (C) Copyright 2020 Hewlett Packard Enterprise Development LP         #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Global variables
set version "2019-12-10"
set script [file tail $::argv0]    ; # This script name.

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
$script - Encode text strings with the 3 basic HTML entities

Usage: $script [options] [text_string_to_encode]

Options:
  -h, --help, -?        Display this help screen
  -V, --version         Display this script version
}]

set initial_argv $argv          ; # Keep the initial command line for debug.
set str ""

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
      if {"$str"==""} {		; # If the string is not set...
	set str "$arg\n"
      } else {                          ; # Anything else is an error.
        Puts stderr "Unrecognized argument: $arg"
	puts stderr $usage
	exit 1
      }
    }
  }
}

# Default if no string is passed as an argument: Read it from stdin
if {$str == ""} {
  set str [read stdin]
}

set map {
  & &amp;
  < &lt;
  > &gt;
}

puts -nonewline [string map $map $str]	; # Encode and output to stdout
