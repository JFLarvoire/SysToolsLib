#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   Function	    b64dec                                        	      #
#                                                                             #
#   Description     Base64 decoder                                            #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2007-09-28 JFL Created this program.                                     #
#    2010-03-04 JFL Bugfix: Change I/O to binary mode, to avoid decoding      #
#                   errors.                                                   #
#    2017-11-17 JFL Added a workaround to correct the output to the console.  #
#                   Use the base64::decode library routine by default.        #
#                   Added option -V.                                          #
#    2017-11-19 JFL Use tell instead of chan tell. This works in Tcl <= 8.4.  #
#                   Added option -q|--quiet.                                  #
#                                                                             #
#         © Copyright 2017 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Global variables
set version "2017-11-20"
set script [file tail $::argv0]    ; # This script name.
set verbosity 1                    ; # 0=quiet; 1=normal; 2=verbose; 3=debug
proc Debug {} { expr $::verbosity > 2 }

# Base 64 character values, as per RFC 3548. (Including non-recommended altern.)
#        0x     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
set codes    " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1"
#        1x
append codes " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1"
#        2x                                      +     -     /
append codes " -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 -1 62 -1 62 -1 63"
#        3x     0  1  2  3  4  5  6  7  8  9           =
append codes " 52 53 54 55 56 57 58 59 60 61 -1 -1 -1 -1 -1 -1"
#        4x        A  B  C  D  E  F  G  H  I  J  K  L  M  N  O
append codes " -1  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14"
#        5x     P  Q  R  S  T  U  V  W  X  Y  Z              _
append codes " 15 16 17 18 19 20 21 22 23 24 25 -1 -1 -1 -1 63"
#        6x        a  b  c  d  e  f  g  h  i  j  k  l  m  n  o
append codes " -1 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40"
#        7x     p  q  r  s  t  u  v  w  x  y  z           ~
append codes " 41 42 43 44 45 46 47 48 49 50 51 -1 -1 -1 63 -1"

set codes [eval list $codes] ; # Reformat as a valid list for fast access.

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
Base64 decoder

Usage: $script <input >output

Options:
  -h, --help, -?        Display this help screen and exit.
  -q, --quiet           Do not display warnings in case base64::decode fails
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
    "-q" - "--quiet" {
      set verbosity 0
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

# Make sure Tcl does not change data.
fconfigure stdin -translation binary
# But using binary translation (and thus encoding) for the output breaks the Windows console output.
# Using lf translation still uses the system encoding. This fixes the console output, but breaks output to files.
# We'd need to use lf translation for the console, and binary translation for files and pipes.
# Unfortunately I could find no reliable way to detect when stdout is the console. (There are documented ways in Unix, but the problem is in Windows.)
# Workaround: Distinguish files from pipes and consoles, as files support tell, whereas the other two don't and return -1.
# This should fix console and files, but still break pipes. Yet it works in all three. I suppose that Tcl detects pipes, and encodes things differently for them.
set translation binary
if {[tell stdout] == -1} { # This is the console or a pipe
  set translation lf
}
fconfigure stdout -translation $translation

set string [read stdin]
set err [catch { # Use modern fast methods if possible.
  package require base64	;# Not all Tcl installations have that package available
  set decoded [base64::decode $string] ;# This may fail if there are invalid characters
  puts -nonewline $decoded	;# Success
} errMsg]
if {$err} { # Revert to my old code, which is slow, but has no dependency, and is more resilient to illegal characters
  if {$verbosity} {
    puts stderr "Warning: $errMsg. Trying again with a different, but slower, method."
  }
  # Convert all characters arriving on standard input
  set bitBuf 0 ; # Bit buffer
  set nBits 0  ; # Number of valid bits in the bit buffer
  # while {![eof stdin]} \{
    # set c [read stdin 1]
  set l [string length $string]
  for {set i 0} {$i < $l} {incr i} {
    set c [string index $string $i]
    scan $c %c a             ; # The ASCII code of the input character
    set n [lindex $codes $a] ; # The base 64 value it encodes
    DebugString [format "Read char $c -> ASCII %X -> bits %X" $a $n]
    if {$n == -1} continue
    # Complement the existing bits
    incr n [expr $bitBuf << 6]
    incr nBits 6
    if {$nBits >= 8} {
      incr nBits -8
      puts -nonewline [format %c [expr $n >> $nBits]]
      set n [expr $n & ((1 << $nBits) - 1)]
    }
    set bitBuf $n
  }
  # Drop the remaining bits
}
