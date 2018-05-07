#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   File name	    unixtime.tcl                                              #
#                                                                             #
#   Description     Convert a Unix timestamp <--> ISO 8601 date/time format   #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2018-04-10 JFL Created this script.                                      #
#                                                                             #
#         © Copyright 2018 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

set version "2018-04-10"

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

set usage [subst -nobackslashes -nocommands {
unixtime version $version - Convert a Unix timestamp <--> ISO 8601 date/time

Usage: $argv0 [OPTIONS] [TIME]

Time: Either a Unix time stamp = Number of seconds since 1970-01-01
          or an ISO 8601 date/time = [YYYY-MM-DD] HH:MM[:SS] [TZ]
          or a relative time. Ex: -3 weeks
      Default if not specified: Output the curent Unix time stamp

Options:
  -h | --help | -?  This help
  -g | --gmt        Output a GMT time. Default: Output a local time
  -V | --version    Display this script version
}]

set time ""
set opts {}

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
    "-g" - "--gmt" {
      lappend opts -gmt 1
    }
    default {
      lappend time $arg
    }
  }
}

set time [join $time]

set err [catch {
  if [regexp {^\d+$} $time -] {
    set who "clock format"
    set format [clock format $time {*}$opts -format "%Y-%m-%d %H:%M:%S %Z"]
    set stamp [clock scan $format]
  } else {
    set who "clock scan"
    set stamp [clock scan $time {*}$opts]
    set format [clock format $stamp {*}$opts -format "%Y-%m-%d %H:%M:%S %Z"]
  }
} output]
if $err {
  puts stderr "$who: $output"
  exit 1
}
puts "$stamp = $format"

exit 0
