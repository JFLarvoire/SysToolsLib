#!/usr/bin/tclsh
#*****************************************************************************#
#                                                                             #
#   File name	    DebugLib.tcl                                              #
#                                                                             #
#   Description     A library of Tcl output, logging, and debugging routines  #
#                                                                             #
#   Notes:	    This module defines a set of functions and variables in   #
#                   the ::debug namespace.				      #
#                   Then it continues with a set of self-test routines,       #
#                   used for demonstrating the usage of the debug library,    #
#                   and for basic testing of itself as a standalone script.   #
#                                                                             #
#                   Usage as a debug library:                                 #
#                   source debuglib.tcl                                       #
#                   ::debug::Import ; # Optional: Import public names         #
#                                                 from the ::debug namespace  #
#                                                                             #
#                   Then enclose your own routines in TraceProcs {} blocks,   #
#                   and add various Puts, DebugPuts, etc, calls in your code  #
#                   as needed. See the section headers further down for       #
#                   details about the available functions.                    #
#                                                                             #
#                   Finally, define command-line options for conditionally    #
#                   enabling debug and log modes, as shown in the main        #
#                   routine at the end of the self-test section below.        #
#                                                                             #
#                   Note that that whole self-test section is safely ignored  #
#                   when loading DebugLib.tcl with a source directive.        #
#                                                                             #
#                   Alternative usage as a debug library:                     #
#                   Instead of sourcing this as an external module,           #
#                   copy the whole namespace ::debug definition,              #
#                   and paste it at the beginning of your standalone script.  #
#                                                                             #
#                   Usage as a standalone self-test script:                   #
#                   Invoke with the -? option to display a help screen with   #
#                   all available options.                                    #
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
#   History:								      #
#    2005       JFL Started a debug framework for various personal tools.     #
#                   Initially cut and paste from one tool to the next.        #
#    2007-01-16 JFL Integrated debuglib.tcl in SFS, for debugging o2ib.       #
#    2009-09-03 JFL Created this standalone version, for easy reuse.          #
#    2009-09-30 JFL Added the Exec routine, with debug instrumentation.       #
#    2010-04-06 JFL Abandoned PutString in favor of the more standard Puts.   #
#    2011-02-16 JFL Added conditional execution and logging routine Do.       #
#                   Moved all command line reformating to DebugArgLine.       #
#    2011-09-09 JFL Renamed XxxxString as XxxxPuts.                           #
#                   Changed the log mechanism to work better with Expect.     #
#    2011-10-18 JFL Minor tweaks of the expect logging mechanism.             #
#    2014-10-24 JFL Added the ability to change the debug output channel.     #
#    2014-11-06 JFL Bug fix: Function Escape needs to escape [brackets].      #
#    2014-11-13 JFL Added functions SetExec, SetNoExec, and NoExec.           #
#                   Changed "stream" to "channel" everywhere.                 #
#    2014-11-14 JFL Rewrote the Escape and CondQuote routines.                #
#		    Added a workaround for debug issues on old Linux systems. #
#                   Bug fix: Return traces did escape the return value twice. #
#    2014-11-18 JFL Removed an old instance of Which, not used anymore.       #
#                   Fixed OpenLogChannel, which failed outside of expect.     #
#                   Changed the default log directory selection algorithm.    #
#                   Fixed a few bugs in the new version of CondQuote & Escape.#
#                   Fixed NowMS on older Tcl versions with 32-bits time maths.#
#    2014-11-26 JFL Use CondQuote in PutVars.                                 #
#                   Added option -f to Do.                                    #
#                   Added BSD-style license in the header.                    #
#    2015-06-22 JFL Fixed bug in routine bg.                                  #
#                                                                             #
###############################################################################

namespace eval ::debug {
  set version "2015-06-22"
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

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    ErrorCode                                    	      #
#                                                                             #
#   Description     Get the exit code from the last external program executed.#
#                                                                             #
#   Parameters      err	      The TCL error caught when executing the program.#
#                             Optional; Defaut: -1 (An invalid exit code,     #
#                             allowing to detect non-completion cases.)       #
#                                                                             #
#   Returns 	    0 to 255 for the exit code.                               #
#                   $err if program did not complete. (-1 if no arg specified)#
#                   If -1, get the err. descr. from [lindex $::errorCode end].#
#                                                                             #
#   Notes:          TCL's exec only updates ::errorCode if it throws an error.#
#                                                                             #
#   Common error cases, with the first word of ::errorCode in each case:      #
#   "POSIX" when the program does not exists, or failed to start.             #
#   "CHILDKILLED" when it ran but was killed by a signal.                     #
#   "CHILDSTATUS" when it ran and exited with an error..                      #
#   "NONE" when it ran and exited with no error, but did output on stderr.    #
#                                                                             #
#   Usage 1: Get the exit code. Drawback: Cannot distinguish non-completion.  #.
#   set err [catch {exec program arguments} output]                           #
#   set ret [ErrorCode $err] ; # Returns 0 to 255. (1 if not completed).      #
#                                                                             #
#   Usage 2: Distinguish non-completion cases. Drawback: More complex code.   #
#   set err [catch {exec program arguments} output]                           #
#   if {$err && ($::errorCode != NONE)} { # Non-completion or exit with error.#
#     set ret [ErrorCode] ; # Returns 1 to 255, or -1 if not completed.       #
#     if {$ret == -1} { puts stderr [lindex $::errorCode end] ; set ret 1 }   #
#   } else { set ret 0 ; # success, even if {$err && {$::errorCode == NONE}} }#
#                                                                             #
#   History:								      #
#    2004       JFL Created this routine.                                     #
#    2005/09/08 JFL Updated to return 0 in the NONE case. Added header doc.   #
#                                                                             #
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

# Breakpoint handler. Includes a minimal debugger.
# arg1 = Breakpoint name. Optional.
xvariable bp_skip {} ; # List of breakpoint names to ignore.
xproc bp {{s {}}} {
  variable bp_skip
  if {[lsearch -exact $bp_skip $s]>=0} return
  if [catch {info level -1} who] {set who ::}
  while 1 {
    puts -nonewline "$who/$s> "; flush stdout
    gets stdin line
    switch -- $line {
      "c" {puts "continuing.."; break}
      "h" - "?" {puts "
c   Continue
h   This help screen
i   Display local variables"; continue}
      "i" {set line "info locals"; continue}
      default {}
    }
    catch {uplevel 1 $line} res
    puts $res
  }
}

# Log background errors. Redefine ::bgerror to call this one.
xproc bgerror {msg} {
  variable script
  set msg "Background error: $msg.\nError code = $::errorCode\n$::errorInfo"
  catch { # Make sure there's not a double error!
    Puts -log 1 -noprefix stderr "$script: $msg"
  }
}

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

###############################################################################
#                         Test of the above routines                          #
###############################################################################

# If this module is invoked directly...
if {"[file tail [info script]]" == "[file tail $::argv0]"} {

debug::Import

TraceProcs { # Trace the main test routines!

proc TestNoReturn {args} {
  # Do nothing, return nothing
}

proc TestReturn {args} {
  eval return $args
}

proc TestReturnReturn {args} {
  eval TestReturn -code return $args
  error "TestReturnReturn failed: TestReturn should have triggered a return here."
}

proc TestReturnBreak {} {
  for {set i 0} {$i < 1} {incr i} {
    TestReturn -code break
    error "TestReturnBreak failed: TestReturn should have prevented arriving here."
  }
  if {$i} {
    error "TestReturnBreak failed: TestReturn should have prevented the completion of the first loop."
  }
  DebugPuts "# TestReturnBreak succeeded"
}

proc TestReturnContinue {} {
  for {set i 0} {$i < 1} {incr i} {
    TestReturn -code continue
    error "TestReturnContinue failed: TestReturn should have prevented arriving here."
  }
  if {!$i} {
    error "TestReturnContinue failed: TestReturn should have forced the completion of the first loop."
  }
  DebugPuts "# TestReturnContinue succeeded"
}

proc TestError {args} {
  eval error $args
}

proc TestERROR {args} {
  eval Error $args
}

proc TestSleep {ms} {
  after $ms
  return
}

proc TestExec {args} {
  set err [catch {eval Exec $args} out]
  puts "err = $err"
  puts "returned = [list $out]"
}

proc TestAll {args} {
  TestNoReturn
  TestReturn
  TestReturn 1
  TestReturn -code ok 1
  TestReturnReturn
  TestReturnBreak
  TestReturnContinue
  catch { TestError "I told you so!" }
  catch { TestERROR "Don't complain if it hurts!" }
  TestSleep 500
}

} ; # End of traced routines

proc TestLogCB {msg} {     # Do not put it in TraceProcs, else infinite loop!
  puts "TestLogCB: $msg" ; # Do not use Puts, else infinite loop!
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
#    2008/12/01 JFL Created this routine, based on hpls_ilo.                  #
#    2008/12/18 JFL Added the ability to specify ranges of nodes.             #
#                                                                             #
#-----------------------------------------------------------------------------#

# Usage string
set usage [subst -nobackslashes -nocommands {
Debug library routines test

Usage: $script [OPTIONS] [TESTACTION]

Options:
-c, --callback        Test the log callback, sending the output to screen
-e, --exec ARGS       Test the Exec routine
-d, --debug           Enable the debug mode, and display debug messages
-d0                   Disable debug output to the console
-d1                   Send debug output to stdout (default)
-d2                   Send debug output to stderr
-h, --help, -?        Display this help screen and exit.
-l, --log FILENAME    Define a log file
-q, --quiet           Display only critical messages
-v, --verbose         Display verbose messages
-X, --noexec          Display, but do not execute, external command lines

Test Actions:         (Use -v or -d to see the effect.)
all                   Run all self-test routines
TESTNAME [args]       Run a single test

}]

# Process arguments
set args $argv
set action { puts $usage }
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-c" - "--callback" { # 
      SetLogCallBack TestLogCB
    }
    "-d" - "--debug" { # debug flag.
      SetDebug
    }
    "-d0" - "--debug2nul" - "--debug2null" {
      SetDebugChannel [OpenNull]
    }
    "-d1" - "--debug2stdout" {
      SetDebugChannel stdout
    }
    "-d2" - "--debug2stderr" {
      SetDebugChannel stderr
    }
    "-d8" - "--utf8isbuggy" {
      set debug::utf8isbuggy 1
    }
    "-e" - "--exec" { # debug flag.
      set action [linsert $args 0 TestExec]
      set args ""
    }
    "-h" - "--help" - "-?" - "/?" {
      puts $usage
      exit 0
    }
    "-l" - "--log" {
      OpenLogChannel [PopArg]
    }
    "-q" - "--quiet" { # Quiet flag
      SetQuiet
    }
    "-v" - "--verbose" { # Verbose flag.
      SetVerbose
    }
    "-V" - "--version" { # Version.
      puts "$::debug::version"
      exit 0
    }
    "-xd" - "--xdebug" {
      SetXDebug
    }
    "-X" - "--noexec" { # Display commands, but don't execute them
      SetNoExec
    }
    default {
      if {[string index $arg 0] == "-"} {
	puts stderr "Unsupported option: $arg"
	puts stderr "Run \"$script -?\" to display a help screen with the list of all options."
	exit 1
      }
      if {![string compare -nocase $arg "all"]} {
      	set arg TestAll
      }
      set action [concat [list $arg] $args]
      break
    }
  }
}

# See the set of available test routines, just before the main routine header.

Puts "(by Puts) Starting tests"
VerbosePuts "(by VerbosePuts) At [Now]"
DebugSVars "(by DebugSVars) Command line arguments:" argc argv
if [XDebug] {
  PutSVars "(by if \[XDebug\] PutSVars) Environment:" env
}
if [catch {
  eval $action
} msg] {
  puts "Error: $msg"
  exit 1
}  
exit 0
}

