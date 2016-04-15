#!/usr/bin/tclsh
#*****************************************************************************#
#                                                                             #
#   Script name     fixmht                                                    #
#                                                                             #
#   Description     Fix relative links in a .mht file                         #
#                                                                             #
#   Author          Jean-François Larvoire, jean-francois.larvoire@hp.com     #
#                                                                             #
#   History                                                                   #
#    2009/12/17 JFL Created this script.                                      #
#                                                                             #
#*****************************************************************************#

set script [file tail $::argv0]         ; # This script name
set verbosity 1

proc Normal {} {expr $::verbosity > 0 } ; # Test if we're in normal mode.
proc Verbose {} {expr $::verbosity > 1} ; # Test if we're in verbose mode.
proc Debug {} {expr $::verbosity > 2}   ; # Test if we're in debug mode.

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

# Log into a private log file
proc Log {args} {
  # Not needed here
}

# Routine for outputing and logging strings
proc Puts {args} {
  set show [Normal]
  set log 0
  set category "$::script"
  set severity notice
  set prefix "$::script: "
  while {"$args" != ""} {
    set arg [PopArg]
    switch -- $arg {
      -d        { set show [Debug] }
      -v        { set show [Verbose] }
      -show     { set show [PopArg] }
      -log      { set log  [PopArg] }
      -category { set category [PopArg] }
      -severity { set severity [PopArg] }
      -noprefix { set prefix "" }
      --        { break }
      default   { set args [linsert $args 0 $arg] ; break }
    }
  }
  set msg [lindex $args end]
  if $show {
    # set args [lreplace $args end end "$prefix$msg"]
    eval puts $args                         ; # Output message
  }
  if $log { # Log to the system log and to our private log
    SysLog $msg $severity $category
  }
  Log $msg ; # Always log to our private log file if present.
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

# Write data to a file.
# Useful alternative for onError: {return $out}
proc WriteFile {name data {onError {error "Error writing $filename. $out"}}} {
  set err [catch {
    set hf [open $name w]
    puts -nonewline $hf $data
    close $hf
  } out]
  if {$err} {
    eval $onError
  }
  return ""
}

# Find a backup file name, with a unique sequence number suffix
proc GetBackupName { filename } {
  for {set i 1} {1} {incr i} {
    set name [format "$filename.%03d" $i]
    if {![file exists $name]} {
      return $name
    }
  }
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
#    2007/05/14 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
Fix relative links in a .mht file.

Usage: $script [OPTIONS] [PATHNAME]

Options:
  -h, --help, -?    Display this help screen.
  -v, --verbose     Verbose attributes
}]

set files ""

# Scan all arguments.
set args $argv
while {"$args" != ""} {
  set arg [PopArg]
  if {[string match -* $arg]} {
    switch -- $arg {
      "-d" - "--debug" { # Debug flag
	incr verbosity 2
      }
      "-h" - "--help" - "-?" {
	puts $usage
	exit 0
      }
      "-v" - "--verbose" { # Verbose flag
	incr verbosity
      }
      "-V" - "--version" { # Version.
	set err [catch {exec grep <$argv0 \$Revision:} rev]
	if !$err { # Found. Extract the CVS version tag in our header.
	  regexp {Revision:\s*(\S+)\s*\$} $rev match rev
	} ; # Gotcha: Never put two dollars around the Revision words above!
	puts "$script: $rev"
	exit 0
      }
      default {
        puts stderr "Unrecognized switch $arg. Ignored."
      }
    }
  } else {
    lappend files $arg
  }
}

# Action
if {"$files" == ""} { # If no file specified, then
  puts stderr "$script: Error: No file specified. Use option -? to get help."
  exit 1
}

set pwd [pwd]
set lpwd [string length $pwd]

foreach file $files {
  set text [ReadFile $file {return ""}]
  set file2 [GetBackupName $file]
  file rename -force $file $file2
  file copy $file2 $file
  set rx {Content-Location: file://(.*)}
  if {![regexp -line $rx $text - location]} {
    puts stderr "Error: Cannot find the reference Content-Location for $file"
    exit 1
  }
  regsub -all {\\} $location {/} location
  Puts -v "HTML document location: $location"
  set rx {href=3D[""](file:/*([^#""]+))#([^""]+)[""]}
  set index 0
  set nRemoved 0
  while {[regexp -start $index -indices $rx $text match url path anchor]} {
    foreach {m0 m1} $match break
    foreach {u0 u1} $url break
    foreach {p0 p1} $path break
    foreach {a0 a1} $anchor break
    set path [string range $text $p0 $p1]
    set anchor [string range $text $a0 $a1]
    set index [expr $m1 + 1]
    regsub -all "=\n" $path "" path
    regsub -all "%20" $path " " path
    regsub -all "=\n" $anchor "" anchor
    Puts -d "path = \"$path\""
    Puts -d "anchor = \"$anchor\""
    if {"$path" == "$location"} {
      Puts -d "Removing path"
      incr nRemoved
      set text [string replace $text $a0 $a1 $anchor]
      set text [string replace $text $u0 $u1]
      incr index [expr $p0 - $p1]
    }
  }
  Puts -v "Removed $nRemoved useless document location references."
  WriteFile $file $text
}

