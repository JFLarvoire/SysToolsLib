#!/usr/bin/tclsh 
###############################################################################
#                                                                             #
#   File name	    regsub.tcl                                                #
#                                                                             #
#   Description     Regular expression substitution			      #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2017-07-31 JFL Created this script.                                      #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

set version "2017-08-01"
set script [file rootname [file tail $argv0]]
set verbosity 1
set noexec 0

proc Verbose {} {expr $::verbosity > 1} ; # Test if we're in verbose mode.
proc Debug {{n 0}} {expr $::verbosity > (2+$n)} ; # Test if we're in debug mode.

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

# Get the contents of a file.
# Useful alternative for onError: {return ""}
proc ReadFile {filename {onError {error "Error reading $filename. $out"}}} {
  set err [catch {
    set hf [open $filename]
    set data [read $hf]
    close $hf
  } out]
  if {$err} {
    eval $onError
  }
  return $data
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    tcl2sh	                                    	      #
#                                                                             #
#   Description     Convert de tcl command list into an sh command.	      #
#                                                                             #
#   Parameters      cmd                Tcl list with command and arguments    #
#                                                                             #
#   Returns 	    sh command string, properly quoted.                       #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2005-10-17 JFL Created this routine.                                     #
#    2008-05-13 JFL Fixed the case with strings with both "s and 's.          #
#                   Also escape the other special shell characters ` \ $ .    #
#                                                                             #
#-----------------------------------------------------------------------------#

proc tcl2sh {cmd} { # Convert a tcl command list into an sh command string.
  set sh ""
  foreach item $cmd {
    if {"$sh" != ""} {append sh " "}
    if {[regexp {[" "'`\\$]} $item -]} { # Arg w. spaces or special shell chars.
      # Constraints:
      # - 'strings' cannot contain 's.
      # - We try to minimize the number of \s.
      # - We prefer "s first (More natural for strings), then 's on a second
      #   level call. (2-level shell encodings are common, ex. with ssh).
      if {   ([regexp {[""`\\$]} $item -])
          && ([string first "'" $item] == -1)} { # If there are "s and no 's
        set item "'$item'"    ; # Surround with 's to preserve everything else.
      } else { # Either there are 's, or there are neither 's nor "s
	regsub -all {[""`\\$]} $item {\\&} item ; # Escape special shell chars.
	set item "\"$item\""  ; # Surround with "s.
      }
    }
    if {"$item" == ""} {set item "\"\""} ; # Void arguments
    append sh $item
  }
  return $sh
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
#    2017-07-31 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Usage string
set usage [subst -nobackslashes -nocommands {
$script - Regular expression substitution tool

Usage: $script [options] REGEXP REGSUB [NAMES]

Options:
  -h, --help, -?        Display this help screen
  -V, --version         Display this library version

RegExp: Regular expression. Use \N for back-references.
RegSub: Substitution string. Use \N to insert sub-expressions of REGEXP.
Names:  Pathnames of file to update. Default: Read from stdin & write to stdout
        Wildcards allowed.
}]

set rx ""
set sx ""
set nx 0
set names {}

# Scan all arguments.
set args $argv
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-d" - "--debug" {		# Enable debug mode
      incr verbosity 2
    }
    "-h" - "--help" - "-?" {	# Display a help screen and exit.
      puts $usage
      exit 0
    }
    "-q" - "--quiet" {		# Enable quiet mode
      set verbosity 0
    }
    "-v" - "--verbose" {	# Increase the verbosity level
      incr verbosity
    }
    "-V" - "--version" {	# Display this library version
      puts $version
      exit 0
    }
    "-X" - "--noexec" {		# No Execute
      set noexec 1
    }
    default {
      if {$nx == 0} {		; # If the regular expression is not set...
	set rx $arg
	incr nx
      } elseif {$nx == 1} {	; # If the substitution expression is not set...
	set sx $arg
	incr nx
      } else {			; # Anything else is a pathname
	regsub -all {\\} $arg "/" name
	if [regexp {[?*]} $name -] { # Expand wildcards
	  set names [concat $names [lsort -dictionary [glob -nocomplain $name]]]
	} else {
	  set names [lappend $names $name]
	}
      }
    }
  }
}
if {$nx != 2} {
  puts $usage
  exit 0
}

set nNames [llength $names]

if ($noexec) {
  set input -
  set cmdList [list regsub -all -- $rx $input $sx]
  puts [tcl2sh $cmdList]
} else {
  if {$nNames == 0} {	# No file names specified. Use stdin/stdout.
    set input [read stdin]
    set cmdList [list regsub -all -- $rx $input $sx]
    puts -nonewline [eval $cmdList]
  } else {		# File names specified
    set slash [file separator]
    foreach name $names {
      set local_name [regsub -all "/" $name $slash] ; # Name with local OS path separators
      set input [ReadFile $name {return ""}]
      if {"$input" == ""} continue ; # No such file, or empty file
      set cmdList [list regsub -all -- $rx $input $sx]
      set output [eval $cmdList]
      if {"$output" != "$input"} {
	puts $local_name    ; # Show which file is getting updated 
      	set hFile [open $name w]
	puts -nonewline $hFile $output
	close $hFile
      }
    }
  }
}
