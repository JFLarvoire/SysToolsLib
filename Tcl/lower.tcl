#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   File name	    lower.tcl                                        	      #
#                                                                             #
#   Description     Convert strings to all lower case                         #
#                                                                             #
#   Notes	    For use in pipes. Ex: 1clip | lower | 2clip               #
#                                                                             #
#   History								      #
#    2018-11-16 JFL Created this program.                                     #
#                                                                             #
#         © Copyright 2018 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Global variables
set version "2018-11-16"
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

# Output a string in debug mode only.
proc DebugString {args} {
  if [Debug] {
    eval puts $args
  }
}

#-----------------------------------------------------------------------------#
#                                Main routine                                 #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
Convert a string to all lower case

Usage: $script <input >output

Options:
  -h, --help, -?        Display this help screen and exit.
  -V, --version         Display this script version
}]

# Process arguments
set args $argv
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

# Correct the input and output encodings in Windows.
# This is necessary because Tcl uses the unicode encoding for I/Os to the console,
# and the system encoding for any redirected I/Os on stdin or stdout.
# But Windows uses the current console code page for pipes, and that console code page
# is usually different from the system code page. Ex: CP437 and CP1252 resp. for USA.  
if {$tcl_platform(platform) == "windows"} {
  if [catch {	# First try getting the CP using twapi. This is faster.
    package require twapi
    set icp [twapi::get_console_input_codepage]
  } msg] {	# Can't find twapi. Try running chcp.exe to get the info. (Slower)
    if ![regexp {\d+} [exec chcp] icp] { # If chcp fails also
      set icp 65001 ;# Then assume input is UTF-8
    }
  }
  switch $icp {
    65000 {
      set cp "utf-7"
    }
    65001 {
      set cp "utf-8"
    }
    default {
      set cp "cp$icp"
    }
  }
  # Correct the input and output encodings in Windows.
  foreach handle [list stdin stdout] {
    if {"[fconfigure $handle -encoding]" != "unicode"} { # If redirected
      fconfigure $handle -encoding $cp ;# Change the encoding
    }
  }
}

# Convert the data
set string [read stdin]
set string [string tolower $string]
puts -nonewline $string
