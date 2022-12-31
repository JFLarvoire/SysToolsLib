#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   File name	    nlines.tcl                                                #
#                                                                             #
#   Description     Count source lines in a multi-language project            #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2016-10-17 JFL Created this script.                                      #
#    2022-04-04 JFL Added support for Python.                                 #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Set defaults
set version "2022-04-04"
set script [file tail $argv0]

#=============================================================================#
#                          General Purpose routines                           #
#=============================================================================#

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    ReadFile                                        	      #
#                                                                             #
#   Description     Read a whole file at once.                                #
#                                                                             #
#   Parameters      filename           The file name                          #
#                   onError            What to do then. Default: error out.   #
#                                                                             #
#   Returns 	    The file contents                                         #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2005-01-15 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

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

#=============================================================================#
#                          Script-specific routines                           #
#=============================================================================#

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    CountLines                                        	      #
#                                                                             #
#   Description     Count the number of lines in a file, by category          #
#                                                                             #
#   Parameters      filename           The file name                          #
#                                                                             #
#   Returns 	    An array with the total # of lines, and 3 categories:     #
#                   - Blank lines = Empty lines, or lines with just spaces.   #
#                   - Comment lines = Lines with just a comment, and no code. #
#                   - Non-Commented Source Lines = Everything else.           #
#                   total = blank + comment + ncsl                            #
#                                                                             #
#   Notes:	    In most languages, exactly identifying all comments would #
#                   require a full language parser. As this is not practical  #
#                   here, we default to using simple regular expressions that #
#                   correctly identify most comments.                         #
#                                                                             #
#   History:								      #
#    2016-10-17 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc RemoveComment. {text} { # Unidentified file type. Remove nothing.
  return $text
}

proc RemoveComment.Doc {text} { # Documentation is comment. Remove everything.
  return ""
}

proc RemoveComment.Asm {text} { # Remove comment lines from Assembler sources
  regsub -all -line {^\s*;.*} $text {} text
  return $text
}

proc RemoveComment.C {text} { # Remove comments from C sources
  regsub -all -line {//.*$} $text {} text		;# C++ Line comments
  regsub -all {/\*.*?\*/} $text {} text			;# C Block comments
  # (Will fail if both types of comments are mixed in tricky ways)
  # (Also does not handle continuation lines after a trailing \. Rare, but possible)
  return $text
}

proc RemoveComment.Batch {text} { # Remove comment lines from Windows batch files
  regsub -all -line {^\s*:[:#].*} $text {} text		;# Labels used as comments
  set rx {^\s*rem(\s.*)?$}
  regsub -all -line -nocase $rx $text {} text		;# Remarks
  # Not handled: %inexistent variables used as comments%
  return $text
}

proc RemoveComment.Shell {text} { # Remove comment lines from Unix shell scripts
  regsub -all -line {^\s*#.*} $text {} text
  # Not handled: Continuation lines
  return $text
}

proc RemoveComment.Makefile {text} { # Remove comment lines from makefiles
  regsub -all -line {^\s*#.*} $text {} text
  # Not handled: Continuation lines, common in makefiles
  return $text
}

proc RemoveComment.PowerShell {text} { # Remove comments from PowerShell scripts
  regsub -all -line {^\s*#.*} $text {} text		;# Line comments
  regsub -all {<#.*?#>} $text {} text			;# Block comments
  return $text
}

proc RemoveComment.Python {text} { # Remove comment lines from Python scripts
  regsub -all -line {^\s*#.*} $text {} text
  # Not handled: File-scope """multi-line strings"""
  return $text
}

proc RemoveComment.Tcl {text} { # Remove comment lines from Tcl scripts
  regsub -all -line {^\s*#.*} $text {} text		;# Line comments
  set rx {if\s+(0|{\s*0\s*})\s+{([^{}]|{[^{}]*})*}}
  regsub -all $rx $text {} text				;# Block comments
  return $text
}

proc RemoveComment.XML {text} { # Remove comments from XML documents
  regsub -all {<!--.*-->} $text {} text			;# Block comments
  return $text
}

proc RemoveComment.Ini {text} { # Remove comment lines from Windows configuration files
  regsub -all -line {^\s;.*} $text {} text
  return $text
}

proc CountLines {name {type ""}} {
  set lines(lines) 0
  set lines(blank) 0
  set lines(comment) 0
  set lines(ncsl) 0

  set content [ReadFile $name {return ""}]
  if ![string length $content] {
    return [array get lines]
  }

  # Count the total number of lines
  set lines(lines) [expr [regsub -all {\n} $content {\n} -] + 1]

  # Count blank lines
  set nTextLines [regsub -all {[^\n]*\S[^\n]*} $content {\0} -]
  set lines(blank) [expr $lines(lines) - $nTextLines]

  # Remove comments
  set content [RemoveComment.$type $content]

  # Count non-commented source lines
  set lines(ncsl) [regsub -all {[^\n]*\S[^\n]*} $content {\0} -]
  set lines(comment) [expr $nTextLines - $lines(ncsl)]

  return [array get lines]
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    ScanFiles                                        	      #
#                                                                             #
#   Description     Scan files, and count the total number of lines           #
#                                                                             #
#   Parameters      retVar          The parent array name for results         #
#                   baseDir	    The base directory                        #
#                   patterns        A list of wildcard patterns to search for #
#                   opts	    A list of named options                   #
#                                                                             #
#   Returns 	                                                              #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2016-10-18 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc ScanFiles {retVar {baseDir .} {patterns *} {opts ""}} {
  if {$::debug} {
    puts "ScanFiles $retVar {$baseDir} {$patterns} {$opts}"
  }
  array set total {count 0}
  # Variables that may be overridden by the caller
  set recurse 0
  set formatString "%-10s %7s %7s %7s %7s   %s"
  set alignFolderName "           "
  foreach {var val} $opts {
    set $var $val
  }

  if {$recurse && ("$baseDir" != "")} {
    set baseDirName [regsub {^\.[/\\]} [file nativename "$baseDir"] {}]
    puts ""
    puts "$alignFolderName$baseDirName"
    puts ""
  }

  # Scan the base directory
  foreach pattern $patterns {
    regsub -all {\\} $pattern {/} pattern
    set fullPattern $pattern
    if {"$baseDir" != ""} {
      set fullPattern "$baseDir/$pattern"
    }
    foreach f [glob -nocomplain -types f $fullPattern] {
      set ext [file extension $f]
      set name [file tail $f]
      if {![regexp {\.} $name] && [regexp {\#!\s*(\S+)\s} [ReadFile $f {return ""}] - interpreter]} {
      	# Unix convention: The first line of an executable text file contains a #! comment with the interpreter pathname
      	set ext ".[file tail $interpreter]" ;# Use the interpreter name as a pseudo extension.
      }
      if [regexp -nocase {makefile$} $name] { # Makefile or NMakefile are make files
	set ext ".mak"
      }
      if [regexp {^((.*~)|(#.*#)|(.*\.\d\d\d))$} $name] { # Backup files left by various tools
	set ext ".bak"
      }
      if [regexp {^\..*} $name -] { # Unix convention: Files names beginning with a dot are hidden
	set ext ".hidden"
      }
      set type ""
      switch [string tolower $ext] {
	".asm" -
	".mac" -
	".equ" -
	".inc" {
	  set type Asm
	}
	".c" -
	".cc" -
	".cpp" -
	".h" -
	".cs" -
	".java" -
	".js" -
	".php" -
	".rc" -
	".r" {
	  set type C
	}
	".mak" {
	  set type Makefile
	}
	".bat" -
	".cmd" {
	  set type Batch
	}
	".bash" -
	".sh" -
	".awk" {
	  set type Shell
	}
	".tcl" -
	".tk" -
	".tclsh" -
	".expect" -
	".exp" {
	  set type Tcl
	}
	".ps1" -
	".psm1" {
	  set type PowerShell
	}
	".py" {
	  set type Python
	}
	".xml" -
	".manifest" {
	  set type XML
	}
	".ini" -
	".inf" -
	".reg" -
	".def" -
	".conf" {
	  set type Ini
	}
	".htm" -
	".html" -
	".css" -
	".md" -
	".txt" {
	  set type Doc
	}
	".bak" -
	".tmp" -
	".log" -
	".lis" -
	".dif" -
	".lst" -
	".cod" -
	".map" -
	".sym" -
	".sbr" -
	".bsc" -
	".vcw" -
	".dbg" -
	".wsp" -
	".res" -
	".zip" -
	".jar" -
	".pdb" -
	".lib" -
	".exe" -
	".dll" -
	".obj" -
	".com" -
	".vxd" -
	".bin" -
	".sys" -
	".o" -
	".so" -
	".a" -
	".ico" -
	".png" -
	".bmp" -
	".gif" -
	".jpg" -
	".pdf" -
	".doc" -
	".docx" -
	".hidden" {
	  incr total(skipped)	;# Ignore non source code file
	}
	default {
	  puts stderr [format $formatString "???" "" "" "" "" [file tail $f]]
	  incr total(unknown)
	}
      }
      if {"$type" != ""} {
	array set lines [CountLines $f $type]
	puts [format $formatString $type $lines(lines) $lines(blank) $lines(comment) $lines(ncsl) [file tail $f]]
	incr total(count)
	incr total(lines)   $lines(lines)
	incr total(blank)   $lines(blank)
	incr total(comment) $lines(comment)
	incr total(ncsl)    $lines(ncsl)
      }
    }
  }

  if {$total(count) > 1} {
    puts ""
    puts [format $formatString Total $total(lines) $total(blank) $total(comment) $total(ncsl) "${total(count)} files"]
  }
  set nLocalFiles $total(count)

  if ($recurse) {
    if {"$baseDir" != ""} {
      set baseDirs [list $baseDir]
    } else {
      set baseDirs {}
      foreach pattern $patterns {
      	lappend baseDirs [file dirname $pattern]
      }
    }
    foreach baseDir $baseDirs {
      foreach dir [glob -nocomplain -types d "$baseDir/*"] {
	if ![regexp {^\.} [file tail $dir] -] { # Except for folders like .gitignore
	  ScanFiles total $dir $patterns $opts
	}
      }
    }
  }

  if {$total(count) > $nLocalFiles} {
    puts ""
    puts "$alignFolderName$baseDirName[file separator]..."
    puts ""
    puts [format $formatString Total $total(lines) $total(blank) $total(comment) $total(ncsl) "${total(count)} files"]
  }

  upvar $retVar parent
  foreach key {count lines blank comment ncsl skipped unknown} {
    if {"[array names total $key]" != ""} {
      incr parent($key) $total($key)
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
#    2005/01/15 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Usage string
set usage [subst -nobackslashes -nocommands {
Non-Commented Source Lines counter version $version

Usage: $script [options] [[DIRNAME/]PATTERN] [PATTERN] [...]
or     $script [options] @INPUTFILE

Options:
  -h, --help, -?        Display this help screen
  -r, --recursive       Recursively scan all subdirectories
  -V, --version         Display this library version
}]

set initial_argv $argv          ; # Keep the initial command line for debug.
set patterns ""
set recurse 0
set baseDir .
set debug 0

# Scan all arguments.
set args $argv
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-d" - "--debug" {		# Debug mode
      set debug 1
    }
    "-h" - "--help" - "-?" {	# Display a help screen and exit.
      puts $usage
      exit 0
    }
    "-r" - "--recursive" {	# Recursive mode
      set recurse 1
    }
    "-V" - "--version" {	# Display this library version
      puts $version
      exit 0
    }
    default {
      if [regexp {@(.*)} $arg - inputFile] { # If this specifies an input file, read it
	set baseDir ""
      	foreach line [split [ReadFile $inputFile {return ""}] \n] {
      	  regsub {^\s*#.*} $line {} line    ;# Allow commenting-out a line
      	  set line [string trim $line]	    ;# Make sure there are no invisible spaces left
      	  set line [string trim $line {""}] ;# Allow quoting invisible spaces we do want
      	  if {"$line" != ""} {
	    if [file isdirectory $line] {
	      set line "$line/*"
	    }
	    lappend patterns [string trim $line]
	  }
      	}
      } else {
	if [file isdirectory $arg] {
	  set baseDir $arg
	} else {
	  if [regexp {[/\\]} $arg -] {
	    set baseDir [file dirname $arg]
	    set arg [file tail $arg]
	  }
	  lappend patterns $arg
	}
      }
    }
  }
}

if {![llength $patterns]} {
  set patterns "*"
}

set total(count) 0	;# Number of files processed
set total(lines) 0	;# Total number of lines
set total(blank) 0	;# Number of blank lines
set total(comment) 0	;# Number of comment lines
set total(ncsl) 0	;# Number of Non-Commented Source Lines
set total(skipped) 0	;# Number of files skipped (non-source files)
set total(unknown) 0	;# Number of unidentified files

set formatString "%-10s %7s %7s %7s %7s   %s"
puts [format $formatString Type Lines Blank Comment NCSL Name]
if !$recurse {puts ""}

set opts {}
lappend opts recurse $recurse formatString $formatString
regsub -all {\\} $baseDir {/} baseDir 
ScanFiles total $baseDir $patterns $opts

if {$total(unknown)} {
  puts stderr "\n$script: Warning: ${total(unknown)} unidentified files were not measured"
}
