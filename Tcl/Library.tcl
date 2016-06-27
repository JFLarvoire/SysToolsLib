#!/usr/bin/expect --
###############################################################################
#                                                                             #
#   File name	    Library.tcl                                               #
#                                                                             #
#   Description     A library of useful Tcl and Expect routines               #
#                                                                             #
#   Notes:	    Can be used as a template for new programs.               #
#                   Can be used as a standalone test script for its own procs.#
#                                                                             #
#   History:								      #
#    2005-05-25 JFL Created this library.                                     #
#    2014-11-30 JFL Added routine RomanNumeral.                               #
#    2015-06-22 JFL Removed all debug code, and use DebugLib.tcl.             #
#                   Fixed bugs in routines TrueName, GetPassword.             #
#                   Added routines PackageRequire, DnsSearchList.	      #
#    2016-05-02 JFL Fixed ReadHosts and EtcHosts2IPs.                         #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Set defaults
set version "2016-05-02"

source "[file dirname $argv0]/debuglib.tcl"	;# Include the debug library
::debug::Import

###############################################################################
#                       General infrastructure routines                       #
###############################################################################

# Return true if invoked directly. Useful for library testing.
proc IsScript {} {
  expr    [info exists ::argv0] \
       && ("[file tail [info script]]" == "[file tail $::argv0]")
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    PackageRequire                                            #
#                                                                             #
#   Description     Download, load and initialize an external TCL package     #
#                                                                             #
#   Parameters      Same as "package require"                                 #
#                                                                             #
#   Returns 	    Same as "package require"                                 #
#                                                                             #
#   Notes:	    If the initial load attempt fails, attempts to download   #
#                   the package from the configured teapot repository.        #
#                                                                             #
#   History:								      #
#    2004/10/25 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set checkedProxy 0
set webProxy ""

# Try to guess the web proxy for the 
proc PackageCheckProxy {} {
  if $::checkedProxy return ; # Don't do it again if we already did it.
  switch $::tcl_platform(platform) {
    "windows" {
      set err [catch {exec ipconfig | findstr IPv4} output]
    }
    default {
      set err [catch {exec ifconfig  | grep "inet add"} output]
    }
  }
  if $err {
    set ::checkedProxy 1
    error "Can't get this system IP address while searching proxy. $output"
  }
  # Find all IP addresses
  set ips [regexp -all -inline {\d+\.\d+\.\d+\.\d+} $output]
  foreach ip $ips {
    DebugVars ip
    if [regexp {^1[56]\.} $ip -] {
      set ::webProxy "web-proxy:8080"
      break
    }
  }
  set ::checkedProxy 1
  return
}

proc PackageDownload {package} {
  if {"[Which teacup]" == ""} {
    error "No teacup package manager here. Please install the Tcl package \"$package\" manually."
  }
  if ![file writable $::tcl_library] { # The install will fail if we don't have write access.
    if {"$::tcl_platform(platform)" == "windows"} {
      set admin Administrator
    } else { # Any Unix variant
      set admin root
    }
    error "Cannot install $package: Must run as $admin to install Tcl packages."
  }
  PackageCheckProxy
  set cmd {exec teacup install}
  if {"$::webProxy" != ""} {
    lappend cmd "--http-proxy" $::webProxy
  }
  if {!$::exec} {
    lappend cmd "--dry-run"
  }
  lappend cmd $package
  if {[Verbose] || !$::exec} {
    puts $cmd
    lappend cmd ">@stdout" "2>@stderr"
  }
  eval $cmd
}

proc PackageRequire {package args} {
  set err [catch {uplevel 1 package require $package $args} output]
  if $err {
    if [string match -nocase "*can't find package*" $output] {
      Puts $::stderr "Info: The required Tcl package \"$package\" is missing. Trying to download and install it."
      PackageDownload $package
      Puts $::stderr "Info: Package \"$package\" installation succeeded."
      # And try again
      return [uplevel 1 package require $package $args]
    }
    error $output
  }
  return $output
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    require                                                   #
#                                                                             #
#   Description     Load an external TCL package, and initialize it           #
#                                                                             #
#   Parameters      lib                The package name                       #
#                                                                             #
#   Returns 	    0 = success; 1 = error.                                   #
#                                                                             #
#   Notes:	    Older version from SFS.                                   #
#                                                                             #
#                   Using [info procs ${lib}Init] to detect the init routine  #
#                   does not work:   (Same thing with [info commands ...])    #
#                   The routine is only defined after it is called!?!         #
#                                                                             #
#   History:								      #
#    2004/10/25 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

if [file exist /usr/opt/hpls/lib/tcl] {
lappend auto_path /usr/opt/hpls/lib/tcl

# Load a library, and call its Init procedure if any.
proc require lib {
  if {   [catch {package require $lib}]
      || (   [catch {${lib}Init} msg]
          && ([string first "invalid command name" $msg] == -1) )
     } {
    Puts stderr "Failed to load the $lib shared Tcl library"
    return 1
  }
  DebugPuts "$lib init done."
  return 0
}

proc InitRequiredPackages {} {
  foreach package {hplsLog lscommon lscluster lsdb lsevlog lsnet lsiLO 
                   lslustre lsmon lspwr lsstorage lssyscheck} {
    require $package
  }
}
}

###############################################################################
#                         String management routines                          #
###############################################################################

# Append a string like puts.
proc appends {name line} {
  upvar 1 $name data
  append data "$line\n"
}

# Set a variable, concatenating multiple string pieces.
proc Set {varName args} {
  upvar 1 $varName var
  set var [join $args ""]
}

# Alternative renaming the Tcl set command itself
# Set a variable, concatenating multiple string pieces.
rename set Tcl_set
proc set {varName args} {
  upvar 1 $varName var
  if {"$args" != ""} {
    Tcl_set var [join $args ""]
  } else {
    Tcl_set var
  }
}

# Append values to a variable up in the call chain
proc AppendUpvar {upvar args} { # upvar = {level varName}
  foreach {level varName} $upvar break
  upvar #$level $varName var
  eval append var $args
}

# Extension of "string replace" to allow inserting like "lreplace".
proc StringReplace {string ix0 ix1 newString} {
  if {$ix1 >= $ix0} {
    return [string replace $string $ix0 $ix1 $newString]
  } else {
    set ix1 [expr $ix0 - 1]
    set before [string range $string 0 $ix1]
    set after [string range $string $ix0 end]
    return "$before$newString$after"
  }
}

# Delete a line from a text. textVar=Text variable name; rxLine=Regular expr.
proc DeleteLine {textVar rxLine} {
  upvar 1 $textVar text
  set signature "DeleteLinePlaceHolder"
  if ![regsub -line -all $rxLine $text $signature text] {
    return 1 ; # Line not found
  }
  while {[regsub "\n *$signature *"   $text "" text]} {} ; # Any but 1st line
  while {[regsub   " *$signature *\n" $text "" text]} {} ; # First line
  while {[regsub   " *$signature *"   $text "" text]} {} ; # The only line
  return 0 ; Done
}

# Append a line to a text buffer. textVar=Text variable name; line=line to add.
# Note: Contrary proc appends above, the \n is put _before_ the line if needed.
proc AppendLine {textVar line} {
  upvar 1 $textVar text
  if {"$text" != ""} { # If the text already contains other lines
    append text "\n" ; # Then add a new line
  }
  append text $line
}

# Replace a string with another one, either in place or into another variable
proc ReplaceString {textVar oldString newString {newVar ""}} {
  upvar 1 $textVar oldText
  if {"$newVar" != ""} {
    upvar 1 $newVar newText
  } else {
    upvar 1 $textVar newText
  }
  set newText [string map [list $oldString $newString] $oldText]
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

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    TrimDecimal                                        	      #
#                                                                             #
#   Description     Trim extra zeros at the left of a decimal integer.        #
#                                                                             #
#   Parameters      decimal            Name or value of a decimal variable    #
#                                                                             #
#   Returns 	    The value with extra 0s trimmed on the left               #
#                                                                             #
#   Notes:	    If passed as a name, then the variable in the caller's    #
#                   scope is trimmed.                                         #
#                                                                             #
#   History:								      #
#    2005/11/07 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc TrimDecimal {decimal} {
  if [string is integer $decimal] {
    set n $decimal
  } else {
    upvar 1 $decimal n
  }
  set n [string trimleft $n 0]
  if {"$n" == ""} {
    set n 0
  }
  return $n
}

# A simpler version
proc TrimLeftZeros {num} {
  set num [string trimleft $num 0]
  if {"$num" == ""} {
    set num 0
  }
  return $num
}

###############################################################################
#                     Lists and Sets management routines                      #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    SubLists                                                  #
#                                                                             #
#   Description     Return the list of all sublists of a given list.          #
#                                                                             #
#   Parameters      list             Input list                               #
#                                                                             #
#   Returns 	    A Tcl list of lists.                                      #
#                                                                             #
#   History:								      #
#    2006/09/04 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc SubLists {list} {  # Straight version with single-element sublists first.
  if ![llength $list] { # If the list is empty
    return [list ""]  ; # Return a list with one empty list.
  }
  # Else build the sublist recursively, by splitting the problem in two.
  set head [lrange $list 0 end-1] ; # Shorter list with all but the last element.
  set tail [lindex $list end]     ; # and the last element.
  set halfList [SubLists $head]   ; # All sublists of that head part
  set subLists $halfList          ; # That's half of the final list
  foreach subList $halfList {       # Build the second half with the tail element.
    lappend subLists [concat $subList $tail]
  }
  return $subLists
}

proc SubListsR {list} { # Reversed version with multi-element sublists first.
  if ![llength $list] { # If the list is empty
    return [list ""]  ; # Return a list with one empty list.
  }
  # Else build the sublist recursively, by splitting the problem in two.
  set head [lindex $list 0]       ; # The first element,
  set tail [lrange $list 1 end]   ; # and everything behind.
  set halfList [SubListsR $tail]  ; # All sublists of that tail part
  set subLists {}
  foreach subList $halfList {       # Build the first half with the head element.
    lappend subLists [concat $head $subList]
  }
  return [concat $subLists $halfList] ; # And the second half without
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   namespace	    set                                                       #
#                                                                             #
#   Description     Manage sets, i.e. lists where each element is unique      #
#                                                                             #
#   Parameters      setName          List variable name                       #
#                   args             Values to manipulate in the list         #
#                                                                             #
#   Returns 	    The contents of the list after the update.                #
#                                                                             #
#   History:								      #
#    2006/09/28 JFL Created this routine as AddToList.                        #
#    2009/01/29 JFL Restructured as a namespace, after the C++ STL sets.      #
#                   Renamed AddToList as Add, and added Remove.               #
#                                                                             #
#-----------------------------------------------------------------------------#

namespace eval set {

# Append items to a list iff they're not already in the list.
proc Add {setName args} {
  upvar 1 $setName list
  if ![info exists list] { # Avoid errors if the list is not yet defined.
    set list {}
  }
  foreach arg $args {
    if {[lsearch -exact $list $arg] == -1} {
      lappend list $arg
    }
  }
  set list
}

# Remove items from a list if present. Ignore if absent.
proc Remove {setName args} {
  upvar 1 $setName list
  foreach arg $args {
    set ix [lsearch -exact $list $arg]
    if {$ix != -1} {
      set list [lreplace $list $ix $ix]
    }
  }
  set list
}

} ; # End of the set namespace

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
#    2005/10/17 JFL Created this routine.                                     #
#    2008/05/13 JFL Fixed the case with strings with both "s and 's.          #
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
#   Function	    sh2tcl	                                    	      #
#                                                                             #
#   Description     Convert a shell command line into a tcl command list.     #
#                                                                             #
#   Parameters      shellCmd           Shell-like string w. quoted arguments  #
#                                                                             #
#   Returns 	    Tcl list of tokens.                                       #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2005/11/15 JFL Created this routine.                                     #
#    2006/06/01 JFL Rewritten to support both ' and ", be strictly compliant. #
#                                                                             #
#-----------------------------------------------------------------------------#

proc sh2tcl {shellCmd} {
  set rx1Quote {'([^']*)'} ; # Regular expression for parsing a '-quoted string
  set rx2Quote {"((\\"|\\\\|[^"])*)"} ; # Reg. expr. for parsing a "-quoted string

  set tokens {} ; # Result list of shell arguments.
  set token {}  ; # Current shell argument.
  set inToken 0 ; # 1 = A new argument has begun.

  set nChars [string length $shellCmd]
  for {set i 0} {$i < $nChars} {incr i} {
    set c [string index $shellCmd $i]
    switch -- "$c" {
      "'" {
	set inToken 1
	regexp -start $i $rx1Quote "$shellCmd'" - string
	set lstring [string length $string]
	append token $string
	incr i [expr $lstring + 1] ; # + 2 for quotes - 1 incr. again by loop.
      }
      "\"" {
	set inToken 1
	regexp -start $i $rx2Quote "$shellCmd\"" - string
	set lstring [string length $string]
	regsub -all {\\\\} $string {\\} string ; # Only backspaces and
	regsub -all {\\"} $string {"} string ; # double-quotes are escaped.
	append token $string
	incr i [expr $lstring + 1] ; # + 2 for quotes - 1 incr. again by loop.
      }
      "\\" { # Escapes the next char, whatever it is
	set inToken 1
	append token [string index $shellCmd [incr i]]
      }
      ";" - " " - "\t" - "\n" {
	if $inToken {
	  lappend tokens $token
	  set inToken 0
	  set token {}
	}
	if {"$c" == ";"} {
	  lappend tokens ";" ; # The command separator is a token on its own
	}
      }
      default {
	set inToken 1
	append token $c
      }
    }
  }
  if $inToken {
    lappend tokens $token
  }
  return $tokens
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    SplitOptionsTree                                          #
#                                                                             #
#   Description     Split comma-separated option lists, such as fs mount opts.#
#                                                                             #
#   Parameters      options          Option list from fstab or sfstab         #
#                                                                             #
#   Returns 	    A Tcl list                                                #
#                                                                             #
#   Notes:	    Takes special care not to split ranges in brackets.       #
#                   Ex: atlas[1-2,4,8]                                        #
#                   Or sublists in parenthesis. Ex: tcp(eth1,eth2)            #
#                                                                             #
#   History:								      #
#    2005/05/05 JFL Created this routine for mount options.                   #
#    2006/07/03 JFL Generalized to any kind or comma-separated flat tree.     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc SplitOptionsTree {options} {
  set list {}		; # The result list
  set option ""		; # Current option being processed
  set depth 0		; # Depth level with [] pairs
  foreach c [split "$options," ""] { # Append a comma to force output last opt.
    switch -exact -- $c {
      "\[" - "(" - "{" {
	append option $c
        incr depth
      }
      "\]" - ")" - "}" {
	append option $c
        incr depth -1
      }
      "," {			# The normal split character is the comma.
	if {$depth > 0} {	# But don't split if we're inside a [] range.
	  append option $c
	} else {
	  if {"$option" != ""} {
	    lappend list $option
	  }
	  set option ""
	}
      }
      default {
	append option $c
      }
    }
  }
  return $list
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        lsearchX                                    	      #
#                                                                             #
#   Description     Provide some Tcl 8.4 lsearch options under 8.3 or less.   #
#                                                                             #
#   Parameters      Same as that of Tcl 8.4 lsearch                           #
#                                                                             #
#   Returns         Same as that of Tcl 8.4 lsearch                           #
#                                                                             #
#   Notes:          Emaulates tcl 8.4's lsearch -inline -all                  #
#                   No attempt is made to emulate other unsupported 8.4       #
#                   options such as -start, -dictionary, -not, -sorted, ...   #
#                                                                             #
#                   Works by feature detection, not version test.             #
#                   It will use the native version whenever available.        #
#                                                                             #
#   History:                                                                  #
#    2006/04/03 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc lsearchX {args} {
  # Detect options supported by this interpretor
  if {! [info exists ::lsearchOpts]} {
    catch {lsearch -badoption list pattern} msg ; # Error prints valid options
    regexp {: must be ([^\n]+)} $msg - opts ; # Extract the valid options line
    regsub ", or" $opts "," opts            ; # Correct the last separator
    regsub -all "," $opts "" ::lsearchOpts  ; # Remove all comma separators
  }

  # Detect options used this time
  set opts [lrange $args 0 end-2] ; # Last 2 args are always list & pattern.
  set ok 1
  foreach opt $opts { # Check if they're all supported?
    if {[lsearch $::lsearchOpts $opt] == -1} {
      set ok 0 ; # At least one unsupported option
      break
    }
  }

  if $ok { # Use the standard lsearch if possible. Will be faster!
    return [eval lsearch $args]
  }

  # Now emulate functions we can
  set iInline [lsearch $opts "-inline"] ; # Are we actually using -inline?
  if {$iInline != -1} { # This one we know how to emulate
    set opts [lreplace $opts $iInline $iInline] ; # Delete the -inline argument
    set args [lreplace $args $iInline $iInline] ; # Delete the -inline argument

    set iAll [lsearch $opts "-all"]
    if {$iAll != -1} { # This one we know too, in conjunction with -inline.
      set args [lreplace $args $iAll $iAll] ; # Delete the -all argument
      set iAll 1
    } else {
      set iAll 0
    }

    set result ""
    set list [lindex $args end-1]
    while 1 {
      set ix [eval lsearch $args]
      if {$ix == -1} break ; # Done search
      lappend result [lindex $list $ix]
      set list [lrange $list [expr $ix + 1] end]
      set args [lreplace $args end-1 end-1 $list]
      if {! $iAll} break   ; # Done search
    }

    return $result
  }

  # We don't know how to emulate this option
  eval lsearch $args ; # Let Tcl complain itself
}

###############################################################################
#                         Program management routines                         #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    FindProcesses                                    	      #
#                                                                             #
#   Description     Find all processes matching a given regular expression    #
#                                                                             #
#   Parameters      search      Command line to search.			      #
#                                                                             #
#   Returns 	    A list of pairs {pid command_line}                        #
#                                                                             #
#   Notes:                                                                    #
#                                                                             #
#   History:								      #
#    2005       JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Find all processes with the command line matching a given regular expression.
proc FindProcesses {{search .*}} {
  set processList ""
  # List processes.
  # Note: DO NOT pipe ps output to grep, as this _sometimes_ lists the forked
  #       process _before_ it has changed identity to grep.
  #       ==> This would list this process twice! (timing-dependant)
  set err [catch {exec /bin/ps -ewwo pid,cmd} out]
  if {$err && ("$::errorCode" != "NONE")} {
    return {} ; # No such process found.
  }
  # Scan the output to extract PIDs
  foreach line [split $out \n] {
    set found [regexp "^ *(\[0-9\]*) *($search.*)" $line - pid cmdLine]
    if !$found continue
    DebugPuts "Found process: $line"
    lappend processList [list $pid $cmdLine]
  }
  return $processList
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    echo_status                                    	      #
#                                                                             #
#   Description     Call status procedures from /etc/rc.d/init.d/functions    #
#                                                                             #
#   Parameters      type               success | failure | passed | warning   #
#                                      [  OK  ] [failed]  [passed] [warning]  #
#                                                                             #
#   Returns 	    0 for success or 1 for failure                            #
#                                                                             #
#   Notes:	    For use by system services at boot and shutdown time.     #
#                                                                             #
#   History:								      #
#    2005/07/25 JFL Created this routine.                                     #
#    2007/03/21 JFL Added support for Suse distributions.                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# The equivallent commands under Suse. There's no warning, but a yellow unknown.
set ::suse_status(success) {$rc_done}
set ::suse_status(failure) {$rc_failed}
set ::suse_status(passed)  {${rc_unknown/unknown/passed}}
set ::suse_status(warning) {${rc_unknown/unknown/warning}}
foreach key {success failure passed warning} {
  set ::suse_status($key) "echo \$rc_save$::suse_status($key)\$rc_restore"
}

proc echo_status {type} {
  catch { # May fail with console I/O error!
    flush stdout ; # Make sure the last message did go through.
    flush stderr ; # idem
  }
  set err [catch {
    if [file exists /etc/rc.d/init.d/functions] { # RedHat-style distribution
      exec /bin/sh -c ". /etc/rc.d/init.d/functions ; echo_$type" >@stdout 2>@stderr
      puts ""
    } elseif [file exists /etc/rc.status] { # Suse-style distribution
      exec /bin/sh -c ". /etc/rc.status ; $::suse_status($type)" >@stdout 2>@stderr
    } else { # Unknown distribution
      puts " ... $type"
    }
  } out]
  return $err
}

# Status procedures adapted from /etc/rc.d/init.d/functions
proc echo_success {} {
  echo_status success
}

proc echo_warning {} {
  echo_status warning
}

proc echo_failure {} {
  echo_status failure
}

proc echo_passed {} {
  echo_status passed
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        ExecNonBlocking                                    	      #
#                                                                             #
#   Description     Execute a program, killing it after a timeout if it hangs.#
#                                                                             #
#   Parameters      -t N               Change timeout to N seconds. Def: 30   #
#                   args               Command line to execute                #
#                                                                             #
#   Returns         0=The program terminated in time; 1=It timed out.         #
#                                                                             #
#   Notes:          This routine is useful for running unreliable commands    #
#                   that may hang.                                            #
#                                                                             #
#   History:                                                                  #
#    2006/04/03 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc ExecNonBlocking {args} {
  set timeout 30 ; # Timeout in seconds.
  # Process optional switches.
  while 1 {
    switch -- [lindex $args 0] {
      "-t" - "--timeout" {
	set timeout [lindex $args 1]
	set args [lrange $args 2 end]
      }
      default {
	break
      }
    }
  }
  # Run the command in the background.
  set err [catch {eval exec -- $args &} out]
  if $err {return 1}
  set allPids $out ; # List of all pids in the pipe
  set lastPid [lindex $allPids end]
  # Wait for its completion
  set prog [lindex $args 0]
  set timeout ${timeout}0 ; # Change unit to 1/10 seconds
  for {set i 0} {$i < $timeout} {incr i} {
    if {! [file exists /proc/$lastPid/exe]} { # The process has ended
      # Note: Test /proc/$lastPid/exe, not /proc/$lastPid: When the process
      # goes to defunct state, the former disappears, while the latter remains.
      # Do another exec. It calls wait, which cleans up the defunct process
      # entry in the ps list. (including the whole /proc/$lastPid tree.)
      catch {exec /bin/true}
      return 0
    }
    after 100 ; # Wait 100 milli-seconds
  }
  VerbosePuts "Timeout waiting for $prog completion. Killing it."
  # Timeout. Kill the hung process.
  foreach pid $allPids {
    VerbosePuts "Killing process $pid..."
    catch {exec kill -9 $pid}
  }
  # Do another exec. It calls wait, which cleans up the defunct process
  # entry in the ps list. (including the whole /proc/$lastPid tree.)
  catch {exec /bin/true}
  if {! [file exists /proc/$lastPid]} { # The process has ended
    VerbosePuts "$prog now killed."
  } else {
    Puts stderr "Failed to kill hung $prog, pid=$lastPid."
  }
  return 1
}

###############################################################################
#                          File management routines                           #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    TrueName                                                  #
#                                                                             #
#   Description     Get the canonic name of a file.                           #
#                                                                             #
#   Parameters      pathname           A relative or absolute pathname        #
#                                                                             #
#   Returns 	    The absolute pathname, with all links resolved.           #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2005/01/15 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Convert a pathname (Relative or absolute) into its canonic absolute form.
# Note: Does not expand ~.
proc AbsName { pathname {basedir .}} {
  # Extract parts.
  set parts [file split "$pathname"]

  # If this is not an absolute path, then prepend the working directory.
  if { [lindex $parts 0] != "/" } {
    if { "$basedir" == "." } {
      set basedir [pwd]
    }
    set parts [concat [file split $basedir] $parts]
  }

  # Process the parts list to remove relative links.
  set parts2 {}
  foreach part $parts {
    switch $part {
      "." {     # Drop this.
      }
      ".." {    # Drop this and the previous part.
        set parts2 [lrange $parts2 0 end-1]
      }
      default { # Keep this.
        lappend parts2 $part
      }
    }
  }

  # Rebuild the absolute path and return it.
  return [eval file join $parts2]
}

# Idem, but first resolve the links, if any.
# Tcl 8.4 equivalent: file normalize [file normalize $pathname/-]/..
# (normalize resolves links in paths, but not in files => Trick it to think it's
#  a path with a subfile called "-", then remove the extra level)
proc TrueName { pathname } {
  # Make sure the name is absolute.
  set pathname [AbsName "$pathname"]
  # First get the true directory name.
  set dir [file dirname "$pathname"]
  # If not the root directory, or for Windows the root dir for one of Windows drives
  if {("$dir" != "/") && ([string range $dir end-1 end] != ":/")} {
    set dir [TrueName "$dir"]
    set pathname [file join "$dir" [file tail "$pathname"]]
  }
  # Get the target of the link, if any.
  if { ! [catch {file readlink "$pathname"} target] } {
    return [TrueName [AbsName "$target" "$dir"]]
  }
  return "$pathname"
}

# The various names of this script
set absscript [AbsName $argv0]       ; # The absolute pathname of this script.
set truescript [TrueName $argv0]     ; # And what really lies behind links.
set script [file tail $truescript]   ; # File name to use for messages.

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
#    2005/01/15 JFL Created this routine.                                     #
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

# Read a file, trimming end spaces, and returning an empty string on error.
proc TrimReadFile {filename} {
  return [string trim [ReadFile $filename {return ""}]]
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

# Write data to a file, appending a newline for readability.
proc PutsFile {name data} {
  WriteFile $name "$data\n"
}

# Touch a file, like the Unix touch command
proc Touch {name} {
  WriteFile $name [ReadFile $name [list return ""]]
}

# Unix-specific version
# Find a program among optional absolute pathnames, else in the PATH.
proc Which {prog args} { # prog=Program Name; args=Optional absolute pathnames
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

# Windows + Unix generic version
# Find a program among optional absolute pathnames, else in the PATH.
switch $tcl_platform(platform) { # Platform-specific PATH delimiter
  "windows" {
    set pathDelim ";"
    set pathExts {.com .exe .bat .cmd} ; # Default if not explicitely defined
  }
  "unix" - default {
    set pathDelim ":"
    set pathExts {} ; # Unix does not have implicit program extensions.
  }
}
if [info exists ::env(PATHEXT)] { # Windows list of implicit program extensions
  set pathExts [split $::env(PATHEXT) $pathDelim]
}
set pathExts [linsert $pathExts 0 ""] ; # In all cases, try the exact name first.
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
    foreach ext $::pathExts {
      if [file executable "$name$ext"] {
	return "$name$ext"
      }
    }
  }
  return ""
}

# Make a link, deleting conflicting files, if any, to avoid errors.
proc MakeLink {link target} {
  file mkdir [file dirname $link]
  set err [catch {
    if {"[file link $link]" != "$target"} error
  }]
  if $err {
    file delete -force $link
    file link $link $target ; # Create the OCF init script as a link
  }
}

# Flush all pending input from a channel (Typically stdin)
proc FlushInput {{channel stdin}} {
  set buffering [fconfigure $channel -buffering]
  set blocking [fconfigure $channel -blocking]
  fconfigure $channel -buffering none -blocking 0
  set flushed [read $channel 10000]
  fconfigure $channel -buffering $buffering -blocking $blocking
  return "$flushed"
}

###############################################################################
#                         Web file management routines                        #
###############################################################################

# Note: Use the more portable http package: "package require http" etc.

# Get the content of a web file
proc wget {url} {
  set err [catch {exec wget -q -O - $url} output]
  if $err {
    set output ""
  }
  return $output
}

###############################################################################
#                         Network management routines                         #
###############################################################################

# Get the host short name
proc Hostname {} {
  # Get qualified host name "host.domain.realm"
  set hostname [info hostname] ; # Depending on system, returns short or long name.
  # Extract the local host name, before the first dot
  regexp {[^\.]*} $hostname hostname
  # return the host name
  return $hostname
}
set hostname [Hostname]

# Get the IP address of a host from /etc/hosts. Return "" if not found.
proc EtcHosts2IP {host} {
  set hosts [ReadFile /etc/hosts {return ""}]
  regsub -all {\\\n} $hosts " " hosts ; # Merge continued lines
  regsub -all {[#][^\r\n]*} $hosts {} hosts ; # Remove comments
  set ip ""
  set rx {([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)([ \t]+\S*)*[ \t]}
  append rx "$host" {[ \t\r\n]}
  regexp $rx "$hosts " - ip
  return $ip
}

# Get the host address directly or indirectly. Return "" if not found.
proc GetHostIP {host} {
  set ip [EtcHosts2IP $host] ; # 1) Look in /etc/hosts
  if {"$ip" == ""} {         ; # 2) Use external resolution
    set err [catch {exec host $name} output]
    if !$err {
      regexp {.*has address (.*)} $output line ip
    }
  }
  return $ip
}

# Get the IP address of a remote host using sockets for resolution.
proc Host2IP {host} {
  set addr ""
  set err [catch {
    set s [socket $host ssh]
    set conf [fconfigure $s -peername]
    close $s
    set addr [lindex $conf 0]
    DebugPuts "Host2IP: $host address is $addr"
  } errMsg]
  if $err {DebugPuts "Host2IP: Cannot get $host address: $errMsg"}
  return $addr ; # Host not found.
}

# Get the known names for a given IP address
proc Ip2Names {ip} {
  set hosts [ReadFile /etc/hosts {return ""}]
  regsub -all {\\\n} $hosts " " hosts ; # Merge continued lines
  regsub -all {[#][^\r\n]*} $hosts {} hosts ; # Remove comments
  set names {}
  foreach {- tail} [regexp -all -line -inline "^\\s*$ip\\s+(.*)$" $hosts] {
    foreach name $tail {
      lappend names $name
    }
  }
  return $names
}

# Dual mode Unix + Windows versions

# Read the OS-specific hosts file
proc ReadHosts {} {
  if {"$::tcl_platform(platform)" == "windows"} {
    set hosts "$::env(windir)\\System32\\Drivers\\etc\\hosts"
  } else { # Unix
    set hosts "/etc/hosts"
  }
  set hosts [TrimReadFile $hosts]
  regsub -all {\\\n} $hosts " " hosts ; # Merge continued lines
  regsub -all {[#][^\r\n]*} $hosts {} hosts ; # Remove comments
  return $hosts
}

# Get the IP address of a host from /etc/hosts. Return "" if not found.
proc EtcHosts2IPs {host} {
  set hosts [ReadHosts]
  set rx {([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)([ \t]+\S+)*[ \t]+}
  append rx "$host" {[ \t\r\n]}
  set ips {}
  # Ignore case as Windows does not care. As for Unix, having homonym systems
  # differing only by case is definitely bad practice, so assuming there's none.
  foreach {- ip -} [regexp -all -nocase -inline $rx "$hosts "] {
    lappend ips $ip
  }
  return $ips
}
proc EtcHosts2IP {host} {
  lindex [EtcHosts2IPs $host] 0
}

# Get the host address directly or indirectly. Return "" if not found.
proc GetHostIPs {host} {
  set ips [EtcHosts2IPs $host] ; # 1) Look in /etc/hosts
  if {"$ips" == ""} {          ; # 2) Use external resolution
    if {"$::tcl_platform(platform)" == "windows"} {
      PackageRequire dns
      # Get the domain name suffix search list
      set key {HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\Tcpip\Parameters}
      set domains [split [registry get $key "SearchList"] ","]
      if {[string first "." $host] != -1} { # If the host name is already qualified
        set hosts [list $host]            ; # Use it as it is
      } else {                              # Else build a list of possible qualified names
        set hosts {}
        foreach domain $domains {
          lappend hosts "$host.$domain"
        }
      }
      foreach name $hosts {
        set tok [dns::resolve $name]
        if {"[dns::status $tok]" == "ok"} {
          set ips [dns::address $tok]
          dns::cleanup $tok
          break
        }
        dns::cleanup $tok
      }
    } else { # Unix
      set err [catch {exec host $host} output]
      if !$err {
        foreach {- ip} [regexp -all -inline {has address (\S+)} $output] {
          lappend ips $ip
        }
      }
    }
  }
  return $ips
}
proc GetHostIP {host} {
  lindex [GetHostIPs $host]
}

# Get the known names for a given IP address
proc GetIpNames {ip} {
  set names ""
  if {"$::tcl_platform(platform)" == "windows"} {
    PackageRequire dns
    set tok [dns::resolve $ip]
    if {"[dns::status $tok]" == "ok"} {
      set names [dns::name $tok]
    }
    dns::cleanup $tok
  } else { # Unix
    set err [catch {exec host $ip} output]
    if !$err {
      if [regexp {name pointer (\S+)\.$} $output - name] {
        set names [list $name]
      }
    }
  }
  return $names
}

# Convert an IP Address string 1.2.3.4 to a 32-bits integer
proc Ip2DWord {ip} {
  set dw 0
  foreach byte [split $ip .] {
    set dw [expr ($dw << 8) + $byte]
  }
  return $dw
}

# Convert a 32-bits integer to an IP Address string 1.2.3.4
proc DWord2Ip {dw} {
  set ip ""
  for {set i 0} {$i < 4} {incr i} {
    set byte [expr $dw % 256]
    set dw [expr $dw / 256]
    set ip [linsert $ip 0 $byte]
  }
  join $ip .
}

# Compute the number of bits that differ between two IP addresses.
proc IpBitsDiff {ip0 ip1} {
  set dw0 [Ip2DWord $ip0]
  set dw1 [Ip2DWord $ip1]
  set dw0 0x[format %X $dw0] ; # Make sure it's hexadecimal, else XOR fails.
  set dw1 0x[format %X $dw1] ; # Make sure it's hexadecimal, else XOR fails.
  set dw [expr $dw0 ^ $dw1]
  for {set nb 0} {$dw != 0} {incr nb} {set dw [expr $dw >> 1]}
  return $nb
}

# Convert an IP subnet mask into a number of bits. Ex: 255.255.255.0 -> 24
proc Mask2Bits {mask} {
  set dw [Ip2DWord $mask] ; # Top N bits set
  set dw 0x[format %X $dw] ; # Make sure it's hexadecimal, else XOR fails.
  set dw [expr $dw ^ 0xFFFFFFFF]        ; # Complement => low 32-N bits set.
  for {set nb 32} {$dw != 0} {incr nb -1} {set dw [expr $dw >> 1]}
  return $nb
}

# Convert a number of bits into an IP subnet mask. Ex: 24 -> 255.255.255.0
proc Bits2Mask {nb} {
  set dw [expr ((1 << (32 - $nb)) - 1) ^ 0xFFFFFFFF]
  DWord2Ip $dw
}

# Make sure the IP address is in its canonic form, ie the shortest one.
proc TrimIpAddress {ip} {
  set bytes {}
  foreach byte [split $ip .] {
    regsub -all {^0*} $byte {} byte
    if {"$byte" == ""} {set byte 0}
    lappend bytes $byte
  }
  join $bytes .
}

# Remove a server name from the hosts data. Returns the updated hosts data.
proc EtcHostsRemove {hosts name {infoVar ""}} {
  if {"$infoVar" != ""} {
    upvar 1 $infoVar info
  }
  set newHosts ""
  # Split lines, avoiding to add a final empty line if hosts ends by \n.
  set lines [split $hosts \n]
  if {"[string index $hosts end]" == "\n"} {
    set lines [lrange $lines 0 end-1]
  }
  foreach line $lines {
    regexp {^([^#]*)(#.*)?$} $line - data comment ; # Split the data and comments
    regsub -all {\s+} $data { } data  ; # Shorten separators to a single space
    set data [string trim $data]      ; # Remove outside separators
    set fields [split $data]          ; # Build a list of words
    set ipaddr [lindex $fields 0]     ; # The IP address is the first word
    set hostnames [lrange $fields 1 end] ; # And hosts names follow
    set ix [lsearch $hostnames $name] ; # Search for the host
    if {$ix != -1} { # Found our target host.
      set msg "Remove $ipaddr $name from /etc/hosts"
      DebugPuts "EtcHostsRemove: $msg"
      appends info "  -> $msg"
      if [Debug] { # In debug mode, leave the old line commented out.
	appends newHosts "# $line"
      }
      set hostnames [lreplace $hostnames $ix $ix] ; # Remove the hosts
      if ![llength $hostnames] continue ; # Remove lines with no more host.
      # Else rebuild the line
      set data [concat $ipaddr $hostnames]
      if {"$comment" != ""} {
	set comment [list $comment]
      }
      set line [join [concat $data $comment]]
    }
    appends newHosts $line
  }
  return $newHosts
}

# Return the system's list of DNS domain name suffixes to search for unqualified names.
# 2014-11-12 JFL Added Windows Group Policy & WMI methods.
# 2015-06-22 JFL Added search of primary and connection-specific DNS names.
#                This avoids the dependancy on twapi in most cases.
proc RegistryGetValueDefault {key value {default ""}} { # Get a registry value, and use a default if the value does not exist 
  set data $default
  catch {set data [registry get $key $value]}
  return $data
}
proc DnsSearchList {} {
  if {"$::tcl_platform(platform)" == "windows"} {
    package require registry
    set list {}
    # Get the domain name suffix search list
    # 1) If set by a Group Policy, it's defined in the registry here:
    # HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows NT\DNSClient\SearchList
    set key {HKEY_LOCAL_MACHINE\SOFTWARE\Policies\Microsoft\Windows NT\DNSClient}
    set list [split [RegistryGetValueDefault $key SearchList] ,]
    XDebugPuts "$key\\SearchList: $list"
    # 2) If set from the network control panel, it's defined in the registry there:
    # HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters\SearchList
    set key {HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters}
    if {"$list" == ""} {
      set list [split [RegistryGetValueDefault $key SearchList] ,]
    }
    XDebugPuts "$key\\SearchList: $list"
    # 3) Else Windows will use the primary and connection-specific DNS names, in the subkeys there:
    # HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\services\Tcpip\Parameters\Interfaces\*\Domain or DhcpDomain
    if {"$list" == ""} {
      # Get the primary domain
      set domain [RegistryGetValueDefault $key Domain]	;# First try the manually set domain
      XDebugPuts "$key\\Domain: $domain"
      # Gotcha: Don't use DhcpDomain, which contains the last interface that got a DhcpSetting, even if it lost it since then!
      # if {$domain == ""} {				 # If there's none, try the one provided by DHCP
      #   set domain [RegistryGetValueDefault $key DhcpDomain]
      #   XDebugPuts "$key\\DhcpDomain: $domain"
      # }
      if {$domain != ""} {
	if {[lsearch $list $domain] == -1} { # Avoid reentering it if it's already listed.
	  lappend list $domain
	}
      }
      # Then find every connection-specific domain
      set subkeys ""
      catch {set subkeys [registry keys "$key\\Interfaces"]}
      foreach subkey $subkeys {
	set subkey "$key\\Interfaces\\$subkey"
	set domain [RegistryGetValueDefault $subkey Domain]	;# First try the manually set domain
	XDebugPuts "$subkey\\Domain: $domain"
	if {$domain == ""} {				    	 # If there's none, try the one provided by DHCP
	  # Gotcha: VPNs have the DhcpDomain setting left in, even when they're disconnected.
	  # Check the IP address to know if it's connected.
	  set ipaddress [RegistryGetValueDefault $subkey DhcpIPAddress "0.0.0.0"]
	  if {$ipaddress != "0.0.0.0"} {			 # So if the network is currently connected
	    set domain [RegistryGetValueDefault $subkey DhcpDomain]
	    XDebugPuts "$subkey\\DhcpDomain: $domain"
	  }
	}
	if {$domain != ""} {
	  if {[lsearch $list $domain] == -1} { # Avoid reentering it if it's already listed.
	    lappend list $domain
	  }
	}
      }
    }
    # 4) If set by DHCP option 119:
    # Not stored in the registry. Use WMI to query the network adapters configuration
    if {"$list" == ""} {
      PackageRequire twapi
      set wmi [twapi::_wmi]
      $wmi -with {
	{ExecQuery "select * from Win32_NetworkAdapterConfiguration"}
      } -iterate conf {
      	# DebugPuts "[$conf Index] [$conf Description]"
	set list [$conf DnsDomainSuffixSearchOrder]
	XDebugPuts "[$conf Description]: $list"
	# Stop at the first adapter that has the list defined, as they all have the same one
	if {"$list" != ""} break
      }
      $wmi -destroy
    }
  } else { # Unix
    set hConf [open /etc/resolv.conf r]
    set list {}
    while {[gets $hConf line]} {
      if [regexp {^\s*search\s+(.*)} $line - list] break
    }
    close $hConf
  }
  return $list
}

###############################################################################
#                           TcXML trees management                            #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    TreeGet                                                   #
#                                                                             #
#   Description     Extract data from a file tree serialized as a Tcl tree.   #
#                                                                             #
#   Parameters      tree            Serialized file tree                      #
#                   path            File relative pathname                    #
#                                                                             #
#   Returns 	    File or directory contents, or error out if not found.    #
#                                                                             #
#   Notes:	    Similar in concept to XML/XPath, Tcl-style => readable!   #
#                   Format:                                                   #
#                      filename value                                         #
#                      dirname/ subtree                                       #
#                                                                             #
#   History:								      #
#    2008/03/25 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

proc TreeGet {tree path} {
  set path [string trim $path /]
  if {"$path" == ""} {
    return $tree
  }
  foreach {name value} $tree {
    if {"[string index $name end]" == "/"} { # This is a subdirectory.
      set listPath [split $path /]
      set headPath [lindex $listPath 0]
      set tailPath [lrange $listPath 1 end]
      if {"$name" == "$headPath/"} {
	if {"$tailPath" != ""} {
	  set value [TreeGet $value [join $tailPath /]]
	} else {
	  regexp {\n?(.*\S)\s*$} $value - value ; # Remove blank head & tail lines.
	}
	regsub -line -all {^  } $value "" value
	return $value
      }
    } else {				    # This is a file.
      if {"$name" == "$path"} {
	return $value
      }
    }
  }
  error "Path not found: $path"
}

# Get the list of subdirectories
proc TreeGetDirs {tree {path ""}} {
  if {"$path" != ""} {
    set tree [TreeGet $tree $path]
  }
  set dirs {}
  foreach {name value} $tree {
    if {[regexp {(.*)/$} $name - dir]} { # This is a subdirectory.
      lappend dirs $dir
    }
  }
  return $dirs
}

# Get the list of files
proc TreeGetFiles {tree {path ""}} {
  if {"$path" != ""} {
    set tree [TreeGet $tree $path]
  }
  set files {}
  foreach {name value} $tree {
    if {"[string index $name end]" != "/"} { # This is a file
      lappend files $name
    }
  }
  return $files
}

###############################################################################
#                                Misc routines                                #
###############################################################################

# Check if a procedure exists
proc ProcExists {p} {
  expr [lsearch -exact [info procs $p] $p] != -1
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    GetPeerName		                           	      #
#                                                                             #
#   Description     Get the peer node name in a RedHat cluster pair.	      #
#                                                                             #
#   Parameters      None                                                      #
#                                                                             #
#   Returns 	    Peer short name, or "" if failed.                         #
#                                                                             #
#   Notes:	    Gets information from the local clumanager database.      #
#		    Does not need to connect to the SFS system database.      #
#                                                                             #
#   History:								      #
#    2005/10/06 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set peerName ""

proc GetPeerName {} {
  if {"$::peerName" != ""} {
    return $::peerName ; # Return the cached result if available
  }

  regexp {[^\.]*} [info hostname] myname ; # set myname = My short name

  set err [catch "exec cludb -g members%member0%name" name0]
  if $err { set name0 "" }
  set err [catch "exec cludb -g members%member1%name" name1]
  if $err { set name1 "" }

  if {"$name0" == "$myname"} {
    set ::peerName $name1
  } else {
    set ::peerName $name0
  }

  return $::peerName
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    GetPassword		                           	      #
#                                                                             #
#   Description     Input a password, displaying only stars.		      #
#                                                                             #
#   Parameters      None                                                      #
#                                                                             #
#   Returns 	    The password string.                                      #
#                                                                             #
#   Notes:	    See http://wiki.tcl.tk/14693 for details.                 #
#                                                                             #
#   History:								      #
#    2009/03/10 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

switch $tcl_platform(platform) {
  "unix" { # Unix version of the routines
    # Set the console driver raw mode, to avoid it buffering input lines and echoing them.
    proc EnableRawMode {{channel stdin}} {
      exec /bin/stty raw -echo <@$channel
    }
    # Set the console driver cooked mode, buffering input lines and echoing them.
    proc DisableRawMode {{channel stdin}} {
      exec /bin/stty -raw echo <@$channel
    }
  }
  "windows" { # Windows NT/2000/XP/Vista version of the routines
    # Set the console driver raw mode, to avoid it buffering input lines and echoing them.
    proc EnableRawMode {{channel stdin}} {
      PackageRequire twapi
      set console_handle [twapi::GetStdHandle -10]
      set oldmode [twapi::GetConsoleMode $console_handle]
      set newmode [expr {$oldmode & ~6}] ;# Turn off the echo and line-editing bits
      twapi::SetConsoleMode $console_handle $newmode
    }
    # Set the console driver cooked mode, buffering input lines and echoing them.
    proc DisableRawMode {{channel stdin}} {
      PackageRequire twapi
      set console_handle [twapi::GetStdHandle -10]
      set oldmode [twapi::GetConsoleMode $console_handle]
      set newmode [expr {$oldmode | 6}] ;# Turn on the echo and line-editing bits
      twapi::SetConsoleMode $console_handle $newmode
    }
  }
  default { # Macintosh for example - To be implemented
    proc EnableRawMode {{channel stdin}} {}
    proc DisableRawMode {{channel stdin}} {}
  }
}
# Get a password from the user, displaying stars instead of the actual input
proc GetPassword {} {
  set pw ""
  set buffering [fconfigure stdin -buffering]
  set blocking [fconfigure stdin -blocking]
  EnableRawMode
  fconfigure stdin -buffering none -blocking 1
  while {![eof stdin]} {
    set c [read stdin 1]
    switch -- $c {
      "\r" - "\n" {
	puts -nonewline "\n"
	break
      }
      "\x08" - "\x7F" {
	if {"$pw" != ""} {
	  set pw [string replace $pw end end]
	  puts -nonewline "\x08 \x08"
	  flush stdout
	}
      }
      default {
	append pw "$c"
	puts -nonewline "*"
	flush stdout
      }
    }
  }
  DisableRawMode
  fconfigure stdin -buffering $buffering -blocking $blocking
  return $pw
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function	    RomanNumeral		                       	      #
#                                                                             #
#   Description     Convert an integer to its roman numeral representation    #
#                                                                             #
#   Parameters      i = The integer to convert                                #
#                                                                             #
#   Returns 	    The corresponding roman numeral                           #
#                                                                             #
#   Notes:	                                                              #
#                                                                             #
#   History:								      #
#    2014-11-30     Copied from http://aidanhs.github.io/emtcl/               #
#                                                                             #
#-----------------------------------------------------------------------------#

proc RomanNumeral {i} {
  set num_map {
    1000 M 900 CM 500 D 400 CD
    100 C 90 XC 50 L 40 XL
    10 X 9 IX 5 V 4 IV 1 I
  }
  set res ""
  foreach {value roman} $num_map {
    while {$i >= $value} {
      append res $roman
      incr i -$value
    }
  }
  return $res
}

###############################################################################
#                                Main routines                                #
###############################################################################

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
Usage: $argv0 [options] [command [subcommand]]

Commands:
  start                 Starts the program
  stop                  Stops the program

Options:
  -c, --commands CMDLINES  Execute each command
  -d, --debug           Enable the debug mode, and display debug messages
  -h, --help, -?        Display this help screen
  -q, --quiet           Output as little as possible
  -v, --verbose         Display verbose messages
  -V, --version         Display this library version
  -xd, --xdebug         Enable the xdebug mode, and display debug debug messages
}]

set initial_argv $argv          ; # Keep the initial command line for debug.
set command ""
set subcommand ""

# Scan all arguments.
set args $argv
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-c" - "--commands" {	# Execute each command
      foreach command $args {
      	DebugPuts $command
      	set result [Indent $command]
      	DebugPuts "  return $result"
      }
      set args {}
    }
    "-d" - "--debug" {		# Enable debug mode
      SetDebug
    }
    "-h" - "--help" - "-?" {	# Display a help screen and exit.
      puts $usage
      exit 0
    }
    "-q" - "--quiet" {		# Enable quiet mode
      set verbosity 0
    }
    "-v" - "--verbose" {	# Increase the verbosity level
      SetVerbose
    }
    "-V" - "--version" {	# Display this library version
      puts $version
      exit 0
    }
    "-xd" - "--xdebug" {
      SetXDebug
    }
    default {
      if {"$command"==""} {		; # If the command is not set...
	set command $arg   
      } elseif {"$subcommand"==""} {	; # If the subcommand is not set...
	set subcommand $arg
      } else {                          ; # Anything else is an error.
        Puts stderr "Unrecognized argument: $arg"
	puts stderr $usage
	exit 1
      }
    }
  }
}
if {$command == ""} {
  puts $usage
  exit 0
}

