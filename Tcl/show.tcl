#!/usr/bin/tclsh
#*****************************************************************************#
#                                                                             #
#   Script name     show                                                      #
#                                                                             #
#   Description     Convert a directory tree to a single structured text file.#
#                                                                             #
#   Design notes    The text format design goals are:                         #
#                   1) Default output format very easily readable by humans.  #
#                   2) Verbose version with full meta-data information.       #
#                   3) XML equivalence, for easy parsing and data retrieval.  #
#                                                                             #
#                   It uses XML-style semantic elements (a tag, attributes,   #
#                   and a body), but formatted as a C or Tcl-style text tree. #
#                                                                             #
#                   Note: Some features below are not implemented. They're    #
#                   (marked with parenthesis)                                 #
#                                                                             #
#                   Default output format:                                    #
#                                                                             #
#                   A file is coded as one XML element.                       #
#                   The tag is the file name.                                 #
#                   Attributes (In verbose mode only)                         #
#                     size=N                        Explicit file size        #
#                     type=TYPE                     Ex: chardev, blockdev ... #
#                     error="permission denied"     Error encountered, if any #
#                     time="2007-05-14 17:48:55"    Date/Time in ISO format   #
#                     format="base64"               Encoding. Default=text    #
#                    (flags="0766"                  Unix flags               )#
#                    (indent="8"                    Indentation. Default=2   )#
#                   The body is the file content, like XML CDATA.             #
#                   Text data is currently left unchanged (format="text"),    #
#                   whereas binary data is shown as an hexadecimal dump, plus #
#                   an ascii dump on the side (format="hexdump").             #
#                   One-line files follow as a quoted string on the same line.#
#                   The quotes can be omitted if there's a single word with   #
#                   no reserved character. (One of: =";{}<>#\ )               #
#                   Multi-line bodies follow in an indented CDATA block.      #
#                   As most text files end with a new line, even 1-line files,#
#                   the shortest notation has been selected for that case. Ex:#
#                   The three representations below are equivalent, and       #
#                   encode the 1-line file with either the 5 bytes "hello",   #
#                   or the 6 bytes "hello\n", or the 7-byte "hello\r\n".      #
#                     hello.txt hello                                         #
#                     hello.txt "hello"                                       #
#                     hello.txt {                                             #
#                       hello                                                 #
#                     }                                                       #
#                   To distinguish theses three cases, use verbose mode.      #
#                   In the above example, we'd have size=5 for file "hello",  #
#                   or size=6 for "hello\n".                                  #
#                                                                             #
#                   A directory is coded as one XML element.                  #
#                   The tag is the directory name, ending with a /.           #
#                   The text body contains a series of indented file elements,#
#                   each one on a new line.                                   #
#                   Ex:                                                       #
#                     my_dir/ {                                               #
#		        one.txt                1                              #
#		        hello.txt              "hello world"                  #
#			"a long file name.txt" "My name must be quoted"       #
#		      }                                 		      #
#                                                                             #
#                   A link is flagged with an arrow. Ex: sh -> bash           #
#                                                                             #
#                   Other types of files are flagged by an [x] attribute in   #
#                   the default mode, or type="Type" in verbose mode.         #
#                   I tried appending a special character to the name, as     #
#                   some other Linux tools do, but this is not as readable.   #
#                     Type                Attribute         (not) appended    #
#                     socket              [s]               =                 #
#                     fifo (aka pipe)     [f]               |                 #
#                     chardev             [c]                                 #
#                     blockdev            [b]                                 #
#                   Ex: /dev/mydisk [b] ""                                    #
#                                                                             #
#                   Note: The standard file type indicators for "ls -F" are:  #
#                     *     Regular files that are executable                 #
#                     /     Directories                                       #
#                     @     Symbolic links                                    #
#                     |     FIFOs                                             #
#                     =     Sockets                                           #
#                     >     Doors (?)                                         #
#                                                                             #
#                   By default, the output for each file ends after 10 lines. #
#                   Truncated files are flagged by ellipsis ... after the     #
#                   content block, and a size=N and show=N attribute in       #
#                   verbose mode.                                             #
#                                                                             #
#                   Optionally, for use of the output by other programs,      #
#                   it's possible to use the SML [or XML] output formats.     #
#                   SML is more verbose than the default format,              #
#                   but more suitable for parsing by programs.                #
#                   It is strictly equivalent to XML in essence.              #
#                   Currently the XML output is not implemented, but an       #
#                   outside tool for converting SML -> XML is available.      #
#                   The tag is the file type: file|directory|link|socket|...  #
#                   Then an attribute gives the name: name="hello.txt"        #
#                                                                             #
#                   Experimental:                                             #
#                   SML++ CDATA sections distinguish cases with a trailing \n #
#                   (= =: ={...}) and those without (== ==: =={...}).         #
#                   ={ or =={ begins an indented CDATA section.               #
#                   It must be indented by 2 times the parenthesis depth.     #
#                   The content ends where the indentation is less than that. #
#                   Ex: The three representations below encode the same file  #
#                   containing the 6 bytes: h e l l o \n                      #
#                     file name="hello.txt" = hello                           #
#                     file name="hello.txt" = "hello"                         #
#                     file name="hello.txt" ={                                #
#                       hello                                                 #
#                     }                                                       #
#                   whereas the following have only 5 bytes: h e l l o        #
#                     file name="hello.txt" == hello                          #
#                     file name="hello.txt" == "hello"                        #
#                     file name="hello.txt" =={                               #
#                       hello                                                 #
#                     }                                                       #
#                                                                             #
#                   Things tried and abandonned:                              #
#                   * Adding a shown=N attribute to flag the fact that not    #
#                     all the file content is shown. (Either because the      #
#                     final end of line is truncated, or because the maximum  #
#                     number of lines requested has been reached.)            #
#                     This brings no additional information.                  #
#                                                                             #
#   Known bugs      Will probably fail on a Mac, as we do binary reads, but   #
#                   use \n as new-line markers in many places.                #
#                                                                             #
#                   file readlink works with windows junctions, but not with  #
#                   Windows links. To be retested with more recent Tcl vers?  #
#                                                                             #
#   License         Copyright (c) 2007-2013, Jean-François Larvoire	      #
#		    All rights reserved.                                      #
#                                                                             #
#		    Redistribution and use in source and binary forms, with   #
#		    or without modification, are permitted provided that the  #
#		    following conditions are met:                             #
#                                                                             #
#		    * Redistributions of source code must retain the above    #
#		       copyright notice, this list of conditions and the      #
#		       following disclaimer.                                  #
#		    * Redistributions in binary form must reproduce the above #
#		       copyright notice, this list of conditions and the      #
#		       following disclaimer in the documentation and/or other #
#		       materials provided with the distribution.              #
#		                                                              #
#		    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND    #
#		    CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED           #
#		    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    #
#		    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A           #
#		    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE  #
#		    COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,#
#		    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL#
#		    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF    #
#		    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR       #
#		    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON  #
#		    ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT      #
#		    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    #
#		    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  #
#		    IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.             #
#		                                                              #
#   Author          Jean-François Larvoire, jf.larvoire@free.fr               #
#                                                                             #
#   History                                                                   #
#    2007-05-14 JFL Initial implementation, limited to unencoded text files.  #
#                   Encoded file data inside {}, and had an implicit \n after #
#                   one-line data in ""s instead of {}s.                      #
#    2007-05-16 JFL Changed the encoding to be more SML compliant, with new   #
#                   CDATA encodings marked by =, ==, ={ ... }, =={ ... } .    #
#    2007-05-21 JFL Bug fix: Do not stop at the first error, but output an    #
#                   error="message" attribute, and continue.                  #
#                   Bugfix: Corrected the 'can't read "maxNC"' error.         #
#                   Bugfix: File length reported was too big by 1 byte.       #
#                   Bugfix: Special files display was broken in version 0.03. #
#                   Bugfix: Link names were broken when link -> link.         #
#                   New: For names, use dictionary sort, not ascii sort.      #
#                   Version 0.04.                                             #
#    2007-06-15 JFL Moved all show formatting routines into a namespace for   #
#                   easy recycling into other projects.                       #
#                   Added path globbing support for Windows.                  #
#                   Version 0.05.                                             #
#    2007-06-27 JFL Updated the debug package to support debugging namespaces.#
#    2007-07-09 JFL Bug fix: Can't use regexp c{$n} with n>255.               #
#    2007-07-10 JFL Changed the default format to encode the final \n, if any.#
#                   Version 0.06.                                             #
#    2007-07-13 JFL Changed back to not encode it, and rely on the show= and  #
#                   size= attributes to know it. Version 0.07.                #
#    2008-09-16 JFL Minor change in ReadFile to fix syntax colouring issues.  #
#                   Added option -C/--checksum. Version 0.08.                 #
#    2008-10-08 JFL Added routine WalkFiles; Restructured ReadFile to use it, #
#                   and added ListFiles using it too. Fixed a few minor bugs. #
#                   Version 0.09.                                             #
#    2009-09-02 JFL Several {rx} changes to satisfy my editor syntax checker. #
#                   Merged in the latest version of my debug library.         #
#                   Added the -i/--in and -s/--sort options.                  #
#                   Added support for SML comments in input trees.            #
#                   Version 1.0.                                              #
#    2013-07-28 JFL Merged in my latest debugging framework version.          #
#    2013-07-29 JFL Report error gracefully when denied access to a directory.#
#    2013-09-18 JFL Improved the output compatibility with sml.               #
#    2013-09-21 JFL In SML mode, only display a dir / suffix w. new option -F.#
#                   In own mode, don't display it in quiet mode.              #
#                   Removed the shown attribute, which gives no useful info.  #
#                   Added the time attribute in verbose mode.                 #
#                   Support text files with any type of line endings.         #
#    2013-09-25 JFL Added BSD-style license in the header.                    #
#    2017-08-29 JFL Added "no such device" to the list of errors to ignore.   #
#                                                                             #
#*****************************************************************************#

set version "2017-08-29"	      ; # The version of this script

# Set global defaults
set paths {}                          ; # List of pathnames to process.
set options(maxData) 0                ; # Max # of bytes to convert. 0=No max.
set options(maxLines) 10              ; # Max # of lines to output. 0=No max.
set options(maxDepth) 0               ; # Max # of dir levels. 0=No max.
set options(format) own               ; # Output format. own=simplified sml
set options(own) "0"                  ; # Variants of our own format. 0=Normal.
				      ; # 0 = = attrib. for files without crlf.
                                      ; # 1 = \r\n suffix for files with crlf.
				      ; # 2 = - suffix for files without crlf.
set options(sml) "0"                  ; # Variants of the sml format. 0=Normal.
				      ; # 1 = indented CDATA
set options(showCmd) puts             ; # Command to run to process the output.
set options(showRef) -nonewline       ; # Reference argument.
set options(tabSize) 8                ; # Tabulation size.
set options(checksum) 0               ; # 1=Output checksum; 0=Output data
set options(classify) 1		      ; # 1=Append a class suffix to file names

###############################################################################
#                          General purpose routines                           #
###############################################################################

# Check if a command line argument is a switch
proc IsSwitch {arg} {
  string match -?* $arg
}

# Compare strings with the lsort -dictionary algorithm
proc StrCmpDict {s1 s2} {
  if {"$s1" == "$s2"} {
    return 0
  }
  set l1 [list $s1 $s2]
  set l2 [lsort -dict $l1]
  if {"$l1" == "$l2"} {
    return -1
  } else {
    return 1
  }
}

###############################################################################
#                 Output, logging and debug library routines                  #
###############################################################################

#=============================================================================#
# Features:                                                                   #
# - An output, logging and debug framework.                                   #
# - An optional execution tracing framework.                                  #
# - Use the best system logging functions available.                          #
# - Use Tcl's tracing capabilities, if available, else emulate them.          #
# - In its own namespace to avoid name collisions.                            #
# - Extends standard Tcl commands (puts->Puts, exec->Exec, etc.)              #
#                                                                             #
# Usage:                                                                      #
#   source debuglib.tcl                                                       #
#   debug::Import ; # Optional: Import public names from the ::debug namespace#
#                                                                             #
# Categories of routines:                                                     #
# - Namespace management routines:                                            #
#     Define a public proc: xproc                                             #
#     Define public variables: xvariable, xvars                               #
#     Import all public procs and variables: Import                           #
# - General utility routines, used internally, and useful for more than debug:#
#     Pop an argument off a variable arg list: PopArg, PeekArg, PopLast       #
#     Get a date/time stamp: Now, NowMS                                       #
#     Indent a text string: IndentString                                      #
#     Get the name and value of a set of variables: VarsValue                 #
#     Find a program in the PATH: Which                                       #
# - Debug output and logging routines:                                        #
#   Put strings to a choice of stdout, private logs, the system log, callbacks#
#     Log strings: LogString (Private logs), LogSystem (System event log)...  #
#     Output and log strings: Puts, VerbosePuts, DebugPuts...                 #
#     Output and log variables values: PutVars, DebugVars, DebugSVars....     #
#     Indent the output of a command or a block of code: Indent               #
#     Check the verbosity mode: Quiet, Normal, Verbose, Debug, XDebug         #
#     Set the verbosity mode: SetQuiet, SetNormal, SetVerbose, SetDebug, ...  #
# - Execution trace routines.                                                 #
#     Get the current procedure name: ProcName.                               #
#     Trace the entry in a routine with its parameters: TraceEntry            #
#     Trace the return value from a routine: Use Return instead of return.    #
#     Trace one routine entry and exit: Define it with Proc instead of proc.  #
#     Trace a whole set of routines entry and exit: TraceProcs. Usage:        #
#       debug::TraceProcs { # Begin proc tracing                              #
#         # Put routines to trace here. No need to modify them.               #
#       } ;          # End proc tracing.                                      #
#     Tracing goes to the default debug output file: /var/log/$scriptname.log.#
#     This can be changed by inserting an optional filename argument. Ex:     #
#       debug::TraceProcs /tmp/tmpfile.log { # Begin proc tracing ... }       #
# - Miscelleanneous other routines.                                           #
#     A sample background error handler using this framework: bgerror         #
#     Generate an error, inclusing the call stack: Error                      #
# - Program Execution Management routines                                     #
#     Execute a program, logging and optionally displaying ins & outs: Exec   #
#     Get the exit code of a program: ErrorCode                               #
#                                                                             #
# See section comments in the code below for further details.                 #
#=============================================================================#

namespace eval ::debug {

#=============================================================================#
#                            Namespace management                             #
#=============================================================================#

# These namespace management routines can be defined in this, or the root namespace.
# They can be used to define public procs and variables in any namespace.

# If defined in the root namespace, these 2 directives are not necessary...
namespace export xproc ; # Make sure xproc itself is exported
variable xprocs xproc  ; # List of public procs exported from this namespace

# Define a procedure to export from the namespace it's used in.
# This automatically defines an Import proc in the namespace it's used in.
proc xproc {name args body} {
  set ns [uplevel 1 namespace current]
  set Import ${ns}::Import
  if {[lsearch [info procs $Import] $Import] == -1} { # Define Import once.
    uplevel 1 namespace export Import
    if {![info exists "${ns}::xvariables"]} {
      set ${ns}::xvariables ""
    }
    # Import all this namespace routines into the caller's namespace.
    proc $Import {} {
      set ns [namespace current]
      uplevel 1 [list namespace import -force "${ns}::*"]
      # Duplicate Tcl execution trace operations, if any.
      variable xprocs
      catch { # This will fail in Tcl <= 8.3
	foreach proc $xprocs {
	  foreach trace [trace info execution ${ns}::$proc] {
	    foreach {ops cmd} $trace break
	    uplevel 1 [list trace add execution $proc $ops $cmd]
	  }
	}
      }
      # And import xvariables too
      variable xvariables
      foreach var $xvariables {
        uplevel 1 [list upvar 0 ${ns}::$var $var]
      }
    }
  }
  uplevel 1 namespace export $name
  proc ${ns}::$name $args $body
  lappend ${ns}::xprocs $name ; # List of all procedures exported from this namespace.
}

# Define a variable to export from the namespace it's used in.
# Allow overriding it by defining it _before_ defining the namespace: set NS::VAR VALUE
xproc xvariable {name args} {
  set ns [uplevel 1 namespace current]
  if {![info exists "${ns}::$name"]} {
    uplevel 1 variable [list $name] $args
  }
  if {![info exists "${ns}::xvariables"]} {
    set ${ns}::xvariables ""
  }
  lappend ${ns}::xvariables $name ; # List of all variables exported from this namespace.
}

# Define multiple variables at once. Use [list name value] for initialized vars.
xproc xvars {args} {
  foreach name $args {
    uplevel 1 xvariable $name
  }
}

#=============================================================================#
#                          General Purpose routines                           #
#=============================================================================#

# Remove an argument from the head of a routine argument list.
xproc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

# Remove an argument from the head of a routine argument list.
xproc PeekArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
}

# Remove an argument from the head of a routine argument list.
xproc PopLast {{name args}} {
  upvar 1 $name args
  set arg [lindex $args end]            ; # Extract the first list element.
  set args [lreplace $args end end]     ; # Remove the last list element.
  return $arg
}

# Build a time stamp with the current time.
xproc Now {{sep " "}} { # For ISO 8601 strict compatibility, use sep "T".
  clock format [clock seconds] -format "%Y-%m-%d$sep%H:%M:%S"
}

# Idem with milli-seconds.
# Warning: Under Tcl 8.4, this will yield an incorrect date and time.
# Still useful in log files to see relative times with a ms resolution.
xproc NowMS {{sep " "}} {  # For ISO 8601 strict compatibility, use sep "T".
  set ms [clock clicks -milliseconds]
  set s  [expr $ms / 1000]
  set ms [expr $ms % 1000]
  format "%s.%03d" [clock format $s -format "%Y-%m-%d$sep%H:%M:%S"] $ms
}

# Indent multiple lines
xproc IndentString {text {indent 2}} {
  set spaces [string repeat " " $indent]
  # regsub -all -line {^} $text $spaces text
  # regsub "\n$spaces$" $text "\n" text ; # Do not indent after the final \n
  regsub -all -line {^[^\r\n]} $text $spaces& text
  return $text
}

# Mimimum of N numbers
xproc Min {min args} {
  foreach arg $args {
    if {$arg < $min} {
      set min $arg
    }
  }
  return $min
}

# Maximum of N numbers
xproc Max {max args} {
  foreach arg $args {
    if {$arg > $max} {
      set max $arg
    }
  }
  return $max
}

# Format array contents with one element (name value) per line
xproc FormatArray {a} {
  upvar 1 $a a1
  set string ""
  set names [lsort -dictionary [uplevel 1 array names $a]]
  # Find good column width for names.
  set n 0       ; # Number of names
  set maxLen 0  ; # Maximum length of a name
  set total 0   ; # Total length of all names
  foreach name $names {
    incr n
    set l [string length $name]
    set maxLen [Max $l $maxLen]
    incr total $l
  }
  if $n {
    set average [expr $total / $n]
    set limit [expr $average + 10] ; # Reasonable limit to avoid oversize names
    set width [Min $maxLen $limit] ; # Choose the smaller of the two.
    # Output the data using that column width
    foreach {name} $names {
      append string [format "%-${width}s %s\n" [list $name] [list $a1($name)]]
    }
  }
  return $string
}

# Find a program among optional absolute pathnames, else in the PATH.
# Arguments:
#   prog       File name to search in the PATH
#   args       Optional full pathnames (including file name) to search first
# Returns:
#   The pathname of the first executable program found, or "" if none found.

# Find a program among optional absolute pathnames, else in the PATH.
# Unix-specific version
xproc Which {prog args} { # prog=Program Name; args=Optional absolute pathnames
  if [info exists ::env(PATH)] { # May not exist when started as a service.
    foreach path [split $::env(PATH) :] {
      lappend args "$path/$prog"
    }
  }
  foreach name $args {
    if [file executable $name] {
      return $name
    }
  }
  return ""
}

# Find a program among optional absolute pathnames, else in the PATH.
# Windows + Unix generic version
switch $tcl_platform(platform) { # Platform-specific PATH delimiter
  "windows" {
    variable pathDelim ";"
    variable pathExts {.com .exe .bat .cmd} ; # Default if not explicitely defined
  }
  "unix" - default {
    variable pathDelim ":"
    variable pathExts {} ; # Unix does not have implicit program extensions.
  }
}
if [info exists ::env(PATHEXT)] { # Windows list of implicit program extensions
  set pathExts [split $::env(PATHEXT) $pathDelim]
}
set pathExts [linsert $pathExts 0 ""] ; # In all cases, try the exact name first.
xproc Which {prog args} { # prog=Program Name; args=Optional absolute pathnames
  variable pathDelim
  variable pathExts
  if [info exists ::env(PATH)] { # May not exist when started as a service.
    set paths [split $::env(PATH) $pathDelim]
    if {"$::tcl_platform(platform)" == "windows"} {
      set paths [linsert $paths 0 ""] ; # Search in the current directory first
    }
    foreach path $paths {
      lappend args [file join $path $prog]
    }
  }
  foreach name $args {
    foreach ext $pathExts {
      if [file executable "$name$ext"] {
	return "$name$ext"
      }
    }
  }
  return ""
}

# Escape a string. ie. change special string charaters to \c & \xNN sequences.
# Does the reverse of {subst -nocommands -novariables $text}
xproc Escape {text} {
  regsub -all {\\} $text {\\\\} text ; # Double every pre-existing backslash.
  foreach c {"\"" a b f n r t v} {
    regsub -all [subst \\$c] $text \\$c text ; # Escape C string control chars.
  }
  foreach xx {00 01 02 03 04 05 06                      0E 0F
              10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F} {
    regsub -all [subst \\x$xx] $text \\x$xx text ; # Escape other control chars.
  }
  return $text
}

# Quotify a string if needed. ie. when spaces, quotes, or a trailing \.
# Prefer {} for multi-line strings, and "" for single line strings.
xproc CondQuote {text} {
  if {"$text" == ""} {
    return {""}
  }
  # If there are quotes or newlines, but no invisible characters (including \r)
  if {[regexp {[""\n]} $text -] && ![regexp {[\x01-\x09\x0B-\x1F]} $text -]} {
    # TO DO: Verify that {parenthesis} are balanced. Else escape them!
    return "{$text}" ; # Then use parenthesis to avoid escaping quotes.
  }
  # If there are quotes, spaces, or invisible characters
  if {[regexp {[""\s[:cntrl:]]} $text -]} {
    set text [Escape $text]
    if [regexp {\s} $text -] { # Surround with quotes if there are spaces.
      set text "\"$text\""
    }
  }
  return $text
}

#=============================================================================#
#                              Logging routines                               #
#									      #
# Several alternative logging options exists, which can be combined:	      #
#									      #
# LogEvent        Calls both LogSystem and LogString.			      #
#   LogSystem       It logs into the system event log, using ONE method among #
#                     1) SFS library routine hplsLog -> The cluster log	      #
#                     2) evlsend -> The cluster log			      #
#                     3) logger -> The local /var/log/messages log	      #
#   LogString       It logs into application-specific logs, using ALL methods:#
#     LogToFile       Log into a named log file. "" = none.     	      #
#       SetLogFile    Define a default log file name. "" = none.     	      #
#     LogToCallBack   Log into a user-defined callback. "" = none.	      #
#       SetLogCallBack Define a log callback routine. "" = none. 	      #
#     LogToChannel    Log into channel Ilo::hLogFile. "" = none.      	      #
#       OpenLogChannel  Create a log file and store the handle into hLogFile. #
#       CloseLogChannel Close Ilo::hLogFile, and clear the variable.          #
#=============================================================================#

# Global settings. Can be overriden by defining them before referencing this pkg.
# Script name.
if [info exists ::argv0] { # argv0 is not always defined!
xvariable script [file tail $::argv0] ; # Use it if present.
} else {
xvariable script debuglib.tcl         ; # Else use a clever default.
}
# Log file name. Deprecated mechanism. Use OpenLogChannel or open your own $hLogFile instead. 
xvariable logFile "" ; # File name, or "" for none.
# Whether to create the above log file if absent.
xvariable createLog 0 ; # 0=Don't create; 1=Create it.
# Handle for a capture file opened by the user.
xvariable hLogFile "" ; # Tcl open channel, or "" for none.
variable hLogFileName ""
variable hLogFileDir ""
# Name of a user-defined callback logging routine, or "" for None.
xvariable logCallBack ""

# Name: "-"=stdout. Default path: ~/log/$script. Default name: timestamp.pid.log
xproc OpenLogChannel {{name ""}} {
  variable hLogFile
  variable hLogFileDir
  variable hLogFileName
  variable script
  if {"$hLogFile" != ""} return ; # Already started
  if {"$name" == "-"} { # Special case of logging to stdout
    set hLogFileDir ""
  } elseif {[file isdirectory $name]} {
    set hLogFileDir $name
    set name ""
  } elseif {[llength [file split $name]] > 1} {
    set hLogFileDir [file dirname $name]
    set name [file tail $name]
  } elseif [file exists $hLogFileDir] { # Just reuse the previous one
  } elseif [file writable /var/log] { # Unix root user
    set hLogFileDir "/var/log/[file rootname $script]"
  } else { # Non-root user
    set hLogFileDir "$::env(HOME)/log/[file rootname $script]"
  }
  if {"$hLogFileDir" != ""} {
    file mkdir $hLogFileDir ; # Create the log directory if needed.
  }
  if {"$name" == ""} {
    set name "[clock format [clock seconds] -format "%Y%m%d_%H%M%S"].[pid].log"
  }
  set hLogFileName [file join $hLogFileDir $name]
  if {"$hLogFileName" == "-"} {
    set hLogFile stdout
    puts "Logging iLO session and debug messages to stdout."
  } else {
    set hLogFile [open $hLogFileName a+] ; # And log everything into the given log file.
    puts "Logging iLO session and debug messages into [file nativename $hLogFileName]."
    DebugPuts -1 "$::argv0 $::argv"
  }
  SetExpectLogFile ; # Send Expect logging there too. Ignore error if no Expect.
  return $hLogFile
}

xproc CloseLogChannel {} {
  if {"$hLogFile" != ""} {
    close $hLogFile
    set hLogFile ""
  }
}

xproc SetExpectLogFile {} {
  variable hLogFile
  variable hLogFileName
  catch { # This will fail when not running under expect. Ignore that.
    log_file ; # Stop any previous expect logging
    log_user 0 ; # Stop any previous expect logging
    if {"$hLogFileName" == "-"} {
      log_user 1
    } elseif {"$hLogFile" != ""} {
      log_file -open $hLogFile ; # And log everything into the given log file.
    }
  }
  InitTraceSend ; # And while we're at it, make sure send is traced too.
}

# Record a string in the system event log.
# Arguments:
#   string        String to record
#   category      server|storage|lustre|admin. Default: server
#   severity      debug|info|notice|warning|err|crit|alert|emerg. Default: info
# Notes:
#                 Standard Linux log facilities: (Same as category?)
#		   auth, authpriv, cron, daemon, ftp, kern, lpr, mail, news,
#                  syslog, user, uucp, and local0 to local7
# Returns:
#   0=Success; !0=Failure
xvariable evlsend [Which evlsend /sbin/evlsend] ; # An event log manager, working with evlview.
xvariable logger [Which logger /usr/bin/logger] ; # Another log file manager -> /var/log/messages
xproc LogSystem {string {severity info} {category user}} {
  variable script
  variable evlsend
  variable logger
  set tag $script
  # Log it into the system event log, using one of several alternative methods.
  if {"[info commands hplsLog]" == "hplsLog"} { # 1st choice: Use SFS' hplsLog.
    set string "$tag: $string"
    hplsLog $category $severity $string
  } elseif {"$evlsend" != ""} { # 2nd choice: Use evlog's evlsend.
    set string "$tag: $string"
    # Note: Redirect stdin to /dev/null, as this may be executed with stdin closed.
    catch [list exec $evlsend -f $category -t 1 -s $severity -m "$string" </dev/null]
  } elseif {"$logger" != ""} { # 3rd choice: Use Linux logger.
    # Contrary to others, logger accepts any tag instead of limited categories.
    catch [list exec $logger -p syslog.$severity -t $tag "$string" </dev/null]
  }
}

# Let the user define his own logging routine. For example use Expect send_log.
xproc LogToCallBack {string} {
  variable logCallBack
  if {"$logCallBack" != ""} {
    $logCallBack $string
  }
}

# Set the user-defined logging routine. Use "" to cancel.
xproc SetLogCallBack {callback} {
  variable logCallBack
  set logCallBack $callback
}

# Append a string to a private log file.
# Only output something if the file already exists.
# Open and close the file every time. This allows sharing it with other scripts.
# Arguments:
#   string          The string to log.
#   fileName        Log file name, or "" for $logFile, or "-" for stdout.
xproc LogToFile {string {fileName ""}} {
  variable logFile
  variable createLog
  if {"$fileName" == ""} { # If not defined, use the default variable
    set fileName $logFile
  }
  if {"$fileName" == ""} return ; # If still not defined, just return
  if {("$fileName" == "-") || ($createLog || [file exists $fileName])} {
    set header [format "[NowMS] %5d" [pid]]
    catch { # exec echo "$header $string" >>$fileName
      if {"$fileName" != "-"} {
	set hf [open $fileName a+]
      } else {
	set hf stdout
      }
      puts $hf "$header $string"
      if {"$fileName" != "-"} {
	close $hf
      }
    }
  }
}

# Log a string into all our private log alternatives.
# Arguments:
#   -file NAME      Opt. local log file name to use. Default: variable logFile.
#   string          The last argument = the string to output.
xproc LogString {args} {
  variable logFile
  variable hLogFile
  set fileName $logFile
  set log 0             ; # 1=Log the string into the system event log.
  set string ""
  while {"$args" != ""} {
    set arg [PopArg]
    switch -- $arg {
      -f - -file { set fileName [PopArg] }
      default    { set string $arg ; break }
    }
  }
  LogToFile $string $fileName ; # Log to the private log file, if any.
  LogToCallBack $string       ; # Log to the user-defined callback, if any.
  if {"hLogFile" != ""} {     # Log to the user-provided channel, if any.
    catch {puts $hLogFile "[format "[NowMS] %5d" [pid]] $string"}
  }
}

# Routine for logging both to the system event log and to our private logs.
xproc LogEvent {string {severity info} {category server}} {
  catch {LogString $string} ; # Log into our private logs; ignore I/O errors.
  set err [LogSystem $string $severity $category] ; # Log into the system event log.
  if $err {
    DebugPuts "Error $err logging system event: \"$message\"" $severity $category
  }
  return $err
}

#=============================================================================#
#                          Debug output and logging                           #
#									      #
# Output procedures:.						 	      #
# The core routine is Puts, which is a superset of puts.		      #
# Usage: XxxxPuts [options] [stream] string				      #
#									      #
# verbosity N	Test proc Output proc	    Notes			      #
# --------  --  --------  ----------------  --------------------------------- #
# quiet	    0		  ForcePuts         Quiet proc tests verbosity < 1    #
# normal    1   Normal	  Puts	            Everything logged in private logs #
# verbose   2   Verbose   VerbosePuts	    				      #
# debug     3	Debug	  DebugPuts	    Indents output based on call depth#
# extra deb 4   Xdebug	  XDebugPuts	    For extreme cases		      #
#									      #
# Other useful routines:						      #
# VarsValue     Generate a string listing variables names=values	      #
# PutVars	Display variables names and values			      #
# PutSVars	Display a string, then variables names and values	      #
# DebugVars	Display variables names and values, in debug mode only	      #
# DebugSVars	Display a string, then var. names and values, in dbg mode only#
#=============================================================================#

# Global settings. Can be overriden by defining them before referencing this pkg.
# Output verbosity on stdout.
xvariable verbosity 1 ; # 0=Quiet 1=Normal 2=Verbose 3=Debug 4=XDebug
# Optional prefix to prepend to strings to output on stdout 
# variable prefix "$script: "
xvariable prefix ""
# Optional indentation of the output
xvariable indent 0

# Procedures checking if the current verbosity is at least at a given level.
foreach {name value} {Normal 1 Verbose 2 Debug 3 XDebug 4 XXDebug 5} {
  xproc $name {} "
    variable verbosity
    expr \$verbosity >= $value
  "
  xproc Set$name {} "
    variable verbosity
    set verbosity $value
  "
}
# Except for Quiet which checks it's at most that level
foreach {name value} {Quiet 0} {
  xproc $name {} "
    variable verbosity
    expr \$verbosity <= $value
  "
  xproc Set$name {} "
    variable verbosity
    set verbosity $value
  "
}

# Increase/decrease the output indentation
xproc IncrIndent {{step 2}} { # Increase the indentation by one step.
  variable indent
  incr indent $step
}
xproc DecrIndent {{step 2}} { # Decrease the indentation by one step.
  variable indent
  incr indent -$step
}
xproc Indent {args} { # Run code, outputing at an increased indentation level.
  IncrIndent
  if {[llength $args] == 1} { # Block of code: Indent { code ; code ; ... }
    set err [catch {uplevel 1 eval $args} result]
  } else {                    # Inline command: Indent COMMAND ARG1 ARG2 ...
    set err [catch {uplevel 1 $args} result]
  }
  DecrIndent
  return -code $err $result
}

# Output a string and log it.
# Options:
#   -1              Ignore 1 indent level. Ignored.
#   -show [0|1]     Whether to output on stdout. Default: 1=yes
#   -file NAME      Local log file name to use. Default: variable logFile.
#   -log [0|1|SEV]  Whether to log in the system event log. Default: 0=no
#   -category CAT   System event log category. Default: server
#   -severity SEV   System event log severity. Default: info
#   -noprefix       Do not prefix the application name before the output.
#   --              End of Puts options.
#   -nonewline      Don't output a new line.
# Arguments:
#   tclChannel      Tcl file handle for output. Optional. Default: stdout
#   string          The last argument = the string to output.
variable atNewLine 1    ; # Record if the last output ended with a new line
xproc Puts {args} {
  variable prefix
  variable logFile
  set show [Normal]     ; # 1=Output the string on stdout, except in quiet mode.
  set log 0             ; # 1=Log the string into the system event log.
  set fileName $logFile ; # Local log file name
  set category server
  set severity info
  variable indent
  set doIndent $indent
  while {"$args" != ""} {
    set arg [PopArg]
    switch -- $arg {
      -1             { }
      -category      { set category [PopArg] }
      -file          { set fileName [PopArg] }
      -i             { set doIndent [PopArg] }
      -log - -syslog { set log  [PopArg] ; # Also allow severities here...
                       if [regexp {^[a-z]+$} $log severity] { set log 1 } }
      -noprefix      { set prefix "" }
      -set           { variable [PopArg] [PopArg] }
      -severity      { set severity [PopArg] }
      -show          { set show [PopArg] }
      --             { break }
      default        { set args [linsert $args 0 $arg] ; break }
    }
  }
  if {"$fileName" == "-"} { # "-" means log to standard output
    set show 0            ; # Will be output by LogString
  }
  set msg [PopLast]
  variable atNewLine
  if {$doIndent && $atNewLine} {
    set msg [IndentString $msg $doIndent]
  }
  LogString -file $fileName $msg ; # Always log the message in our private log.
  if $show {                # Output the message if requested
    if {![catch {eof stdout}]} { # Avoid error if stdout is closed!
      catch {eval puts $args [list "$prefix$msg"]}
    }
  }
  set atNewLine [expr {([lsearch $args -nonewline] == -1)
                   || ("[string index $msg end]" == "\n")
                   || ($atNewLine && ("$msg" == ""))}] ; # Record if at EOL.
  if $log {                 # Log it into the system event log if requested
    LogSystem $msg $severity $category
  }
}

# Output a string in all cases, even in Quiet mode.
# Arguments:
#   args            Arguments to pass to Puts.
xproc ForcePuts {args} {
  eval Puts -show 1 $args
}

# Outputing a string in verbose or debug modes only.
# Arguments:
#   options         Options to pass to Puts. Default: None.
#   string          The last argument = the string to output.
xproc VerbosePuts {args} {
  # Always call Puts: We want to log the string even if we don't display it.
  eval Puts -show [Verbose] $args
}

# Allow extra indentation for routines run at caller's depth. (Using upelvel 1)
variable xdepth 0 ; # Extra depth level.
xproc DebugIndent {args} { # Run code, outputing at an increased indentation level.
  variable xdepth
  incr xdepth
  if {[llength $args] == 1} { # Block of code: DebugIndent { code ; code ; ... }
    set err [catch {uplevel 1 eval $args} result]
  } else {                    # Inline command: Indent COMMAND ARG1 ARG2 ...
    set err [catch {uplevel 1 $args} result]
  }
  incr xdepth -1
  return -code $err $result
}

# Output a string, indented by call depth, in debug modes only.
# Arguments:
#   -1              Ignore 1 indent level. Can be repeated.
#   options         Options to pass to Puts. Default: None.
#   string          The last argument = the string to output.
xproc DebugPuts {args} {
  variable xdepth
  set string [PopLast]
  set indent [expr $xdepth + [info level] - 1]
  set args2 {}
  foreach arg $args {
    if {"$arg" == "-1"} {
      incr indent -1
    } else {
      lappend args2 $arg
    }
  }
  incr indent $indent ; # Each indent level is 2 characters.
  # Always call Puts: We want to log the string even if we don't display it.
  eval Puts -show [Debug] -i $indent $args2 [list $string]
}

# Output a string, indented, in the extra debug mode only.
# Arguments:
#   args            Arguments to pass to DebugPuts.
xproc XDebugPuts {args} {
  eval DebugPuts -1 -show [XDebug] $args
}

# Generate a string listing variables names and values,
# formatted in a way suitable for reentry into tclsh for debugging.
# Arguments:
#   args          A list of variables names
xproc VarsValue {args} {
  set l {}
  foreach arg $args {
    if {![uplevel 1 info exists $arg]} {       # Undefined variable
      lappend l "unset [list $arg]"
    } elseif {[uplevel 1 array exists $arg]} { # Array name
      set value [uplevel 1 [namespace current]::FormatArray $arg]
      lappend l "array set [list $arg] {\n[IndentString $value]}"
    } else {                                   # Scalar variable
      lappend l "set [list $arg] [list [uplevel 1 set $arg]]"
    }
  }
  join $l " ; "
}

# Output variables values.
# Arguments:
#   args          A list of variables names
xproc PutVars {args} {
  Puts [uplevel 1 [namespace current]::VarsValue $args]
}

# Output a string and variables values.
# Arguments:
#   string        An introduction string.
#   args          A list of variables names
xproc PutSVars {string args} {
  Puts "$string [uplevel 1 [namespace current]::VarsValue $args]"
}

# Output variables values in debug mode only.
# Arguments:
#   args          A list of variables names
xproc DebugVars {args} {
  DebugPuts -1 [uplevel 1 [namespace current]::VarsValue $args]
}

xproc XDebugVars {args} {
  XDebugPuts -1 [uplevel 1 [namespace current]::VarsValue $args]
}

# Output a string and variables values in debug mode only.
# Arguments:
#   string        An introduction string.
#   args          A list of variables names
xproc DebugSVars {string args} {
  DebugPuts -1 "$string [uplevel 1 [namespace current]::VarsValue $args]"
}

#=============================================================================#
#                              Execution tracing                              #
#                                                                             #
# Procedures                                                                  #
#   TracePuts       Internal routine through which all trace output goes.     #
#   ShortenString   Limits the size of strings displayed. Ex. for huge args.  #
#   DebugArgLine    Convert an argument list to a string fitting on one line. #
#   ProcName        Return the caller procedure name.                         #
#   TraceEntry      Output a string with the proc name and arguments.         #
#   Return          Replaces return. Traces the return type and value.        #
#   Proc            Replaces proc. Traces entry, return, and duration.        #
#   TraceProcs      Redefines proc to trace all routines in a block. Usage:   #
#                    TraceProcs { # Begin proc tracing                        #
#                      # Put routines to trace here. No need to modify them.  #
#                    } ;          # End proc tracing.                         #
#                   TraceProcs blocks can safely be defined inside others.    #
#   SkipTraceProcs  Use inside a TraceProcs block to skip tracing a sub-block.#
#                                                                             #
#=============================================================================#

# Trace output routine. Redefine to integrate in other debug frameworks.
# Simple version assuming DebugPuts does the indentation
proc TracePuts {args} {
  eval DebugPuts -1 $args
}

# Get the caller's procedure name. (or that of an above caller)
xproc ProcName {{upLevel 1}} {
  set name [lindex [info level [expr [info level] - $upLevel]] 0]
  set name [uplevel $upLevel namespace which -command $name]
  regsub {^:+} $name {} name ; # Remove the root :: , which is painful to read.
  return $name
}

# Shorten a string, to make it fit on a single line
xproc ShortenString {msg {maxsize 0}} {
  # Escape formatting characters, and add quotes around the string if needed.
  set msg [CondQuote $msg]
  # Optionally limit the size to a reasonable length.
  if {$maxsize} {
    set l [string length $msg]
    if {$l > $maxsize} {
      set half [expr ($maxsize - 4) / 2]
      set half1 [expr $half - 1]
      set msg "[string range $msg 0 $half] ... [string range $msg end-$half1 end]"
    }
  }
  return $msg
}

# Shorten an argument list, for improved lisibility
xproc DebugArgLine {list} {
  set line ""
  foreach arg $list {
    if {![XDebug]} {
      set arg [ShortenString $arg 50]
    } else {
      set arg [CondQuote $arg]
    }
    append line "$arg "
  }
  return $line
}

# Trace the caller's procedure name and arguments.
# $args is DebugPuts arguments to pass through, not the traced procedure's arguments.
xproc TraceEntry {args} {
  set cmdLine [info level [expr [info level] - 1]]
  set cmdLine [DebugArgLine $cmdLine] ; # Shorten the arguments if needed
  set cmdLine [lreplace $cmdLine 0 0 [ProcName 2]] ; # Use the full proc. name.
  eval TracePuts -1 -1 $args [list $cmdLine]
}

# Modified return instruction, tracing routine return value.
# Arguments:
#   -args trcArgs  Option list to pass through to TracePuts. Optional.
#   -code retCode  Return type code. Optional.
#   -time duration Duration information string. Optional.
#   args           Optional arguments to pass through to return.
#   retVal         Return value
xproc Return {args} {
  set retCode return  ; # Force returning from the routine that called Return.
  set retInst return  ; # Instruction to display in the debug string.
  set retArgs {}      ; # Other return arguments
  set trcArgs {}      ; # TracePuts options
  set strTime ""      ; # Timing info string
  while {"$args" != ""} {
    set arg [PopArg]
    switch -- $arg {
      "-args" {
	set trcArgs [PopArg]
      }
      "-code" {
	set retCode [PopArg]
	switch $retCode {
	  "0" - "2" { # 0=Implicit return at the end of routine; 2=Explicit return.
	  }
	  "1" - "error" {
	    set retInst error
	  }
		"return" {
	    set retInst "return -code return"
	  }
	  "3" - "break" {
	    set retInst "return -code break"
	  }
	  "4" - "continue" {
	    set retInst "return -code continue"
	  }
	}
      }
      "-time" {
	set strTime ";# ([PopArg])"
      }
      default {
	lappend retArgs $arg
      }
    }
  }
  set string [concat $retInst [CondQuote $retArgs] $strTime]
  eval TracePuts -1 $trcArgs [list $string] ; # Indent 1 level more than entry
  eval return -code $retCode $retArgs
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    Proc                                        	      #
#                                                                             #
#   Description     Modified proc definition, tracing routine entry and exit  #
#                                                                             #
#   Parameters      name        Procedure name                                #
#                   args        Argument list                                 #
#                   body        The code                                      #
#                   proc        The actual proc definition routine (optional) #
#                   args        Optional args to pass through to TracePuts    #
#                                                                             #
#   Returns 	    Returns or errors exactly as a normal proc.               #
#                                                                             #
#   Notes:	    There are two versions of this routine:                   #
#                   - One using the standard Tcl >= 8.4 execution trace func. #
#                   - One written in pure Tcl, for Tcl <= 8.3.                #
#                                                                             #
#   History:								      #
#    2006       JFL Created this routine in pure Tcl.                         #
#    2007-06-27 JFL Added support for routines within namespaces.             #
#    2007-10-02 JFL Added the duration calculation, and display upon return.  #
#    2009-06-26 JFL Combined the new trace package using the standard Tcl     #
#                   execution trace, and the old package using a modified     #
#                   proc routine, into a single package, with two versions    #
#                   of the Proc routine, depending on Tcl's version.          #
#                                                                             #
#-----------------------------------------------------------------------------#

# First method, requiring at least Tcl 8.4.
if {[info tclversion] >= 8.4} {

# Modified procedure definition, tracing routine entry and exit.
# Use the standard Tcl >= 8.4 execution trace system.
# Warning: When importing a traced namespace procedure into another namespace,
#          the Tcl trace callbacks don't apply to the imported routine.
#          Use xproc and Import routines like the ones above, to make sure
#          that the trace callbacks are duplicated for the imported routine.
xproc Proc {name procargs body {proc proc} args} {
  set ns [namespace current]
  uplevel 1 [list $proc $name $procargs $body]
  uplevel 1 [list trace add execution $name enter [list ${ns}::OnEntry $args]]
  uplevel 1 [list trace add execution $name leave [list ${ns}::OnLeave $args]]
}

proc OnEntry {opts cmd op} {
  if {![Debug]} return ; # Avoid wasting time below
  set line [DebugArgLine $cmd]
  eval TracePuts -1 $opts [list $line]
  variable tStamps
  lappend tStamps [clock clicks -milliseconds]
}

proc OnLeave {opts cmd code result op} {
  if {![Debug]} return ; # Avoid wasting time below
  variable tStamps
  set t1 [clock clicks -milliseconds]
  set t0 [PopLast tStamps]
  switch $code {
    "0" - "ok" { # Normal return
      set leave "return"
    }
    "1" - "error" {
      set leave "error"
    }
    "2" - "return" {
      set leave "return -code return"
    }
    "3" - "break" {
      set leave "return -code break"
    }
    "4" - "continue" {
      set leave "return -code continue"
    }
  }
  set result [CondQuote $result]
  if {![XDebug]} {
    set result [ShortenString $result 70]
  }
  eval TracePuts -1 $opts [list "  $leave $result ; # ([expr $t1 - $t0]ms)"]
}

} else { # Alternative implementation working with any version of Tcl

# Modified procedure definition, tracing routine entry and exit.
# Generates a modified body, that explicitely logs entrance and exit
xproc Proc {name procargs body {proc proc} args} {
  set ns [namespace current]
  uplevel 1 [list $proc $name $procargs "
    ${ns}::TraceEntry $args
    ${ns}::SaveContext backupDebugVars $args
    set {Proc.$name.t1} \[clock clicks -milliseconds\] ; # Call time
    set err \[[list catch $body ret]\]
    set {Proc.$name.t2} \[clock clicks -milliseconds\] ; # Return time
    set {Proc.$name.dt} \[expr \${Proc.$name.t2} - \${Proc.$name.t1}\] ; # Duration
    ${ns}::RestoreContext backupDebugVars
    ${ns}::Return -args [list $args] -time \${Proc.$name.dt}ms -code \$err \$ret
  "]
}

# Save namespace variables into a caller's variable, then update some.
proc SaveContext {name args} {
  upvar 1 $name backup
  if [info exists backup] {
    unset backup
  }
  set ns [namespace current]
  foreach var [info vars ${ns}::*] {
    regsub "${ns}::" $var {} var
    if {[lsearch "name value" $var] != -1} continue ; # Don't backup these!
    variable $var
    set backup($var) [set $var]
  }
  # Update namespace variables in known cases.
  while {"$args" != ""} {
    set arg [PopArg]
    switch -- $arg {
      -file          { variable logFile [PopArg] }
      -noprefix      { variable prefix "" }
      -set           { variable [PopArg] [PopArg]}
      default        {}
    }
  }
}

# Restore namespace variables from the caller's backup variable.
proc RestoreContext {name} {
  upvar 1 $name backup
  foreach {name value} [array get backup] {
    variable $name $value
  }
}

} ; # End if Tcl up to version 8.3.

#--------------------------- End of Proc definitions -------------------------#

# Trace a block of procedures.
# Arguments:
#   filename       Output file name. Optional. Default: /var/log/$script.log
#   procs          The block of procedures to debug.
xvariable nTraceProcs 0
xproc TraceProcs {args} {
  variable nTraceProcs
  set procs [PopLast]       ; # The last arg is the procedure definition list.
  if {"$args" != ""} {        # The optional first arg is the output file.
    set args [linsert $args 0 -file]
  }
  if {$nTraceProcs == 0} { # Avoid tracing twice if we're inside another traced block.
    rename ::proc ::InitialProc
    # Redefine ::proc to run Proc above, in the context of the caller's namespace.
    ::InitialProc ::proc {name args body} \
      "namespace inscope \[uplevel 1 namespace current\] \
      [namespace current]::Proc \$name \$args \$body ::InitialProc $args"
  }
  incr nTraceProcs

  uplevel 1 $procs

  incr nTraceProcs -1
  if {$nTraceProcs == 0} {
    rename ::proc ""
    rename ::InitialProc ::proc
  }
}

# Temporarily skip tracing a block of procedures
xproc SkipTraceProcs {procs} {
  variable nTraceProcs
  if {$nTraceProcs > 0} {
    rename ::proc ::ModifiedProc
    # Redefine ::proc to run Proc above, in the context of the caller's namespace.
    ::InitialProc ::proc {name args body} \
      "namespace inscope \[uplevel 1 namespace current\] \
      ::InitialProc \$name \$args \$body"
  }

  uplevel 1 $procs

  if {$nTraceProcs > 0} {
    rename ::proc ""
    rename ::ModifiedProc ::proc
  }
}

} ; # End of debug namespace

debug::Import

###############################################################################
#                  Show File System infrastructure routines                   #
###############################################################################

TraceProcs {

namespace eval TextFS {

# Enumerate all files in a given directory. Default: The current directory.
xproc EnumFiles {{path ""}} {
  set list {}
  # Append a trailing / to any non empty path.
  if {("$path" != "") && ("[string index $path end]" != "/")} {
    append path "/"
  }
  # List directories first.
  set dirs [glob -types d -nocomplain "$path.*" "$path*"]
  set dirs [lsort -dictionary -unique $dirs]
  foreach dir $dirs {
    set name [file tail $dir]
    if {("$name" == ".") || ("$name" == "..")} continue ; # No need to list them
    set type [file type $dir]
    if {"$type" == "link"} continue ; # Links will be listed thereafter
    lappend list $dir
  }
  # Then list files, links, etc.
  set files [glob -types {f l s b c p} -nocomplain "$path.*" "$path*"]
  set files [lsort -dictionary -unique $files]
  foreach file $files {
    lappend list $file
  }
  return $list
}

#-----------------------------------------------------------------------------#
#                  Routines to generate the structured tree                   #
#-----------------------------------------------------------------------------#

if 0 { # This routine is already part of the debug library
# Indent multiple lines
xproc IndentString {text {indent 2}} {
  set spaces [string repeat " " $indent]
  regsub -all -line {^[^\r\n]} $text $spaces& text ; # Do not indent after the final \n
  return $text
}
}

# Escape a string. ie. change special C string charaters to \c sequences.
# Does the reverse of {subst -nocommands -novariables $text}
xproc Escape {text} {
  regsub -all {\\} $text {\\\\} text ; # Double every pre-existing backslash.
  foreach c {\\" \\a \\b \\f \\n \\r \\t \\v} { # Satisfy the syntax colorizer "
    regsub -all [subst $c] $text $c text ; # Escape C string control characters
  }
  return $text
}

# Quotify a string. ie. escape special C string charaters and put quotes around.
xproc Quotify {text} {
  return "\"[Escape $text]\""
}

# Quotify a string if needed. ie. when spaces, quotes, or a trailing \.
# (Also includes constraints inherited from the SML <-> XML conversion).
xproc CondQuotify {text} {
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

# Blockify a string. ie. add backslashes inside and parenthesis outside.
xproc Blockify {text} {
  # Workaround for the line continuation issue: Double the backslashes there.
  regsub -all {\\\n} $text "\\\\\\\\\n" text
  set text [list $text] ; # Build the list
  regsub -all {\\\\\n} $text "\\\\\n" text ; # Restore line continuation \s.
  if {("[string index $text 0]" != "{") || ("[string index $text end]" != "}")} {
    set text "{$text}"
  }
  return $text
}

# Dump data in hexadecimal, 16 bytes per line, with ASCII chars on the side.
xproc Dump {data} {
  binary scan $data H* hexData
  set l [expr [string length $hexData] / 2]
  set output ""
  for {set i 0} {$i < $l} {incr i 16} {
    append output [format "%04X  " $i]
    for {set j 0} {$j < 16} {incr j} {
      set ix [expr ($i + $j) * 2]
      set ix2 [expr $ix + 1]
      set hh [string range $hexData $ix $ix2]
      if {"$hh" == ""} {
	set hh "  "
      }
      append output $hh
      if {[expr $j % 4] == 3} {
	append output " "
      }
    }
    append output " "
    for {set j 0} {$j < 16} {incr j} {
      set ix [expr ($i + $j) * 2]
      set ix2 [expr $ix + 1]
      set hh [string range $hexData $ix $ix2]
      if {"$hh" == ""} {
	set hh "20"
      }
      set n 0x$hh
      if {($n < 0x20) || ($n >= 0x7F)} {
	set c "."
      } else {
	set c [format %c $n]
      }
      append output $c
      if {[expr $j % 8] == 7} {
	append output " "
      }
    }
    append output "\n"
  }
  return $output
}

proc attrib {name value} {
  # Escape illegal characters using the corresponding entities
  regsub -all "&"  $value "&amp;" value
  regsub -all "\"" $value "&quot;" value
  regsub -all "<"  $value "&lt;" value
  return "$name=\"$value\""
}

# Get lines to display from an open file. Returns {type data length eof}
proc GetFileData {hFile optionsV} {
  upvar 1 $optionsV options
  set rxBin {[^\r\n\t\x20-\x7E\x80-\xFF]} ; # Regular expression identifying binary data.
  fconfigure $hFile -translation binary
  set type text
  set data {}
  set eof 0
  set nc 0
  if {$options(checksum)} {
    set err [catch {exec cksum <@$hFile} output]
    if {!$err} {
      foreach {data nc} $output break
      set eof 1
    }
  } else {
    for {set nc 0 ; set nl 0} {![set eof [eof $hFile]]} {incr nc} {
      if {"$type" == "hexdump"} {
	set nl [expr $nc / 16] ; # We'll dump 16 bytes per line.
      }
      if {($options(maxData) > 0) && ($nc >= $options(maxData))} break
      if {($options(maxLines) > 0) && ($nl >= $options(maxLines))} break
      set err [catch {set c [read $hFile 1]} output]
      if {$err || ("$c" == "")} { # A non-readable file, or premature end of file
	set eof 1
	break
      }
      append data $c
      if {("$type" == "text") && [regexp $rxBin $c -]} {
	set type hexdump
      }
      if {("$type" == "text") && ("$c" == "\n")} {
	incr nl
      }
    }
    if {("$type" == "hexdump") && ($options(maxLines) > 0)} {
      set maxNC [expr $options(maxLines) * 16]
      if {$nc > $maxNC} {
	set eof 0
	set nc $maxNC
	set data [string range $data 0 [expr $nc - 1]]
	catch {seek $hFile $nc} ; # Try leaving the pointer where we now stop.
      }
    }
    if {"$type" == "hexdump"} {
      set data [Dump $data]
      if {"$::lf" != "\n"} {
      	regsub -all {\n} $data "$::lf" data
      }
    }
  }
  return [list $type $data $nc $eof]
}

# Output a file element.
xproc ShowFile {path {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
  set type file
  set attribs {}
  set size 0
  if {"$path" != "-"} {
    catch {set size [file size $path]}
    set err [catch {set hFile [open $path]} output]
    if $err {
      foreach msg [list "permission denied" "no such file or directory" "no such device"] {
	if [regexp $msg $output -] {
	  ShowElement file $path "error=[Quotify $msg]" "" $indent options
	  return
	}
      }
      error $output ; # Unknown error. Treat as a real error.
    }
    set time [file mtime $path]
  } else {
    set type fifo
    set hFile stdin
    set time [clock seconds]
  }
  set time [clock format $time -format "%Y-%m-%d %H:%M:%S"]
  if {("$options(format)" == "own") && [Verbose]} {
    lappend attribs [attrib type $type]
  }
  foreach {format data len eof} [GetFileData $hFile options] break
  set err [catch { # Check the actual size for character devices or piped files.
    set xData [read $hFile 9999] ; # A reasonable look ahead
    set size2 0
    catch {set size2 [tell $hFile]}
    if {$size2 <= 0} { # Some devices do not support ftell
      set size2 [expr $len + [string bytelength $xData]]
    }
    if {$size2 > 9999} { # Limit to a recognizable value
      set size2 9999
    }
    if {$size2 > $size} {
      set size $size2
    }
  } output]
  if {$err && [Debug]} {
    puts stderr "Error: $output"
  }
  close $hFile
  if {($len < $size) && $eof} { # Some devices report dummy sizes.
    set size $len             ; # Then use the actual size.
  }
  if {("$format" != "text") || [Verbose]} {
    lappend attribs [attrib format $format]
  }
  if {[Verbose]} {
    lappend attribs [attrib time $time]
  }
  if {((!$eof) && ("$options(format)" != "own")) || [Verbose]} {
    lappend attribs [attrib size $size]
    # lappend attribs [attrib shown $len] ;# We don't need this anymore
  }
  set epilog ""
  if {"$options(format)" == "own"} {
    if {!$eof} {
      set epilog "..."
    }
  }
  ShowElement $type $path $attribs $data $indent options $epilog
}

xproc ReadLink {path} {
  set target ""
  set err [catch {
    set target [file readlink $path]
  } errMsg]
  if {$err && ($::tcl_platform(platform) == "windows")} {
    # Tcl <= 8.5.14 (at least) can read Windows junctions, but fails to read Windows' symlinks.
    # Try using cmd.exe to read the link target instead.
    set err2 [catch {
      set output [exec $::env(COMSPEC) /c dir /al [file nativename [file dirname $path]]]
    } errMsg2]
    if {!$err2} {
      set link [file tail $path]
      foreach line [split $output "\n"] {
      	if [regsub {^.*>\s+} $line {} line] {
      	  set toSearch "$link \["
	  set len [string length $toSearch]
      	  if {[string compare -nocase -length $len $toSearch $line] == 0} {
      	    set target [string range $line $len end]
      	    set target [string trimright $target]
      	    set target [string trimright $target "\]"]
      	    regsub -all {\\} $target {/} target
      	    set err 0
      	    break
      	  }
      	}
      }
    }
  }
  if $err {
    error $errMsg
  }
  return $target
}

# Output a link element.
xproc ShowLink {path {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
  set type link
  set attribs {}
  if {"$options(format)" == "own"} {
    if [Verbose] {
      lappend attribs [attrib type link]
    } elseif {![Quiet]} {
      lappend attribs "->"
    }
  }
  # Tcl <= 8.5 can read Windows junctions, but fails to read Windows symlinks.
  set err [catch {
    set data [ReadLink $path] ;# file readlink does not support Windows' symlinks
  } errMsg]
  if {$err} {
    regexp {[^""]*"[^""]*":\s+(.*)} $errMsg - errMsg ; # Remove the {could not read link "pathname": } prefix to the error.
    lappend attribs [attrib error $errMsg]
    set type error	;# Prevent entering the while loop below
    set data ""		;# We cannot say anything about the link target.
  }
  set i 0
  set path2 $path
  while {"$type" == "link"} { # Follow links until the end to get the end type.
    if {[incr i] > 100} break ; # Avoid infinite loops.
    set err [catch {set target [ReadLink $path2]} output]
    if $err break ; # Broken link chain. Give up silently.
    set path2 [file join [file dirname $path2] $target]
    set err [catch {set type [file type $path2]} output]
    if $err break ; # Broken link chain. Give up silently.
  }
  if {"$type" == "directory"} {
    set data "[eval file join [file split $data]]/"
  }
  ShowElement link $path $attribs $data $indent options
}

# Output a socket element.
xproc ShowSocket {path {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
# Commented out as it makes the output less readable.
#  if {"$options(format)" == "own"} {
#    append path "=" ; # This is the standard suffix for ls -Fla
#  }
  set attribs {}
  if {"$options(format)" == "own"} {
    if [Verbose] {
      lappend attribs [attrib type socket]
    } elseif {![Quiet]} {
      lappend attribs {[s]}
    }
  }
  ShowElement socket $path $attribs {} $indent options
}

# Output a fifo element.
xproc ShowFifo {path {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
# Commented out as it makes the output less readable.
#  if {"$options(format)" == "own"} {
#    append path "|" ; # This is the standard suffix for ls -Fla
#  }
  set attribs {}
  if {"$options(format)" == "own"} {
    if [Verbose] {
      lappend attribs [attrib type fifo]
    } elseif {![Quiet]} {
      lappend attribs {[f]}
    }
  }
  ShowElement fifo $path $attribs {} $indent options
}

# Output any other type of element.
xproc ShowOther {path {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
  set type [file type $path]
  set attribs {}
  set type [string map {
    characterSpecial chardev
    blockSpecial blockdev
  } $type]
  if {"$options(format)" == "own"} {
    if [Verbose] {
      lappend attribs [attrib type $type]
    } elseif {![Quiet]} {
      switch -- $type {
	chardev {lappend attribs {[c]}}
	blockdev {lappend attribs {[b]}}
	default {lappend attribs [attrib type $type]}
      }
    }
  }
  ShowElement $type $path $attribs {} $indent options
}

# Output an unidentified element.
xproc ShowThis {path {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
  set type [file type $path]
  switch "$type" {
    "directory" {
      ShowDir $path $indent options
    }
    "file" {
      ShowFile $path $indent options
    }
    "link" {
      ShowLink $path $indent options
    }
    "socket" {
      ShowSocket $path $indent options
    }
    "fifo" {
      ShowFifo $path $indent options
    }
    default { # devices; Anything else I've missed.
      ShowOther $path $indent options
    }
  }
}

# Output a fully formatted element.
xproc ShowElement {type name attribs body indent optionsV {epilog ""}} {
  upvar 1 $optionsV options
  # Begin the header with the file name.
  if {$indent != 0} {
    set name [file tail $name]
  }
  if {"$options(format)" == "own"} {
    set header [CondQuotify $name]
  } else {
    set header "$type [attrib name $name]"
  }
  # Append the other attributes, if any
  if {"$attribs" != ""} {
    append header " [join $attribs]"
  }
  # Encode the body depending on the encoding format.
  set finalLF $::lf
  switch $options(format) {
    "own" {
      set endcrlf 0
      set crlf ""
      regexp {\r?\n?$} $body crlf
      if {"$epilog" != "..."} { # If the file is complete
	if {"$crlf" != ""} {      # and if it does end with a CRLF
	  set endcrlf 1 ;           # Remember it
	} else {                  # else it has none and one will be shown
	  # if [regexp {shown="?(\d+)"?} $header - shown] {
	  #   regsub {shown="?\d+"?} $header [attrib shown [incr shown]] header
	  # }
	}
      }
      if {"$type" == "comment"} {
      	set header "#"
      	# The body is the comment line following the # character.
      } elseif {(("$type" == "directory") || ("$type" == "dir")) && ("$body" == "")} {
        set body "{}"
      } elseif {   ("$type" != "directory") && ("$type" != "dir")
                && [regexp {^([^\r\n]*)(\r?\n?)$} $body - line crlf]} { # Single line
	if {[Quiet]} {
	  set body [CondQuotify $line] ; # In normal mode, ignore the end crlf.
	} else {
	  switch $options(own) {
	    1 { # Alternative 1: {"line\r\n"} if line ends with a new line.
	      set body [CondQuotify $body] ; # Else encode the full line.
	    }
	    2 { # Alternative 2: {+ "line"} if line ends with a new line.
	      if {"$crlf" != ""} {
		append header " +"
	      }
	      set body [CondQuotify $line] ; # Else encode the trimmed line.
	    }
	    3 { # Alternative 3: {= "line"} if line does not end with a new line.
	      if {"$crlf" == ""} {
		append header " ="
	      }
	      set body [CondQuotify $line] ; # Else encode the trimmed line.
	    }
	    0 - 4 { # Alternative 4: Use size to know if the \r\n is used.
	      set body [CondQuotify $line]
	    }
	    default {
	      error "No such -o $options(own) experimental variation"
	    }
	  }
	}
      } else { # Multiple lines
	if {"$crlf" == ""} {
	  append body "$::lf"
	}
	set body "{$::lf[IndentString $body 2]}"
	regsub "\\n  \}$" $body "\n\}" body ; # Do not indent the closing line.
	if {![Quiet]} {
	  switch $options(own) {
	    1 { # Alternative 1: {{text} \r\n} if text ends with a new line.
	      if {$endcrlf} {
		regsub "\r" $crlf {\r} crlf
		regsub "\n" $crlf {\n} crlf
		set epilog $crlf
	      }
	    }
	    2 { # Alternative 2: {{text}-} if text does not end with a new line.
	      if (!$endcrlf) {
		append body "-"
	      }
	    }
	    3 { # Alternative 3: {= {text}} if text does not end with a new line.
	      if (!$endcrlf) {
		append header " ="
	      }
	    }
	    0 - 4 { # Alternative 4: Use size & show to know if the \r\n is used.
	    }
	    default {
	      error "No such -o $options(own) experimental variation"
	    }
	  }
	regsub -all {\r\n} $body "$::lf" body ; # Avoid problems in Windows.
	}
      }
      if {"$epilog" != ""} {
	append body " $epilog"
      }
      $options(showCmd) $options(showRef) "[IndentString "$header $body" $indent]$finalLF"
    }
    "sml" {
      set eol ""
      if [regexp {^([^\r\n]*)(\r|\n|\r\n)?$} $body - body eol] {
	# "single line" <==> data fits on a single line ending with an \n.
	set body "[CondQuotify $body]$eol"
	if {"$eol" == "$::lf"} {
	  set finalLF "" ; # Don't output a second one if this one is already the right one
	}
	$options(showCmd) $options(showRef) "[IndentString "$header $body" $indent]$finalLF"
      } else {
	regexp {(.*?)(\r\n|\r|\n)?$} $body - body eol
	switch $options(sml) {
	  0 { # Create a CDATA section containing an \n then the exact content of the text file.
	    # But check for the "]]>" sequence, which is forbidden in CDATA sections:
	    regsub -all "]]>" $body "]]]]><\[\[>" body ;# Split these forbidden sequences between two CDATA sections.
	    set body "{<\[\[$body$eol"
	    if {"$eol" != "$::lf"} {
	      append body $::lf
	    }
	    append body "]]>}"
	    $options(showCmd) $options(showRef) "[string repeat " " $indent]$header $body$finalLF"
	  }
	  1 { # ={\n  data ... } <==> indented CDATA content block beginning after the \n.
	    set body "={$::lf[IndentString $body 2]$eol"
	    if {"$eol" != "$::lf"} {
	      append body $::lf
	    }
	    append body "}"
	    $options(showCmd) $options(showRef) "[IndentString "$header $body" $indent]$finalLF"
	  }
	  2 { # {{\n  data ... }} <==> indented CDATA beginning after the \n.
	    set body "{{$::lf[IndentString $body 2]$eol"
	    if {"$eol" != "$::lf"} {
	      append body $::lf
	    }
	    append body "}}"
	    $options(showCmd) $options(showRef) "[IndentString "$header $body" $indent]$finalLF"
	  }
	  default {
	    error "No such -S $options(sml) experimental variation"
	  }
	}
      }
    }
    default {
      error "Format $options(format) not supported yet"
    }
  }
}

# Output a directory element, including all files and subdirectories.
xproc ShowDir {{path .} {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
  if $indent {
    set name [file tail $path]
  } else {
    set name $path
  }
  if {$options(classify)} { # Append a trailing /
    if {"[string index $name end]" != "/"} {
      append name "/"
    }
  } else { # Else remove it
    regsub {/$} $name {} name
  }
  # Begin the header with the file name.
  if {"$options(format)" == "own"} {
    if [Verbose] { # Remove the final / to make it more XML compatible
      regsub {/$} $name {} name
    }
    set header [CondQuotify $name]
    if [Verbose] { # And instead flag the type in an attribute
      append header " [attrib type dir]"
    }
  } else {
    set header "directory [attrib name $name]"
  }
  if {($options(maxDepth) <= 0) || (($indent/2) < $options(maxDepth))} {
    set err [catch {
      set contents [glob -nocomplain "$path/.*" "$path/*"]
    } errMsg]
    if {$err} {
      regexp {[^""]*"[^""]*":\s+(.*)} $errMsg - errMsg ; # Remove the {couldn't read directory "pathname": } prefix to the error.
      set header "$header error=[Quotify $errMsg]"
      $options(showCmd) $options(showRef) [IndentString "$header \{...\}$::lf" $indent]
    } else {
      $options(showCmd) $options(showRef) [IndentString "$header \{" $indent]
      if {[llength $contents] > 2} {
	$options(showCmd) $options(showRef) "$::lf"
	ShowDirContents $path [expr $indent + 2] options
	$options(showCmd) $options(showRef) "[IndentString "\}" $indent]$::lf"
      } else {
	$options(showCmd) $options(showRef) "\}$::lf"
      }
    }
  } else { # We've reached the maximum depth allowed. Don't look further.
    $options(showCmd) $options(showRef) [IndentString "$header \{...\}$::lf" $indent]
  }
}

# Output elements for all files and subdirectories.
xproc ShowDirContents {{path .} {indent 0} {optionsV ""}} {
  upvar 1 $optionsV options
  # List directories first.
  set dirs [concat [glob -types d -nocomplain "$path/*"] \
		   [glob -types {d hidden} -nocomplain "$path/*"]]
  set dirs [lsort -dictionary $dirs]
  foreach dir $dirs {
    set name [file tail $dir]
    if {("$name" == ".") || ("$name" == "..")} continue ; # No need to list them
    set type [file type $dir]
    if {"$type" == "link"} continue ; # Links will be listed thereafter
    ShowDir $dir $indent options
  }
  # Then list files, links, etc.
  set files [concat [glob -types {f l s b c p} -nocomplain "$path/*"] \
		    [glob -types {f l s b c p hidden} -nocomplain "$path/*"]]
  set files [lsort -dictionary $files]
  foreach file $files {
    ShowThis $file $indent options
  }
}

#-----------------------------------------------------------------------------#
#               Routines to extract data from a structured tree               #
#-----------------------------------------------------------------------------#

# Note: I tried using "regexp -start $index" to avoid having to split and copy
#       the text repeatedly, but this fails to work as "^" does not anchor at
#       the specified index in this case.
#       Then I tried using regexp "^.{$index}" to skip the beginning of the
#       string, but this fails as well as there's a regexp compilation error if
#       $index is greater than 255.
#       So I'm back to square one, and have to split the string repeatedly.

proc ReadToken {text index countV} {
  upvar 1 $countV count
  # Skip spaces, if any
  set text2 [string range $text $index end]
  if [regexp {^(\s+)(.*)} $text2 - spaces text2] {
    incr index [string length $spaces]
  }
  # Read the token
  set c [string index $text2 0]
  switch -- $c {
    "\"" {
      return [ReadQuotedString $text $index count]
    }
    "\{" {
      return [ReadIndentedBlock $text $index count]
    }
    default {
      return [ReadWord $text $index count]
    }
  }
}

# Unquotify a string. ie. remove quotes outside and backslashes inside.
proc UnQuotify {string} {
  regexp "^\"(.*)\"$" $string - string ; # Remove outer quotes, if any
  set string [subst -nocommands -novariables $string] ; # Convert \c sequences
  return $string
}

# Extract one word at the specified index
proc ReadWord {text index countV} {
  upvar 1 $countV count
  set text [string range $text $index end]
  set word ""
  if [regexp {^\S+} $text word] {
    incr index [string length $word]
    set count $index
    set word [UnQuotify $word]
  }
  return $word
}

# Extract one quoted string at the specified index
proc ReadQuotedString {text index countV} {
  upvar 1 $countV count
  set text [string range $text $index end]
  set word ""
  if [regexp {^"([^\\""]|\\.)*"} $text word -] {
    incr index [string length $word]
    set count $index
    set word [UnQuotify $word]
  }
  return $word
}

# Compute the indentation of a line
proc GetIndent {line} {
  set indent 0
  set l [string length $line]
  for {set i 0} {$i < $l} {incr i} {
    set c [string index $line $i]
    switch -- $c {
      " " {
	incr indent
      }
      "\t" {
	incr indent $::options(tabSize)
	set indent [expr $indent - ($indent % $::options(tabSize))]
      }
      default {
	break
      }
    }
  }
  return $indent
}

# Extract one indented block at the specified index, removing the indentation.
proc ReadIndentedBlock {text index countV} {
  upvar 1 $countV count
  # First find the indentation of the parent line
  set line ""
  for {set i [expr $index - 1]} {$i >= 0} {incr i -1} {
    set c [string index $text $i]
    if {"$c" == "\n"} break
    set line "$c$line"
  }
  set indent [GetIndent $line]
  # DebugSVars "Initial" text index indent
  # Now on, discard the beginning.
  # Note that regexp -start $index does not allow checking what's immediately there.
  set text [string range $text $index end]
  # Gotcha - Catch one-line unindented blocks
  if [regexp {^[ \t]*{([^\r\n]*)}} $text match block] {
    incr index [string length $match]
    set count $index
    DebugSVars "Return 1" index block
    return $block
  }
  # Now get all following lines with a larger indentation.
  set block ""
  set line ""
  set rx ""
  append rx {^([^\n]*} "\{" {[^\n]*\n?)(.*)}
  regexp $rx $text - line text ; # Skip the EOL
  incr index [string length $line] ; # Skip the EOL
  incr indent 2 ; # The indented text is 2 characters right of the parent block.
  # DebugSVars "While entry" line index indent
  while {"$text" != ""} {
    set line ""
    regexp {^([^\n]*\n?)(.*)} $text - line text
    set n [string length $line]
    # DebugSVars "Loop" index line n
    if {$n == 0} break ; # End of text (Should not happen)
    incr index $n
    set i [GetIndent $line]
    if {$i < $indent} { # We've reached the marker for end of the block.
      if [regexp {^\s*\}-} $line -] { # If that marker is \}- 
	regsub {\r?\n?$} $block {} block ; # Then remove the trailing crlf.
      }
      break
    }
    set tail ""
    regexp {\s*(.*)} $line - tail
    append block [format %*s [expr $i - $indent] ""] ; # Possible head spaces
    append block $tail
  }
  set count $index
  # DebugSVars "Return 2" index block
  return $block
}

xproc WalkTree {text cb args} {
  error "Not implemented yet"
}

# Process every file and subdirectory in the base directory.
xproc WalkFiles {text cb args} {
  set index 0
  set nComments 0
  while 1 {
    set name [ReadToken $text $index index]
    if {"$name" == ""} break
    set attributes {}
    if {"$name" == "#"} { # An SML comment, ending at the end of line.
      set type comment
      append name [incr nComments]
      regexp -line {^.*} [string range $text [incr index] end] value
      incr index [string length $value]
    } else {
      set value [ReadToken $text $index index]
      set type file
      set c [string index $name end]
      switch -- $c {
	"/" {
	  set name [string trimright $name /]
	  set type dir
	}
	"|" {
	  set name [string trimright $name |]
	  set type fifo
	  set value [ReadToken $text $index index]
	}
	"=" {
	  set name [string trimright $name =]
	  set type socket
	  set value [ReadToken $text $index index]
	}
	default {
	  set type l ; # Prepare case ("$value" == "->") below.
	  if {   (("$value" == "->") || [regexp {^\[(.)\]$} $value - type])
	    && [regexp {^([^\n]+)} [string range $text $index end] - line]} {
	    switch -- $type {
	      "b" {set type "blockdev"}
	      "c" {set type "chardev"}
	      "f" {set type "fifo"}
	      "l" {set type "link"}
	      "s" {set type "socket"}
	      default {}
	    }
	  }
	}
      }
      # Process all optional attributes on the line.
      set lastattr ""
      set nAttr 0
      while {[regexp {^[ \t]+\S} [string range $text $index end] -]} {
	set attr $value
	if {![regexp {^(\w+=(".*"|\w+)|\[\w\]|\+|->?|=)$} $attr - -]} break ; # Not an attribute
	lappend attributes $attr
	incr nAttr
	# TO DO: Process the attributes!
	set lastattr $attr
	set value [ReadToken $text $index index]
      }
      # Make the final EOL corrections
      switch {$::options(own)} {
	0 - 1 { # Alternative 1: {"line\r\n"} or {{text} \r\n}
	  if {[regexp {^[ \t]+\S} [string range $text $index end] -]} {
	    set crlf [ReadToken $text $index index]
	    if [regexp {^\\r?\\n?$} $crlf -] { # Don't do for the ... epilog!
	      regsub {\\r} $crlf "\r" crlf
	      regsub {\\n} $crlf "\n" crlf
	      append value $crlf
	    }
	  }
	}
	2 { # Alternative 2: {+ "line"} or {{text}-}
	  if {"$lastattr" = "+"} {
	    append value $::lf
	  }
	  if {[string index $text $index] == "-"} {
	    regsub {$::lf$} $value {} value
	  }
	}
	3 { # Alternative 3: {= "line"} or {= {text}}
	  if {"$lastattr" = "="} {
	    set c [string index $value end]
	    if {("$c" != "") && ("$c" != "\n")} {
	      regsub {\n$} $value {} value
	    }
	  }
	}
      }
    }
    # Call the callback
    set ret [eval $cb [list $name] $type [list $attributes] [list $value] $args]
    if {$ret} { # And return immediately if the callback tells us to.
      return $ret
    }
  }
  return 0
}

proc ShiftLeft {text} {
  # Remove possible tabulations inserted by text editors
  regsub -line -all {^ ? ? ? ? ? ? ?\t} $text "        " text
  # Shift left by 2 characters
  regsub -line -all {^  } $text "" text ; # Shift all lines left by 2 characters.
  return $text
}

proc ReadFileCB {name type attributes value name0 level typeV valueV} {
  if {"$name" != "$name0"} {
    return 0
  }
  uplevel #$level set $typeV $type
  uplevel #$level set $valueV [list $value]
  return 1
}

xproc ReadFile {text path} {
  set nodes [file split $path]
  if {[lindex $nodes 0] == "/"} {
    set nodes [lrange $nodes 1 end]
  }
  set node0 [lindex $nodes 0]
  set rest [join [lrange $nodes 1 end] /]

  set found [WalkFiles $text ReadFileCB $node0 [info level] type value]
  if {$found} {
    if {("$type" == "dir") && ("$rest" != "")} {
      set value [ReadFile $value $rest]
    } elseif {("$type" == "file") && ("$rest" != "")} {
      error "$path not found"
    }
    return $value
  }
  error "$path not found"
}

# Simplified version, that only works if file contents are valid Tcl strings.
# (That is no unbalanced { } etc)
xproc ReadFile001 {text path} {
  set nodes [file split $path]
  if {[lindex $nodes 0] == "/"} {
    set nodes [lrange $nodes 1 end]
  }
  set node0 [lindex $nodes 0]
  set rest [join [lrange $nodes 1 end] /]
  foreach {name value} $text {
    set type file
    if {"[string index $name end]" == "/"} {
      set name [string trimright $name /]
      set type dir
    }
    if {"$name" == "$node0"} {
      if {("$type" == "dir") && ("$rest" != "")} {
	set value [ReadFile $value $rest]
	if [regexp {\n} $value -] {
	  set value [ShiftLeft $value]
	}
      } elseif {("$type" == "file") && ("$rest" != "")} {
	error "$path not found"
      } elseif {("$type" == "file")} {
	if [regexp {\n} $value -] {
	  set value [ShiftLeft $value]
	  set value [string trim $value \n]
	}
      }
      return $value
    }
  }
  error "$path not found"
}

proc ListDirCB {name type attributes value types level listV} {
  if {("$types" == "*") || ([lsearch $types $type] != -1)} {
    switch $type {
      dir {append name "/"}
      fifo {append name "|"}
      socket {append name "="}
      default {}
    }
    uplevel #$level lappend $listV [list $name]
  }
  return 0
}

xproc ListDir {text {path /} {types *}} {
  if {("$path" != "") && ("$path" != "/")} {
    set text [ReadFile $text $path]
  }
  set files {}
  WalkFiles $text ListDirCB $types [info level] files
  return $files
}

# Split a text tree into individual subtrees. Returns the list of names.
proc SplitTreeCB {name type attributes value level listV dataV} {
  switch $type {
    dir {append name "/"}
    fifo {append name "|"}
    socket {append name "="}
    default {}
  }
  uplevel #$level lappend $listV [list $name]
  uplevel #$level set ${dataV}(type,$name) [list $type]
  uplevel #$level set ${dataV}(attrs,$name) [list $attributes]
  uplevel #$level set ${dataV}(value,$name) [list $value]
  return 0
}
xproc SplitTree {text upvar} {
  upvar 1 $upvar subtrees
  set files {}
  WalkFiles $text SplitTreeCB [info level] files subtrees
  return $files
}

# Compare file names, with optional comparison criteria.
# Criteria are coded by characters:
#   d   Dictionary sort (Case insensitive, numbers in sequence)
#   f   Group files first
#   g   Group directories first (default)
#   G   Don't group files and directories
#   i   Ignore case
#   I   Don't
variable revType ; # Array used to reverse the by-type grouping.
set revType(~) /
set revType(|) =
set revType(=) |
set revType(/) ~
proc CompareFileNames {opts name1 name2} {
  # Analyse options
  set cmd "string compare" ; # Comparison command
  set group "df"           ; # Grouping
  foreach c [split $opts ""] {
    switch -- $c {
      "d" {
      	set cmd "StrCmpDict"
      }
      "f" {
      	set group "fd" ; # Files then directories
      }
      "g" {
      	set group "df" ; # Directories then files
      }
      "G" {
      	set group ""   ; # No grouping
      }
      "i" {
	set cmd "string compare -nocase"
      }
      "I" {
	set cmd "string compare"
      }
      default {
      }
    }
  }
  if {"$group" != ""} {
    set rx {(.*)([/|=])}
    set type1 "~"
    regexp $rx $name1 - name1 type1 ; # Extract the special charater suffix
    set type2 "~"
    regexp $rx $name2 - name2 type2 ; # Idem
    if {"$group" == "fd"} {
      variable revType
      set type1 $revType($type1)
      set type2 $revType($type2)
    }
    set name1 "$type1$name1" ; # Prepend the suffix, hence the grouping.
    set name2 "$type2$name2" ; # Idem
  }
  lappend cmd $name1 $name2
  eval $cmd
}

# Sort file names, with optional sort criteria.
xproc SortFiles {files {opts ""}} {
  lsort -command [list [namespace current]::CompareFileNames $opts] $files
}

# Append values to a variable up in the call chain
proc AppendUpvar {upvar args} { # upvar = {level varName}
  foreach {level varName} $upvar break
  upvar #$level $varName var
  eval append var $args
}

# Sort a whole file tree, with optional sort criteria.
xproc SortTree {text {opts ""}} {
  set new "" ; # The sorted tree returned.
  array set options [array get ::options] ; # Make a local copy of the global options
  set options(showCmd) [namespace current]::AppendUpvar ; # Command to run to process the output.
  set options(showRef) [list [info level] new] ; # Reference argument.
  set files [SplitTree $text info]
  set files [eval SortFiles [list $files] $opts]
  foreach f $files {
    set type $info(type,$f)
    set value $info(value,$f)
    if {"$type" == "dir"} {
      set value [SortTree $value $opts]
    }
    ShowElement $type $f $info(attrs,$f) $value 0 options
  }
  return $new
}

} ; # End of namespace eval

} ; # End of TraceProcs

###############################################################################
#               End of Show File System infrastructure routines               #
###############################################################################

TextFS::Import

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
#    2007-05-14 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
Show a directory tree and file contents as structured text.

Usage: $script [OPTIONS] [PATHNAMES]

Options:
  --                End of options
  -h, --help, -?    Display this help screen.
  -b, --bytes, -c N Maximum file size converted. 0=No max. Default: $options(maxData)
  -C, --checksum    Output a file checksum instead of the file contents.
  -d, --debug       Increase the verbosity level to debug level
  -F, --classify    Append a class suffix to the file name. /=dir ->=link
  -i, --in PATH     Show files inside the given path, not the path itself.
  -l, --lines, -n N Maximum # of lines per file. 0=No max. Default: $options(maxLines)
  -L, --list [PATH] Read a tree from stdin and list files [in the given subdir]
  -m, --maxdepth N  Maximum subdirectory depth. 0=No max. Default: $options(maxDepth)
  -o, --own [N]     Use our own output format (default), or alternative #N.
  -p, --pwd         Show the current directory. Default: Everything inside
  -q, --quiet       Minimalist output. Remove classification suffixes.
  -r, --read PATH   Read a tree from stdin and extract a file or sub-tree
  -s, --sort [SOPS] Sort a tree from stdin. See Sort Options below for details
  -S, --sml [N]	    Use a fully SML-compliant format (or experimental alternative #N)
  -t, --tab N       Tabulation size on input. Default: $options(tabSize) spaces
  -v, --verbose     Increase the verbosity level
  -V, --version     Display the script version.
  -N                Same as -n N

Pathnames:
  A list of file and/or directory pathnames.
  Default: Everything inside the current directory.
  Use - to display (either as text or HEX dump) what comes in from stdin.

Sort Options: A string with one or more of the following characters:
  d     Dictionary sort (Case insensitive, numbers sequencially)
  f     Group files first
  g     Group directories first (Default)
  G     No grouping
  i     Case insensitive
  I     Case sensitive (Default)
}]

# Scan all arguments.
set args $argv
set noOptions 0
while {"$args" != ""} {
  set arg [PopArg]
  if {[string match -* $arg] && !$noOptions} {
    switch -- $arg {
      "-" {
	append paths -
      }
      "--" {
	set noOptions 1
      }
      "-b" - "--bytes" - "-c" { # Maximum data size. (-c|--bytes are head options)
	set options(maxData) [PopArg]
	set options(maxLines) 0
      }
      "-C" - "--checksum" { # Note: Option -c is used as an alias for -b above.
	set options(checksum) 1
      }
      "-d" - "--debug" { # debug flag.
	incr ::debug::verbosity 2
      }
      "-F" - "--classify" { # Append a classification symbol (/ -> | =) to the name
      	set options(classify) 1
      }
      "-h" - "--help" - "-?" - "/?" {
	puts $usage
	exit 0
      }
      "-i" - "--in" {
      	cd [PopArg]
      }
      "-l" - "--lines" - "-n" { # Maximum number of lines. (-n|--lines are head o)
	set options(maxData) 0
	set options(maxLines) [PopArg]
      }
      "-L" - "--ls" - "--dir" {
	set err [catch {
	  set nFiles 0
	  foreach file [ListDir [read stdin] [PopArg]] {
	    if {$nFiles > 0} {puts -nonewline " "}
	    puts -nonewline [CondQuotify $file]
	    incr nFiles
	  }
	  puts ""
	} msg]
	if $err {
	  puts stderr "$msg"
	}
	exit $err
      }
      "-m" - "--maxdepth" { # Maximum depth
	set options(maxDepth) [PopArg]
      }
      "-o" - "--own" { # Use our own simplfied sml output format.
	set options(format) own
	set options(own) 0		; # 0 = Most simple output
	set nextArg [PeekArg]
      	if {(![IsSwitch $nextArg]) && ([string is integer -strict $nextArg])} { # Or experimental variation #N of the same
      	  set options(own) [PopArg]
      	}
      }
      "-p" - "--pwd" { # Display the current directory
	lappend paths [pwd]
      }
      "-q" - "--quiet" { # Quiet flag
        incr ::debug::verbosity -1
      	set options(classify) 0
      }
      "-r" - "--read" { # Read a file embedded in a structured tree.
	set err [catch {puts -nonewline [ReadFile [read stdin] [PopArg]]} msg]
	if $err {
	  puts stderr "$msg"
	}
	exit $err
      }
      "-s" - "--sort" { # Sort a tree
      	set sopts ""
      	if ![IsSwitch [PeekArg]] {
      	  set sopts [PopArg]
      	}
	set err [catch {puts -nonewline [SortTree [read stdin] $sopts]} msg]
	if $err {
	  puts stderr "$msg"
	}
	exit $err
      }
      "-S" - "--sml" { # Use a fully SML-compliant output format.
	set options(format) sml
	set options(sml) 0		; # 0 = Standard SML
	set options(classify) 0		; # Do not append / suffix to directories
	# Can them be converted to XML by piping the output to the sml script.
	set nextArg [PeekArg]
      	if {(![IsSwitch $nextArg]) && ([string is integer -strict $nextArg])} { # Or use experimental variation #N of SML
      	  set options(sml) [PopArg]
      	}
      }
      "-t" - "--tab" { # Tabulation size
	set options(tabSize) [PopArg]
      }
      "-T" - "--test" { # Test an alternative output method.
	set options(showCmd) append   ; # Command to run to process the output.
	set options(showRef) ::output ; # Reference argument.
	foreach path $paths {
	  set err [catch {ShowThis $path 0 options} output]
	}
	puts -nonewline $::output
	exit $err
      }
      "-v" - "--verbose" { # Verbose flag.
	incr ::debug::verbosity
      }
      "-V" - "--version" { # Version.
	puts $version
	exit 0
      }
      "-x" - "--xml" { # Use an xml-compliant output format (Not implemented yet)
	# Meanwhile, pipe the output of the --sml option to the sml script.
	set options(format) xml
      }
      default {
	if [regexp {^-(\d+)$} $arg - options(maxLines)] {
	  set options(maxData) 0
	} else {
	  puts stderr "Unrecognized switch $arg. Ignored."
	}
      }
    }
  } else {
    set path [eval file join [file split $arg]] ; # Remove tail / if any.
    if ![regexp {\*} $path -] {
      lappend paths $path
    } else {
      set paths [concat $paths [EnumFiles $path]]
    }
  }
}

set xlat [fconfigure stdout -translation]
switch $xlat {
  "cr" {
    set lf "\x0D"
  }
  "crlf" {
    set lf "\x0D\x0A"
  }
  "lf" - default {
    set lf "\x0A"
  }
}
fconfigure stdout -translation lf

# Action
if {"$paths" == ""} { # If no file specified, then
  set paths [EnumFiles] ; # process all files in the current directory.
}

foreach path $paths {
  if {"$path" != "-"} {
    set err [catch {ShowThis $path 0 options} output]
  } else {
    set err [catch {ShowFile - 0 options} output]
  }
  if $err {
    puts stderr "$output"
  }
}
fconfigure stdout -translation $xlat
exit $err

