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
#    2019-07-08 JFL Added options -a, -c, -l, and their inverse -o, -C, -L.   #
#                   Added support for C \t \xXX etc sequences in substitutions.
#    2021-03-22 JFL Fixed the input and output encoding in Windows.           #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

set version "2021-03-22"
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
  -a, --all             Replace all occurences (Default)
  -c, --case            Case-dependant matching (Default)
  -C, --nocase          Case-independant matching
  -h, --help, -?        Display this help screen
  -l, --line            Regexp in line mode
  -L, --noline          Regexp in global mode (Default)
  -o, --one             Replace the first occurence
  -V, --version         Display this library version
  -X, --noexec          Display the commands to execute, but don't run them

RegExp: Regular expression. Use \N for back-references.
RegSub: Substitution string. Use \N to insert sub-expressions of REGEXP.
        Use \t, \xXX, etc, to specify C character escape sequences.
Names:  Pathnames of file to update. Default: Read from stdin & write to stdout
        Wildcards allowed.
}]

set rx ""
set sx ""
set nx 0
set names {}
set all "-all"
set line ""
set case ""

# Scan all arguments.
set args $argv
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-a" - "--all" {		# Enable all mode
      set all "-all"
    }
    "-c" - "--case" {		# Case-dependant matching
      set case ""
    }
    "-C" - "--nocase" {		# Case-independant matching
      set case "-nocase"
    }
    "-d" - "--debug" {		# Enable debug mode
      incr verbosity 2
    }
    "-h" - "--help" - "-?" {	# Display a help screen and exit.
      puts $usage
      exit 0
    }
    "-l" - "--line" {		# Enable line mode
      set line "-line"
    }
    "-L" - "--noline" {		# Disable line mode
      set line ""
    }
    "-o" - "--one" {		# Disable all mode
      set all ""
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
set opts [concat $all $line $case]

# Kludge to work around the lack of support for \t, \xXX, etc, in substitution strings
set sx0 $sx
regsub -all {\\(.)} $sx0 {\\\\<\1>} sx
regsub -all {\\\\<([abefnrtuUvx])>} $sx {\\\1} sx ;# Don't support \0, as \0 refers to the whole matching block
regsub -all {\\\\<\\>} $sx {\\\\\\\\} sx
regsub -all {\\\\<(.)>} $sx {\\\\\1} sx
set sx [subst $sx]

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

if ($noexec) {
  set input -
  set cmdList [concat regsub $opts [list -- $rx $input $sx0]]
  puts [tcl2sh $cmdList]
} else {
  if {$nNames == 0} {	# No file names specified. Use stdin/stdout.
    set input [read stdin]
    set cmdList [concat regsub $opts [list -- $rx $input $sx]]
    puts -nonewline [eval $cmdList]
  } else {		# File names specified
    set slash [file separator]
    foreach name $names {
      set local_name [regsub -all "/" $name $slash] ; # Name with local OS path separators
      set input [ReadFile $name {return ""}]
      if {"$input" == ""} continue ; # No such file, or empty file
      set cmdList [concat regsub $opts [list -- $rx $input $sx]]
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
