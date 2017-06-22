#!/usr/bin/tclsh
###############################################################################
#                                                                             #
#   Filename        cfdt.tcl                                                  #
#                                                                             #
#   Description     Display or change a file date & time                      #
#                                                                             #
#   Notes           Once upon a time, there was a DOS tool called CFDT.COM.   #
#                   This tool stopped working in year 2000.                   #
#                   Here's an attempt at doing the same and more in 2009.     #
#                                                                             #
#                   To do: Find a way to change the Create Time in Windows.   #
#                   To do: Find a way to change only the date or the time.    #
#                   To do: Make it a wish application, triggered by a         #
#                          context menu entry in Windows Explorer...          #
#                   To do: Find a way to change times with us, ns resolution. #
#                                                                             #
#   History                                                                   #
#    2009-10-18 JFL Created this program                                      #
#    2010-01-06 JFL Added support for JPEG images time.                       #
#                   Added options -c, -a and -s.                              #
#    2010-01-07 JFL Added option -i for JPEG images time.                     #
#    2010-10-29 JFL Allow reordering arguments as DATE TIME NAME [NAME ...]   #
#                   Added option -d and -X.                                   #
#    2011-01-04 JFL Bug fix: Support reordered dates in format YYYY/MM/DD.    #
#    2011-08-09 JFL Bug fix: Look for JPEG DateTimeOriginal before DateTime.  #
#                   Added a script version and option -V to display it.       #
#    2011-10-09 JFL Fixed bug if there is no image creation time in a .jpg.   #
#    2013-02-19 JFL Rewrote the argument parsing to make it order-independant.#
#    2014-09-18 JFL Added option --sequence.                                  #
#    2015-09-17 JFL Added option --from to copy the time of another file.     #
#    2015-10-15 JFL Added option --f2d to set dir times from files times.     #
#    2015-10-29 JFL Added options -z and --z2m to manage 7-Zip archive times. #
#    2017-06-22 JFL Bug fix: Do not display help if wildcards produce 0 names.#
#                   Display the list of files processed, to monitor progress. #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

set version "2017-06-22"
set script [file rootname [file tail $argv0]]
set verbosity 1
set noexec 0

proc Verbose {} {expr $::verbosity > 1} ; # Test if we're in verbose mode.
proc Debug {{n 0}} {expr $::verbosity > (2+$n)} ; # Test if we're in debug mode.

# Remove an argument from the head of a routine argument list.
proc PopArg {{name args}} {
  upvar 1 $name args
  set arg [lindex $args 0]              ; # Extract the first list element.
  set args [lrange $args 1 end]         ; # Remove the first list element.
  return $arg
}

# Get JPEG image time
set jpegInitDone 0
proc GetJpegImageTime {name} {
  if {!$::jpegInitDone} {
    set ::jpegInitDone 1
    set err [catch {
      package require jpeg
    } output]
    if $err {
      puts stderr "Error: $output"
      set ::jpegInitDone -1
    }
  }
  if {$::jpegInitDone == -1} {
    return ""
  }
  array set props [jpeg::getExif $name main]
  set time ""
  set rx {(\d\d\d\d)\D(\d\d)\D(\d\d)\D+(\d\d)\D(\d\d)\D(\d\d)}
  foreach prop {DateTimeOriginal DateTimeDigitized DateTime} { # Use the first valid one
    # DateTimeOriginal = Date picture taken 
    # DateTime = Date of the last modification 
    if {[info exists props($prop)] && [regexp $rx $props($prop) -]} {
      set datetime $props($prop)
      regsub $rx $datetime {\1-\2-\3 \4:\5:\6} datetime
      set time [clock scan $datetime]
      break
    }
  }
  return $time
}

# Get Windows file create time
set twapiInitDone 0
proc GetWindowsFileCreateTime {name} {
  if {!$::twapiInitDone} {
    set ::twapiInitDone 1
    set err [catch {
      package require twapi
    } output]
    if $err {
      puts stderr "Error: $output"
      puts stderr "See http://twapi.magicsplat.com/ for more information"
      set ::twapiInitDone -1
    }
  }
  if {$::twapiInitDone == -1} {
    return ""
  }
  set windowsTime [lindex [twapi::get_file_times $name -ctime] 1]
  twapi::large_system_time_to_secs_since_1970 $windowsTime
}

# Generic functions for getting various times
proc GetFileCreateTime {name} {
  set time ""
  if {"$::tcl_platform(platform)" == "windows"} {
    set time [GetWindowsFileCreateTime $name]
  }
  return $time
}

proc GetFileModifyTime {name} {
  file mtime $name
}

proc GetFileAccessTime {name} {
  file atime $name
}

proc GetFileImageTime {name} {
  set time ""
  if [regexp -nocase {.*\.jpe?g$} $name -] {
    catch {
      set time [GetJpegImageTime $name]
    }
  }
  return $time
}

set 7Zip ""
foreach var [list "ProgramFiles(x86)" "ProgramFiles" "ProgramW6432"] {
  set err [catch {
    set pgm $env($var)
  } msg]
  if $err continue
  if [file exists "$pgm\\7-Zip\\7z.exe"] {
    set 7Zip "$pgm\\7-Zip\\7z.exe"
  }
}

proc GetFileZipTime {name} {
  set mtime [file mtime $name]
  if {"$::7Zip" == ""} {
    error "This requires the 7-Zip program"
  }
  set err [catch {
    set listing [exec $::7Zip l $name]
  } msg]
  if $err {
    error "File $name is not a valid archive"
  }
  set mtime 0
  foreach line [split $listing \n] {
    if [regexp {^\d\d\d\d.\d\d.\d\d \d\d:\d\d:\d\d} $line date] {
      if [Debug] {puts "Found $date"}
      set date [clock scan $date]
      if {$date > $mtime} {
	set mtime $date
      }
    }
  }
  return $mtime
}

# Recursively copy the latest file time to the directory time
proc FilesTime2Dir {dirname {what mtime}} {
  if {[Debug]} {
    puts "FilesTime2Dir $dirname $what"
  }
  if ![file isdirectory $dirname] {
    error "'$dirname' is not a directory."
  }
  set dtime 0
  set names [lsort -dictionary [glob -nocomplain "$dirname/*"]]
  foreach name $names {
    if [file isdirectory $name] {
      set ftime [FilesTime2Dir $name $what] ;# Recursively update and return that time
    } else {
      set ftime [file $what $name]
    }
    if {$ftime > $dtime} {
      set dtime $ftime
    }
  }
  # Set the new date/time
  if $dtime { # If the directory is not empty, set its time based on its contents
    set err [catch {
      if {[Debug] || $::noexec} {
	set date [clock format $dtime -format "%Y-%m-%d"]
	set time [clock format $dtime -format "%H:%M:%S"]
	puts "file $what \"$dirname\" $dtime ;# $date $time"
      }
      if {!$::noexec} {
	file $what $dirname $dtime
      }
    } errMsg]
    if $err {
      puts stderr "Error: Cannot set directory $dirname $what. $errMsg"
      set ::exitCode 1
    }
  }
  return $dtime
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
#    2009-10-18 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

set usage [subst -nobackslashes -nocommands {
$script version $version
Display or change a file date & time

Usage: $script [OPTIONS] [FILENAME] [DATE] [TIME]
   or: $script [OPTIONS] DATE TIME FILENAME [FILENAME ...]

Options:
-a, --atime           Get/set the file access time
-c, --ctime           Get/set the file create time (Windows only)
-f, --from FILE       Copy the time from another file
    --f2d             Recursively copy the latest file time to the direct. time
-h, --help, -?        Display this help screen and exit
-i, --itime           Get the image create time (JPEG files only)
    --i2m             Set the file modify time from the image create time
    --i2n             Prefix the name with the image create time
    --m2n             Prefix the name with the file modify time
-m, --mtime           Get/set the file modify time (default)
-s, --shift N         Shift time by N seconds
-S, --sequence N      Set multiple files times, separated by N seconds each.
-v, --verbose         Display verbose information. Ex: Display all times.
-V, --version         Display the script version.
-X, --noexec          Display the commands to execute, but don't execute them
-z, --ztime           Get the most recent content time from a 7-Zip archive
    --z2m             Set the archive modify time from the latest content time

Filename:
A file pathname. May include wildcards if a single name is provided.

Date time:
Any combination of date and time recognized by Tcl. Ex: 2009-10-18 16:15:45
Default date: today
Default time: 00:00:00

Author: Jean-François Larvoire - jf.larvoire@hpe.com or jf.larvoire@free.fr
}]

# Process arguments
set args $argv
set name ""
set names {}
set date ""
set time ""
set what mtime
set add 0
set sequence 0
set action display ; # What to do. One of: display set copy rename add
set typeNames {ctime Create mtime Modify atime Access}
set nNames 0
while {"$args" != ""} {
  set arg [PopArg]
  switch -- $arg {
    "-a" - "--atime" { # Access time
      set what atime
    }
    "-c" - "--ctime" { # Create time
      set what ctime
    }
    "-d" - "--debug" { # Debug mode
      incr verbosity 2
    }
    "-f" - "--from" { # Copy time from that of another file
      set fromFile [PopArg]
      set err [catch {
        set ftime [file mtime $fromFile]
        set date [clock format $ftime -format "%Y-%m-%d"]
        set time [clock format $ftime -format "%H:%M:%S"]
      } msg]
      if {!$err} {
	set action set
      } else {
      	puts "Error: Failed to get file $fromFile time"
      	exit 1
      }
    }
    "--f2d" { # Copy the latest file time to the directory time
      set action files2dir
    }
    "-h" - "--help" - "-?" - "/?" {
      puts -nonewline $usage
      exit 0
    }
    "-i" - "--itime" { # Image time
      set what itime
      lappend typeNames itime Image
    }
    "--i2m" { # Copy Image time to Modification time
      set action copy
      set from itime
      lappend typeNames itime Image
    }
    "--i2n" { # Prefix the name with the image time
      set action rename
      set from itime
      lappend typeNames itime Image
    }
    "--m2n" { # Prefix the name with the file modify time
      set action rename
      set from mtime
    }
    "-m" - "--mtime" { # Modification time
      set what mtime
    }
    "-s" - "--shift" { # Change time by N seconds
      set action add
      set add [PopArg]
    }
    "-S" - "--sequence" { # Change time by N seconds
      set sequence [PopArg]
    }
    "-v" - "--verbose" { # Verbose flag.
      incr verbosity
    }
    "-V" - "--version" { # Display the script version.
      puts $version
      exit 0
    }
    "-X" - "--noexec" { # noexec mode
      set noexec 1
    }
    "-z" - "--ztime" { # 7-Zip archive time
      set what ztime
      lappend typeNames ztime Zip
    }
    "--z2m" { # Copy Zip content time to Modification time
      set action copy
      set from ztime
      lappend typeNames ztime Zip
    }
    default {
      if {("$date" == "") && [regexp {^\d+[-/]\d+([-/]\d+)?$} $arg -]} {
        set action set
        set date $arg
        continue
      }
      if {("$time" == "") && [regexp {^\d+:\d+(:\d+)?$} $arg -]} {
        set action set
        set time $arg
        continue
      }
      # It's a file name. Resolve wildcards, if any.
      incr nNames ;# Count names provided. Wildcards may produce 0 names, which is valid.
      regsub -all {\\} $arg "/" name
      if [regexp {[?*]} $name -] {
	set names [concat $names [lsort -dictionary [glob -nocomplain $name]]]
      } else {
      	set names [lappend $names $name]
      }
      continue
    }
  }
}

if {"$names" == ""} {
  if (!$nNames) { # If no name was actually provided, then display help
    puts -nonewline $usage
  }
  exit 0
}

# Convert dates to the ISO format, with - instead of /
regsub -all / $date - date

if [Debug] {
  puts "set date \"$date\""
  puts "set time \"$time\""
  foreach name1 $names {
    puts "set name \"$name1\""
  }
}

# Generate the new time stamp
if {"$date $time" != " "} {
  set time [clock scan "$date $time"]
}

if {[llength $names] > 1} {
  set showNames 1
} else {
  set showNames 0
}

# Process every file
set exitCode 0
set err [catch {
  foreach name $names {
    if {"$action" == "files2dir"} {
      if ![file isdirectory $name] {
	puts stderr "Error: '$name' is not a directory."
	continue
      }
      FilesTime2Dir $name $what
      continue
    }

    if {"$action" != "set"} {
      foreach {var verb} $typeNames {
	set $var [GetFile${verb}Time $name]
      }
    }

    if {"$action" == "display"} { # Display the old date
      if [Verbose] { # Display everything
	if $showNames {
	  puts $name
	}
	foreach {var verb} $typeNames {
	  set time [set $var]
	  if {"$time" != ""} {
	    set time [clock format $time -format "%Y-%m-%d %H:%M:%S"]
	    puts [format "%12s %s" "$verb time:" $time]
	  }
	}
      } else { # Just display the one requested. (Usually the mtime)
	set time [set $what]
	if {"$time" != ""} {
	  set time [clock format $time -format "%Y-%m-%d %H:%M:%S"]
	  puts -nonewline $time
	  if $showNames {
	    puts -nonewline "  $name"
	  }
	  puts ""
	}
      }
      continue
    }

    if {"$action" == "copy"} { # Copy the image time to the modification time
      set time [set $from]
      if {"$time" == ""} continue ; # Nothing to copy
    }

    if {"$action" == "rename"} { # Prefix the file name with a file or image time
      set time [set $from]
      if {"$time" == ""} continue ; # Nothing to set
      set time [clock format $time -format "%Y-%m-%d_%Hh%Mm%Ss_"]
      set dir [file dirname $name]
      set name2 "$time[file tail $name]"
      if {[Debug] || $noexec} {
	puts "file rename $name $name2"
      } else { # Display the list of files processed, to allow monitoring progress
	puts $name2
      }
      if {!$noexec} {
	file rename $name $name2
      }
      continue
    }

    # Compute the modified time if needed
    if {$add != 0} {
      set time [set $what]
      incr time $add
    }

    # Set the new date/time
    set err [catch {
      if {[Debug] || $noexec} {
	puts "file $what \"$name\" $time"
      } else { # Display the list of files processed, to allow monitoring progress
	puts $name
      }
      if {!$noexec} {
	file $what $name $time
      }
    } errMsg]
    if $err {
      puts stderr "Error: Cannot set file $name $what. $errMsg"
      set exitCode 1
      continue
    }

    # Compute the modified time if needed
    if {$sequence != 0} {
      incr time $sequence
    }
  }
} msg]
if $err {
  puts "Error: $msg"
  set exitCode 1
}
exit $exitCode
