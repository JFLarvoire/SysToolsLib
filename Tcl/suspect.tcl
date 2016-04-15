#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   File name	    suspect.tcl                                               #
#                                                                             #
#   Description     Pure Tcl subset of Expect. A library and a test program.  #
#                                                                             #
#   Notes:	    This was not as straightforward as it seemed!             #
#                                                                             #
#   History:								      #
#    2009/06/18 JFL Created this script.                                      #
#                                                                             #
###############################################################################

###############################################################################
#                         Test of the above routines                          #
###############################################################################

# If this module is invoked directly...
if {   [info exists use_suspect_test_routines] 
    || ("[file tail [info script]]" == "[file tail $::argv0]")} {

# Set global defaults
set script [file tail $argv0]   ; # This script name.

set verbosity 1			; # 0=Quiet 1=Normal 2=Verbose 3=Debug
proc Normal {} {expr $::verbosity > 0}
proc Verbose {} {expr $::verbosity > 1}
proc Debug {} {expr $::verbosity > 2}
proc XDebug {} {expr $::verbosity > 3}
proc XXDebug {} {expr $::verbosity > 4}

# Select the log file and create the log directory.
if {[info exists env(USER)] && ("$env(USER)" == "root")} {
  set logDir /var/log/$script
} else {
  set logDir ~/log/$script
}
file mkdir $logDir
set logFile $logDir/${script}.log       ; # A copy of all output goes there.

#*****************************************************************************#
#                          General purpose routines                           #
#*****************************************************************************#

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

# Remove an argument from the tail of a routine argument list.
proc PopLast {{name args}} {
  upvar 1 $name args
  set arg [lindex $args end]            ; # Extract the first list element.
  set args [lreplace $args end end]     ; # Remove the last list element.
  return $arg
}

# Check if a procedure exists
proc ProcExists {p} {
  expr [lsearch -exact [info procs $p] $p] != -1
}

# Build a time stamp with the current time.
proc Now {{sep " "}} { # For ISO 8601 strict compatibility, use sep "T".
  clock format [clock seconds] -format "%Y-%m-%d$sep%H:%M:%S"
}

# Idem with milli-seconds.
proc NowMS {{sep " "}} {  # For ISO 8601 strict compatibility, use sep "T".
  set ms [clock clicks -milliseconds]
  set s  [expr $ms / 1000]
  set ms [expr $ms % 1000]
  format "%s.%03d" [clock format $s -format "%Y-%m-%d$sep%H:%M:%S"] $ms
}

# Find a program among optional absolute pathnames, else in the PATH.
switch $tcl_platform(platform) { # Platform-specific PATH delimiter
  "windows" {
    set pathDelim ";"
    set pathExts {.com .exe .bat .cmd} ; # Default if not explicitely defined
  }
  "unix" - default {
    set pathDelim ":"
    set pathExts {} ; # Unix does not have that notion
  }
}
if [info exists ::env(PATHEXT)] { # Windows list of implicit program extensions
  set pathExts [split $::env(PATHEXT) $pathDelim]
}
proc Which {prog args} { # prog=Program Name; args=Optional absolute pathnames
  if [info exists ::env(PATH)] { # May not exist when started as a service.
    set paths [split $::env(PATH) $::pathDelim]
    if {"$::tcl_platform(platform)" == "windows"} {
      set paths [linsert $paths 0 ""] ; # Search in the current directory first
    }
    foreach path $paths {
      lappend args [file join $path $prog]
    }
  }
  foreach name $args {
    if [file executable "$name"] {
      return "$name"
    }
    foreach ext $::pathExts {
      if [file executable "$name$ext"] {
	return "$name$ext"
      }
    }
  }
  return ""
}

#-----------------------------------------------------------------------------#
#                          Debug output and logging                           #
#-----------------------------------------------------------------------------#
# Note: This is a stripped-down version of my usual Tcl debug framework.

# Routine for outputing strings, indented in debug mode.
proc PutString {args} {
  set string [PopLast]
  set indent [expr [info level] - 1]
  set putArgs {}
  foreach arg $args {
    switch -- $arg {
      "-1" {
	incr indent -1
      }
      default {
	lappend putArgs $arg
      }
    }
  }
  if [Debug] {
    set string "[NowMS] [pid] [IndentString $string [expr $indent * 2]]"
  }
  lappend putArgs $string
  eval puts $putArgs
}

# Routine for outputing debug strings
proc DebugString {args} {
  if [Debug] {
    eval PutString -1 $args
  }
}

# Escape a string. ie. change special string charaters to \c & \xNN sequences.
# Does the reverse of {subst -nocommands -novariables $text}
proc Escape {text} {
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

# Indent multiple lines
proc IndentString {text {indent 2}} {
  set spaces [string repeat " " $indent]
  regsub -all -line {^.+$} $text "$spaces&" text ; # Indent all non-empty lines.
  return $text
}

# Mimimum of N numbers
proc Min {min args} {
  foreach arg $args {
    if {$arg < $min} {
      set min $arg
    }
  }
  return $min
}

# Maximum of N numbers
proc Max {max args} {
  foreach arg $args {
    if {$arg > $max} {
      set max $arg
    }
  }
  return $max
}

# Format array contents with one element (name value) per line
proc FormatArray {a} {
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
  set average [expr $total / $n]
  set limit [expr $average + 10] ; # Reasonable limit to avoid oversize names
  set width [Min $maxLen $limit] ; # Choose the smaller of the two.
  # Output the data using that column width
  foreach {name} $names {
    append string [format "%-${width}s %s\n" $name [list $a1($name)]]
  }
  return $string
}

# Routine for outputing variable values
proc VarsValue {args} {
  set l {}
  foreach arg $args {
    set arg [list $arg] ; # Make sure this survives the uplevel calls below.
    if {![uplevel 1 info exists $arg]} {       # Undefined variable
      lappend l "$arg=!undefined!"
    } elseif {[uplevel 1 array exists $arg]} { # Array name
      lappend l "$arg=array{\n[IndentString [uplevel 1 FormatArray $arg]]}"
    } else {                                   # Scalar variable
      lappend l "$arg=[list [uplevel 1 set $arg]]"
    }
  }
  Escape [join $l " "]
}

# Routine for outputing variable values
proc PutVars {args} {
  PutString -1 [uplevel 1 eval VarsValue $args]
}

# Routine for outputing variable values
proc PutSVars {msg args} {
  PutString -1 "$msg [uplevel 1 VarsValue $args]"
}

# Routine for outputing variable values
proc DebugVars {args} {
  if [Debug] {
    PutString -1 [uplevel 1 VarsValue $args]
  }
}

# Routine for outputing a message and variable values
proc DebugSVars {msg args} {
  if [Debug] {
    PutString -1 "$msg [uplevel 1 VarsValue $args]"
  }
}

# Log a call to an external program
proc Exec {args} {
  if [Debug] { # Redundant test, but avoids useless tcl2sh if false.
    DebugString -1 [tcl2sh $args]
  }
  set err [catch {
    eval exec $args
  } output]
  set err [ErrorCode $err] ; # Convert the Tcl error to the shell error
  if {$err >= 0} {
    DebugString -1 "  exit $err"
  } else {
    DebugString -1 "  [lindex $::errorCode end]" ; # Get the error description
  }
  if $err {
    error $output
  }
  return $output
}

#-----------------------------------------------------------------------------#
#                              Execution tracing                              #
#                                                                             #
# Procedures                                                                  #
#   TraceString     Internal routine through which all trace output goes.     #
#   ShortenString   Limits the size of strings displayed. Ex. for huge args.  #
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
#-----------------------------------------------------------------------------#

namespace eval ::trace {

# Trace output routine. Redefine to integrate in other debug frameworks.
proc TraceString {args} {
  eval DebugString -1 $args
}

# Define a public procedure, exported from this namespace
proc xproc {name args body} {
  namespace export $name
  proc $name $args $body
  variable xprocs ; # List of all procedures exported from this namespace.
  lappend xprocs $name
}

# Allow overriding namespace variables _before_ loading this package.
proc xvariable {name args} {
  if {![info exists "[namespace current]::$name"]} {
    uplevel 1 variable [list $name] $args
  }
}

# Import all public procedures from this namespace into the caller's namespace.
proc Import {{pattern *}} {
  namespace eval [uplevel 1 namespace current] \
    "namespace import -force [namespace current]::$pattern"
  # Duplicate Tcl execution trace operations, if any. See Proc comments below.
  variable xprocs ; # List of all procedures exported from this namespace.
  catch { # This will fail in Tcl <= 8.3
    foreach proc $xprocs {
      foreach trace [trace info execution [namespace current]::$proc] {
	foreach {ops cmd} $trace break
	uplevel 1 [list trace add execution $proc $ops $cmd]
      }
    }
  }
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
  # Escape formatting characters.
  regsub -all {\\} $msg "\\\\" msg
  regsub -all {\r} $msg "\\r" msg
  regsub -all {\n} $msg "\\n" msg
  # Add quotes around the string if needed.
  if [regexp {["\s"]} $msg -] {
    regsub -all {[""]} $msg "\\\"" msg
    set msg "\"$msg\""
  }
  if {"$msg" == ""} {
    set msg {""}
  }
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

# Trace the caller's procedure name and arguments.
xproc TraceEntry {args} {
  set cmdLine [info level [expr [info level] - 1]]
  set cmdLine [lreplace $cmdLine 0 0 [ProcName 2]] ; # Use the full proc. name.
  eval TraceString -1 -1 $args [list $cmdLine]
}

# Modified return instruction, tracing routine return value.
# Arguments:
#   -args trcArgs  Option list to pass through to TraceString. Optional.
#   -code retCode  Return type code. Optional.
#   -time duration Duration information string. Optional.
#   args           Optional arguments to pass through to return.
#   retVal         Return value
xproc Return {args} {
  set retCode return  ; # Force returning from the routine that called Return.
  set retInst return  ; # Instruction to display in the debug string.
  set retArgs {}      ; # Other return arguments
  set trcArgs {}      ; # TraceString options
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
	set strTime "([PopArg])"
      }
      default {
	lappend retArgs $arg
      }
    }
  }
  set string [concat $retInst $retArgs $strTime]
  eval TraceString -1 $trcArgs [list $string] ; # Indent 1 level more than entry
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
#                   args        Optional args to pass through to traceString  #
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
  set line ""
  foreach arg $cmd {
    if {![XXDebug]} {
      set arg [ShortenString $arg 40]
    }
    append line "$arg "
  }
  eval TraceString -1 $opts [list $line]
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
  }
  eval TraceString -1 $opts [list "  $leave $result ([expr $t1 - $t0]ms)"]
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
  foreach var [info vars [namespace current]::*] {
    regsub "[namespace current]::" $var {} var
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

} ; # End of namespace trace

trace::Import

} ; # End of it this module is invoked directly

#-----------------------------------------------------------------------------#
#                 Expect makeshift implementation in pure Tcl                 #
#-----------------------------------------------------------------------------#

#*****************************************************************************#
#                                                                             #
#   Namespace	    suspect                                       	      #
#                                                                             #
#   Description     Pure Tcl derivative of the expect program control procs.  #
#                                                                             #
#   Usage           # Example 1: Interact dynamically with one program        #
#                   suspect::Import ; # Import commands into the global NS.   #
#                   set channel [Spawn program args] ; # Open a cmd pipeline. #
#                   set pid [CVar $channel pid] ; # Get the pid of the prog.  #
#                   Expect $channel ?options? switchBlock ; # Wait 4 strings. #
#                   set data [CVar $channel received] ; # Get all until match.#
#                   Send $channel "Do this\r" ; # Send a command.             #
#                   ... ; # Repeat as many times as necessary.                #
#                   Close $channel ; # Close the pipeline and free variables. #
#                                                                             #
#                   # Example 2: Run several programs in parallel and wait    #
#                   #  for their completion (in any order)                    #
#                   proc OnProgramExit {channel} { # Callback run on pgm exit #
#                     set output [CVar $channel received] ; # Program output  #
#                     set exitCode [CVar $channel exitCode] ; # Pgm exit code #
#                     Close $channel ; # Close the pipeline and free vars.    #
#                   }                                                         #
#                   suspect::Import ; # Import commands into the global NS.   #
#                   set channels {} ; # List of open command pipelines        #
#                   lappend channels [Spawn program1 args] ; # Start program1 #
#                   ...                                                       #
#                   lappend channels [Spawn programN args] ; # Start programN #
#                   WaitForAll $channels -onEOF OnProgramExit ; # Wait 4 exit #
#                                                                             #
#   Notes 	    The routines are not compatible with expect, in an        #
#                   attempt to fix some of expect's shortcomings:             #
#                   - expect uses global variables, which makes it difficult  #
#                     to interact with several pipelines at the same time.    #
#                     All suspect functions use a pipeline handle, and store  #
#                     data in pipeline-specific namespace variables.          #
#                   - I've been bitten by some powerful, but dangerous,       #
#                     options of the expect routine. These were disabled      #
#                     here. See the Expect routine header below for details.  #
#                                                                             #
#                   Known issues:                                             #
#                   - Expect will fail (actually time-out) if the pipelined   #
#                     program does not flush its prompt output. (Even if that #
#                     program does work fine when invoked in the shell.)      #
#                   - It will also fail with programs that require a pseudo-  #
#                     tty to send a prompt. (One of the big superiorities of  #
#                     the real expect!)                                       #
#                                                                             #
#   History 								      #
#    2003/03    ST  Sample code written by Stephen Trier and placed in the    #
#                   public domain. See: http://wiki.tcl.tk/8531               #
#    2009/06/18 JFL Created these routines, loosely based on ST's sample code.#
#    2009/07/09 JFL Added routine WaitForAll, to do parallel waits.           #
#                                                                             #
#*****************************************************************************#

namespace eval suspect {
  variable timeout 10 ; # Default timeout, in seconds. 0 = No timeout.

# Define a public procedure, exported from this namespace
proc xproc {name args body} {
  namespace export $name
  proc $name $args $body
  variable xprocs ; # List of all procedures exported from this namespace.
  lappend xprocs $name
}

# Import all public procedures from this namespace into the caller's namespace.
proc Import {{pattern *}} {
  namespace eval [uplevel 1 namespace current] \
    "namespace import -force [namespace current]::$pattern"
  # Duplicate Tcl execution trace operations, if any.
  variable xprocs ; # List of all procedures exported from this namespace.
  catch { # This will fail in Tcl <= 8.3
    foreach proc $xprocs {
      foreach trace [trace info execution [namespace current]::$proc] {
	foreach {ops cmd} $trace break
	uplevel 1 [list trace add execution $proc $ops $cmd]
      }
    }
  }
}

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

# Get the error code returned by an external program
proc ErrorCode {{err -1}} { # err = The TCL error caught when executing the program
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

TraceProcs {

# Get/Set a channel-specific variable
SkipTraceProcs {
xproc CVar {channel var args} {
  variable $channel
  if {"$args" == ""} {
    set ${channel}($var)
  } else {
    set ${channel}($var) [join $args ""]
  }
}
proc CAppend {channel var args} {
  variable $channel
  append ${channel}($var) [join $args ""]
}
} ; # End SkipTraceProcs

# Open a command pipeline
xproc Spawn {args} {
  if {"$args" == ""} {
    error "Spawn: No command specified"
  }
  set channel [open "|$args" RDWR]
  set msStart [clock clicks -milliseconds]
  CVar $channel msStart $msStart ; # Record the startup time
  CVar $channel msStop $msStart  ; # Make sure it's defined (In case of timeout)
  fconfigure $channel -blocking 0 -buffering none
  set ns [namespace current]
  fileevent $channel readable "${ns}::TriggerEvent $channel readable 1"
#  fileevent $channel writable "${ns}::TriggerEvent $channel writable 1"
  CVar $channel cmd $args ; # Record the command line for future diagnostics.
  CVar $channel pid [pid $channel] ; # Record the pipeline pid
  return $channel
}

# Send data to the command pipeline.
xproc Send {channel string} {
  puts -nonewline $channel $string
  # flush $channel ; # Useful only in buffering line mode
}

# Manage pipe I/O events
SkipTraceProcs {
proc TriggerEvent {channel event {value 1}} {
  CVar $channel $event $value ; # Set the channel-specific event variable
  variable events
  lappend events [list $channel $event $value] ; # Useful for parallel waits
}
proc WaitEvent {channel event} {
  vwait [namespace current]::${channel}($event)
# puts "Done waiting for $channel $event"
  CVar $channel $event
}
} ; # End SkipTraceProcs

# Read from channel, with an optional timeout. Event driven, using vwait.
proc Read {channel args} { # Usage: Read channel [N]
  set readCmd [linsert $args 0 read $channel] ; # The read command
  set readable [WaitEvent $channel readable]
  if {!$readable} {
    error TIMEOUT
  }
  if [eof $channel] {
    CVar $channel msStop [clock clicks -milliseconds]
    error EOF
  }
# PutVars readCmd
  set data [eval $readCmd] ; # Read the requested data.
# puts [Escape $data]
  return $data
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    Expect                                        	      #
#                                                                             #
#   Description     Pure Tcl derivative of the expect command                 #
#                                                                             #
#   Parameters      channel            R/W channel to a command pipeline      #
#                   OPTIONS            See the options list below             #
#                   switchBlock        The various alternatives and action    #
#                                                                             #
#   Options 	    -exact             Use exact strings matching (default)   #
#                   -glob              Use glob-style matching                #
#                   -regexp            Use regular expressions matching       #
#                   -timeout N         Timeout after N seconds. Default: 10   #
#                   -onTIMEOUT BLOCK   What to do in case of timeout          #
#                   -onEOF BLOCK       What to do in case of End Of File      #
#                                                                             #
#   Returns 	    User defined. By default: Nothing if found, or errors out #
#                   in case of EOF or TIMEOUT.                                #
#                                                                             #
#   Notes 	    This routine is incompatible with the real expect on      #
#                   purpose, to fix some of its shortcomings:                 #
#                   - expect's ability to specify either one switch block, or #
#                     multiple block items (Like Tcl's own exec), is nice in  #
#                     simple cases, but always backfires when the program     #
#                     complexity grows. Suspect::Expect requires one block.   #
#                   - I've been bitten by expect's inability to expect the    #
#                     word timeout. (I found the workaround, but too late.)   #
#                     Suspect::Expect handles EOF and TIMEOUT in options only.#
#                   - expect allows options within the switch block. Very     #
#                     powerful to use distinct search criteria for distinct   #
#                     strings. But at the cost of making these very options   #
#                     difficult to be themselves expected. Suspect::Expect    #
#                     only allows options before the switch block.            #
#                                                                             #
#                   Things like exp_continue are not yet supported.           #
#                                                                             #
#   History 								      #
#    2009/06/18 JFL Created these routines, loosely based on ST's sample code.#
#                                                                             #
#-----------------------------------------------------------------------------#

xproc Expect {channel args} { # Usage: Expect channel [options] switchBlock
  # Namespace variables
  variable timeout
  # Local variables
  set sMode -exact ; # Switch mode. One of: -exact -glob -regexp
  set msTimeout [expr 1000 * $timeout] ; # Timeout, in milli-seconds
  set onEof "error {Expect: EOF reading from command pipeline $channel :\
             [CVar $channel cmd]}" ; # What to do in case of end of file
  set onTimeout "error {Expect: TIMEOUT waiting for command pipeline $channel :\
                 [CVar $channel cmd]}" ; # What to do in case of timeout

  # Separate the last switch block from the options
  if {"$args" == ""} {
    error "Expect: No switch block defined."
  }
  set expectBlock [lindex $args end]
  set args [lrange $args 0 end-1]

  # Process the options
  while {"$args" != ""} {
    set opt [PopArg]
    switch -- $opt {
      "-exact" - "-glob" - "-regexp" {
	set sMode $opt
      }
      "-onEOF" - "eof" {
	set onEof [PopArg]
      }
      "-onTIMEOUT" - "timeout" {
	set onTimeout [PopArg]
      }
      "-timeout" {
	set msTimeout [expr [PopArg] * 1000]
      }
      default {
	error "Expect: Unsupported option $opt"
      }
    }
  }

  # Build the switch statement we will use for matching
  set switchBlock {}
  foreach {match script} $expectBlock {
    set match0 $match
    set before {}
    set after {}
    switch -- $sMode {
      -exact {
	set before {***=}
      }
      -glob {
	if {[string index $match 0] eq "^"} {
	  set match [string range $match 1 end]
	} else {
	  set before *
	}
	if {[string index $match end] eq "\$"} {
	  set match [string range $match 0 end-1]
	} else {
	  set after *
	}
      }
    }
    lappend switchBlock $before$match$after
    lappend switchBlock "
      after cancel \$idTimeout
      set channelVar(match) [list $match0] ;
      return \[uplevel 1 [list $script]\]"
  }
  if {"$sMode" == "-exact"} {
    set sMode -regexp
  }

  # Manage optional timeouts
  set idTimeout "" ; # "after cancel $idTimeout" will silently ignore this id.
  set ns [namespace current]
  if {$msTimeout} {
    set idTimeout [after $msTimeout "${ns}::TriggerEvent $channel readable 0"]
  }

  # Gather characters from the channel and run them through our new switch statement.
  CVar $channel received ""
  while {1} {
    if [catch {set c [Read $channel 1]} errMsg] {
      switch -- $errMsg {
	"TIMEOUT" {
	  return [uplevel 1 $onTimeout]
	}
	"EOF" {
	  after cancel $idTimeout
	  return [uplevel 1 $onEof]
	}
	default {
	  error "Error reading $channel: $errMsg"
	}
      }
    }
    CAppend $channel received $c
    switch $sMode -- [CVar $channel received] $switchBlock
  }
}

# Common case where we expect a single exact string
xproc ExpectString {channel string} {
  Expect $channel [list $string]
}

# Close a command pipeline, and return the program exit code.
xproc CloseCommand {channel} {
  variable $channel
  if [info exists ${channel}(exitCode)] {
    return [CVar $channel exitCode]
  }
  fconfigure $channel -blocking 1 ; # Make sure close checks for the exit code
  set err [catch {close $channel} errMsg] ; # Get the Tcl error code
  set err [ErrorCode $err] ; # Get the command exit code
  CVar $channel exitCode $err
  set dt [expr [CVar $channel msStop] - [CVar $channel msStart]]
  DebugString "exit $err (${dt}ms) \[[CVar $channel cmd]\]"
  return $err
}

# Close a command pipeline, and free all local resources. Return the exit code.
xproc Close {channel} {
  variable $channel
  set err 0
  if [info exists $channel] {
    set err [CloseCommand $channel] ; # Get the command exit code
    unset $channel
  }
  return $err
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    WaitForAll                                        	      #
#                                                                             #
#   Description     Wait for the completion of several parallel programs      #
#                                                                             #
#   Parameters      channels           List of spawned tasks                  #
#                   -onEOF proc        Call $proc $channel after each EOF.    #
#                                                                             #
#   Returns 	    Nothing, or errors out in case of TIMEOUT.                #
#                                                                             #
#   Notes 	    Timeout out not implemented yet.                          #
#                                                                             #
#   History 								      #
#    2009/07/09 JFL Created this routine.                                     #
#    2009/09/28 JFL Added the -onEOF option.                                  #
#                                                                             #
#-----------------------------------------------------------------------------#

xproc WaitForAll {channels args} {
  variable events
  set onEOF ""
  # Process the options
  while {"$args" != ""} {
    set opt [PopArg]
    switch -- $opt {
      "-onEOF" - "eof" {
	set onEOF [PopArg]
      }
      default {
	error "WaitForAll: Unsupported option $opt"
      }
    }
  }
  # Wait for the EOF on all channels
  set nLeft [llength $channels]
  foreach channel $channels {
    # fconfigure $channel -buffering full ; # Minimize the # of read events.
  }
  while {$nLeft} {
    vwait [namespace current]::events
    foreach event $events {
      foreach {channel event value} $event break
# PutSVars "WaitForAll:" event channel value
      if {("$event" != "readable") || ($value != 1)} continue
      set input [read $channel]
# PutSVars "WaitForAll: Read [string length $input] bytes:" input
      CAppend $channel received $input
      if {[eof $channel] && ([set ix [lsearch $channels $channel]] != -1)} {
# puts "WaitForAll: Channel $channel exited."
        CVar $channel msStop [clock clicks -milliseconds]
        set channels [lreplace $channels $ix $ix]
	incr nLeft -1
	if {"$onEOF" != ""} {
	  eval $onEOF $channel
	}
      }
    }
    set events {}
  }
}

} ; # End of TraceProcs

} ; # End of namespace suspect

###############################################################################
#                 Main routine for testing the Suspect library                #
###############################################################################

# If this module is invoked directly...
if {"[file tail [info script]]" == "[file tail $::argv0]"} {

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

# Set the default command
switch $::tcl_platform(platform) {
  "unix" { # Unix version of the routines
    set cmd bash
    set cmdArgs [list -i]
    set env(TERM) "dumb" ; # Make sure spawned programs don't generate fancy escape sequences
    set prompt "[exec echo exit | bash -i |& grep exit$ | sed s/exit$//]"
  }
  "windows" { # Windows NT/2000/XP/Vista version of the routines
    set cmd cmd
    set cmdArgs {}
    set prompt "\n[exec cmd /c cd]>" ; # Don't use pwd, which resolves links.
  }
}

# Usage string
set usage [subst -nobackslashes -nocommands {
Usage: $::argv0 [options] [command [arguments]]

Options:
  -h, --help, -?        Display this help screen
  -d, --debug           Debug mode
  -p, --prompt PROMPT   Specify the expected prompt
}]

# Scan all arguments.
set args $argv
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-d" - "--debug" {		# Enable debug mode
      incr ::verbosity 2
    }
    "-h" - "--help" - "-?" {	# Display a help screen and exit.
      puts $usage
      exit 0
    }
    "-p" - "--prompt" {		# Specify the expected prompt
      set prompt [PopArg]
    }
    "-q" - "--quiet" {		# Enable quiet mode
      set ::verbosity 0
    }
    "-v" - "--verbose" {	# Increase the verbosity level
      incr ::verbosity
    }
    default {
      if {"[string index $arg 0]" == "-"} {
	puts stderr "Unrecognized argument: $arg"
	puts stderr $usage
	exit 1
      }
      set cmd $arg
      set cmdArgs $args
      break
    }
  }
}

suspect::Import

set cmdLine [Which $cmd] ; # Find command in the system PATH
if {"$cmdLine" == ""} {
  puts stderr "Cannot find $cmd"
  exit 1
}
set cmdLine [linsert $cmdArgs 0 $cmdLine]

set pipe ""
if [catch {
  DebugVars cmdLine prompt
  set pipe [eval Spawn $cmdLine]
  # Get the initial prompt
  ExpectString $pipe $prompt
  PutString "Got the initial prompt: [Escape [CVar $pipe received]]"
  for {set done 0} {!$done} {} {
    PutString -nonewline stdout "Command to send to $cmd? "
    flush stdout
    set command [gets stdin]
    # PutVars pipe command
    Send $pipe "$command\n"
    # ExpectString $pipe "\n" ; # Wait for the echo from the command, and discard it
    Expect $pipe -onEOF {PutString "EOF"; set done 1} [list $prompt]
    set output [CVar $pipe received]
    regsub "***=$prompt" $output "" output ; # Remove the prompt
    PutVars output
  }
} errmsg] {
  puts stderr "$::script error: $errmsg"
  if {"$pipe" != ""} {
    set output [CVar $pipe received]
    PutVars output
    # And as we've seen many cases where the spawned program hung in the background,
    # then kill it, without even trying to see if it's dead already.
    after 250 ; # Wait a little bit to give it a chance to exit gracefully
    set err [catch {
      switch $::tcl_platform(platform) {
	"unix" { # Unix version of the routines
	  exec kill -9 [CVar $pipe pid]
	}
	"windows" { # Windows NT/2000/XP/Vista version of the routines
	  exec taskkill /f /pid [CVar $pipe pid]
	}
      }
    } msg]
    DebugSVars "Kill attempt:" err msg
  }
}

if {"$pipe" != ""} {
  set err [Close $pipe]
  if $err {
    puts stderr "The program returned error code $err"
  }
}

} ; # End of it this module is invoked directly

