#!/usr/bin/tclsh
#-----------------------------------------------------------------------------#
#                                                                             #
#   Script name     cascade.tcl                                               #
#                                                                             #
#   Description     Arrange PuTTY windows in a regular series.                #
#                                                                             #
#   Notes           Uses the twapi library, available at                      #
#                   http://twapi.magicsplat.com/                              #
#                                                                             #
#   History                                                                   #
#    2008-08-22 JFL Created this script.                                      #
#    2008-10-27 JFL Adjust the steps to avoid going beyond the screen limits. #
#    2009-10-20 JFL Adapted to TWAPI 2.1.                                     #
#    2012-09-26 JFL Use GetSystemMetrics available in TWAPI 2.2.2.            #
#                   Tested from Windows XP to Windows 8. (Win8 misaligned)    #
#    2016-04-17 JFL Added option -x to force the X indent.                    #
#    2017-08-31 JFL Bugfix: get_window_coordinates and minimize_window may    #
#                   throw exceptions.					      #
#                   Added a -V|--version option.                              #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
#-----------------------------------------------------------------------------#

# Set defaults
set version "2017-08-31"

set err [catch {
  set twapiVersion [package require twapi]
} output]
if $err {
  puts stderr "Error: The required package twapi is missing. See: http://twapi.magicsplat.com"
  exit 1
}
# namespace import twapi::*
::twapi::import_commands

if [package vsatisfies $twapiVersion 2.2.2-] {
  set GetNonClientMetrics GetNonClientMetrics2 ; # Use the new version based on twapi::GetSystemMetrics
} elseif [package vsatisfies $twapiVersion 2.0-] {
  set GetNonClientMetrics GetNonClientMetrics1 ; # Use the intermediate version based on twapi::SystemParametersInfo
} else {
  set GetNonClientMetrics GetNonClientMetrics0 ; # Use the old version based on twapi::get_system_parameters_info
}

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

#=============================================================================#
#                         Other useful debug routines                         #
#=============================================================================#

# Conditionally execute a potentially dangerous Tcl command. Log it.
# Similar in spirit to Exec, but for internal commands.
xvariable exec 1
xproc Do {args} {
  variable exec
  if [Debug] {
    DebugPuts -1 [DebugArgLine $args]
  } elseif {!$exec} {
    Puts [DebugArgLine $args]
  }
  if {$exec} {
    uplevel 1 $args
  }
}

} ; # End of namespace

###############################################################################
#                        End of debug library routines                        #
###############################################################################

debug::Import

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    bscan                                        	      #
#                                                                             #
#   Description     Scan a binary C structure into a Tcl list.                #
#                                                                             #
#   Parameters      format   A format string for the Tcl binary scan function #
#                   data     The binary data to scan                          #
#                                                                             #
#   Returns 	    A Tcl list                                                #
#                                                                             #
#   Notes:	    The problem comes from that fact that the Tcl binary scan #
#                   function expects one variable argument per format item.   #
#                   It's like the C scanf routine, whereas we want a single   #
#                   Tcl list as the output.                                   #
#                                                                             #
#                   Drawbacks:                                                #
#                   - ASCII strings may be interpreted as an N-element list,  #
#                     instead of a single string.                             #
#                   - Will produce an invalid list if an ASCII string         #
#                     happens to be an invalid Tcl list.                      #
#                                                                             #
#   History:								      #
#    2008-08-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc bscan {format data} {
  set vars [regexp -all -inline -indices {[a-wA-WyYzZ]\d*} $format]
  eval binary scan \$data \$format $vars
  set result {}
  foreach var $vars {
    set result [concat $result [set $var]]
  }
  return $result
}

#-----------------------------------------------------------------------------#
#                        Application-specific routines                        #
#-----------------------------------------------------------------------------#

proc Ncm2Xncm {metrics} {
  set vars {
    cb borderWidth scrollWidth scrollHeight captionWidth captionHeight 
    smallWidth smallHeight menuWidth menuHeight paddedBorderWidth
  }
  foreach $vars $metrics break
  if [Debug] {
    foreach var $vars {PutVars $var}
  }
  # For Vista, vertical border width is actually $borderWidth + 3.
  #            Caption height is actually $borderWidth + $captionHeight + 4
  #            $captionWidth is slightly more than a caption button width
  set dx [expr (2 * $captionWidth) + $borderWidth - 9]
  set dy [expr $borderWidth + $captionHeight + 4]

  set wFrame [expr $borderWidth + 3]
  set yCaption [expr $captionHeight + 1]

  #             xBorder  yBorder    xFrame  yFrame    yCaption   xSysMenu  ySysMenu     xClose  yClose
  return [list     1        1      $wFrame $wFrame   $yCaption  $menuWidth $menuHeight $captionWidth $captionHeight ]
}

# Initial version using twapi 1.x low level proc get_system_parameters_info
proc GetNonClientMetrics0 {} {
  # Make sure get_system_parameters_info has been initialized at least once.
  if {![info exists twapi::SystemParametersInfo_uiactions_get]} {
    get_system_parameters_info GETBEEP
  }
  # Add the entry for SPI_GETNONCLIENTMETRICS if missing.
  # Notes:
  #   Structure NONCLIENTMETRICS size is 340 in ASCII mode and 500 in Unicode mode.
  #   The 6 first integers are:
  #     cbSize; iBorderWidth; iScrollWidth; iScrollHeight; iCaptionWidth; iCaptionHeight;
  if {![info exists twapi::SystemParametersInfo_uiactions_get(SPI_GETNONCLIENTMETRICS)]} {
    set twapi::SystemParametersInfo_uiactions_get(SPI_GETNONCLIENTMETRICS) {0x0029 sz i6 500 cbsize}
  }
  # Return the array
  set metrics [get_system_parameters_info GETNONCLIENTMETRICS]
  Ncm2Xncm $metrics
}

# Version compatible with twapi 2 versions < 2.2.2
proc GetNonClientMetrics1 {} {
  set sz 504
  set mem [twapi::malloc $sz]
  # Clear the whole structure
  twapi::Twapi_WriteMemoryBinary $mem 0 $sz [binary format a$sz ""]
  # Check Windows version as up to XP it was only 500 bytes long.
  set winVer [get_os_version]
  set sz0 $sz
  if {[lindex $winVer 0] < 6} {
    set sz 500
  }
  twapi::Twapi_WriteMemoryBinary $mem 0 $sz [binary format i $sz]
  twapi::SystemParametersInfo 0x29 $sz $mem 0
  set data [twapi::Twapi_ReadMemoryBinary $mem 0 $sz0]
  set format i6x92i2x92i2x276i
  set metrics [bscan $format $data]
  Ncm2Xncm $metrics
}

# Version requiring twapi versions >= 2.2.2
proc GetNonClientMetrics2 {} {
  set cxBorder [twapi::GetSystemMetrics $::SM_CXBORDER]
  set cyBorder [twapi::GetSystemMetrics $::SM_CYBORDER]
  set cxEdge [twapi::GetSystemMetrics $::SM_CXEDGE]
  set cyEdge [twapi::GetSystemMetrics $::SM_CYEDGE]
  set cxFrame [twapi::GetSystemMetrics $::SM_CXFRAME]
  set cyFrame [twapi::GetSystemMetrics $::SM_CYFRAME]
  set cxSize [twapi::GetSystemMetrics $::SM_CXSIZE]
  set cySize [twapi::GetSystemMetrics $::SM_CYSIZE]
  set cxIcon [twapi::GetSystemMetrics $::SM_CXICON]
  set cyIcon [twapi::GetSystemMetrics $::SM_CYICON]
  set cxSmIcon [twapi::GetSystemMetrics $::SM_CXSMICON]
  set cySmIcon [twapi::GetSystemMetrics $::SM_CYSMICON]
  set cxMenuSize [twapi::GetSystemMetrics $::SM_CXMENUSIZE]
  set cyMenuSize [twapi::GetSystemMetrics $::SM_CYMENUSIZE]
  # set cxCaption [twapi::GetSystemMetrics $::SM_CXCAPTION]
  set cyCaption [twapi::GetSystemMetrics $::SM_CYCAPTION]
  if [Debug] {
    foreach var {cxBorder cyBorder cxEdge cyEdge cyCaption cxFrame cyFrame cxSize cySize cxIcon cyIcon cxSmIcon cySmIcon cxMenuSize cyMenuSize} {PutVars $var}
  }
  #             xBorder  yBorder    xFrame  yFrame    yCaption   xSysMenu  ySysMenu     xClose  yClose
  return [list $cxBorder $cxBorder $cyFrame $cxFrame $cyCaption $cxMenuSize $cyMenuSize $cxSize $cySize]
}

set SM_ARRANGE 56	; # The flags that specify how the system arranged minimized windows. For more information, see the Remarks section in this topic.
set SM_CLEANBOOT 67	; # The value that specifies how the system is started: 0 Normal boot; 1 Fail-safe boot; 2 Fail-safe with network boot. A fail-safe boot (also called SafeBoot, Safe Mode, or Clean Boot) bypasses the user startup files.
set SM_CMONITORS 80	; # The number of display monitors on a desktop. For more information, see the Remarks section in this topic.
set SM_CMOUSEBUTTONS 43	; # The number of buttons on a mouse, or zero if no mouse is installed.
set SM_CXBORDER 5	; # The width of a window border, in pixels. This is equivalent to the SM_CXEDGE value for windows with the 3-D look.
set SM_CXCURSOR 13	; # The width of a cursor, in pixels. The system cannot create cursors of other sizes.
set SM_CXDLGFRAME 7	; # This value is the same as SM_CXFIXEDFRAME.
set SM_CXDOUBLECLK 36	; # The width of the rectangle around the location of a first click in a double-click sequence, in pixels. The second click must occur within the rectangle that is defined by SM_CXDOUBLECLK and SM_CYDOUBLECLK for the system to consider the two clicks a double-click. The two clicks must also occur within a specified time. To set the width of the double-click rectangle, call SystemParametersInfo with SPI_SETDOUBLECLKWIDTH.
set SM_CXDRAG 68	; # The number of pixels on either side of a mouse-down point that the mouse pointer can move before a drag operation begins. This allows the user to click and release the mouse button easily without unintentionally starting a drag operation. If this value is negative, it is subtracted from the left of the mouse-down point and added to the right of it.
set SM_CXEDGE 45	; # The width of a 3-D border, in pixels. This metric is the 3-D counterpart of SM_CXBORDER.
set SM_CXFIXEDFRAME 7	; # The thickness of the frame around the perimeter of a window that has a caption but is not sizable, in pixels. SM_CXFIXEDFRAME is the height of the horizontal border, and SM_CYFIXEDFRAME is the width of the vertical border. This value is the same as SM_CXDLGFRAME.
set SM_CXFOCUSBORDER 83	; # The width of the left and right edges of the focus rectangle that the DrawFocusRect draws. This value is in pixels. Windows 2000:  This value is not supported.
set SM_CXFRAME 32	; # This value is the same as SM_CXSIZEFRAME.
set SM_CXFULLSCREEN 16	; # The width of the client area for a full-screen window on the primary display monitor, in pixels. To get the coordinates of the portion of the screen that is not obscured by the system taskbar or by application desktop toolbars, call the SystemParametersInfo function with the SPI_GETWORKAREA value.
set SM_CXHSCROLL 21	; # The width of the arrow bitmap on a horizontal scroll bar, in pixels.
set SM_CXHTHUMB 10	; # The width of the thumb box in a horizontal scroll bar, in pixels.
set SM_CXICON 11	; # The default width of an icon, in pixels. The LoadIcon function can load only icons with the dimensions that SM_CXICON and SM_CYICON specifies.
set SM_CXICONSPACING 38	; # The width of a grid cell for items in large icon view, in pixels. Each item fits into a rectangle of size SM_CXICONSPACING by SM_CYICONSPACING when arranged. This value is always greater than or equal to SM_CXICON.
set SM_CXMAXIMIZED 61	; # The default width, in pixels, of a maximized top-level window on the primary display monitor.
set SM_CXMAXTRACK 59	; # The default maximum width of a window that has a caption and sizing borders, in pixels. This metric refers to the entire desktop. The user cannot drag the window frame to a size larger than these dimensions. A window can override this value by processing the WM_GETMINMAXINFO message.
set SM_CXMENUCHECK 71	; # The width of the default menu check-mark bitmap, in pixels.
set SM_CXMENUSIZE 54	; # The width of menu bar buttons, such as the child window close button that is used in the multiple document interface, in pixels.
set SM_CXMIN 28		; # The minimum width of a window, in pixels.
set SM_CXMINIMIZED 57	; # The width of a minimized window, in pixels.
set SM_CXMINSPACING 47	; # The width of a grid cell for a minimized window, in pixels. Each minimized window fits into a rectangle this size when arranged. This value is always greater than or equal to SM_CXMINIMIZED.
set SM_CXMINTRACK 34	; # The minimum tracking width of a window, in pixels. The user cannot drag the window frame to a size smaller than these dimensions. A window can override this value by processing the WM_GETMINMAXINFO message.
set SM_CXPADDEDBORDER 92 ; # The amount of border padding for captioned windows, in pixels. Windows XP/2000:  This value is not supported.
set SM_CXSCREEN 0	; # The width of the screen of the primary display monitor, in pixels. This is the same value obtained by calling GetDeviceCaps as follows: GetDeviceCaps( hdcPrimaryMonitor, HORZRES).
set SM_CXSIZE 30	; # The width of a button in a window caption or title bar, in pixels.
set SM_CXSIZEFRAME 32	; # The thickness of the sizing border around the perimeter of a window that can be resized, in pixels. SM_CXSIZEFRAME is the width of the horizontal border, and SM_CYSIZEFRAME is the height of the vertical border. This value is the same as SM_CXFRAME.
set SM_CXSMICON 49	; # The recommended width of a small icon, in pixels. Small icons typically appear in window captions and in small icon view.
set SM_CXSMSIZE 52	; # The width of small caption buttons, in pixels.
set SM_CXVIRTUALSCREEN 78 ; # The width of the virtual screen, in pixels. The virtual screen is the bounding rectangle of all display monitors. The SM_XVIRTUALSCREEN metric is the coordinates for the left side of the virtual screen.
set SM_CXVSCROLL 2	; # The width of a vertical scroll bar, in pixels.
set SM_CYBORDER 6	; # The height of a window border, in pixels. This is equivalent to the SM_CYEDGE value for windows with the 3-D look.
set SM_CYCAPTION 4	; # The height of a caption area, in pixels.
set SM_CYCURSOR 14	; # The height of a cursor, in pixels. The system cannot create cursors of other sizes.
set SM_CYDLGFRAME 8	; # This value is the same as SM_CYFIXEDFRAME.
set SM_CYDOUBLECLK 37	; # The height of the rectangle around the location of a first click in a double-click sequence, in pixels. The second click must occur within the rectangle defined by SM_CXDOUBLECLK and SM_CYDOUBLECLK for the system to consider the two clicks a double-click. The two clicks must also occur within a specified time. To set the height of the double-click rectangle, call SystemParametersInfo with SPI_SETDOUBLECLKHEIGHT.
set SM_CYDRAG 69	; # The number of pixels above and below a mouse-down point that the mouse pointer can move before a drag operation begins. This allows the user to click and release the mouse button easily without unintentionally starting a drag operation. If this value is negative, it is subtracted from above the mouse-down point and added below it.
set SM_CYEDGE 46	; # The height of a 3-D border, in pixels. This is the 3-D counterpart of SM_CYBORDER.
set SM_CYFIXEDFRAME 8	; # The thickness of the frame around the perimeter of a window that has a caption but is not sizable, in pixels. SM_CXFIXEDFRAME is the height of the horizontal border, and SM_CYFIXEDFRAME is the width of the vertical border. This value is the same as SM_CYDLGFRAME.
set SM_CYFOCUSBORDER 84	; # The height of the top and bottom edges of the focus rectangle drawn by DrawFocusRect. This value is in pixels. Windows 2000:  This value is not supported.
set SM_CYFRAME 33	; # This value is the same as SM_CYSIZEFRAME.
set SM_CYFULLSCREEN 17	; # The height of the client area for a full-screen window on the primary display monitor, in pixels. To get the coordinates of the portion of the screen not obscured by the system taskbar or by application desktop toolbars, call the SystemParametersInfo function with the SPI_GETWORKAREA value.
set SM_CYHSCROLL 3	; # The height of a horizontal scroll bar, in pixels.
set SM_CYICON 12	; # The default height of an icon, in pixels. The LoadIcon function can load only icons with the dimensions SM_CXICON and SM_CYICON.
set SM_CYICONSPACING 39	; # The height of a grid cell for items in large icon view, in pixels. Each item fits into a rectangle of size SM_CXICONSPACING by SM_CYICONSPACING when arranged. This value is always greater than or equal to SM_CYICON.
set SM_CYKANJIWINDOW 18	; # For double byte character set versions of the system, this is the height of the Kanji window at the bottom of the screen, in pixels.
set SM_CYMAXIMIZED 62	; # The default height, in pixels, of a maximized top-level window on the primary display monitor.
set SM_CYMAXTRACK 60	; # The default maximum height of a window that has a caption and sizing borders, in pixels. This metric refers to the entire desktop. The user cannot drag the window frame to a size larger than these dimensions. A window can override this value by processing the WM_GETMINMAXINFO message.
set SM_CYMENU 15	; # The height of a single-line menu bar, in pixels.
set SM_CYMENUCHECK 72	; # The height of the default menu check-mark bitmap, in pixels.
set SM_CYMENUSIZE 55	; # The height of menu bar buttons, such as the child window close button that is used in the multiple document interface, in pixels.
set SM_CYMIN 29		; # The minimum height of a window, in pixels.
set SM_CYMINIMIZED 58	; # The height of a minimized window, in pixels.
set SM_CYMINSPACING 48	; # The height of a grid cell for a minimized window, in pixels. Each minimized window fits into a rectangle this size when arranged. This value is always greater than or equal to SM_CYMINIMIZED.
set SM_CYMINTRACK 35	; # The minimum tracking height of a window, in pixels. The user cannot drag the window frame to a size smaller than these dimensions. A window can override this value by processing the WM_GETMINMAXINFO message.
set SM_CYSCREEN 1	; # The height of the screen of the primary display monitor, in pixels. This is the same value obtained by calling GetDeviceCaps as follows: GetDeviceCaps( hdcPrimaryMonitor, VERTRES).
set SM_CYSIZE 31	; # The height of a button in a window caption or title bar, in pixels.
set SM_CYSIZEFRAME 33	; # The thickness of the sizing border around the perimeter of a window that can be resized, in pixels. SM_CXSIZEFRAME is the width of the horizontal border, and SM_CYSIZEFRAME is the height of the vertical border. This value is the same as SM_CYFRAME.
set SM_CYSMCAPTION 51	; # The height of a small caption, in pixels.
set SM_CYSMICON 50	; # The recommended height of a small icon, in pixels. Small icons typically appear in window captions and in small icon view.
set SM_CYSMSIZE 53	; # The height of small caption buttons, in pixels.
set SM_CYVIRTUALSCREEN 79 ; # The height of the virtual screen, in pixels. The virtual screen is the bounding rectangle of all display monitors. The SM_YVIRTUALSCREEN metric is the coordinates for the top of the virtual screen.
set SM_CYVSCROLL 20	; # The height of the arrow bitmap on a vertical scroll bar, in pixels.
set SM_CYVTHUMB 9	; # The height of the thumb box in a vertical scroll bar, in pixels.
set SM_DBCSENABLED 42	; # Nonzero if User32.dll supports DBCS; otherwise, 0.
set SM_DEBUG 22		; # Nonzero if the debug version of User.exe is installed; otherwise, 0.
set SM_DIGITIZER 94	; # Nonzero if the current operating system is Windows 7 or Windows Server 2008 R2 and the Tablet PC Input service is started; otherwise, 0. The return value is a bitmask that specifies the type of digitizer input supported by the device. For more information, see Remarks. Windows Server 2008, Windows Vista, and Windows XP/2000:  This value is not supported.
set SM_IMMENABLED 82	; # Nonzero if Input Method Manager/Input Method Editor features are enabled; otherwise, 0. SM_IMMENABLED indicates whether the system is ready to use a Unicode-based IME on a Unicode application. To ensure that a language-dependent IME works, check SM_DBCSENABLED and the system ANSI code page. Otherwise the ANSI-to-Unicode conversion may not be performed correctly, or some components like fonts or registry settings may not be present.
set SM_MAXIMUMTOUCHES 95 ; # Nonzero if there are digitizers in the system; otherwise, 0. SM_MAXIMUMTOUCHES returns the aggregate maximum of the maximum number of contacts supported by every digitizer in the system. If the system has only single-touch digitizers, the return value is 1. If the system has multi-touch digitizers, the return value is the number of simultaneous contacts the hardware can provide. Windows Server 2008, Windows Vista, and Windows XP/2000:  This value is not supported.
set SM_MEDIACENTER 87	; # Nonzero if the current operating system is the Windows XP, Media Center Edition, 0 if not.
set SM_MENUDROPALIGNMENT 40 ; # Nonzero if drop-down menus are right-aligned with the corresponding menu-bar item; 0 if the menus are left-aligned.
set SM_MIDEASTENABLED 74 ; # Nonzero if the system is enabled for Hebrew and Arabic languages, 0 if not.
set SM_MOUSEPRESENT 19	; # Nonzero if a mouse is installed; otherwise, 0. This value is rarely zero, because of support for virtual mice and because some systems detect the presence of the port instead of the presence of a mouse.
set SM_MOUSEHORIZONTALWHEELPRESENT 91 ; # Nonzero if a mouse with a horizontal scroll wheel is installed; otherwise 0.
set SM_MOUSEWHEELPRESENT 75 ; # Nonzero if a mouse with a vertical scroll wheel is installed; otherwise 0.
set SM_NETWORK 63	; # The least significant bit is set if a network is present; otherwise, it is cleared. The other bits are reserved for future use.
set SM_PENWINDOWS 41	; # Nonzero if the Microsoft Windows for Pen computing extensions are installed; zero otherwise.
set SM_REMOTECONTROL 0x2001 ; # This system metric is used in a Terminal Services environment to determine if the current Terminal Server session is being remotely controlled. Its value is nonzero if the current session is remotely controlled; otherwise, 0. You can use terminal services management tools such as Terminal Services Manager (tsadmin.msc) and shadow.exe to control a remote session. When a session is being remotely controlled, another user can view the contents of that session and potentially interact with it.
set SM_REMOTESESSION 0x1000 ; # This system metric is used in a Terminal Services environment. If the calling process is associated with a Terminal Services client session, the return value is nonzero. If the calling process is associated with the Terminal Services console session, the return value is 0. Windows Server 2003 and Windows XP:  The console session is not necessarily the physical console. For more information, see WTSGetActiveConsoleSessionId.
set SM_SAMEDISPLAYFORMAT 81 ; # Nonzero if all the display monitors have the same color format, otherwise, 0. Two displays can have the same bit depth, but different color formats. For example, the red, green, and blue pixels can be encoded with different numbers of bits, or those bits can be located in different places in a pixel color value.
set SM_SECURE 44	; # This system metric should be ignored; it always returns 0.
set SM_SERVERR2 89	; # The build number if the system is Windows Server 2003 R2; otherwise, 0.
set SM_SHOWSOUNDS 70	; # Nonzero if the user requires an application to present information visually in situations where it would otherwise present the information only in audible form; otherwise, 0.
set SM_SHUTTINGDOWN 0x2000 ; # Nonzero if the current session is shutting down; otherwise, 0. Windows 2000:  This value is not supported.
set SM_SLOWMACHINE 73	; # Nonzero if the computer has a low-end (slow) processor; otherwise, 0.
set SM_STARTER 88	; # Nonzero if the current operating system is Windows 7 Starter Edition, Windows Vista Starter, or Windows XP Starter Edition; otherwise, 0.
set SM_SWAPBUTTON 23	; # Nonzero if the meanings of the left and right mouse buttons are swapped; otherwise, 0.
set SM_TABLETPC 86	; # Nonzero if the current operating system is the Windows XP Tablet PC edition or if the current operating system is Windows Vista or Windows 7 and the Tablet PC Input service is started; otherwise, 0. The SM_DIGITIZER setting indicates the type of digitizer input supported by a device running Windows 7 or Windows Server 2008 R2. For more information, see Remarks.
set SM_XVIRTUALSCREEN 76 ; # The coordinates for the left side of the virtual screen. The virtual screen is the bounding rectangle of all display monitors. The SM_CXVIRTUALSCREEN metric is the width of the virtual screen.
set SM_YVIRTUALSCREEN 77

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    Cascade                                        	      #
#                                                                             #
#   Description     Redraw all applications instance windows in a cascade.    #
#                                                                             #
#   Parameters      app                Application name. Ex: putty.exe        #
#                   x                  Horizontal position of the 1st window  #
#                   y                  Vertical position of the first window  #
#                   dx                 Horizontal shift between windows       #
#                   dy                 Vertical shift between windows         #
#                                                                             #
#   Returns 	    0 if success                                              #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2008-08-22 JFL Created this routine.                                     #
#    2017-08-31 JFL Bugfix: get_window_coordinates and minimize_window may    #
#                   throw exceptions.					      #
#                                                                             #
#-----------------------------------------------------------------------------#

proc Cascade {app x y dx dy} {
  set pids [get_process_ids -glob -name $app]
  set hWnds {}
  set nWnds 0
  foreach pid $pids {
    set hWnd [find_windows -pids $pid -toplevel 1 -visible 1 -single]
    lappend hWnds $hWnd
    incr nWnds
  }
  # Find the biggest window
  set maxWidth 0
  set maxHeight 0
  foreach hWnd $hWnds {
    if {[catch {
      foreach {left top right bottom} [get_window_coordinates $hWnd] break
    }]} {
      foreach {left top right bottom} {0 0 0 0} break
    }
    set width [expr $right - $left]
    set height [expr $bottom - $top]
    if {$width > $maxWidth} {
      set maxWidth $width
    }
    if {$height > $maxHeight} {
      set maxHeight $height
    }
  }
  # Will the cascade fit?
  # Get the screen usable area size
  foreach {- - width height} [get_system_parameters_info SPI_GETWORKAREA] break
  set xMax [expr $width - $x]
  set yMax $height
  set xLast [expr $x + (($nWnds - 1) * $dx) + $maxWidth]
  set yLast [expr $y + (($nWnds - 1) * $dy) + $maxHeight]
  if {$xLast > $xMax} {
    incr dx -[expr ($xLast + ($nWnds - 1) - $xMax) / $nWnds]
    DebugPuts "Changed dx to $dx"
  }
  if {$yLast > $yMax} {
    incr dy -[expr ($yLast + ($nWnds - 2) - $yMax) / ($nWnds - 1)]
    DebugPuts "Changed dy to $dy"
  }
  # Move all windows
  foreach pid $pids {
    catch {
      set hWnd [find_windows -pids $pid -toplevel 1 -visible 1 -single]
      minimize_window $hWnd -sync ; # Makes sure the show_window below redraws it.
      show_window $hWnd -normal -sync -activate
      move_window $hWnd $x $y ; # Note: Does not work when minimized.
    }
    incr x $dx
    incr y $dy
  }
  return 0
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
#    2008-08-22 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set app putty.exe
set x [get_system_parameters_info SPI_ICONHORIZONTALSPACING] ; # Leave 1 icon column
set y 0
set dx 0
set dy 0
set noexec 0        ; # 1=Do not execute the changes
set dxForce 0

set usage [subst -nobackslashes -nocommands {
Cascade all instance windows of a given application.

Usage: $argv0 [OPTIONS] [PROGRAM]

Options:
  -f, --from X Y    Start point. Default: $x $y
  -h, --help, -?    Display this help screen.
  -s, --step DX DY  Increment step. Default: $dx $dy
  -v, --verbose     Verbose attributes
  -V, --version     Display this script version
  -x, --xindent     Horizontal indentation. Default: Width of the system button

Program:            Program name. Default: putty.exe. Default extension: .exe
}]

# Scan all arguments.
set args $argv
set noOptions 0
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-d" - "--debug" { # debug flag.
      incr ::debug::verbosity 2
    }
    "-f" - "--from" {
      set x [PopArg]
      set y [PopArg]
    }
    "-g" - "--getproc" { # Test the alternative GetNonClientMetrics implementations
      set GetNonClientMetrics GetNonClientMetrics[PopArg]
    }
    "-h" - "--help" - "-?" {
      puts -nonewline $usage
      exit 0
    }
    "-s" - "--step" {
      set dx [PopArg]
      set dy [PopArg]
    }
    "-v" - "--verbose" { # Verbose flag.
      incr ::debug::verbosity
    }
    "-V" - "--version" {	# Display this library version
      puts $version
      exit 0
    }
    "-x" - "--xindent" {
      set dxForce [PopArg]
    }
    "-X" - "--noexec" {
      set noexec 1
    }
    default {
      if [string match -* $arg] {
	puts stderr "Unknown option: $arg. Ignored."
      } else {
	set app $arg.exe
	regsub {\.exe\.exe} $app {.exe} app
      }
    }
  }
}

if {$dx == 0 && $dy == 0} {
  # Get the screen usable area size
  foreach {- - width height} [get_system_parameters_info SPI_GETWORKAREA] break
  DebugVars width
  DebugVars height
  # Default to one caption bar vertically, and 2 caption buttons horizontally.
  set metrics [$GetNonClientMetrics]
  set vars {xBorder yBorder xFrame yFrame yCaption xSysMenu ySysMenu xClose yClose}
  foreach $vars $metrics break
  if [Debug] {
    Puts ""
    foreach var $vars {PutVars $var}
    Puts ""
  }
  set dy [expr $yFrame + $yCaption]
  # set dx [expr $xFrame + $xSysMenu + $xBorder]
  # set dx [expr ($dy * 4) / 3] 
  # align the next minimize button with the previous close button
  # set dx [expr (2 * $captionWidth) + $borderWidth - 9]
  set dx [expr (2 * $xClose) - (2 * $xBorder)]
  # Correct for style differences between 2000/XP-style and Vista/7/8 style
  if {$xClose > $xSysMenu} {
    incr dx [expr -2 * $xBorder]
  } else {
    incr dy [expr -1 * $yBorder]
  }
}

if $dxForce {
  set dx $dxForce
}

if [Verbose] {
  foreach var {app x y dx dy} {PutVars $var}
}

if {$noexec} {
  exit 0
}

Cascade $app $x $y $dx $dy

