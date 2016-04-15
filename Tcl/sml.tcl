#! /usr/bin/tclsh
###############################################################################
#                                                                             #
#   File name	    sml                                                       #
#                                                                             #
#   Description     Convert XML <-> SML, a Simple Markup Language.            #
#                                                                             #
#   Notes 	    XML is the standard for representing structured data.     #
#		    But, XML has the drawback of being extremely verbose.     #
#		    Contrary to what XML designers hoped, it is hard for      #
#		    humans to manually edit anything but trivial XML files.   #
#		    The goal of this program is to experiment with an	      #
#		    alternative representation for XML data, that is more     #
#		    human-friendly (at least for C family programmers).       #
#		    The goal is not to create an alternative to XML. SML is   #
#		    XML. It's just a different presentaton of the same data.  #
#		    Any valid XML file should be convertible into SML, and    #
#		    back into XML, with no binary difference between the two. #
#                   A second goal is to minimize differences with XML.        #
#                                                                             #
#                   Principle:                                                #
#                   XML elements: <tag attr="val" ...>content</tag>           #
#                   SML elements: tag attr="val" ... {content}                #
#                                                                             #
#                   SML rules:                                                #
#                   * Elements normally end at the end of the line.           #
#                   * They continue on the next line if there's a trailing \. #
#                   * They also continue if there's an unmatched "quote" or   #
#                     open {curly braces} block.                              #
#                   * ; separates multiple elements and text on the same line.#
#                   * The element contents are normally inside curly braces.  #
#                   * The {braces} can be omitted if the whole content is     #
#                     just one block of text. (ie. no markup, no CDATA.)      #
#                   * Text data is normally within double quotes.             #
#                   * The "quotes" can be omitted if the parentheses are      #
#                     omitted, and the text does not contain blanks,          #
#                     ", =, ;, #, {, }, <, >, nor a trailing \.               #
#                     (ie. cannot be confused with an attribute or comment.)  #
#                   * This is a #-- Comment -- .                              #
#                   * Simplified case for a # One-line comment.               #
#                   * This is a <[[ CDATA section ]]> .                       #
#                   * A newline immediately following the <[[ is discarded.   #
#                                                                             #
#                   Note: Quotification rules are not the same for attributes #
#                   (Normal XML quotification: " forbidden inside string)     #
#                   and quoted text (All " and \ are prepended with a \).     #
#                                                                             #
#                   Known problems with this script:                          #
#                   - The output contains line endings for the local OS.      #
#                     This breaks binary compatibility with files coming from #
#                     another OS. But this does not break XML compatibility,  #
#                     as the XML spec says all line endings become \n.        #
#                                                                             #
#                   Refer any problem or feedback to jf.larvoire@free.fr,     #
#                   with [SML] in the email subject.                          #
#                                                                             #
#                   Experimental ideas, implemented in the script:            #
#                   - An {\n  Indented CDATA section\n}. The CDATA is between #
#                     the two \n. The CDATA must be indented by 2 more spaces #
#                     then the previous line. The indentations are discarded: #
#                       Some PCDATA{                                          #
#                         The CDATA, indented w. 2 more spaces than prev line.#
#                       }More PCDATA. The } is aligned with the first line.   #
#                   - Content blocks with a CDATA section spanning the whole: #
#                       tag ={                                                #
#                         Indented CDATA section with a trailing \n           #
#                       }                                                     #
#                       tag =={                                               #
#                         Indented CDATA section without a trailing \n        #
#                       }                                                     #
#                       tag =: One-line CDATA section with a trailing \n      #
#                       tag ==: One-line CDATA section without a trailing \n  #
#                                                                             #
#                   Other possible changes:                                   #
#                   - Simplify multiline quotification, ex <<EOF ... EOF ?    #
#                   - Manage distinct encodings for the two sides?            #
#                   - Manage a #! header line, to make executable sml scripts?#
#                     (For example for XSLT scripts)                          #
#                   - Store SML options in a dedicated XML comment?           #
#                   - Store SML options in a dedicated ?sml processing instr? #
#                                                                             #
#   License         Copyright (c) 2005-2014, Jean-François Larvoire	      #
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
#   History 								      #
#    2005-12-13 JFL Created this program as xml2sml.                          #
#    2006-08-28 JFL Redesigned completely. Preserve element alignment.        #
#    2006-10-02 JFL Restructured to better match XML syntax and terminology.  #
#                   Bug fix: Accept spaces around attributes = sign.          #
#    2006-10-04 JFL Fixed several bugs.                                       #
#                   Rewrote execution tracing using new Proc and Return procs.#
#    2007-05-16 JFL Added SML CDATA extensions = == =: ==: ={} =={}           #
#    2008-09-08 JFL Updated proc IsXml to detect invalid but well-formed XML. #
#    2010-04-02 JFL Minor fix to the -V option: Exit after displaying it.     #
#    2010-04-06 JFL Merged in my latest debugging framework.                  #
#                   Fixed a bug the made the program crash under Tcl 8.3.     #
#    2010-04-18 JFL Fixed a bug in SML->XML conversion: Spaces after an       #
#                   unquoted SML value were included in the XML value.        #
#                   Unrelated, added a heuristic to better handle XML->SML    #
#                   conversion of elements containing just spaces.            #
#    2010-06-21 JFL Improved the heuristic for empty blocks encoding:         #
#                   Use {} for multiline blocks, and "" for single-line ones. #
#    2011-08-17 JFL Added experimental support for extended tags names with   #
#                   spaces, using the heuristic that a quoted SML string on   #
#                   a new line is actually an element name. (Except for HTML) #
#                   Bugfix: Attribs. conversion failed on continuation lines. #
#    2013-07-23 JFL Merged in my latest debugging framework version.          #
#    2013-09-21 JFL Added experimental {indented CDADA sections}.             #
#    2013-09-22 JFL Added support for non-binary encodings.                   #
#    2013-09-23 JFL Added the -t option for self testing.                     #
#    2013-09-24 JFL Fixed a bug with empty elements followed by space or tab. #
#                   Added support for SGML definitions [subsections].         #
#    2013-09-25 JFL Removed several XDebug* calls, which improves perf a lot. #
#                   Bugfix: \xA0 is NOT an XML space. Use [ \t\r\n] in regxps.#
#                   Added BSD-style license in the header.                    #
#    2014-11-30 JFL Merged in my latest debugging framework version.          #
#                                                                             #
###############################################################################

set version "2014-11-30"	   ; # The version of this script

# Global variables
set inFile stdin                   ; # Input file handle. Default: stdin
set outFile stdout                 ; # Output file handle. Default: stdout
set inFileName ""                  ; # Input file name. "" = Not yet specified.
set outFileName ""                 ; # Output file name. "" = Not yet specified.

# List of HTML tags known to be often used without an end tag.
set endlessTags {
  hr br img
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
# (See the section headers further down for details about each routine.)      # 
# - Namespace management routines:                                            #
#     Define a public proc: xproc                                             #
#     Define public variables: xvariable, xvars                               #
#     Import all public procs and variables: Import                           #
# - General utility routines, used internally, and useful for more than debug:#
#     Pop an argument off a variable arg list: PopArg, PeekArg, PopLast       #
#     Get a date/time stamp: Now, NowMS                                       #
#     Indent a (possibly multi-line) text string: IndentString                #
#     Get the name and value of a set of variables: VarsValue                 #
#     Find a program in the PATH: Which                                       #
# - Debug, output and logging routines:                                       #
#   Output strings to a choice of channel, private log, system log, callback. #
#     Log strings: LogString (Private logs), LogSystem (System event log)...  #
#     Output and log strings: Puts, VerbosePuts, DebugPuts...                 #
#     Output and log variables values: PutVars, DebugVars, DebugSVars....     #
#     Indent the output of a command or a block of code: Indent               #
#     Check the verbosity mode: Quiet, Normal, Verbose, Debug, XDebug         #
#     Set the verbosity mode: SetQuiet, SetNormal, SetVerbose, SetDebug, ...  #
#     Set the debug output channel: SetDebugChannel, OpenNull                 #
# - Execution trace routines.                                                 #
#     Trace a whole set of routines entry and exit: TraceProcs. Usage:        #
#       TraceProcs { # Begin proc tracing                                     #
#         # Put routines to trace here. No need to modify them.               #
#       } ;          # End proc tracing.                                      #
#     Tracing goes to screen (if debug is on), and to the default log file.   #
#     This can be changed by inserting an optional filename argument. Ex:     #
#       TraceProcs /tmp/tmpfile.log { # Begin proc tracing ... }              #
#     Other routines used internally by TraceProcs: (Rarely needed anymore)   #
#     Get the current procedure name: ProcName.                               #
#     Trace the entry in a routine with its parameters: TraceEntry            #
#     Trace the return value from a routine: Use Return instead of return.    #
#     Trace one routine entry and exit: Define it with Proc instead of proc.  #
# - Miscelleanneous other routines.                                           #
#     A sample background error handler using this framework: bgerror         #
#     Generate an error, inclusing the call stack: Error                      #
# - Program Execution Management routines                                     #
#     Conditionally execute a program, w. logging and tracing options: Exec   #
#     Get the exit code of a program: ErrorCode                               #
#     Conditionally execute a Tcl command: Do                                 #
#     Enable/Disable conditional execution: SetExec, SetNoExec                #
#     Test if we're in no-exec mode: NoExec                                   #
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
# Warning: The Tcl doc states that [clock clicks -milliseconds] returns only a relative time.
# As far as I can tell, it does return 1000 times [clock seconds] on all machines I have access to,
# but some older Tcl versions use a 32-bits integer for the [clock clicks -milliseconds] result, which can wrap around.
if {[expr 1000000 * 1000000] == 1000000000000} { # 64-bits math
  # puts "Defining 64-bits NowMS"
xproc NowMS {{sep " "}} {  # For ISO 8601 strict compatibility, use sep "T".
  set ms [clock clicks -milliseconds]
  set s  [expr $ms / 1000]
  set ms [expr $ms % 1000]
  format "%s.%03d" [clock format $s -format "%Y-%m-%d$sep%H:%M:%S"] $ms
}
} else { # 32-bits math
  # puts "Defining 32-bits NowMS"
  variable s0 [clock seconds]
  variable ms0 [clock clicks -milliseconds]
  variable s1 [clock seconds]
  while {$s1 != $s0} { # Make sure there's no ambiguity on the $ms0 base second
    set s0 $s1
    set ms0 [clock clicks -milliseconds]
    set s1 [clock seconds]
  }
  variable deltaS [expr ($ms0 / 1000) - $s0]
  xproc NowMS {{sep " "}} {  # For ISO 8601 strict compatibility, use sep "T".
    variable deltaS
    set ms [clock clicks -milliseconds]
    set s  [expr ($ms / 1000) - $deltaS]
    set ms [expr $ms % 1000]
    format "%s.%03d" [clock format $s -format "%Y-%m-%d$sep%H:%M:%S"] $ms
  }
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
xproc FormatArray {a {maxDev 10}} {
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
    set limit [expr $average + $maxDev] ; # Reasonable limit to avoid oversize names
    set width [Min $maxLen $limit] ; # Choose the smaller of the two.
    # Output the data using that column width
    foreach {name} $names {
      # Note: If performance is critical, use [list] instead of [CondQuote] in this line:
      append string [format "%-${width}s %s\n" [list $name] [CondQuote $a1($name)]]
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
# 2014-11-14 JFL Rewrote Escape to run faster, scanning the whole string only once.
#		 Added support for Unicode characters > \xFF.
#		 Added support for unbalanced {curly braces}.
variable controlChar
array set controlChar {
   0 {\x00}  1 {\x01}  2 {\x02}  3 {\x03}  4 {\x04}  5 {\x05}  6 {\x06}  7 {\a}
   8 {\b}    9 {\t}   10 {\n}   11 {\v}   12 {\f}   13 {\r}   14 {\x0E} 15 {\x0F}
  16 {\x10} 17 {\x11} 18 {\x12} 19 {\x13} 20 {\x14} 21 {\x15} 22 {\x16} 23 {\x17}
  24 {\x18} 25 {\x19} 26 {\x1A} 27 {\x1B} 28 {\x1C} 29 {\x1D} 30 {\x1E} 31 {\x1F}
}
variable utf8isbuggy 0 ;# Some older Linux machines do not support \xA0-\xFF correctly
xproc Escape {text} {
  variable controlChar
  variable utf8isbuggy
  set l [string length $text]
  set result {}
  set depth 0
  set openBraces {}
  # Convert every character in the input text
  for {set i 0} {$i < $l} {incr i} {
    set c [string index $text $i]
    scan $c "%c" n
    if {$n < 0x20} {		# ASCII control character between 0x00 and \x1F
      set c $controlChar($n)
    } elseif {$n < 0x7F} {	# ASCII printable character between 0x20 and \x7E
      switch -- $c {
      	"\\" {set c "\\\\"}
      	"\[" {set c "\\\["}
      	"\]" {set c "\\\]"}
      	"\"" {set c "\\\""}
      	"\$" {set c "\\\$"}
      	"\{" {
      	  incr depth
      	  lappend openBraces [string length $result]
      	}
      	"\}" {
      	  if {$depth > 0} {
	    incr depth -1
	    PopLast openBraces
	  } else {
	    set c "\\\}"
	  }
      	}
      }
    } elseif {$n < 0xA0} {	# ASCII DEL + extended control characters between \x80 and \x9F
      set c "\\x[format %02X $n]"
    } elseif {$n < 0x100} {	# 8-bits character between \xA0 and \xFF
      if {$utf8isbuggy} { # Some older Linux machines do not support \xA0-\xFF correctly
	set c "\\x[format %02X $n]"
      }
    } else {			# 16-bits Unicode character > \xFF
      set c "\\u[format %04X $n]"
    }
    append result $c
  }
  # Finally correct unbalanced braces, if any is left
  while {$depth} { # There are unbalanced open parenthesis
    set n [PopLast openBraces]
    set result [string replace $result $n $n "\\\{"]
    incr depth -1
  }
  return $result
}

# Quotify a string if needed. ie. when spaces, quotes, or a trailing \.
# Prefer {} for multi-line strings, and "" for single line strings.
xproc CondQuote {text} {
  if {"$text" == ""} {
    return {""}
  }
  # If there are brackets, quotes, backspaces, dollars, or newlines, but no invisible characters (including \r)
  # Also exclude cases with \{, \}, or a trailing \, as these cannot be escaped properly in a curly brace block.
  if {[regexp {[][""\\$\n]} $text -] && ![regexp {[\x00-\x09\x0B-\x1F\x7F-\x9F]|\\[{}]|\\$} $text -]} {
    # Then enclose text in curly braces, to avoid escaping quotes, etc.
    set result "\{" ;# The opening curly brace that will enclose the result
    # Make sure that inner curly braces are balanced in result.
    # Scan all text curly braces, and escape unbalanced closing braces.
    set l [string length $text]
    set depth 0
    set openBraces {}
    for {set i 0} {$i < $l} {incr i} {
      set c [string index $text $i]
      switch -- $c {
      	"\{" {
      	  incr depth
      	  lappend openBraces [string length $result]
      	}
      	"\}" {
      	  if {$depth > 0} {
	    incr depth -1
	    PopLast openBraces
	  } else {
	    set c "\\\}"
	  }
      	}
      }
      append result $c
    }
    # Escape unbalanced opening braces, if any is left
    while {$depth} { # There are unbalanced open parenthesis
      set n [PopLast openBraces]
      set result [string replace $result $n $n "\\\{"]
      incr depth -1
    }
    append result "\}"
    return $result
  }
  # Escape all special and invisible characters
  set text [Escape $text]
  if [regexp {\s} $text -] { # Surround with quotes if there are spaces.
    set text "\"$text\""
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
  set nParts [llength [file split $name]]
  set lastChar [string index $name end]
  if {"$name" == "-"} { # Special case of logging to stdout
    set hLogFileDir ""
  } elseif {[file isdirectory $name] || ("$lastChar" == "/") || ("$lastChar" == "\\")} {
    set hLogFileDir $name
    set name ""
  } elseif {$nParts > 1} {
    set hLogFileDir [file dirname $name]
    set name [file tail $name]
  } elseif {"$name" != ""} { # A name was specified, thus relative to the current directory
    set hLogFileDir ""
  # All following alternatives are for the case where no name is specified
  } elseif [file exists $hLogFileDir] { # Just reuse the previous directory
  } elseif {   ("$::tcl_platform(platform)" == "windows")
            && [file writable "$::env(windir)/logs"]} { # Windows Administrator user
    set hLogFileDir "$::env(windir)/logs/[file rootname $script]"
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
    puts "Logging to stdout."
    # No need to log the command line, as it's a few lines above in the console output.
  } else {
    set exists [file exists $hLogFileName]
    set hLogFile [open $hLogFileName a+] ; # And log everything into the given log file.
    puts "Logging to file [CondQuote [file nativename $hLogFileName]]."
    if {$exists} { # If the log file existed already, put a line delimiter
      LogString [string repeat "-" 80]
  }
    LogString "$::argv0 $::argv"
    LogString "# pid [pid]"
  }
  SetExpectLogFile ; # Send Expect logging there too. Ignores error if no Expect.
  return $hLogFile
}

xproc CloseLogChannel {} {
  variable hLogFile
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
      log_file -leaveopen $hLogFile ; # And log everything into the given log file.
    }
    uplevel 1 InitTraceSend ; # And while we're at it, make sure send is traced too.
  }
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
    # No need to log the pid here, as only one process can log at the same time.
    catch {
      puts $hLogFile "[NowMS] $string"
      flush $hLogFile
    }
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
# Output procedures:						 	      #
# The core routine is Puts, which is a superset of puts.		      #
# Usage: XxxxPuts [options] [channel] string				      #
#									      #
# verbosity N	Test proc Output proc	    Notes			      #
# --------  --  --------  ----------------  --------------------------------- #
# quiet	    0		  ForcePuts         Quiet proc tests verbosity < 1    #
# normal    1   Normal	  Puts	            Everything logged in private logs #
# verbose   2   Verbose   VerbosePuts	    				      #
# debug     3	Debug	  DebugPuts	    Indents output based on call depth#
# extra dbg 4   Xdebug	  XDebugPuts	    For extreme cases		      #
#									      #
# Variable redefinition procedures:					      #
# These routines output variable names and values, formated as valid tcl      #
# command lines, allowing to reenter the variable in another tcl interpreter. #
# Ex: "set name value" or "array set name {key1 value1 key2 value2}"	      #
# VarsValue     Generate a string with variables definitions		      #
# PutVars	Display variables definitions				      #
# PutSVars	Display a string, then variables definitions		      #
# DebugVars	Display variables definitions, in debug mode only	      #
# DebugSVars	Display a string, then variables definitions, in dbg mode only#
#									      #
# Debug ouput channel control:						      #
# The main debug output goes to channel $debugChannel. Default: stdout	      #
# SetDebugChannel   Change it to stderr, or any other writable file handle.   #
# OpenNull	    Open /dev/null or NUL, for use with SetDebugChannel.      #
#=============================================================================#

# Global settings. Can be overriden by defining them before referencing this pkg.
# Output verbosity on stdout.
xvariable verbosity 1 ; # 0=Quiet 1=Normal 2=Verbose 3=Debug 4=XDebug
# Optional prefix to prepend to strings to output on stdout 
# variable prefix "$script: "
xvariable prefix ""
# Optional indentation of the output
xvariable indent 0
# Optional ability to change the default debug output channel
xvariable debugChannel stdout

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

# Optional: Use a different channel for the debug output
xproc SetDebugChannel {channel} {
  variable debugChannel
  set debugChannel $channel
}

# Open the null file, possibly for disabling debug output
xproc OpenNull {} {
  switch $::tcl_platform(platform) {
    "windows" {
      set null "NUL"
    }
    default {
      set null "/dev/null"
    }
  }
  open $null
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
  variable debugChannel
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
  eval Puts -show [Debug] -i $indent $args2 $debugChannel [list $string]
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
      # Note: If performance is critical, use [list] instead of [CondQuote] in this line:
      lappend l "set [list $arg] [CondQuote [uplevel 1 set $arg]]"
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
  if {![XDebug]} {
    set result [ShortenString $result 70]
  } else {
    set result [CondQuote $result]
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

#-----------------------------------------------------------------------------#
#                        Program Execution Management                         #
#-----------------------------------------------------------------------------#

# Get the exit code from the last external program executed.
xproc ErrorCode {{err -1}} { # err = The TCL error caught when executing the program
  if {$err != 0} { # $::errorCode is only meaningful if we just had an error.
    switch -- [lindex $::errorCode 0] {
      "NONE" { # The exit code _was_ 0, only pollution on stderr.
	return 0
      }
      "CHILDSTATUS" { # Non-0 exit code.
	return [lindex $::errorCode 2]
      }
      "POSIX" { # Program failed to start, or was killed.
	return -1
      }
    }
  }
  return $err
}

proc GetTmpDir {} {
  switch $::tcl_platform(platform) { # Platform-specific PATH delimiter
    "windows" {
      foreach var {TEMP TMP} {
        if [info exists ::env($var)] {
          return $::env($var)
        }
      }
      return "C:/Temp"
    }
    "unix" - default {
      return /tmp
    }
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    Exec                                        	      #
#                                                                             #
#   Description     Execute an external command. Log the command and output.  #
#                                                                             #
#   Parameters      options     See below                                     #
#                   command     The command to execute                        #
#                   arguments   The arguments to pass to the command          #
#                                                                             #
#   Options         -e VAR      Put the exit code in variable VAR             #
#                   -E          Ignore errors generated by the command.       #
#                   -f          Force running the command, even if ::exec==0. #
#                   -i N        Indent the output by N spaces. Default: Don't.#
#                   -k          Keep the trailing new line in the output.     #
#                   -t          Tee the output to stdout in realtime.         #
#                   -v          Always display the command line.              #
#                   -vt         Tee the output, but only in verbose mode.     #
#                   --                                                        #
#                                                                             #
#   Returns 	    Returns the output of the external command                #
#                                                                             #
#   Notes:	    Uses namespace variables:                                 #
#                   script      This script name                              #
#                   exec        1=Execute external commands; 0=Don't          #
#                                                                             #
#   History:								      #
#    2009/09/30 JFL Added the ability to display the output in real time,     #
#                   AND log it and return it afterwards.                      #
#    2009/10/06 JFL Made it compatible with the standard exec command.        #
#                                                                             #
#-----------------------------------------------------------------------------#

xvariable exec 1

xproc SetExec {} {
  variable exec
  set exec 1
}

xproc SetNoExec {} {
  variable exec
  set exec 0
}

xproc NoExec {} {
  variable exec
  expr ! $exec
}

TraceProcs {
xproc Exec {args} { # Optional leading options: [-e VAR] [-E] [-f] [-i N] [-k] [-t] [-vt] [-s] [-vs] [--]
  variable script
  variable exec
  set show [expr [Verbose] || [NoExec]] ; # Whether to display the command
  set ignoreErrors 0 ; # 1=Do not report errors
  set doExec $exec   ; # Whether to actually execute the command
  set tee [Debug]    ; # Whether to copy the output to stdout
  set indent 0       ; # Indent the output by N more characters.
  set keepNL 0       ; # 1=keep the trailing new line in the command output.
  # Process optional arguments
  while {"$args" != ""} {
    set oldArgs $args
    set arg [PopArg]
    switch -- $arg {
      -e                { upvar 1 [PopArg] err }
      -E                { set ignoreErrors 1 }
      -f                { set doExec 1 }
      -i                { set indent [PopArg] }
      -k - -keepnewline { set keepNL 1 }
      -o                { set retVar output }
      -s                { set show 1 }
      -vs               { set show [Verbose] }
      -t                { set tee 1 }
      -vt               { set tee [Verbose] }
      --                { break }
      default           { set args $oldArgs ; break }
    }
  }
  # Display the command in verbose mode
  Puts -show $show $args
  set program [lindex $args 0]
  set newIndent [IncrIndent $indent] ; # Indent the command output as specified
  # Prepare to run the command, unless forbidden by the global exec flag
  set err 0
  set output "" ; # Make sure there's always something to return
  # Check for the recursive exec of this very script, with the -X option too...
  set rx "^(.*\\s)?$script\\s(.*\\s)?-X"
  if {$doExec || [regexp $rx $args - - -]} { # If authorized, or recursive exec -X, go!
    set cmd [concat exec $args]
    set tmpFile ""
    set stdoutRedirected [expr [lsearch $cmd ">*"] != -1]
    set stderrRedirected [expr [lsearch $cmd "2>*"] != -1]
    if {$stdoutRedirected && $stderrRedirected} {
      set tee 0 ; # There would be nothing to output.
    }
    if {$tee} { # Display the output in real time
      set tmpFile [GetTmpDir]/Exec.tmp.[pid]
      set pipe "|&" ; # By default, tee both stdout and stderr
      if {$stderrRedirected} { # If stderr is already redirected,
        set pipe "|"         ; # then don't override this redirection.
      }
      lappend cmd $pipe tee $tmpFile ; # Tee the output to a temporary file.
      if {$newIndent} {
        lappend cmd $pipe sed "s/^.\\+/[string repeat " " $newIndent]&/"
      }
      if {!$stdoutRedirected} { # Tee stdout if it's not already redirected..
        lappend cmd >@stdout
      }
      if {!$stderrRedirected} { # Tee stderr if it's not already redirected..
        lappend cmd 2>@stderr
      }
    }
    set err [catch $cmd output] ; # Returns the Tcl error
    set err [ErrorCode $err]    ; # Returns the command exit code
    if {"$tmpFile" != ""} { catch { # Retrieve the output from the temp. file.
      set hFile [open $tmpFile]
      set output "[read $hFile]$output"
      close $hFile
      file delete $tmpFile
      if {[Debug] && ("$output" != "") && ("[string index $output end]" != "\n")} {
        puts "" ; # Make sure the debugging output below is aligned on a new line.
      }
    }}
    if {!$keepNL} { # Remove the trailing new line in the output, if any.
      set output [string trimright $output]
    }
    if $err {
      if {$ignoreErrors} {
      	LogString "Benign error: $output"
      } else {
	DebugPuts $output ; # Log the error, and display it in debug mode.
        DecrIndent $indent; # Restore the initial indent
        error "Exec $program failed. Exit_code=$err. $output"
      }
    } elseif {"$output" != ""} { # If success, always log the output.
      LogString $output
    }
  }
  DebugPuts "exit $err"
  DecrIndent $indent; # Restore the initial indent
  return $output
}
}

# Conditionally execute a potentially dangerous Tcl command. Log it.
# Similar in spirit to Exec, but for internal commands.
xproc Do {args} {
  variable exec
  set doExec $exec   ; # Whether to actually execute the command
  # Process optional arguments
  while {"$args" != ""} {
    set oldArgs $args
    set arg [PopArg]
    switch -- $arg {
      -f                { set doExec 1 }
      --                { break }
      default           { set args $oldArgs ; break }
    }
  }
  if [Debug] {
    DebugPuts -1 [DebugArgLine $args]
  } elseif {!$doExec} {
    Puts [DebugArgLine $args]
  }
  if {$exec} {
    uplevel 1 $args
  }
}

#=============================================================================#
#                         Other useful debug routines                         #
#=============================================================================#

# Return a call stack for debugging
xproc CallStack {{delta 0}} {
  set result {}
  set N [info level] ; # Current depth
  incr N -$delta ; # Where to stop descent
  for {set n 1} {$n < $N} {incr n} {
    append result "$n [info level $n]\n"
  }
  return $result
}

# Generate an error, specifying the call stack in debug mode.
xproc Error {msg} {
  if [Debug] {
    append msg "\nCall Stack:\n[CallStack 1]"
  }
  uplevel 1 [list error $msg]
}

} ; # End of namespace

###############################################################################
#                        End of debug library routines                        #
###############################################################################

debug::Import

###############################################################################
#                          General Purpose Routines                           #
###############################################################################

# Initialize the input buffer
proc FlushBuf {inFile} {
  set ::peekBuf($inFile) ""
}

# Peek the next character from the file, or the Nth.
# Note: Keep that routine as fast and simple as possible, as it's heavily used!
proc PeekChar {inFile {n 0}} {
  set missing [expr $n + 1 - [string length $::peekBuf($inFile)]]
  if {$missing > 0} {
    append ::peekBuf($inFile) [read $inFile $missing]
  }
  set c [string index $::peekBuf($inFile) $n]
  # XDebugPuts [list Peekchar $inFile $n: return $c]
  return $c
}

# Peek the next N characters
proc PeekChars {inFile n} {
  set string ""
  for {set i 0} {$i < $n} {incr i} {
    append string [PeekChar $inFile $i]
  }
  # XDebugPuts [list Peekchars $inFile $n: return $string]
  return $string
}

# Peek to the first non-space character
proc PeekSpaces {inFile {n 0}} {
  for {} {1} {incr n} {
    set c [PeekChar $inFile $n]
    if {"$c" == ""} break ; # End of file!
    if ![string is space $c] break
  }
  return $n
}

# Get the next character from the file.
proc GetChar {inFile} {
  set c [PeekChar $inFile]
  # Remove it from the peek cache.
  set ::peekBuf($inFile) [string range $::peekBuf($inFile) 1 end]
  # XDebugPuts [list Getchar: return $c]
  return $c
}

# Get n characters from the file
proc GetChars {inFile n} {
  set string ""
  for {} {$n > 0} {incr n -1} {
    append string [GetChar $inFile]
  }
  # XDebugPuts [list GetChars: return $string]
  return $string
}

# Put characters back into the input buffer
proc UngetChars {inFile data} {
  set ::peekBuf($inFile) "$data$::peekBuf($inFile)"
}

# Get all consecutive blanks characters.
proc GetBlanks {inFile} {
  set blanks ""
  while {[regexp {[ \t\r\n]} [PeekChar $inFile] -]} { # Will stop on "" = EOF too.
    append blanks [GetChar $inFile]
  }
  # XDebugPuts [list GetBlanks: return $blanks]
  return $blanks
}

# Skip blanks and peek the next first non-blank character.
proc SkipBlanks {inFile} {
  while {[regexp {[ \t\r\n]} [set c [PeekChar $inFile]] -]} { # Will stop on "" = EOF too.
    GetChar $inFile          ; # Throw away the character.
  }
  return $c
}

# Get a whole line
proc GetLine {inFile} {
  set line ""
  while 1 {
    set c [GetChar $inFile]
    if {("$c" == "\n") || ("$c" == "")} break
    append line $c
  }
  return $line
}

# Put characters without appending a new line
proc Write {args} {
  eval puts -nonewline $args
  set ::wasDebug 0
}

# Put a new line with the given indent
proc NewLine {outFile indent} {
  Write $outFile [format "\n%${indent}s" ""]
}

# Get the end of a quoted string (Assuming we already have the first quote)
proc GetEndOfQuotedString {inFile} {
  set string ""
  while 1 {
    set c [GetChar $inFile]
    if {"$c" == ""} break    ; # End of file
    set c0 $c
    if {"$c" == "\\"} {
      set c [PeekChar $inFile]
      switch -- $c {
        "\\" - "\"" { # Supported escape. Throw away the \.
	  set c [GetChar $inFile]
	}
	default {     # Unsupported escape. Copy the \.
	  set c $c0
	}
      }
    }
    append string $c
    if {"$c0" == "\""} break  ; # End of string
  }
  return $string
}

TraceProcs {

# Get a quoted string
proc GetQuotedString {inFile} {
  set c [PeekChar $inFile]
  if {"$c" != "\""} {
    return ""
  }
  set c [GetChar $inFile]
  append c [GetEndOfQuotedString $inFile]
  return $c
}

# Add quotes if needed, escaping embedded " and \ in the process.
proc Quotify {string {force 0}} {
  if {$force || [regexp {[ \t\r\n=";#"{}<>]} $string -]
      || ("[string index $string end]" == "\\")} {
    regsub -all {\\} $string {\\\\} string
    regsub -all "\"" $string "\\\"" string
    set string "\"$string\""
  }
  return $string
}

# XML tag names may not contain !"#$%&'()*+,/;<=>?@[\]^`{|}~, nor a space,
# and cannot start with -, ., or a digit.
set invalidTagChars " !\"#$%&'()*+,/;<=>?@\[\\\]^`{|}~"
set invalidTagChars0 "-.0123456789"
# Create maps for converting these invalid chars to XML entities.
foreach c [split $invalidTagChars ""] {
  scan $c "%c" code
  set code [format "%02X" $code]
  lappend invalidTagCharsMapChar2Entity $c "&#x$code;"
}
foreach {a b} $invalidTagCharsMapChar2Entity {
  lappend invalidTagCharsMapEntity2Char $b $a
}
# TO DO: Also support $invalidTagChars0
proc EscapeTagName {string} {
  set string [string map $::invalidTagCharsMapChar2Entity $string]
  return $string
}
proc UnEscapeTagName {string} {
  set string [string map $::invalidTagCharsMapEntity2Char $string]
  return [Quotify $string]
}

# Convert an IANA charater encoding name to its equivalent Tcl encoding name
# See Tcl's [encoding names] command for the list of supported encodings
proc IANA2TclEncoding {encoding} {
  set encoding [string tolower $encoding]
  regsub {iso-} $encoding {iso} encoding
  regsub {windows-1252} $encoding {cp1252} encoding
  return $encoding
}

###############################################################################
#                          XML -> SML Transformation                          #
###############################################################################

# Get XML litteral data quoted with either ' or ".
proc GetXmlQuotedString {inFile} {
  set c0 [GetChar $inFile]
  set string $c0
  while 1 {
    set c [GetChar $inFile]
    if {"$c" == ""} break ; # End of file.
    append string $c
    if {"$c" == "$c0"} break ; # End of XML litteral
  }
  return $string ; # Return the string, including the enclosing quotes.
}

# Get a token
proc GetXmlToken {inFile {varName ""}} {
  # Initialize the result variable
  if {"$varName" != ""} {
    upvar 1 $varName token
  }
  set token ""
  # First check for special delimiter tokens
  set c [SkipBlanks $inFile]
  switch -- $c {
    "" {              # End of file
      return $token
    }
    "?" - "!" - "<" - ">" - "=" - "/" { # One-character keyword
      set token [GetChar $inFile] ; # Flush the char
      return $token
    }
    "-" {             # Possibly the beginning of an HTML -- comment
      set token [GetChar $inFile] ; # Flush that first -
      if {"[PeekChar $inFile]" == "-"} {
	append token [GetChar $inFile] ; # Flush the second -
	return $token
      }
    }
    "\"" - "'" {
      set token [GetXmlQuotedString $inFile]
      return $token
    }
    default {
      # Anything else falls through
    }
  }
  # Then feed all non-blank characters until the next delimiter
  while {"[set c [PeekChar $inFile]]" != ""} {
    if [regexp {[?<>=/ \t\r\n]} $c  -] break
    append token [GetChar $inFile]
  }
  return $token
}

# Get data from a body
proc GetXmlData {inFile} {
  set data ""
  while {"[set c [PeekChar $inFile]]" != ""} {
    if {"$c" == "<"} break
    append data [GetChar $inFile]
  }
  return $data
}

# Get a header attribute
proc GetXmlAttribute {inFile {varName ""}} {
  # Initialize the result variable
  if {"$varName" != ""} {
    upvar 1 $varName attrib
  }
  set attrib ""
  SkipBlanks $inFile
  # First the attribute name.
  set attrib [GetXmlToken $inFile]
  if [regexp {^[?!<>=/]?$} $attrib -] { # Will stop on "" = EOF too.
    return $attrib
  }
  # Then optional spaces.
  append attrib [GetBlanks $inFile]
  # Next should be the = sign.
  set c [PeekChar $inFile]
  if {"$c" != "="} { # Anything else is the end of the attribute.
    return $attrib
  }
  append attrib [GetChar $inFile] ; # Flush that =
  # Then optional spaces again.
  append attrib [GetBlanks $inFile]
  # Next should be the attribute value (Quoted or not).
  set c [PeekChar $inFile]
  if {![regexp {[> \t\r\n]} $c -]} { # OK, this looks like a value
    append attrib [GetXmlToken $inFile]
  }
  return $attrib
}

# Get an SGML comment. (Can be anywhere within the tag)
proc GetSgmlComment {inFile} {
  set comment ""
  set y ""
  set z ""
  while {("$y" != "-") || ("$z" != "-")} {
    append comment $y
    set y $z
    set z [GetChar $inFile]
    if {"$z" == ""} { # End of file
      append comment $y
      break
    }
  }
  return $comment
}

# Get a CDATA section content. Assumes the start mark has already been read.
proc GetCdata {inFile} {
  set data ""
  while {"[PeekChars $inFile 3]" != {]]>}} { # End of CDATA section is ]]>.
    set c [GetChar $inFile]
    if {"$c" == ""} break ; # End of file
    append data "$c"
  }
  GetChars $inFile 3 ; # Eat up the end mark.
  return $data
}

# Convert an XML start tag. Returns ""=normal element or "/"=empty element.
proc ConvertXmlStartTag {inFile outFile depth} {
  set ret ""
  # Convert the attributes
  while 1 {
    set blanks [GetBlanks $inFile]
    if {"$blanks" != ""} {
      regsub -all "\n" $blanks "\\\n" blanks ; # Mark continuation lines.
      Write $outFile $blanks
    }
    set attrib [GetXmlAttribute $inFile]
    switch -- $attrib {
      "" { # End of file
	break
      }
      ">" { # End of tag
	break
      }
      "/" { # End of Empty Element tag: "/>"
	set c [PeekChar $inFile]
	if {"$c" == ">"} {
	  GetChar $inFile ; # Throw it away.
	  set ret "/"
	  break
	}
	# Else it's a / that's part of the attributes (illegal?). Output it.
      }
      "--" { # SGML comment
	set comment [GetSgmlComment $inFile]
	set attrib "#--${comment}--"
      }
      default { # Attribute
	regexp {^([^= \t\r\n]*)([ \t\r\n]*=[ \t\r\n]*)?(.*)} $attrib - name eq value
	regsub -all "\n" $eq "\\\n" eq ; # Mark continuation lines.
	set attrib "$name$eq$value"
      }
    }
    Write $outFile $attrib
  }
  return $ret
}

# Convert an XML processing instruction.
proc ConvertXmlProcessingInstruction {inFile outFile depth} {
  # Convert the processing instruction directives
  while 1 {
    set blanks [GetBlanks $inFile]
    if {"$blanks" != ""} {
      regsub -all "\n" $blanks "\\\n" blanks ; # Mark continuation lines.
      Write $outFile $blanks
    }
    set attrib [GetXmlToken $inFile]
    switch -- $attrib {
      "" { # End of file
	break
      }
      "?" { # End of Processing Instruction: "?>"
	set c [PeekChar $inFile]
	if {"$c" == ">"} {
	  GetChar $inFile ; # Throw it away.
	  break
	}
	# Else it's a ? that's part of the instruction. Output it.
      }
      "--" { # SGML comment
	set comment [GetSgmlComment $inFile]
	set attrib "#--${comment}--"
      }
      default { # PI directive.
	# Note that by definition tokens cannot contain newlines.
      }
    }
    Write $outFile $attrib
  }
  return
}

# Get an Sgml token
proc GetSgmlToken {inFile} {
  set c [PeekChar $inFile]
  switch -- $c {
    "\[" {    # Begin a subsection
      set token [GetChar $inFile]
    }
    "\]" {    # End a subsection
      set token [GetChar $inFile]
    }
    "\"" {    # Quoted value, like for attribute values
      set token [GetXmlQuotedString $inFile]
    }
    default { # Else return a standard XML token
      set token [GetSmlToken $inFile]
    }
  }
  return $token
}

# Convert an SGML Definition [section].
proc ConvertSgmlDefinitionSection {inFile outFile depth} {
  while 1 {
    set c [PeekChar $inFile]
    switch -- $c {
      "" {      # End of file
	break
      }
      "\]" {    # End of section
	break
      }
      "<" {     # Sgml Definition
        set tag [ConvertXmlElement $inFile $outFile $depth]
      }
      default { # Anything else is plain data
      	Write $outFile [GetChar $inFile]
      }
    }
  }
  return
}

# Convert an SGML Definition.
proc ConvertSgmlDefinition {inFile outFile depth} {
  # Convert the attributes
  while 1 {
    set blanks [GetBlanks $inFile]
    if {"$blanks" != ""} {
      regsub -all "\n" $blanks "\\\n" blanks ; # Mark continuation lines.
      Write $outFile $blanks
    }
    set attrib [GetSgmlToken $inFile]
    switch -- $attrib {
      "" { # End of file
	break
      }
      "<" { # New inner SGML definition
      	UngetChars $inFile "<"
      	ConvertXmlElement $inFile $outFile $depth
	set attrib ""
      }
      ">" { # End of definition
	break
      }
      "\[" { # Begin Definition Subsection
      	Write $outFile "\["
      	ConvertSgmlDefinitionSection $inFile $outFile $depth
      	set attrib ""
      }
      "\]" { # End Definition Subsection
      }
      "--" { # SGML comment
	set comment [GetSgmlComment $inFile]
	set attrib "#--${comment}--"
      }
      default { # Definition data
	# Note that by definition tokens cannot contain newlines.
      }
    }
    Write $outFile $attrib
  }
  return
}

# Convert an XML tag. Returns the tag name, including the leading PI character.
proc ConvertXmlTag {inFile outFile depth} {
  # Skip the opening <
  if {"[GetXmlToken $inFile tok]" == ""} { return "" } ; # End of file
  if {"$tok" != "<"} { 
    return "" ; # Not an XML element
  }
  # Now the name comes immediately behind.
  GetXmlToken $inFile tagName
  set type [string index $tagName 0]
  switch -- $type {
    "" { # End of file
    }
    ">" { # End of tag
    }
    "?" { # Processing Instruction
      append tagName [GetXmlToken $inFile]
      Write $outFile $tagName
      ConvertXmlProcessingInstruction $inFile $outFile $depth
    }
    "!" { # SGML Definition
      append tagName [GetXmlToken $inFile]
      if {"$tagName" == "!--"} { # This is a valid XML comment
	Write $outFile "#"
	set comment [GetSgmlComment $inFile]
	SkipBlanks $inFile ; # There should not be any, but just in case
	GetChar $inFile ; # Eat up the closing >
	set c [PeekChar $inFile]
	if {   ([string first \n $comment] != -1)
	    || (("$c" != "") && ("$c" != "\n")) } {
	      set comment "--${comment}--"
	}
	Write $outFile $comment
      } else {
	Write $outFile $tagName
	ConvertSgmlDefinition $inFile $outFile $depth
      }
    }
    "/" { # End tag
      append tagName [GetXmlToken $inFile] ; # Eat it up
      append tagName [GetBlanks $inFile] ; # Eat up optional spaces
      GetXmlToken $inFile ; # Eat up the > end of tag.
    }
    default { # Start tag
      Write $outFile [UnEscapeTagName $tagName]
      set type [ConvertXmlStartTag $inFile $outFile $depth] ; # "" or "/"
      set tagName "$type$tagName" ; # Prepend / if it's an empty element.
    }
  }
  return $tagName
}

# Convert an XML element
proc ConvertXmlElement {inFile outFile depth} {
  # Convert the start tag
  set tagName [ConvertXmlTag $inFile $outFile $depth]
  # Is there a body?
  set type [string index $tagName 0] ; # Tag type. Either "", "/", "!" or "?"
  switch -- $type {
    "/" - "!" - "?" { # These types have no body.
      # set tagName [string range $tagName 1 end]
      set c [PeekChar $inFile]
      if {("$c" == " ") || ("$c" == "\t")} { # Avoid dragging that space inside the empty element.
	Write $outFile ";" ;# Mark the end of the element. 
      }      	
    }
    default {
      set type ""                                        ; # Assume a normal tag
      set ltag [string tolower $tagName]
      if {[lsearch -exact $::endlessTags $ltag] != -1} { ; # Nope, has no closure.
	set type "/" ; # This special html tag does not have a body either.
      }
      # Note: If there is an end tag anyway, it will be thrown away!
    }
  }
  # If there is, convert it.
  if {"$type" == ""} {
    Write $outFile " " ; # Alway add one space for readability.
    # Convert the data and tail blocks
    ConvertXmlContent $inFile $outFile [expr $depth + 1]
  }
  return "$tagName" ; # Includes type in the 1st char.
}

# Convert the element contents
# Complexified by the need to avoid outputing parentheses if possible.
proc ConvertXmlContent {inFile outFile depth} {
  set dirty 0      ; # 0=No output on the current line yet. 1=Yes there is.
  set inBlock 0    ; # 0=Not opened parentheses yet. 1=Yes we have.
  set string0 ""   ; # The string put aside for putting in block, if any.
  set empty 1      ; # 0=No content output yet; 1=Some output done.
  set endSpaces "" ; # Spaces inside the end tag, if any. (Rare but legal)
  while 1 {
    set c [PeekChar $inFile]
    switch -- $c {
      "" {      # End of file
	break
      }
      "<" {     # Either an inner XML element, or the end of this one.
	if {"[PeekChar $inFile 1]" == "/"} { # This is the end of this one.
	  set endTag [ConvertXmlTag $inFile $outFile $depth] ; # Eat it up
	  regexp {[ \t\r\n]*$} $endTag endSpaces ; # Optional and rarely used spaces
	  break ; # The end of this element
	}
	if {!$inBlock} { # If the block is not opened yet, do it.
	  if {$depth > 0} {
	    Write $outFile "\{"
	    set dirty 0
	  }
	  if {"$string0" != ""} { # If we put a string aside, output it first.
	    if [regexp {^([ \t\r\n]*)(.*[^ \t\r\n])?([ \t\r\n]*)?$} $string0 - bk1 string bk2] {
	      if {"$string" != ""} {
		set string [Quotify $string 1] ; # Make sure it is quoted.
		if {[string first "\n" "$bk2"] != -1} {
		  set string0 "$bk1$string$bk2"
		  set dirty 0
		} else {
		  set string0 "$bk1$string;$bk2" ; # Notice the ; after the string
		  set dirty 0
		}
	      } elseif {[string first "\n" "$bk1$bk2"] != -1} {
		set dirty 0
	      }
	    }
	    Write $outFile "$string0"
	  }
	  set inBlock 1
	}
	set empty 0 ; # We're going to output something in any case.
	if {"[PeekChars $inFile 9]" == "<!\[CDATA\["} { # This is a CDATA section
	  GetChars $inFile 9 ; # Eat up the start mark.
	  Write $outFile "<\[\[" ; # Output the sml CDATA begin mark
	  set cdata [GetCdata $inFile]
	  if {[string first \n $cdata] != -1} {
	    Write $outFile \n ; # Help make alignment verification easy.
	  }
	  Write $outFile $cdata
	  Write $outFile "\]\]>" ; # Output the CDATA end mark
	  set dirty 1
	  continue ; # Look for more data.
	}
	if {$dirty} {
	  Write $outFile ";"
	}
        set tag [ConvertXmlElement $inFile $outFile $depth]
	set type [string index $tag 0] ; # Tag type. Either "", "/", "!" or "?".
	set dirty 1 ; # We've output the end of an element on this line.
      }
      default { # Anything else is plain data
	set text [GetXmlData $inFile]
	regexp {^([ \t\r\n]*)(.*[^ \t\r\n])?([ \t\r\n]*)?$} $text - blanks1 string blanks2
	if {!$inBlock} {
	  if {"$string0" == ""} {
	    set string0 $text ; # Put aside for later output
	    continue
	  }
	  error "This should never happen"
	  if {"$string0" != ""} {
	    Write $outFile "$string0"
	    if [regexp {[^ \t\r\n]} $string0 -] {set dirty 1}
	  }
	  set inBlock 1
	}
	set empty 0
	if {[string first \n $blanks1] != -1} {
	  set dirty 0
	}
	if {$dirty && ("$string" != "")} {
	  Write $outFile ";"
	}
	if {"$blanks1" != ""} {
	  Write $outFile "$blanks1"
	}
	if {"$string" != ""} {
	  Write $outFile [Quotify $string $inBlock]
	  set dirty 1
	}
	if {[string first \n $blanks2] != -1} {
	  set dirty 0
	}
	if {"$blanks2" != ""} {
	  Write $outFile "$blanks2"
	}
      }
    }
  }
  DebugVars inBlock dirty empty string0 depth endSpaces
  if {!$inBlock} {
    if {"$string0" != ""} {
      regexp {^([ \t\r\n]*)(.*[^ \t\r\n])?([ \t\r\n]*)?$} $string0 - blanks1 string blanks2
      if {"$string" == ""} { # A single item must not be blank.
	# 2010-06-18 set string0 [Quotify $string0 1] ; # Quotify it to make it visible.
	# 2010-06-18 Changed the above to the heuristic:
	if [regexp "\n" $string0 -] { # If it spans multiple lines, assume it's an empty block
          set string0 "{$string0}"    ; # Make the spaces block visible.
        } else { # If it's a single line, assume it's a blank string.
          set string0 "\"$string0\""  ; # Quotify it to make it visible.
        }
      } elseif {"$endSpaces" != ""} { # Also must quotify if end tag spaces
	if $empty {
	  Write $outFile "\{"
	  set inBlock 1
	}
	set string0 [Quotify $string0 1] ; # Quotify it to make it visible.
      } else {
	set string0 [Quotify $string0] ; # Quotify it if needed
      }
      Write $outFile "$string0"
    } elseif {$empty && (($depth > 0) || ("$endSpaces" != ""))} {
      Write $outFile "\{"
      set inBlock 1
    }
  }
  if {$inBlock && ($depth > 0)} {
    Write $outFile "\}"
  }
  if {"$endSpaces" != ""} {
    Write $outFile "{$endSpaces}"
  }
  return
}

# Heuristics to identify an XML file
proc IsXml {inFile} {
  # Read just enough ASCII characters to ascertain this.
  # (Avoid peeking into characters that may change meaning if we change the encoding)
  set buf ""
  for {set n 0} {$n < 100} {incr n} {
    set c [PeekChar $inFile $n]
    append buf $c
    if [string is space -strict $c] continue
    if {"$c" == "<"} {
      incr n
      break
    }
    return 0
  }
  for {} {$n < 100} {incr n} {
    set c [PeekChar $inFile $n]
    append buf $c
    if {"$c" == ">"} break
  }
  # Look for the XML header
  DebugVars buf
  if {[string first "<?xml" $buf] != -1} { # Official XML header. No doubt.
    DebugPuts "Found the XML header"
    # Update the encoding, if one is specified
    if [regexp {encoding[ \t\r\n]*=[ \t\r\n]*["'"]([^"'"]+)["'"]} $buf - encoding] {
      # Changing the encoding now is not a problem, because so far we've peeked
      # only at the XML header, which is guarantied to be only ASCII.
      set encoding [IANA2TclEncoding $encoding]
      fconfigure $inFile -encoding $encoding
    }
    return 1
  }
  # OK, so far this looks like an XML tag, and not the ?xml header.
  # The XML spec says the default encoding MUST be utf-8 or utf-16, which we already distinguished.
  DebugPuts "Found an XML element"
  return 1 ; 
}

# Convert a whole file
proc xml2sml {inFile outFile} {
  set depth 0 ; # Current depth in the object tree.
  set ::docType XML ; # Assume XML
  if {"[string toupper [PeekChars $inFile 5]]" == "<HTML"} { # The input is HTML
    set ::docType HTML ; # Change endlessTags behaviour to html compatible.
  }
  ConvertXmlContent $inFile $outFile $depth
  return 0
}

###############################################################################
#                          SML -> XML Transformation                          #
###############################################################################

# Get a token from a header tag.
proc GetSmlToken {inFile {varName ""}} {
  # Initialize the result variable
  if {"$varName" != ""} {
    upvar 1 $varName token
  }
  set token ""
  # First check for special delimiter tokens
  while 1 {
    set c [PeekChar $inFile]
    switch -- $c {
      "" {              # End of file
	return $token
      }
      "\n"  {           # End of line
	set token ";"     ; # This is an end of element. But leave the \n there.
	return $token
      }
      " " - "\t" {      # Other blanks
	# XDebugPuts [list GetSmlToken: Discarding $c]
	GetChar $inFile   ; # Throw it away
	continue
      }
      "#" - ";" - "=" - "\{" { # One-character keyword
	set token [GetChar $inFile] ; # Flush the char
	return $token
      }
      "\}"  {           # End of outer element
	set token $c    ; # Do not flush the char!
	return $token
      }
      "\"" {
	set token [GetQuotedString $inFile]
	return $token
      }
      "\\" {		# Line continuation
	set c [PeekChar $inFile 1]
	if {"$c" == "\n"} {
	  set token [GetChar $inFile] ; # Flush the char
	return $token
	}
	break
      }
      default {
	break ; # Anything else falls through
      }
    }
  }
  # Then feed all non-blank characters until the next delimiter
  while {"[set c [PeekChar $inFile]]" != ""} {
    if [regexp {[#;= \t\r\n{}]} $c  -] break
    if {"$c" == "\\"} {
      set c [PeekChar $inFile 1]
      if {"$c" == "\n"} {
	break
      }
    }
    append token [GetChar $inFile]
  }
  return $token
}

# Get blanks from a header tag.
proc GetSmlTagBlanks {inFile {varName ""}} {
  # Initialize the result variable
  if {"$varName" != ""} {
    upvar 1 $varName blanks
  }
  set blanks ""
  # First check for special delimiter tokens
  while 1 {
    set c [PeekChar $inFile]
    switch -- $c {
      "" {              # End of file
	break
      }
      "\n"  {           # End of line
	break             ; # This is an end of element. But leave the \n there.
      }
      " " - "\t" {      # Other blanks
	append blanks [GetChar $inFile]
      }
      "\\" {            # Continuation
	set c [PeekChar $inFile 1]
	if {"$c" != "\n"} {
	  break ; # Oops, this is NOT a continuation. Leave the \ there.
	}
	GetChar $inFile ; # Throw away the \ character
	append blanks [GetChar $inFile] ; # and record the new line.
      }
      default {         # Anything else is not blank.
	break ; # Leave in place for now.
      }
    }
  }
  return $blanks
}

# Get data from a body.
proc GetSmlData {inFile} {
  set data ""
  while {"[set c [PeekChar $inFile]]" != ""} {
    switch -- $c {
      ";" {
	# XDebugPuts [list GetSmlData: Discarding $c]
	GetChar $inFile   ; # Throw it away
      }
      "\"" {
	append data [string range [GetQuotedString $inFile] 1 end-1]
      }
      " " - "\t" - "\n" {
	append data [GetChar $inFile]
      }
      default {
	break
      }
    }
  }
  return $data
}

# Get a header attribute
proc GetSmlAttribute {inFile {varName ""}} {
  # Initialize the result variable
  if {"$varName" != ""} {
    upvar 1 $varName attrib
  }
  set attrib ""
  # First the attribute name.
  set attrib [GetSmlToken $inFile]
  if {"$attrib" == "="} { # One of = == =: ==: =\{ ==\{ CDATA markers
    set cc [PeekChars $inFile 2]
    if [regexp {=?[:\{]?} $cc cc] {
      foreach c [split $cc ""] {
	append attrib [GetChar $inFile] ; # Flush that =
      }
      return $attrib
    }
  }
  if [regexp {^[?!#;=/\\\{\}]?$} $attrib -] { # Will stop on "" = EOF too.
    return $attrib
  }
  if {"[string index $attrib 0]" == "\""} { # Actually a quoted element body.
    return $attrib
  }
  # Peek around optional spaces, and possibly continuation lines.
  set n 0
  while 1 {
    set n [PeekSpaces $inFile $n]
    set c [PeekChar $inFile $n]
    if {"$c" != "\\"} break
    set c2 [PeekChar $inFile [expr $n+1]]
    if {"$c2" != "\n"} break
    incr n 2
  }
  # Next should be the = sign.
  if {"$c" != "="} { # Anything else is the end of the attribute.
    return $attrib
  }
  if $n {
    append attrib [GetSmlTagBlanks $inFile] ; # Flush those spaces
  }
  append attrib [GetChar $inFile] ; # Flush that =
  # Then optional spaces again.
  append attrib [GetSmlTagBlanks $inFile]
  # Next should be the attribute value (Quoted or not).
  set c [PeekChar $inFile]
  if {"$c" == "\""} { # OK, this is a quoted value (quoted a-la XML)
    append attrib [GetXmlQuotedString $inFile]
  } elseif {![regexp {[; \t\r\n]} $c -]} { # OK, this is an unquoted value
    append attrib [GetSmlToken $inFile]
  }
  return $attrib
}

# Get an SML comment
proc GetSmlComment {inFile} {
  set c [PeekChar $inFile]
  if {"$c" == "#"} {
    GetChar $inFile ; # Flush the #.
  }
  set cc [PeekChars $inFile 2]
  if {"$cc" == "--"} {
    GetChar $inFile ; # Discard the 1st -
    GetChar $inFile ; # Discard the 2nd -
    set comment [GetSgmlComment $inFile]
  } else {
    set comment ""
    while 1 { # Get the end of line
      set c [PeekChar $inFile]
      if {"$c" == ""} break ; # End of file
      if {"$c" == "\n"} break ; # End of line
      append comment [GetChar $inFile]
    }
  }
  return $comment
}

# Get an SML CDATA section
proc GetSmlCdataBlock {inFile} {
  set cdata {}
  set c [GetChar $inFile] ; # Drop the opening parenthesis.
  set c0 ""
  while 1 {
    set c [GetChar $inFile]
    switch -- $c {
      "" {         # End of file
	break
      }
      "\}" {       # End of cdata section
	break
      }
      "\\" {       # Escape char
	append cdata [GetChar $inFile]
      }
      default {
	append cdata $c
      }
    }
  }
  return $cdata
}

# Experimental - Convert an SML extended CDATA section.
proc ConvertSmlXCdataSection {inFile outFile depth type} {
  switch $type {
    "=" {
      set c [SkipBlanks $inFile]
      if {"$c" == "\""} {   # Quoted data
	Write $outFile [string range [GetQuotedString $inFile] 1 end-1]
      } else {
	Write $outFile [GetSmlToken $inFile]
      }
      Write $outFile "\n"
    }
    "==" {
      set c [SkipBlanks $inFile]
      if {"$c" == "\""} {   # Quoted data
	Write $outFile [string range [GetQuotedString $inFile] 1 end-1]
      } else {
	Write $outFile [GetSmlToken $inFile]
      }
    }
    "=:" {
      GetChar $inFile ; # Eat up the following blank
      set line [GetLine $inFile]
      Write $outFile "<!\[CDATA\[$line\n\]\]>"
      UngetChars $inFile "\n" ; # Allow the caller to see the element end
    }
    "==:" {
      GetChar $inFile ; # Eat up the following blank
      set line [GetLine $inFile]
      Write $outFile "<!\[CDATA\[$line\]\]>"
      UngetChars $inFile "\n" ; # Allow the caller to see the element end
    }
    "=\{" {
      GetLine $inFile ; # Eat up the following newline
      Write $outFile "<!\[CDATA\["
      set blanks [format "%*s" [expr ($depth + 1) * 2] ""]
      while 1 {
	set line [GetLine $inFile]
	if {![regsub $blanks $line {} line]} break
	Write $outFile "$line\n"
      }
      Write $outFile "\]\]>"
      UngetChars $inFile "\n" ; # Allow the caller to see the element end
    }
    "==\{" {
      GetLine $inFile ; # Eat up the following newline
      Write $outFile "<!\[CDATA\["
      set blanks [format "%*s" [expr ($depth + 1) * 2] ""]
      set nl ""
      while 1 {
	set line [GetLine $inFile]
	if {![regsub $blanks $line {} line]} break
	Write $outFile "$nl$line"
	set nl "\n"
      }
      Write $outFile "\]\]>"
      UngetChars $inFile "\n" ; # Allow the caller to see the element end
    }
    "\{" { # Indented CDATA section within the PCDATA stream
      GetLine $inFile ; # Eat up the initial newline
      Write $outFile "<!\[CDATA\["
      set nBlanks [expr ($depth) * 2]
      set blanks [format "%*s" $nBlanks ""]
      set newLine ""
      while 1 {
      	set line [GetChars $inFile $nBlanks] ;# Check the expected blanks
      	if {"$line" != "$blanks"} { # We've reached the end line, less indented than the others
      	  set nEaten 0
      	  foreach c [split $line ""] { # Eat-up the blank characters until the final \}
      	    if {"$c" != " "} { # This should be the final \}.
      	      if {"$c" != "\}"} { # If it's not, we've got invalid input
      	      	error "Invalid indented CDATA section"
      	      }
      	      incr nEaten
      	      break
      	    }
      	    incr nEaten
      	  }
      	  UngetChars $inFile [string range $line $nEaten end]
      	  break
      	}
	set line [GetLine $inFile]
	Write $outFile "$newLine$line"
	set newLine "\n"
      }
      Write $outFile "\]\]>" ;# The final new line was eaten up with the last line
    }
  }
}

# Convert an SML element header attributes.
proc ConvertSmlAttributes {inFile outFile depth} {
  set slash ""
  # Convert the header
  while 1 {
    # First convert blanks
    set blanks [GetSmlTagBlanks $inFile]
    regexp {(.*?)( ?)$} $blanks - blanks tailSpace ; # Split-off the tail space.
    if {"$blanks" != ""} {
      Write $outFile $blanks
    }
    # Then convert attributes
    set attrib [GetSmlAttribute $inFile]
    switch -- $attrib {
      "" {                # End of file
	set slash "/"       ; # Flag that this is an empty element
	break
      }
      "\{" {              # End of header
	break
      }
      ";" - "\n" - "\}" { # End of element
	if {"$tailSpace" != ""} {
	  Write $outFile $tailSpace
	  set tailSpace ""
	}
	set slash "/"       ; # Flag that this is an empty element
	break
      }
      "#" {               # Comment
	set comment [GetSmlComment $inFile]
	if {"$tailSpace" != ""} {
	  Write $outFile $tailSpace
	  set tailSpace ""
	}
	Write $outFile "--${comment}--"
	continue
      }
      "=" - "==" - "=:" - "==:" - "=\{" - "==\{" {
	set slash $attrib ; # End of header and beginning of CDATA body section
	break
      }
    }
    if {![regexp {^[^""=]+=} $attrib -]} { # Oops, this is actually the body "
      if {"[string index $attrib 0]" == "\""} {
	set attrib [string range $attrib 1 end-1]
      }
      set attrib [Quotify $attrib 1] ; # Make sure it is quoted
      UngetChars $inFile "$attrib\}" ; # Put the body string back into the input buffer
      break
    }
    if {"$tailSpace" != ""} {
      Write $outFile $tailSpace
      set tailSpace ""
    }
    Write $outFile $attrib ; # Plain ol' good attribute.
  }
  return $slash
}

# Get a token from an SGML definition.
proc GetSmlDefinitionToken {inFile} {
  set c [PeekChar $inFile]
  switch -- $c {
    "\[" {              # Begin a subsection
      set token [GetChar $inFile]
      return $token
    }
    default {		# Else use the standard tokenizer
      return [GetSmlToken $inFile]
    }
  }
  return $token
}

# Convert an SGML Definition [section].
proc ConvertSmlDefinitionSection {inFile outFile depth} {
  while 1 {
    set c [PeekChar $inFile]
    switch -- $c {
      "" {                # End of file
	break
      }
      "\]" {		  # End of section
	Write $outFile [GetChar $inFile]
	break
      }
      ";" {               # Element separator
	# XDebugPuts [list ConvertSmlContent: Discarding $c]
	GetChar $inFile      ; # Throw it away
      }
      "\n" {              # Element end
	Write $outFile [GetChar $inFile]
      }
      "\"" {              # Quoted data OR tag name with spaces
	Write $outFile [string range [GetXmlQuotedString $inFile] 1 end-1]
      }
      "#" {		  # Sgml Comment
	set comment [GetSmlComment $inFile]
	Write $outFile "<!--${comment}-->"
      }
      "!" {		  # Sgml Definition
        set tag [ConvertSmlElement $inFile $outFile $depth]
      }
      default { # Anything else is plain data
      	Write $outFile [GetChar $inFile]
      }
    }
  }
  return
}

# Convert an SML Processing instruction or SGML definition
proc ConvertSmlHeaderTokens {inFile outFile depth} {
  # Convert the header
  while 1 {
    # First convert blanks
    set blanks [GetSmlTagBlanks $inFile]
    if {"$blanks" != ""} {
      Write $outFile $blanks
    }
    # Then convert tokens
    set c [PeekChar $inFile]
    if {"$c" == "\""} { # This is a quoted value (quoted a-la XML)
      set token [GetXmlQuotedString $inFile]
    } else { # This is an unquoted token.
      set token [GetSmlToken $inFile]
    }
    switch -- $token {
      "" {                # End of file
	break
      }
      ";" - "\n" - "\}" { # End of element
	break
      }
      "#" {               # Comment
	set comment [GetSmlComment $inFile]
	set token "--${comment}--"
      }
      "\[" {              # Begin an SGML Definition subsection
      	Write $outFile $token
      	ConvertSmlDefinitionSection $inFile $outFile $depth
      	set token ""
	break
      }
    }
    Write $outFile $token
  }
  return ""
}

set supportTagsWithSpaces 0 ;# 2013-09-23 Disable this, as this breaks compatibility with some XML files.

# Convert an SML element header. Returns the tag name, including the leading PI character.
proc ConvertSmlHeader {inFile outFile depth} {
  # We just read a <. The (optional) type and name are immediately behind.
  set type [GetChar $inFile]
  set name ""
  switch -- $type {
    "" { # End of file
    }
    ">" { # End of tag
      set type ""
    }
    "?" { # Processing Instruction
      GetSmlToken $inFile name
      Write $outFile "$type$name"
      ConvertSmlHeaderTokens $inFile $outFile $depth
    }
    "!" { # SGML Definition
      GetSmlToken $inFile name
      Write $outFile "$type$name"
      ConvertSmlHeaderTokens $inFile $outFile $depth
    }
    "#" {               # Comment
      set type "!"        ; # There's no end tag.
      set name {--}
      set comment [GetSmlComment $inFile]
      Write $outFile "!--${comment}--"
    }
    "<" {               # CDATA section begins with <[[
      set type "!"        ; # There's no end tag.
      set name "\[CDATA\["
      GetChars $inFile 2 ; # Eat up the end of the start mark.
      Write $outFile "!\[CDATA\[" ; # Output the sml CDATA begin mark
      if {"[PeekChar $inFile]" == "\n"} {
	GetChar $inFile ; # Remove the alignment new line, if any.
      }
      Write $outFile [GetCdata $inFile]
      Write $outFile "\]\]" ; # Output the CDATA end mark; the closing > will be later.
    }
    default { # Start tag
      UngetChars $inFile $type ; # OK, this was actually the first char of the tag name.
      set type ""
      set name [GetSmlToken $inFile]
      if ![string compare -nocase $name "body"] { 
        set ::supportTagsWithSpaces 0 ; # Heuristic: In HTML body, don't.
      }
      if {"[string index $name 0]" == "\""} {
        set name [EscapeTagName [string range $name 1 end-1]]
      }
      Write $outFile $name
      set type [ConvertSmlAttributes $inFile $outFile $depth] ; # "" or "/"
    }
  }
  return "$type$name"
}

set ::emptyErrorCount 0

# Convert an SML element
proc ConvertSmlElement {inFile outFile depth} {
  # Convert the header block
  Write $outFile "<"
  set tag [ConvertSmlHeader $inFile $outFile $depth]
  if {"$tag" == ""} { # Oops, not an element. Avoid an infinite loop.
    Write $outFile ">"
    incr ::emptyErrorCount
    if {$::emptyErrorCount > 3} error
    return ""
  }
  # Is there a content body?
  set type [string index $tag 0] ; # Tag type. Either "", "/", "!" or "?"
  switch -- $type {
    "/" - "!" - "?" { # These element types have no body.
      set tag [string range $tag 1 end]
    }
    "=" { # These have a CDATA body
      regexp {^(==?[:\{]?)(.*)} $tag - type tag
    }
    default {
      set type ""                                      ; # Assume a normal tag
    }
  }
  if {"$::docType" == "HTML"} {
    set ltag [string tolower $tag]
    if {[lsearch -exact $::endlessTags $ltag] != -1} { ; # Nope, has no closure.
      set type "!" ; # This special html tag does not have a body either.
    }
  }
  # If there is, convert it.
  switch -- $type {
    "" { # Normal XML element
      Write $outFile ">" ; # End the header
      # Convert the data and tail blocks
      ConvertSmlContent $inFile $outFile [expr $depth + 1]
      set c [PeekChar $inFile]
      set endSpaces ""
      if {"$c" == "\{"} { # Rare case with spaces in end tag
	GetChar $inFile ; # Eat up the {
	set endSpaces [GetBlanks $inFile]
	GetChar $inFile ; # Eat up the }
      }
      Write $outFile "</$tag$endSpaces>" ; # End the header
    }
    "?" { # Processing instruction
      Write $outFile "?>" ; # End the header
    }
    "!" { # SGML declaration
      Write $outFile ">" ; # End the header
    }
    "=" - "==" - "=\{" - "==\{" - "=:" - "==:" {
      Write $outFile ">" ; # End the header
      ConvertSmlXCdataSection $inFile $outFile $depth $type
      Write $outFile "</$tag>" ; # End the header
      set type "" ; # A normal tag as far as our caller is concerned.
    }
    "/" - default { # Empty element
      Write $outFile "/>" ; # End the header
    }
  }
  return "$type$tag" ; # Includes type in the 1st char.
}

# Convert the element contents.
proc ConvertSmlContent {inFile outFile depth} {
  set expectData 1
  while 1 {
    set c [PeekChar $inFile]
    switch -- $c {
      "" {                # End of file
	break
      }
      ";" {               # Element separator
	# XDebugPuts [list ConvertSmlContent: Discarding $c]
	GetChar $inFile      ; # Throw it away
	# Heuristic: Anything following an element on the same line is raw data
        set expectData 1
      }
      "\{" {		  # Indented CDATA block
	ConvertSmlXCdataSection $inFile $outFile $depth $c
      }
      "\}" {              # Block end
	# XDebugPuts [list ConvertSmlContent: Discarding $c]
	GetChar $inFile      ; # Throw it away
	break
      }
      "\n" {              # Element end
	Write $outFile [GetChar $inFile]
	# Heuristic: Anything after new lines is another element tag, not raw data
	if {$::supportTagsWithSpaces} {
          set expectData 0
        }
      }
      " " - "\t" {        # Spacing
	Write $outFile [GetChar $inFile]
      }
      "\"" {              # Quoted data OR tag name with spaces
        if {$expectData} {
          Write $outFile [string range [GetQuotedString $inFile] 1 end-1]
        } else {
          ConvertSmlElement $inFile $outFile $depth ; # Eat up one element
          # Heuristic: Anything following an element on the same line is raw data
          set expectData 1
        }
      }
      default {           # Anything else is an embedded element
	ConvertSmlElement $inFile $outFile $depth ; # Eat it up
	# Heuristic: Anything following an element on the same line is raw data
        set expectData 1
      }
    }
  }
  return
}

# Heuristics to identify an SML file
proc IsSml {inFile} {
  # Read just enough ASCII characters to ascertain this.
  # (Avoid peeking into characters that may change meaning if we change the encoding)
  set buf ""
  for {set n 0} {$n < 100} {incr n} {
    set c [PeekChar $inFile $n]
    append buf $c
    if [string is space -strict $c] continue
    if {"$c" == "<"} { # Looks like an XML tag.
      return 0 ;# Can't be SML
    }
    incr n
    break
  }
  # Now read more data until the end of the first SML tag
  for {} {$n < 100} {incr n} {
    set prev $c
    set c [PeekChar $inFile $n]
    append buf $c
    # End of SML tag is the first new line without a preceding \.
    if {[regexp {[\r\n]} "$c" -] && ![regexp {[\\\r\n]} $prev]} break
  }
  # Now look for the XML header
  DebugVars buf
  if {[regexp {^[ \t\r\n]*\?xml[ \t\r\n]} $buf -]} { # Official XML marker. No doubt.
    DebugPuts "Found the XML header"
    if [regexp {encoding[ \t\r\n]*=[ \t\r\n]*["'"]([^"'"]+)["'"]} $buf - encoding] {
      # Changing the encoding now is not a problem, because so far we've peeked
      # only at the XML header, which is guarantied to be only ASCII.
      set encoding [IANA2TclEncoding $encoding]
      fconfigure $inFile -encoding $encoding
    }
    return 1
  }
  # Insufficient to decide, read some more
  for {} {$n < 100} {incr n} {
    set c [PeekChar $inFile $n]
  }
  if {(![regexp {<} $buf -]) || ([regexp {<[][][][]} $buf -])} {
    return 1 ; # No XML or HTML tag, or there is an SML CDATA. Assume it's SML.
  }
  return 0
}

# Convert a whole file
proc sml2xml {inFile outFile} {
  set depth 0 ; # Current depth in the object tree.
  set ::docType XML ; # Assume XML
  if {"[string toupper [PeekChars $inFile 4]]" == "HTML"} { # The input is HTML
    set ::docType HTML ; # Change endlessTags behaviour to html compatible.
  }
  ConvertSmlContent $inFile $outFile $depth
  return 0
}

} ; # End of TraceProcs

###############################################################################
#                   Test XML -> SML -> XML Transformations                    #
###############################################################################

proc TestSml {args} {
  set targets $args
  if {"$targets" == ""} {
    set targets "*.xml *.xhtml *.xsl *.xsd *.xaml *.kml"
  }

  set sml $::argv0
  if {"$::tcl_platform(platform)" == "windows"} {
    set sml [list $::env(COMSPEC) /c $::argv0]
  }

  puts "# Converting XML -> SML -> XML, and comparing the initial and final version."
  set t0 [clock clicks -milliseconds]
  set success 1
  set err 0
  foreach file [eval glob -nocomplain $targets] {
    puts "$file"
    set basename [file rootname $file]
    set smlFile ${basename}.sml
    set file2 ${file}2
    set err [catch {
      Exec {*}$sml $file $smlFile
    } output]
    if {$err} {
      puts "Error. $output"
      set success 0
      continue
    }
    set err [catch {
      Exec {*}$sml $smlFile $file2
    } output]
    if {$err} {
      puts "Error. $output"
      set success 0
      continue
    }
    set err [catch {
      Exec diff $file $file2 >@stdout 2>@stderr
    } output]
    set err [ErrorCode $err]
    if {"$output" != ""} {
      set err 1
    }
    if {!$err} {
      puts OK
      file delete $file2 ;# No need to keep it since they're identical
    } else {
      puts "Failure. $file2 is different from $file"
      puts "" ;# # Add a blank line, to help see the failure line
      set success 0
    }
  }
  set t1 [clock clicks -milliseconds]

  set t [expr $t1 - $t0]
  puts "Tests completed in [format "%.2f" [expr $t / 1000.0]]s"
  if $success {
    puts "All tests successful"
  } else {
    puts "Errors found. Please review the messages above"
  }

  return $err
}

###############################################################################
#                                Main Routine                                 #
###############################################################################

# Main routine
set usage [subst -nobackslashes -nocommands {
Convert files from XML -> SML and back from SML -> XML.

Usage: $script <options> [infile [outfile]]

Options:
  -h | --help              This help screen
  -v | --verbose           Display more verbose information. (May be repeated).
  -V | --version           Display the script version.

infile:                    Input file pathname. Default: stdin

outfile:                   Output file pathname. Default: stdout
  *.ext                    Same as input_file, changing the extension to .ext.
  path*.ext                Same as input_file, changing both the path & extens.
}]

set args $argv
while {$args != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-d" - "--debug" {
      incr verbosity 2
    }
    "-h" - "--help" - "-?" {
      puts $usage
      exit 0
    }
    "-t" - "--test" {
      exit [eval TestSml $args]
    }
    "-v" - "--verbose" {
      incr verbosity
    }
    "-V" - "--version" {
      puts $version
      exit 0
    }
    "-X" - "--noexec" {
      set exec 0
    }
    default {
      if {"$inFileName" == ""} {
	set inFileName $arg
	if {"$inFileName" == "-"} continue ; # - = stdin
	set err [catch {open $inFileName r} inFile]
	if $err {
	  puts stderr "Error: Failed to open $inFileName. $inFile"
	  exit 1
	}
	continue
      }
      if {"$outFile" == "stdout"} {
	set outFileName $arg
	if {"$outFileName" == "-"} continue ; # - = stdout
	if [regexp {(.*)?\*\.([^\.]+)$} $outFileName - base ext] {
	  set outFileName $inFileName ; # Default if inFile has no extension.
	  if {"$base" != ""} {
	    set outFileName "$base[file tail $outFileName]"
	  }
	  regsub {\.[^\.]*$} $outFileName {} outFileName ; # Remove the extension
	  append outFileName ".$ext" ; # Append the new extension
          VerbosePuts "Outputing to file $outFileName"
	}
	set err [catch {open $outFileName w} outFile]
	if $err {
	  puts stderr "Error: Failed to open $outFileName. $outFile"
	  exit 1
	}
	continue
      }
      puts stderr "$script: Warning: Unknown option $arg ignored."
    }
  }
}

FlushBuf $inFile ; # Create the input buffer.

# First look for the special case of UTF-16 files
set oldEncoding [fconfigure $inFile -encoding]
fconfigure $inFile -encoding binary
# Read just enough byte characters to ascertain this.
# (Avoid peeking into characters that will change meaning when we change the encoding)
set byteOrderMark [GetChars $inFile 2]
foreach {char1 char2} [split $byteOrderMark ""] {}
set encoding ""
# Look for UTF-16 byte-order marks
if {("$byteOrderMark" == "\xFF\xFE") || ("$char2" == "\x00")} {
  set encoding "unicode" ; # Unicode little indian
  fconfigure $inFile -encoding $encoding
  if {"$char2" == "\x00"} {
    UngetChars $inFile $char1
    set byteOrderMark "\xFF\xFE"
  }
} elseif {("$byteOrderMark" == "\xFE\xFF") || ("$char1" == "\x00")} {
  set encoding "unicode big endian"
  puts stderr "Error: Encoding $encoding not supported."
  exit 1
  if {"$char1" == "\x00"} {
    UngetChars $inFile $char2
    set byteOrderMark "\xFE\xFF"
  }
} elseif {("$byteOrderMark" == "\xEF\xBB")} {
  append byteOrderMark [GetChar $inFile] ;# This byte order mark is 3-bytes long
  set encoding "utf-8"
  fconfigure $inFile -encoding $encoding
} else {
  set encoding "utf-8" ;# The default, unless overridden in the ?xml header
  fconfigure $inFile -encoding $encoding
  UngetChars $inFile $byteOrderMark
  set byteOrderMark ""
}  

if {[IsXml $inFile]} {       # The input is XML, and its encoding configured.
  VerbosePuts "The input is XML"
  set command xml2sml
} elseif {[IsSml $inFile]} { # The input is SML, and its encoding configured.
  VerbosePuts "The input is SML"
  set command sml2xml
} else {
  puts stderr "Error: Unrecognized input format"
  exit 1
}

set encoding [fconfigure $inFile -encoding]
VerbosePuts "The input encoding is $encoding"
if {("$outFile" == "stdout") && ("[fconfigure stdout -encoding]" == "unicode")} {
  set outIsConsole 1
  # Note: Stdout encoding must be left as unicode when writing to the console.
  #       It is cp1252 when redirected to a file or a pipe, and can be changed.
} else {
  set outIsConsole 0
}
# Send the same byte order mark to the output
if {("$byteOrderMark" != "") && (!$outIsConsole)} {
  if {"$encoding" != "unicode"} { # Unicode already does it automatically
    fconfigure $outFile -encoding binary
    puts -nonewline $outFile $byteOrderMark
  }
}
# Use the same encoding for the output as for the input
if {!$outIsConsole} {
  fconfigure $outFile -encoding $encoding
}
if {"$encoding" == "unicode"} {
  puts -nonewline $outFile "\uFEFF"
}
# Finally do the conversion
set err [catch {
  $command $inFile $outFile
} output]
if $err {
  puts stderr "Error: $output"
  exit 1
}
exit 0

