#!/usr/bin/tclsh
#-----------------------------------------------------------------------------#
#                                                                             #
#   Script name     rxrename                                                  #
#                                                                             #
#   Description     Batch rename using regular expressions.                   #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2008/12/31 JFL Created this script.                                      #
#    2009/12/17 JFL Quote the output, to make it easier to verify correctness.#
#                   Added options -i and -I.                                  #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
#-----------------------------------------------------------------------------#

# Set global defaults
set script [file tail $argv0]   ; # This script name.
set verbosity 1			; # 0=Quiet 1=Normal 2=Verbose 3=Debug

proc Verbose {} {expr $::verbosity > 1}
proc Debug {} {expr $::verbosity > 2}

#-----------------------------------------------------------------------------#
#                          General purpose routines                           #
#-----------------------------------------------------------------------------#

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

# Escape a string. ie. change special C string charaters to \c sequences.
# Does the reverse of {subst -nocommands -novariables $text}
proc Escape {text} {
  regsub -all {\\} $text {\\\\} text ; # Double every pre-existing backslash.
  foreach c {\\" \\a \\b \\f \\n \\r \\t \\v} { # Satisfy the syntax colorizer "
    regsub -all [subst $c] $text $c text ; # Escape C string control characters
  }
  return $text
}

# Quotify a string. ie. escape special C string charaters and put quotes around.
proc Quotify {text} {
  return "\"[Escape $text]\""
}

# Quotify a string if needed. ie. when spaces, quotes, or a trailing \.
# (Also includes constraints inherited from the SML <-> XML conversion).
proc CondQuotify {text} {
  if {"$text" == ""} {
    return {""}
  }
  if {[regexp {[\s"=;{}<>#\\]} $text -]} {
    set text [Escape $text]
    if [regexp {\s} $text -] { # Surround with quotes if there are spaces.
      set text "\"$text\""
    }
  }
  return $text
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
#    2008/12/31 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
Batch rename using regular expressions.

Usage: $argv0 [OPTIONS] rxOld [rxNew]

Options:
  -h, --help, -?    Display this help screen
  -q, --quiet       No output
  -v, --verbose     Verbose output
  -X, --noexec      Do not actually rename anything. (Useful for testing rx.)

rxOld: Regular expression. See Tcl's regsub syntax for details.
rxNew: Replacement string. Default: Unchanged. Use \1 for first sub-expr. etc.
}]

set rxOld ""
set rxNew ""
set nocase "-nocase" ; # Whether to ignore case

# Scan all arguments.
set args $argv
set noExec 0
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-d" - "--debug" {
      incr verbosity 2
    }
    "-h" - "--help" - "-?" {
      puts -nonewline $usage
      exit 0
    }
    "-i" - "--ignorecase" {
      set nocase "-nocase"
    }
    "-I" - "--noignorecase" {
      set nocase ""
    }
    "-q" - "--quiet" { # Verbose flag
      incr verbosity -1
    }
    "-v" - "--verbose" { # Verbose flag
      incr verbosity
    }
    "-X" - "--noexec" {
      set noExec 1
    }
    default {
      if [string match -* $arg] {
	puts stderr "Unknown option: $arg. Ignored."
      } elseif {"$rxOld" == ""} {
	set rxOld $arg
      } elseif {"$rxNew" == ""} {
	set rxNew $arg
      }
    }
  }
}

if {"$rxOld" == ""} {
  puts stderr "Error: Missing required argument. Try option -? for help."
  exit 1
}

if {"$rxNew" == ""} {
  set rxNew {\0} ; # Identity
}

set rxOld "^$rxOld$"

# Test the regular expression validity
set err [catch {
  regsub "$rxOld" "image001.jpg" $rxNew newName
} output]
if {$err} {
  set detail ""
  regexp {:\s*([^\n]+)} $output - detail
  puts stderr "Error: Invalid regular expression: $detail."
  puts stderr "       Old name: $rxOld"
  puts stderr "       New name: $rxNew"
  exit 1
}

# Go for it
set nFiles 0
foreach name [glob -nocomplain *] {
  if ![eval regsub $nocase [list $rxOld $name $rxNew newName]] continue
  incr nFiles
  if {$verbosity >= 1} {
    if {"$rxNew" != {\0}} {
      puts "rename [CondQuotify $name] [CondQuotify $newName]"
    } else {
      puts $name
    }
  }
  if {(!$noExec) && ("$rxNew" != {\0})} {
    file rename $name $newName
  }
}
if {$verbosity > 1} {
  if {(!$noExec) && ("$rxNew" != {\0})} {
    puts "$nFiles files renamed"
  } else {
    puts "$nFiles files found"
  }
}

