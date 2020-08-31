#!/usr/bin/tclsh
# -*- coding: utf-8 -*-
#-----------------------------------------------------------------------------#
#                                                                             #
#   Script name     FlipMails.tcl                                             #
#                                                                             #
#   Description     Reverse the order of mails in a mail thread.              #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2011-10-04 JFL Created this script.                                      #
#    2012-05-02 JFL Updated the debugging framework to its current version.   #
#                   Merged in the code from realign.tcl, and added options    #
#                   -r and -R to enable or disable its use.                   #
#    2012-09-12 JFL Use a trailing letter plus - as a continuation indicator. #
#                   Added option -f to force merging lines in more cases.     #
#    2013-02-08 JFL Fine tuned the numbered lists detection.                  #
#    2013-02-11 JFL Fixed a bug in the last change.                           #
#    2013-08-07 JFL Convert the ellipsis character to ...                     #
#    2013-08-26 JFL Convert the wide arrow character to =>                    #
#    2013-09-06 JFL Convert another kind of wide arrow character to ->        #
#    2014-09-15 JFL Added "Expéditeur" as another kind of mail separator.     #
#    2014-10-07 JFL Restructured to be language-independant.		      #
#                   Merge mail header lines, like long distribution lists.    #
#    2014-10-15 JFL Added a routine to remove > thread quoting.               #
#    2015-03-24 JFL Changed several DebugPuts showing variables to DebugVars. #
#                   Bug fix: Do not merge lines with just spaces with the     #
#		    next one. Leave an empty line instead.		      #
#    2016-02-25 JFL Added support for Asian mail headers with Unicode chars.  #
#    2016-04-17 JFL Improved French headers recognition.                      #
#    2018-09-11 JFL Changed the source encoding to utf-8.                     #
#                   Make sure the I/O encodings match the console code page.  #
#		    Recognize several new Unicode bullet types.		      #
#    2018-09-18 JFL If the mail has double interline, halve interlines.       #
#    2018-10-09 JFL Decode many common Unicode emoticons to ASCII art.        #
#    2019-05-13 JFL Remove URL defense call wrappers.			      #
#    2020-06-19 JFL Also reformat Yammer threads.
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
#-----------------------------------------------------------------------------#

# Set global defaults
set version "2020-06-22"

# Force running the script as UTF-8, if executed in a system with a different encoding.
# This is necessary because we have Unicode strings in this script encoded as UTF-8.
# And Tcl assumes the script is encoded using the default system encoding.
# This is generally true for Linux, which uses UTF-8 for everything;
# But in Windows the system encoding varies by localization. Ex: ANSI = CP1252 for USA.
if {([encoding system] != "utf-8") && (![info exists running_as_utf8])} {
  # puts "System encoding is [encoding system]; Retrying as utf-8"
  set running_as_utf8 1
  source -encoding "utf-8" $argv0
  return
}

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
  set retStrings {}   ; # Other return arguments, quoted
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
	lappend retStrings [CondQuote $arg]
      }
    }
  }
  set string [concat $retInst [join $retStrings] $strTime]
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
#    2007/06/27 JFL Added support for routines within namespaces.             #
#    2007/10/02 JFL Added the duration calculation, and display upon return.  #
#    2009/06/26 JFL Combined the new trace package using the standard Tcl     #
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

#-----------------------------------------------------------------------------#
#                          General purpose routines                           #
#-----------------------------------------------------------------------------#

# Set a string made from several concatenated parts
proc Set {name args} {
  upvar $name set.name
  set set.name ""
  foreach arg $args {
    append set.name $arg
  }
  return ${set.name}
}

# Read from a file
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

# Read a file, trimming end spaces, and returning an empty string on error.
proc TrimReadFile {filename {default ""}} {
  return [string trim [ReadFile $filename [list return $default]]]
}

# Write data to a file
proc WriteFile {name data {binary 0}} {
  catch {
    set hf [open $name w]
    if $binary {
      fconfigure $hf -translation binary
    }
    puts -nonewline $hf $data
    close $hf
  }
}

# Repeat action for each line in the buffer.
proc foreachlinenl {lineVar buf action} {
  upvar 1 $lineVar line
  set l [string length $buf] ; # Buffer length
  for {set i 0} {$i < $l} {set i [expr $m + 1]} { # For each line in the buffer
    set m [string first "\n" $buf $i]
    if {$m == -1} { # The last line may not have a trailing \n.
      set m $l ; # Should be "set m [expr $l - 1]" but faster, and OK with string range below.
    }
    set line [string range $buf $i $m]
    uplevel 1 $action
  }
  return ""
}

TraceProcs {

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    Realign                                        	      #
#                                                                             #
#   Description     Realignment routine					      #
#                                                                             #
#   Parameters      text        A block of text with multiple lines           #
#                   merge       0 = Don't merge lines                         #
#                               1 = Do merge split lines, using a heuristic.  #
#                               2 = Do merge lines more aggressively.         #
#                                                                             #
#   Returns 	    The realigned text                                	      #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2015-03-24 JFL Changed several DebugPuts showing variables to DebugVars. #
#                   Bug fix: Do not merge lines with just spaces with the     #
#		    next one. Leave an empty line instead.		      #
#                                                                             #
#-----------------------------------------------------------------------------#

proc Realign {text {merge 1}} {
  set result ""   ; # Output string
  set nTypes 0    ; # Number of distinct bullet types recognized so far
  set gotText 0   ; # 1 = Found a text line starting without a bullet
  set gotEmpty 0  ; # 1 = Found an empty line
  set endSpace 0  ; # 1 = Previous line ended with a space
  set firstLine 1 ; # 1 = This is the first line
  set nlRemoved 0 ; # 1 = A New Line character was removed from last line
  foreachlinenl line $text {
    DebugSVars "\n" line
    if [regexp {^\s+$} $line -] {
      DebugPuts "# Empty line changed to {}"
      set line ""
    }
    if [regexp {^ *((\S|\d{1,2}|[a-z]{1,2}|[A-Z]{1,2}|[ivxl]{1,5}|[IVXL]{1,5})([-.()<>_/\\:]?))(\t|    +)(.*)} $line - - bullet punct - tail] {
      # We're in a bulleted area
      set gotText 0
      set gotEmpty 0
      # Is this a known bullet type?
      for {set iType 0} {$iType < $nTypes} {incr iType} {
        DebugVars iType type($iType,rx)
        if [regexp -- $type($iType,rx) $bullet -] break ; # Yes it is.
      }
      if {$iType == $nTypes} { # Else this is a new bullet type, not encountered before.
        # Assume it's a special character, not in the special cases below
        set type($iType,rx) $bullet
        set type($iType,repl) $bullet
        # Check special bullet types, and convert them to ASCII art.
        # Note: Numeric and alphabetic types are often followed by a dot. Ex: 1.
        #       Other punctuation characters are also used occasionally. Ex: 1)
        # Note: When executed within an 8-bit code page, the Unicode characters are
        #       converted to characters in that code page. Ex:  becomes è
        #       Many Unicode characters are converted to ASCII '?'.
        foreach {rx repl} [list		  \
          {•|\*} "*"			  \
          {\?||} "-"			  \
          {\+|[""]|} "+"		  \
          {¢|ó|} ">"			  \
          {è|||} "=>"		  \
          {Ò||} "->"			  \
          {} "#"			  \
          {\d+[-.()<>_/\\:]?} {\1}	  \
          {[a-z]+[-.()<>_/\\:]?} {\1}	  \
          {[A-Z]+[-.()<>_/\\:]?} {\1}	  \
        ] {
          DebugVars rx repl
          if [regexp "^$rx$" $bullet -] {
            set type($iType,rx) $rx
            set type($iType,repl) $repl
            break
          }
        }
        incr nTypes
      }
      set indent [format "%*s" [expr 3 * $iType] ""]
      regsub {^ *(\S+)\s+} $line "$indent $type($iType,repl) " line
    } else { # We're not in a bulleted area anymore
      if [regexp {\S} $line -] {
        set gotText 1
      } else {
        set gotEmpty 1
      }
    }
    if {$gotText && $gotEmpty} { # We had a normal paragraph of text
      set nTypes 0 ; # Reset the bullet counter. Future bullets may be indented differently.
    }
    # Remove common smileys.
    # See list in https://en.wikipedia.org/wiki/List_of_emoticons#Western
    # Tcl regular expressions do not handle Unicode chars beyond \uFFFF:
    # They're handled as two 16-bits characters
    # => We can't use [☺🙂😊] ranges, as the leading word would be replaced in all chars in the same Unicode page. 
    # =>          Use |☺|🙂|😊 instead.
    regsub -all {|☺|🙂|😊} $line ":-)" line	;# Smiley, happy face
    regsub -all {😋} $line ":-)" line		;# Licking lips
    regsub -all {😀|😁} $line ":-\]" line	;# Very happy, showing teeth
    regsub -all {😃|😄} $line ":-D" line		;# Laughing
    regsub -all {😆} $line "X-D" line		;# Laughing with eyes crossed
    regsub -all {😍} $line "8-)" line		;# Loving smile
    regsub -all {|☹|🙁|😠} $line ":-("	 line	;# Frown
    regsub -all {😞|😟|🤬} $line ":-\[" line	;# Angry
    regsub -all {😡|😣|😖} $line ":-<" line	;# Pouting
    regsub -all {😢|😭} $line ":'-(" line	;# Crying
    regsub -all {😂} $line ":'-)" line		;# Tears of happiness
    regsub -all {😨|😧|😦|😱|😩} $line "8-(" line	;# Horror, sadness, dismay
    regsub -all {😫} $line "X-(" line		;# Disgust
    regsub -all {😮|😯|😲} $line ":-O" line	;# Surprise, shock
    regsub -all {😗|😙|😚|😘} $line ":-*" line	;# Kiss
    regsub -all {😉|😜|😘} $line ";-)" line	;# Wink
    regsub -all {😛|😝|😜|🤑} $line ":-P" line	;# Tongue sticking out
    regsub -all {🤔|😕|😟|🤨} $line ":-/" line	;# Skeptical
    regsub -all {|😐|😑} $line ":-|" line	;# Straight face
    regsub -all {😳|😞|😖} $line ":-$" line	;# Embarrassed
    regsub -all {🤐|😶} $line ":-X" line		;# Sealed lips
    regsub -all {😇|👼} $line "O:-)" line	;# Angel, innocent
    regsub -all {😈} $line "\}-)" line		;# Devilish
    regsub -all {😎} $line "B-)" line		;# Cool
    regsub -all {😪} $line "|-@" line		;# Yawn
    regsub -all {😏|😒} $line ":-J" line		;# Tongue-in-cheek
    regsub -all {😵|😕|🤕} $line "%-S" line	;# Drunk, confused
    regsub -all {🤒|😷|🤢} $line ":-#" line	;# Sick
    # Remove a non-marking space that the Console and Notepad display as garbage
    regsub -all {‎} $line ""	  line
    # Change fancy quotes to ASCII quotes
    regsub -all {[‘’‚‛′‵]} $line "'"	  line
    regsub -all {[“”„‟″‴‶‷]‎} $line "\""  line
    # Remove bars
    regsub -all {‐|‑|‒|–|—|―} $line "-"	  line
    regsub -all {▬} $line "==" line
    # Remove ellipsis characters, which cause ill-looking results in case there were 4 dots or more.
    regsub -all {…} $line "..."  line
    # Remove other common symbols
    regsub -all {☎|☏|✆|} $line {[Tel]} line	;# Telephone
    regsub -all {📱} $line {[Mob]} line		;# Mobile phone
    regsub -all {📠} $line {[Fax]} line		;# Fax machine
    regsub -all {✉|} $line {[Mail]} line	;# Physical mail
    regsub -all {📧} $line {[E-Mail]} line	;# E-Mail
    # Remove other microsoft-specific symbols
    # Note: Replacements for list-item bullets are done in another section further up!
    regsub -all {|} $line "<-" line
    regsub -all {|} $line "->" line
    regsub -all {|} $line "<=" line
    regsub -all {|} $line "=>" line
    regsub -all {} $line "<=>" line
    #
    # Remove URL defense call wrappers
    #
    # Ex: https://urldefense.proofpoint.com/v2/url?u=https-3A__www.yammer.com_hpe.com_threads_143059452674048-3Ftrk-5Fevent-3Dcom-5Fthread-5Fclick-26allow-5Fapp-5Fredirect-3D1&d=DwMFaQ&c=C5b8zRQO1miGmBeVZ2LFWg&r=HeQwHVgKUDuMuuGjAnjtmPYqoHH-FWAG3YagsV7NNT8&m=Lo_HyNRlWa4H4Gnl9czTTJtdHpNYXxjE7a6bNwUHbyw&s=BUsJfCNVVlg5qE86Z6ItGQwDlr0Ohkh9KhPQwhO8yMM&e=
    package require uri::urn
    set rxUrl {https://urldefense.proofpoint.com/v2/url\?([][\w.~!*'();:@&=+$,/?#%-]+)}
    DebugVars line
    foreach {url query} [regexp -all -inline -line $rxUrl $line] {
      DebugVars url
      DebugVars query
      foreach pair [split $query "&"] {
        DebugVars pair
      	foreach {- name value} [regexp -inline {([^=]+)=(.*)} $pair] {
	  DebugVars name value
      	  if {$name == "u"} {
      	    regsub -all "_" $value "/" value
      	    regsub -all -- "-" $value "%" value
      	    set url2 [uri::urn::unquote $value]
      	    set first [string first $url $line]
      	    set length [string length $url]
      	    set last [expr {$first + $length - 1}]
      	    set line [string replace $line $first $last $url2]
	    DebugVars url2 line
      	  }
      	}
      }
    }
    # Output the modified line
    DebugVars merge endSpace
    if {!$merge} { # The simple case: Output one line for every input line
      append result $line
    } else { # Check if merging lines is necessary
      if {!$firstLine} {
        if {$merge == 2} { # In brute force mode, merge all lines ending with a space
          if {!$endSpace} { 
            append result \n
          }
        } else { # In smart mode, restrict merges to lines with continuations starting with a lower case letter.
          if {(!$endSpace) || (![regexp {^[a-z]} $line -])} {
            append result \n
          }
        }
      }
      set endSpace [regexp {(\w-|\s)\r?\n} $line -]
      DebugPuts endSpace
      set nlRemoved [regsub {\r?\n} $line {} line1]
      append result $line1
    }
    set firstLine 0
  }
  if {$nlRemoved} {
    append result \n
  }
  return $result
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    DeQuote                                        	      #
#                                                                             #
#   Description     >Quotation removal routine				      #
#                                                                             #
#   Parameters      input       Raw email thread                              #
#                                                                             #
#   Returns 	    Corrected email thread, with > Quotations removed.	      #
#                                                                             #
#   Notes:	    Some (mostly Linux?) mail readers prefix every line in    #
#                   the email thread they respond to with a > character.      #
#                                                                             #
#                   Most such readers insert a header line ahead of the       #
#                   >Quoted thread describing the mail date and sender.       #
#                   Other readers do not insert anything, not even a header.  #
#                   Ignore the latter case, because this is indistinguishable #
#                   from a manual quote, with text from the original thread   #
#                   manually copied into the response.                        #
#                                                                             #
#   History:								      #
#    2014-10-13 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set blank {[[:blank:]\xA0]}
set headerMarker "Header:::\nHeader:::\nHeader:::"

proc DeQuote {input} {
  upvar #0 blank blank
  upvar #0 headerMarker headerMarker
  set rxQuotedLine {>[[:print:]]*}
  set rxQuotedLines "(?:$rxQuotedLine)(?:\r?\n$rxQuotedLine)*"
  # The quoted thread is normally preceded by a quoting line like:
  # On Feb 11, 2014, at 10:42 AM, "Mickey" <mickey.mouse@disney.com> wrote:
  # Le 12 novembre 2014 15:37, Axel Curt <curtaxel@yahoo.fr> a écrit :
  set rxQuote {[^>\n][^\n]+\d:\d\d[^\n]+:[[:blank:]]*}
  set rxQuoteAndQuotedLines "\r?\n$rxQuote\r?\n$blank*?\r?\n?$rxQuotedLines"
  # Quoting already quoted text sometimes causes further line splitting
  set rxSplitQuotedLine {^(> >.*[^[:blank:]])[[:blank:]]*\r?\n>[[:blank:]]?([^>[:blank:]].*)}

  foreach quotedBlock [regexp -all -inline -line $rxQuoteAndQuotedLines $input] {
    DebugVars quotedBlock
    set quotedBlock0 $quotedBlock
    # Mark the 1-line header for easier identification later on
    if {![regsub "\r?\n($rxQuote)\r?\n" $quotedBlock "\n$headerMarker\\1\n\n" quotedBlock]} {
      # set quotedBlock "$headerMarker\n\n$quotedBlock" ;# Insert a marker if there was no 1-line header
    }
    # Merge lines that were split due to double >quotation levels
    set n [regsub -all -line $rxSplitQuotedLine $quotedBlock {\1 \2} quotedBlock]
    if {$n} {
      DebugPuts "Merged $n lines split due to double >quotations"
    }
    # Remove 1 quotation level
    regsub -all -line {^>[[:blank:]]?} $quotedBlock {} quotedBlock2
    # Remove further quotation levels, if any
    set quotedBlock2 [DeQuote $quotedBlock2]
    DebugVars quotedBlock2
    # Change the block within the input
    regsub "***=$quotedBlock0" $input $quotedBlock2 input
  }

  return $input
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    FormatYammerThread                             	      #
#                                                                             #
#   Description     Format a Yammer thread				      #
#                                                                             #
#   Parameters      text        A block of text with multiple lines           #
#                                                                             #
#   Returns 	    The reformatted thread, with separators between posts     #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2020-06-19 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc FormatYammerThread {text} {
  # Remove \r to be sure
  regsub -all "\r?\n" $text "\n" text
  # Fix the initial author line
  set rx {^\n*(New: )?([^\n]+?) FollowFollow (\2[^\n]+?)\n}
  regsub -all $rx $text "\\3\n\n\n\n" text
  # Merge the duplicate author lines in each reply, and add separator lines between posts
  set rx {(New: )?([^\n]+?)( in reply to [^\n]+?)? [-–] [^\n]+?\n[^\n]*?\n\2[^\n]*? [-–] ([^\n]+?)\n}
  regsub -all $rx $text "\n$::dashLine\n\n\\2\\3 - \\4\n\n\n\n" text
  # Halve the number of blank lines in the body of each post
  regsub -all "\n\n" $text "\n" text
  # Remove the collapse links
  regsub -all {‹ collapse\n} $text "\n" text
  # Remove LIKE prompts, etc
  set rx {\n LIKE like this message  REPLY reply to this message  SHARE share this message[^\n]+\n}
  regsub -all $rx $text "\n\n" text
  # Remove repeated attached links
  set rx {\nAttached link[^\n]+?\. Click to open in new tab\.\n([^\n]+\n)+More link options}
  regsub -all $rx $text "" text
  # Remove the extra blank lines in the end of each post
  regsub -all "\n+${::dashLine}\n+" $text "\n\n$::dashLine\n\n" text
  # Remove the counter, etc, at the end of the first thread
  set rx {\nSeen by \d+\n(\s*\d+ shares\n)?(Add Topics\n)?((Show \d+ previous replies\n)?\n-{50})}
  regsub $rx $text "\n\\3" text
  # Warn if there are hidden posts
  set n 0
  set rx {Show (\d+) previous replies\n}
  regexp $rx $text - n
  if {$n} {
    puts stderr "Warning: There are $n hidden posts in this thread"
    regsub $rx $text {} text
  }
  # Warn if there are unexpanded posts
  set n [regexp -all {expand ›\n} $text]
  if {$n} {
    puts stderr "Warning: There are $n unexpanded posts in this thread"
  }
  # Done
  return $text
}

} ;# End of TraceProcs section

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
#    2011-10-04 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
$script version $version

Reformat text from an email thread, putting the most recent mails at the bottom.
Realign and indent bullet items, based on a set of heuristics. (Optional)
Merge split lines, based on a set of heuristics. (Optional)
Add a horizontal line between each mail. 

Usage: $script [OPTIONS]

Options:
  -D, --dequote     Dequote >quoted threads
  -h, --help, -?    Display this help screen.
  -f, --force       Force merging lines, even when normal merge would not
  -m, --merge       Merge lines in the same paragraph (default)
  -M, --nomerge     Do not try merging lines
  -Q, --nodequote   Do not dequote >quoted threads
  -r, --realign     Realign bullet items and merge split lines (Default)
  -R, --norealign   Do not realign bullet items and merge split lines
  -v, --verbose     Verbose attributes
  -V, --version     Display this script version and exit
}]

# Scan all arguments.
set args $argv
set noOptions 0
set realign 1
set merge 1
set dequote 1
set yammer 1
while {"$args" != ""} {
  set arg [PopArg]
  if {[string match -* $arg] && !$noOptions} {
    switch -- $arg {
      "--" {
	set noOptions 1
      }
      "-d" - "--debug" {
	SetDebug
      }
      "-D" - "--dequote" { # Dequote >quoted threads
	set dequote 1
      }
      "-f" - "--force" {		# Merge lines, even when normal merge would not
        set merge 2
      }
      "-h" - "--help" - "-?" {
	puts -nonewline $usage
	exit 0
      }
      "-m" - "--merge" {		# Merge lines part of the same paragraph
        set merge 1
      }
      "-M" - "--nomerge" {	# Merge lines part of the same paragraph
        set merge 0
      }
      "-q" - "--quiet" { # Verbose flag
	SetQuiet
      }
      "-Q" - "--nodequote" { # Do not dequote >quoted threads
	set dequote 0
      }
      "-r" - "--realign" { # Realign flag
	set realign 1
      }
      "-R" - "--norealign" { # No Realign flag
	set realign 0
      }
      "-v" - "--verbose" { # Verbose flag
	SetVerbose
      }
      "-V" - "--version" { # Version flag
	puts $version
	exit 0
      }
      "-xd" - "--extradebug" {
	SetXDebug
      }
      default {
        puts stderr "Unkonwn option: $arg. Ignored."
      }
    }
  } else {
    puts stderr "Unrecognized switch ignored: $arg"
  }
}

set dashLine ------------------------------------------------------------------------

set input [read stdin]

# To do: Remove > continuation characters, and merge split lines
if {$dequote} {
  set input [DeQuote $input]
  # puts -nonewline $input
  # exit 1
}

# Realign paragraphs and (possibly multi-level) bullet lists
if {$realign} {
  set input [Realign $input $merge]
}

# Cleanup Yammer threads
if {$yammer && [regexp {LIKE like this message  REPLY reply to this message  SHARE share this message} $input -]} {
  set input [FormatYammerThread $input]
}

# Recognize mail separators
set rxSeparator1 {[[:blank:]]*---+[[:blank:]]*[[:upper:]][[:print:]]+[[:lower:]][[:blank:]]*---+[[:blank:]]*}
set rxSeparator2 {[[:blank:]]*------+[[:blank:]]*}
set rxSeparator3 {[[:blank:]]*______+[[:blank:]]*}
set rxSeparator "$rxSeparator1|$rxSeparator2|$rxSeparator3|\r?\n"

# Regognize mail header lines
# A header line begins with a tag, followed by a colon, then an optional value. Ex: "From: Your boss"
# The tag is usually a capitalized word. Ex: From, To, De, À, Von, An, ...
# But there are cases when the tab contains 2 words. Ex: "Signed By: A cautious person" or "Envoyé le: Di, 17 Avr 2016 11:05"
# Some translations add a space before the colon. Ex: "Envoyé : mercredi 17 septembre 2014 11:25"
# Some versions insert a special space \xA0 before the colon. This matches [[:space:]], but not [[:blank:]].
# Asian languages use ideograms that end up as ? in the code page 1252.
set rxHeaderLine1 {[[:blank:]]*[?[:upper:]][?[:lower:]]*(?: [?[:upper:][:lower:]][?[:lower:]]*)?[[:blank:]\xA0]?:[[:print:]\t]*}
set rxHeaderLineContinued {[[:graph:]][[:print:]\t]*}
# A header line is anything beginning with a header line 1, following by any non-blank lines that is NOT a header line 1.
set rxHeaderLine "${rxHeaderLine1}(?:\r?\n(?!${rxHeaderLine1})$rxHeaderLineContinued)*"
# And there seems to be always at least 3 header lines. Ex: From, Date, Subject
set rxHeaderLines "(?:$rxHeaderLine)(?:\r?\n$rxHeaderLine){2,}"

# A mail header is an optional separator, followed by a number of header lines.
# Note that it's the presence of header lines immediately behind the separator that allow to avoid false positives.
set rxHeader "(?:(?:$rxSeparator)(?:$blank*\r?\n)+)(?:$rxHeaderLines)"

# Note: The initial implementation had a built-in list of known separators,
#       and of known header line tags.
# The drawback was that this list was necessarily incomplete, as different mail readers use different wordings.
# Also it had to be localized in every language.
set rxSeparators [list \
  {---+ ?Mail [Oo]riginal ?---+}  \
  "---+ ?Mail d'origine ?---+" \
  {---+ ?Mail [Tt]ransféré ?---+} \
  "---+ ?Message d'origine ?---+" \
  {---+ ?Message [Tt]ransféré ?---+} \
  "---+ ?Original Message ?---+" \
  "---+ ?Original Appointment ?---+" \
  "---+ ?Original.Nachricht ?---+" \
]
set rxTags [join "^From\\\\s*: ^De\\\\s*: ^Expéditeur\\\\s*: ^Von\\\\s*: $rxSeparators" |]

# Search for all the mail headers in the text.
set rx "$rxHeader"
# Insert blank lines ahead of the text, as a dummy initial separator;
# Append a dummy header after the text, to mark the end of the last mail.
# include a --- line in that trailing header, in case the last mail itself ends with a --- line.
set input "\n\n$input\n\n----------\nFrom:\nTo:\nDate:\nSubject:\n"
# Some mail clients use "Begin forwarded message:" instead of a line separator
regsub -all "Begin forwarded message:" $input "-------------" input
set ixs [regexp -all -inline -indices $rx $input]
DebugVars rx ixs
set ix0 0
set ix1 0
set mails {}
foreach pair $ixs {
  DebugVars pair
  foreach {ix2 ix3} $pair {
    if {$ix1 != 0} { # Start the real work once we have the location of TWO headers.
      set header [string range $input $ix0 $ix1]
      set mail [string range $input [expr $ix1 + 1] [expr $ix2 - 1]]
      DebugVars header mail
      # Now drop the separator line, and merge header lines split on multiple lines.
      set header2 ""
      foreach headerLine [regexp -all -inline $rxHeaderLine $header] {
	DebugVars headerLine
	regsub -all "\r?\n" $headerLine "" headerLine
	append header2 "$headerLine\n"
      }
      regsub {\s*$} $mail "\n\n" mail ; # Make sure each mail ends with 2 LFs
      regsub {^\s*\n} $mail "" mail ; # Make sure there are no empty lines ahead
      regsub "$headerMarker" $header2 "" header2 ;# Remove the special marker added by DeQuote().
      DebugVars header2
      # Check if the mail has double interline
      set nSingle [regexp -all {[^\n]\r?\n[^\r\n]} $mail -]
      set nDouble [regexp -all {[^\n]\r?\n\r?\n[^\r\n]} $mail -]
      set nQuad   [regexp -all {[^\n]\r?\n\r?\n\r?\n\r?\n[^\r\n]} $mail -]
      if {$nQuad > $nDouble} { # Yes it does. Halve interline!
      	regsub -all {(\r?\n)\r?\n} $mail {\1} mail
      	regsub {(\r?\n)$} $mail {\1\1} mail ; # But restore the trailing double interline
      }
      lappend mails "$header2\n$mail"
    } else { # If the text begins by with unstructured block of text, process it too.
      set text [string range $input 0 [expr $ix2 - 1]]
      set text [string trim $text]
      DebugVars text
      if {"$text" != ""} {
      	lappend mails "$text\n\n"
      }
    }
    set ix0 $ix2
    set ix1 $ix3
  }
}
set mails [lreverse $mails]
set mails [join $mails "$dashLine\n\n"]
puts -nonewline "$dashLine\n\n$mails$dashLine\n\n"

