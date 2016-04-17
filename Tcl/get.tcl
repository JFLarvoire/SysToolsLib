#!/usr/bin/tclsh
#-----------------------------------------------------------------------------#
#                                                                             #
#   Script name     get.tcl                                                   #
#                                                                             #
#   Description     Get a file from a web server.                             #
#                                                                             #
#   Notes           Replaces my old get.bat, as this was becoming too complex #
#                   to code in Windows batch language.                        #
#                                                                             #
#   History                                                                   #
#    2007-11-17 JFL Created this script.                                      #
#    2007-12-06 JFL Implemented the -p and -P options. Updated help.          #
#    2010-03-05 JFL Fixed a bug which prevented from using - as the output.   #
#    2011-02-28 JFL Output a new line after all progress dots.                #
#    2011-03-16 JFL Added routines to automatically download missing packages.#
#    2013-05-28 JFL Make sure progress dots are displayed in real time.       #
#                                                                             #
#-----------------------------------------------------------------------------#

# Set global defaults
set script [file tail $argv0]   ; # This script name.
set verbosity 1			; # 0=Quiet 1=Normal 2=Verbose 3=Debug

proc Quiet {} {expr $::verbosity == 0}
proc Verbose {} {expr $::verbosity > 1}
proc Debug {} {expr $::verbosity > 2}

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
proc WriteFile {name data} {
  catch {
    set hf [open $name w]
    puts -nonewline $hf $data
    close $hf
  }
}

# Read a value from Windows registry. Optional 3rd arg = default value.
package require registry
proc GetRegistryValue {key value args} {
  if [llength $args] { # If a default value has been provided
    set onError [list return [lindex $args 0]]
  } else {             # No default value. Error-out.
    set onError [list error "Cannot read registry $key $value"]
  }
  set err [catch {registry get $key value} content]
  if $err $onError
  return $content
}

#-----------------------------------------------------------------------------#
#                      Automatic package loading routine                      #
#-----------------------------------------------------------------------------#

set checkedProxy 0
set webProxy ""

# Try to guess the web proxy
proc PackageCheckProxy {} {
  if $::checkedProxy return ; # Don't do it again if we already did it.
  set ::checkedProxy 1
  switch $::tcl_platform(platform) {
    "windows" {
      set err [catch {exec ipconfig | findstr IPv4} output]
    }
    default {
      set err [catch {exec ifconfig  | grep "inet add"} output]
    }
  }
  if $err {
    error "Can't get this system IP address while searching proxy. $output"
  }
  # Find all IP addresses
  set ips [regexp -all -inline {\d+\.\d+\.\d+\.\d+} $output]
  foreach ip $ips {
    if [Debug] {
      puts "set ip $ip"
    }
    if [regexp {^1[56]\.} $ip -] {
      set ::webProxy "web-proxy:8080"
      return
    }
  }
}

proc PackageDownload {package} {
  if {"$::webProxy" == ""} {
    PackageCheckProxy
  }
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
      puts stderr "Info: The required Tcl package \"$package\" is missing. Trying to download and install it."
      PackageDownload $package
      # And try again
      return [uplevel 1 package require $package $args]
    }
    error $output
  }
  return $output
}

#-----------------------------------------------------------------------------#
#                       Debugging and logging routines                        #
#-----------------------------------------------------------------------------#

# Output a string and log it.
# Arguments:
#   -1              Ignore 1 indent level. Ignored.
#   -show [0|1]     Whether to output on stdout. Optional. Default: 1=yes
#   -noprefix       Do not prefix the application name before the output.
#   --              End of Puts options. Optional.
#   -nonewline      Don't output a new line. Optional.
#   tclChannel      Tcl file handle for output. Optional. Default: stdout
#   string          The last argument = the string to output.
proc Puts {args} {
  set show 1            ; # 1=Output the string on stdout.
  set prefix ""		; # set prefix "${::script}: "
  while {"$args" != ""} {
    set arg [PopArg]
    switch -- $arg {
      -1             { }
      -noprefix      { set prefix "" }
      -show          { set show [PopArg] }
      --             { break }
      default        { set args [linsert $args 0 $arg] ; break }
    }
  }
  set msg [lindex $args end]
  if $show {                # Output the message if requested
    set args [lreplace $args end end "$prefix$msg"]
    if {![catch {eof stdout}]} { # Avoid error if stdout is closed!
      catch {eval puts $args}
    }
  }
}

# Outputing a string in verbose or debug modes only.
# Arguments:
#   options         Options to pass to Puts. Default: None.
#   string          The last argument = the string to output.
proc VerbosePuts {args} {
  eval Puts -show [Verbose] $args
}

# Indent multiple lines
proc IndentString {text {indent 2}} {
  set spaces [format "%*s" $indent ""]
  regsub -all -line {^} $text $spaces text
  return $text
}

# Output a string, indented, in debug modes only.
# Arguments:
#   -1              Ignore 1 indent level. Can be repeated.
#   options         Options to pass to Puts. Default: None.
#   string          The last argument = the string to output.
proc DebugPuts {args} {
  set options [lrange $args 0 end-1]
  set string [lindex $args end]
  set indent [expr [info level] - 1]
  set args2 {}
  foreach arg $options {
    if {"$arg" == "-1"} {
      incr indent -1
    } else {
      lappend args2 $arg
    }
  }
  incr indent $indent ; # Each indent level is 2 characters.
  lappend args2 [IndentString $string $indent]
  eval Puts -show [Debug] $args2
}

# Format array contents with once element per line
proc FormatArray {a} {
  upvar 1 $a a1
  set string ""
  foreach {name} [lsort -dictionary [uplevel 1 array names $a]] {
    append string [format "%-23s %s\n" $name [list $a1($name)]]
  }
  return $string
}

# Generate a string listing variables names=values.
# Arguments:
#   args          A list of variables names
proc VarsValue {args} {
  set l {}
  foreach arg $args {
    if {![uplevel 1 info exists $arg]} {       # Undefined variable
      lappend l "$arg=!undefined!"
    } elseif {[uplevel 1 array exists $arg]} { # Array name
      lappend l "$arg=array{\n[uplevel 1 [namespace current]::FormatArray $arg]}"
    } else {                                   # Scalar variable
      lappend l "$arg=[list [uplevel 1 set $arg]]"
    }
  }
  join $l " "
}

# Output variables values.
# Arguments:
#   args          A list of variables names
proc PutVars {args} {
  Puts [uplevel 1 [namespace current]::VarsValue $args]
}

# Output a string and variables values.
# Arguments:
#   string        An introduction string.
#   args          A list of variables names
proc PutSVars {string args} {
  Puts "$string [uplevel 1 [namespace current]::VarsValue $args]"
}

# Output variables values in debug mode only.
# Arguments:
#   args          A list of variables names
proc DebugVars {args} {
  DebugPuts -1 [uplevel 1 [namespace current]::VarsValue $args]
}

# Output a string and variables values in debug mode only.
# Arguments:
#   string        An introduction string.
#   args          A list of variables names
proc DebugSVars {string args} {
  DebugPuts -1 "$string [uplevel 1 [namespace current]::VarsValue $args]"
}

#-----------------------------------------------------------------------------#
#                      Html and http management routines                      #
#-----------------------------------------------------------------------------#

# Get a text file from a web server
proc wwwget1 {url} {
  set name [http::geturl $url]
  http::data $name
}
proc wwwget {url} {
  set cmd {exec wwwget -a}
  if {"$::webProxy" != ""} {
    lappend cmd "-p$::webProxy"
  }
  # Note: There's  bug in Windows' wwwget stdout output: It inserts dots at
  #       random places. This does not happen when writing to a temp file.
  # (These dots are the progress dots displayed regularly during long dowloads)
  set tmpFile "$::env(TEMP)\\wwwget.tmp"
  lappend cmd $url $tmpFile
  set err [catch $cmd output]
  if $err {
    set output ""
  } else {
    set output [ReadFile $tmpFile {return ""}]
    if [regexp {404 Page Not Found} $output -] {
      set output ""
    }
  }
  # regsub "^Reading from $url " $output "" output ; # Remove the wwwget banner.
  return $output
}

# JFL 2011-03-15 Superseeded by the new PackageRequire routine. 
# Initialize wwwget web proxy option
# Ex: grewebcachevip.bastion.europe.hp.com:8080  or  web-proxy:8088
# set webProxy ""
# package require registry
# set ieKeys {HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings}
# set autoConfigURL [GetRegistryValue $ieKeys AutoConfigURL ""]
# if {"$autoConfigURL" != ""} {
#   set autoConfig [wwwget $autoConfigURL]
#   regexp {"PROXY (\S+)"} $autoConfig - webProxy
# } else {
#   set proxyEnable [GetRegistryValue $ieKeys ProxyEnable 0]
#   if $proxyEnable {
#     set proxyServer [GetRegistryValue $ieKeys ProxyServer ""]
#     if {"$proxyServer" != ""} {
#       set webProxy $proxyServer
#     }
#   }
# }

# Get a file from a web server or from a local cache file.
proc WwwGetCached {url cacheFile} {
  if {$::fromCache && [file exists $cacheFile]} {
    set html [ReadFile $cacheFile {return ""}]
  } else {
    set html [wwwget $url]
    if {$::cache} {
      WriteFile $cacheFile $html
    }
  }
  return $html
}

# Convert %XX codes in URL strings to ASCII characters
proc RemovePercent {string} {
  set result ""
  set l [string length $string]
  for {set i 0} {$i < $l} {incr i} {
    set c [string index $string $i]
    if {"$c" == "%"} {
      set c [string index $string [incr i]]
      append c [string index $string [incr i]]
      set c [format "%c" "0x$c"]
    }
    append result $c
  }
  return $result
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
#    2007/10/27 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
Usage: get [switches] {URL} [filename]

Switches:
 -c|--convert       Convert output to ASCII with CRLFs
 -h|--help          Display this help screen and exit
 -p|--proxy PROXY   Use the specified proxy server
 -P|--no-proxy      Do NOT use a proxy server
 -q|--quiet         Do not output URL and progress information

Default file name: The file name from the URL.
Use - to output to the standard output.

Default proxy server: Look into Internet Explorer internet settings.
}]
#  -l user pwd        User name and password

# Scan all arguments.
set args $argv
set noOptions 0
set url ""
set filename ""
set convert 0
set exec 1
while {"$args" != ""} {
  set arg [PopArg]
  if {[string match -?* $arg] && !$noOptions} {
    switch -- $arg {
      "--" {
	set noOptions 1
      }
      "-c" - "--convert" {
	set convert 1
      }
      "-d" - "--debug" {
	incr verbosity 2
      }
      "-h" - "--help" - "-?" {
	puts $usage
	exit 0
      }
      "-p" - "--proxy" { # Use proxy
	set webProxy [PopArg]
      }
      "-P" - "--no-proxy" { # Do NOT use a proxy
	set webProxy ""
      }
      "-q" - "--quiet" { # Verbose flag
	incr verbosity -1
      }
      "-v" - "--verbose" { # Verbose flag
	incr verbosity
      }
      "-X" - "--noexec" { # No execution flag
	set exec 0
      }
      default {
        puts stderr "Unrecognized option: $arg. Ignored."
      }
    }
  } else {
    if {"$url" == ""} {
      set url $arg
      continue
    }
    if {"$filename" == ""} {
      set filename $arg
      continue
    }
    puts stderr "Argument inattendu: $arg. Ignoré."
  }
}

if {"$url" == ""} {
  puts "No URL specified: $argv0 $argv"
  puts $usage
  exit 1
}

if {"$filename" == ""} {
  set filename [file tail $url]
  set filename [RemovePercent $filename]
}

# PackageRequire tdom
PackageRequire uri
PackageRequire html
::html::init
PackageRequire http
#PackageRequire autoproxy
#autoproxy::init
# autoproxy::configure -basic -username ME -password SEKRET

set cmd wwwget
if {$convert} {
  lappend cmd "-a"
}
if {"$::webProxy" != ""} {
  lappend cmd "-p$::webProxy"
}
lappend cmd $url
if {"$filename" != "-"} {
  lappend cmd $filename
} else {
  # Always use a temporary file, to make sure all progress dots are displayed first.
  set stdoutFile $env(TEMP)\\get[pid].tmp
  lappend cmd $stdoutFile
}
if {!$exec} {
  puts $cmd
  exit 0
}
if ![Quiet] {
  lappend cmd >@stdout
} else {
  lappend cmd >NUL
}
set stderrFile $env(TEMP)\\get[pid].err
lappend cmd 2>$stderrFile
set cmd [linsert $cmd 0 exec]
DebugPuts $cmd
set err [catch $cmd output]
if $err {
  puts stderr "Error: $output"
  set errMsg [TrimReadFile $stderrFile]
  if {"$errMsg" != ""} {
    puts stderr $errMsg
  }
} else {
  if {"$filename" == "-"} {
    if ![Quiet] {
      puts "" ;# wwwget does not output an \n after the last progress dot.
    }
    set fileContent [TrimReadFile $stdoutFile]
    if {"$fileContent" != ""} {
      puts $fileContent
    }
    if [file exists $stdoutFile] { # Cleanup the temp output file
      if ![Debug] {
	file delete $stdoutFile
      }
    }
  }
}
if [file exists $stderrFile] { # Cleanup the temp error file
  if ![Debug] {
    file delete $stderrFile
  }
}

