#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   Function	    b64enc                                        	      #
#                                                                             #
#   Description     Base64 encoder                                            #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2017-11-17 JFL Created this program.                                     #
#                                                                             #
#         © Copyright 2017 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Global variables
set version "2017-11-17"
set script [file tail $::argv0]    ; # This script name.
set verbosity 1                    ; # 0=quiet; 1=normal; 2=verbose; 3=debug
proc Debug {} { expr $::verbosity > 2 }

#-----------------------------------------------------------------------------#
#                              Library routines                               #
#-----------------------------------------------------------------------------#

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

#-----------------------------------------------------------------------------#
#                                Main routine                                 #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
Base64 encoder

Usage: $script <input >output

Options:
  -h, --help, -?        Display this help screen and exit.
  -l, --length L        Set the output line length. Default: 0=No Lines.
  -V, --version         Display this script version
}]

# Process arguments
set args $argv
set length 0

while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-d" - "--debug" {
      incr verbosity 2
    }
    "-h" - "--help" - "-?" - "/?" {
      puts $usage
      exit 0
    }
    "-l" - "--length" {		# Set the output line length
      set length [PopArg]
    }
    "-V" - "--version" {	# Display this library version
      puts $version
      exit 0
    }
    default {
      puts stderr "Unexpected argument: $arg"
      puts stderr $usage
      exit 1
    }
  }
}

# Make sure Tcl does not change anything in our back.
fconfigure stdin -translation binary
# Do not change the output translation, as we're just outputing ASCII.

package require base64

set buf [read stdin]
set encoded [base64::encode -maxlen $length $buf]
puts -nonewline $encoded
