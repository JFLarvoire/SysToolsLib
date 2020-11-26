#!/usr/bin/env bash
###############################################################################
#                                                                             #
#   Filename        Library.bash                                              #
#                                                                             #
#   Description     A library of useful shell routines for Linux/Unix         #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2009-12-16 JFL Created this script.                                      #
#    2013-12-02 JFL Fixed the help screen. Added common command-line options. #
#                   Added Tcl-like list management routines.                  #
#    2013-12-09 JFL Added the -c option, and the fact function for testing.   #
#                   Renamed a few functions for consistency with other langs. #
#    2013-12-16 JFL One minor tweak in test routine fact().		      #
#    2020-03-23 JFL Updated the help screen.               		      #
#    2020-04-01 JFL Fixed cd & export in Exec().           		      #
#    2020-11-24 JFL Use a shebang with the env command.                       #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Global variables
VERSION="2020-11-24"
ARGV=("$0" "$@")		# All arguments, as an array of strings
ARGV0="$0"                      # Full script pathname
SCRIPT="${ARGV0##*/}"           # Extract the script base name...
SCRIPTDIR="$(cd $(dirname "$0") ; /bin/pwd)" # ... and its absolute path
SCRIPTBASE="${SCRIPT%%.*}"	# Script base name without extension

###############################################################################
#                                                                             #
#                              Debugging library                              #
#                                                                             #
###############################################################################

VERBOSITY=1			# Verbosity. 0=Quiet 1=Normal 2=Verbose 3=Debug
EXEC=1				# 1=Execute commands; 0=Don't

# LOGDIR=/var/log		# Preferred log file location with root rights
# LOGDIR=~/log			# Alternate location for non-root users
# LOGFILE=$LOGDIR/$SCRIPTBASE.log	# Where to log what was done and results
LOGFILE=/dev/null		# Default: Do not write to an actual log file
LOGDIR=$(dirname "$LOGFILE")

# Helper routines to test verbosity levels
Quiet() {
  [[ $VERBOSITY == 0 ]]
}
Normal() {
  [[ $VERBOSITY > 0 ]]
}
Verbose() {
  [[ $VERBOSITY > 1 ]]
}
Debug() {
  [[ $VERBOSITY > 2 ]]
}
NoExec() {
  [[ $EXEC == 0 ]]
}

# Log a message into the log file
Log() { # $last=string to log
  if [[ ! -d $LOGDIR ]] ; then
    mkdir -p $LOGDIR
  fi
  echo "$@" >> $LOGFILE
}

# Display a message and log it in the log file
Echo() { # $last=string to display and log
  Log "$@"
  echo "$@"
}

# Display a string in verbose mode
EchoV() { # $*=echo arguments
  Log "$@"
  if Verbose ; then
    echo "$@"
  fi
}

CALL_DEPTH=0
CallIndent() {
  printf '%*s' $CALL_DEPTH ''
}

# Display a string in debug mode
EchoD() { # $*=echo arguments
  Log "$@"
  if Debug ; then
    >&2 echo "$(CallIndent)$*"
  fi
}

# Display a string in NoExec mode
EchoX() { # $*=echo arguments
  if NoExec ; then
    Echo "$(CallIndent)$*"
  fi
}

# Debug routines. Display the values of a series of global variables.
VarsValue() { # $*=Variables names. Display always.
  local var
  for var in "$@" ; do
    declare | grep "^$var="
  done
}
EchoVars() { # $*=Variables names. Display and log always.
  Echo "$(VarsValue "$@")"
}
EchoSVars() { # $1=String; $2...=Variables names. Display and log always.
  local text="$1"
  shift
  Echo "$text $(VarsValue "$@")"
}
EchoVVars() { # $*=Variables names. Display in debug mode only and log always.
  EchoV "$(VarsValue "$@")"
}
EchoDVars() { # $*=Variables names. Display in debug mode only and log always.
  EchoD "$(VarsValue "$@")"
}
EchoDSVars() { # $1=String; $2...=Variables names. Display in debug mode only...
  local text="$1"
  shift
  EchoD "$text $(VarsValue "$@")"
}

# Echo arguments with quotes, so that they can be reentered in bash verbatim.
# User friendly output using the minimal number of quotes needed.
# Makes sure control characters ('\x00' to '\x1F') are visible.
# Note: I've not found how to display the NUL character. (Can it actually occur?)
QuoteArg() { # $1=Argument to quote
  local char=($'\x00' $'\x01' $'\x02' $'\x03' $'\x04' $'\x05' $'\x06' $'\x07' \
              $'\x08' $'\x09' $'\x0A' $'\x0B' $'\x0C' $'\x0D' $'\x0E' $'\x0F' \
              $'\x10' $'\x11' $'\x12' $'\x13' $'\x14' $'\x15' $'\x16' $'\x17' \
              $'\x18' $'\x19' $'\x1A' $'\x1B' $'\x1C' $'\x1D' $'\x1E' $'\x1F' )
  local text=( '\x00'  '\x01'  '\x02'  '\x03'  '\x04'  '\x05'  '\x06'  '\x07' \
               '\b'    '\t'    '\n'    '\x0B'  '\x0C'  '\d'    '\x0E'  '\x0F' \
               '\x10'  '\x11'  '\x12'  '\x13'  '\x14'  '\x15'  '\x16'  '\x17' \
               '\x18'  '\x19'  '\x1A'  '\e'    '\x1C'  '\x1D'  '\x1E'  '\x1F' )
  local s="$1"
  local rx=$'[ \x27"`\$&|#;[:cntrl:]]'
  if [[ $s =~ $rx ]] ; then # This needs some form of quoting
    local rx2=$'[[:cntrl:]]'
    if [[ $s =~ $rx2 ]] ; then # If there are hidden characters
      # echo "there are hidden characters"
      s=${s//\\/\\\\} # Escape anti-slashes
      local i
      for (( i=1 ; $i<32 ; i=$i+1 )) ; do
	local c="${char[$i]}"
	local t="${text[$i]}"
	s=${s//$c/$t} # Replace hidden characters by their C-style encoding
      done
      s="\$'${s//\'/\\x27}'" # Quote with dollar quotes
    elif [[ ! $s =~ "'" ]] ; then # If there are no single quotes
      # echo "there are no single quotes"
      s="'$s'" # Quote with single quotes (simple and fast)
    else # Quote everything that may be interpreted by the shell
      # echo "else case"
      s=${s//\\/\\\\} # Escape anti-slashes
      s=${s//\$/\\\$} # Escape dollars
      s=${s//\"/\\\"} # Escape double quotes
      s=${s//\`/\\\`} # Escape back quotes
      s="\"${s}\"" # Quote with double quotes
    fi
  fi
  if [[ "$s" == "" ]] ; then # Special case for the empty string
    s="''"
  fi
  printf "%s\n" "$s" # Do not use echo, with breaks on -n, -e, etc, arguments
}

if false ; then # Begin section with alternative QuoteArg() versions

# Echo arguments with quotes, so that they can be reentered in bash verbatim.
# User friendly output using the minimal number of quotes needed.
# Does NOT handle control characters in args. ('\x00' to '\x1F')
QuoteArg() { # $1=Argument to quote
  local s="$1"
  if [[ $s =~ $'[ \x27"`\$&|#;]' ]] ; then # This needs some form of quoting
    if [[ ! $s =~ "'" ]] ; then # If there are no single quotes
      s="'$s'" # Quote with single quotes (simple and fast)
    else # Quote everything that may be interpreted by the shell
      s=${s//\\/\\\\} # Escape anti-slashes
      s=${s//\$/\\\$} # Escape dollars
      s=${s//\"/\\\"} # Escape double quotes
      s=${s//\`/\\\`} # Escape back quotes
      s="\"${s}\"" # Quote with double quotes
    fi
  fi
  if [[ "$s" == "" ]] ; then # Special case for the empty string
    s="''"
  fi
  printf "%s\n" "$s" # Do not use echo, with breaks on -n, -e, etc, arguments
}

# Echo arguments with quotes, so that they can be reentered in bash verbatim.
# The quoting is done by Bash's "local" function itself.
# Simple and guarantied correct, but the output is not always user-friendy.
QuoteArg() { # $1=Argument to quote
  local arg="$1"
  arg=$(local)       # Name=Value for the only local variable arg
  arg=${arg/arg=/}   # Remove the Name= field
  if [[ "$arg" == "" ]] ; then
    arg="''"         # Make sure empty strings are visible
  fi
  printf "%s\n" "$s" # Do not use echo, with breaks on -n, -e, etc, arguments
}

fi # End of alternative QuoteArg() versions

# Echo multiple arguments with quotes, so that they can be reviewed easily
QuoteArgs() { # $*=Arguments to quote
  local sep=""
  for arg in "$@" ; do
    printf "%s" "$sep$(QuoteArg "$arg")" # Do not use echo, with breaks on -n, -e, etc, arguments
    sep=" "
  done
  echo ""
}

# Call a subroutine, logging the arguments and return value
# Usage: Call [OPTIONS] [-U] FUNCTION ARGUMENTS
# Options:
#   -U             Do not log the underscore ahead of the function name (Must be last)
#   -v VARNAME     Log variable VARNAME name and value when returning
Call() {
  # Get the list of variables to log upon return
  local lVars="" # List of variables to log
  while [[ "$1" == "-v" ]] ; do
    shift
    lVars="$lVars $1"
    shift
  done
  # Get the function name, and remove the underscore if needed.
  local func="$1"
  if [[ "$1" == "-U" ]] ; then
    shift
    func="${1#_}"
  fi
  local _func="$1"
  shift
  # Log the function name and arguments
  EchoD "$(QuoteArgs $func "$@")"
  CALL_DEPTH=$(( $CALL_DEPTH + 2 ))
  $_func "$@"
  local ret=$?
  # Log global variables that are set by the function
  local var
  for var in $lVars ; do
    EchoD "$(VarsValue $var)"
  done
  # Cleanup and return
  CALL_DEPTH=$(( $CALL_DEPTH - 2 ))
  EchoD "  return $ret"
  return $ret
}

# Redefine a function to log call arguments and return values
TraceProc() { # $1=Function name; $2...=Additional options to pass to Call
  local name=$1
  local _name=_$1
  shift
  # Duplicate function name as _name
  eval "${_name}() $(declare -f $name | tail -n +2)"
  # Redefine function name to call _name and log debug information
  eval "${name}() {
    Call $* -U ${_name} \"\$@\"
  }"
}

# Execute a command and log results.
Exec() { # $1=command $2=argument1 [...] [>|>>output_file]
  # Check if the last argument is a redirection
  local cmd=("$@")
  local last="${cmd[$#-1]}"
  if [[ "$last" == ">>"* ]] ; then # Redirected command. Extract the redirection.
    local redirect=">>"
    local outfile="${last:2}" # Drop the leading >> characters.
    unset cmd[$#-1]
    local cmdline="$(QuoteArgs "${cmd[@]}") >>$(QuoteArgs "$outfile")"
  elif [[ "$last" == ">"* ]] ; then # Redirected command. Extract the redirection.
    local redirect=">"
    local outfile="${last:1}" # Drop the leading > character.
    unset cmd[$#-1]
    local cmdline="$(QuoteArgs "${cmd[@]}") >$(QuoteArgs "$outfile")"
  else # It's not redirected. Use all arguments.
    local redirect=""
    local cmdline="$(QuoteArgs "$@")"
  fi
  if Verbose || [[ $EXEC == 0 ]] ; then
    echo "$cmdline"
  fi
  Log ""
  EchoD "$cmdline"
  local ERR=0
  if [[ $EXEC != 0 ]] ; then
    case "$1" in
      cd|export)
	# Do not pipe output, as this creates a sub-shell, and thus the command
	# has no effect on the current shell.
	"$@"
	ERR=$?
      ;;
      *)
      	if [[ "$redirect" == ">>" ]] ; then
	  "${cmd[@]}" 2>&1 | tee -a $LOGFILE >>"$outfile"
      	elif [[ "$redirect" == ">" ]] ; then
	  "${cmd[@]}" 2>&1 | tee -a $LOGFILE >"$outfile"
	else
	  "${cmd[@]}" 2>&1 | tee -a $LOGFILE
	fi
	ERR=${PIPESTATUS[0]} # Get the exit code from the _first_ command.
      ;;
    esac
  fi
  EchoD "  return $ERR"
  return $ERR
}

#-----------------------------------------------------------------------------#
#                                                                             #
#                         End of Debugging functions                          #
#                                                                             #
#-----------------------------------------------------------------------------#

# Check if a variable exists.
VarExists() {
  eval "[[ \"\${${1}+true}\" == \"true\" ]]"
}

# Get the value of a variable.
VarValue() {
  eval "echo \"\${${1}}\""
}

# Test if a value is an integer
IsInteger() { # $1 = Value to test. Returns: 0=Fully numeric; 1=Nope
  echo "$1" | grep -P '^-?[0-9]+$' >/dev/null 2>&1
}

Now() { # $1 = Date-time separator. Default: " ". (Strict ISO 8601 is "T")
	# $2 = hour-minute-second separator. Default: "h" & "m". Use ":" for ISO 8601
  date +"%04Y-%02m-%02d${1:- }%02H${2:-h}%02M${2:-m}%02S"
}

# Find one file or directory. In case of duplicates, select the newest.
FindOne() { # $@=find arguments
  local files=$(find "$@" 2>/dev/null)
  if [[ -z "$files" ]] ; then
    return 1
  fi
  # Create an array with one file in each array element
  local ifs0="$IFS"
  IFS=$'\n'   # Split fields at new-line characters
  local list  # Do not assign list value in declaration, this fails in bash 3.0.
  list=($files)
  IFS="$ifs0" # Restore field splitting at any space character
  # Sort files by time, and select the first one.
  local f=$(ls -adt1 "${list[@]}" | head -1)
  echo $f
  EchoD "$f"
  return 0
}
TraceProc FindOne

# Find one file
FindFile() { # $1=directory $2=wildcards
  FindOne "$1" -mindepth 1 -maxdepth 1 -type f -name "$2"
}
TraceProc FindFile

# Find one directory
FindDir() { # $1=directory $2=wildcards
  FindOne "$1" -mindepth 1 -maxdepth 1 -type d -name "$2"
}
TraceProc FindDir

# Find a file in a series of possible directories
FindFileInDirs() { # $1=wildcards $2=directory1 ...
  local PATTERN="$1"
  shift
  while (( $# > 0 )) ; do
    FindFile "$1" "$PATTERN" && return # Return if found
    shift
  done
  return 1
}
TraceProc FindFileInDirs

# Get the last argument of an array
LastArg() {
  echo "${@:$#:1}"
}
# Remove the last argument of an array
DropLast() {
  echo "$(QuoteArgs "${@:1:$#-1}")"
}

# Execute a command, sending output to a file, and log results.
Exec2() { # $1=output file $2=command $3=argument1 Etc...
  local file="$1"
  shift
  if Verbose || [[ $EXEC == 0 ]] ; then
    echo "$(QuoteArgs "$@") >$(QuoteArgs "$file")"
  fi
  EchoD ""
  EchoD "$(QuoteArgs "$@") >$(QuoteArgs "$file")"
  Exec -q "$@" >"$file"
}

# Convert a YYYYMMDDHHMMSS timestamp to seconds since the epoch (1970-01-01)
# Support ISO 8601-style timestamps, with any or no separator between fields.
TimeStamp2UnixTime() { # $1=ISO 8601-style timestamp.
  local T="${1}000000" # Append zeros to make the HHMMSS fields optional
  local year=${T:0:4}  ; T=${T#????} ; T=${T#[" "-/,:-z]} # Skip optional non-digit separator
  local month=${T:0:2} ; T=${T#??}   ; T=${T#[" "-/,:-z]} # ""
  local day=${T:0:2}   ; T=${T#??}   ; T=${T#[" "-/,:-z]} # ""
  local hour=${T:0:2}  ; T=${T#??}   ; T=${T#[" "-/,:-z]} # ""
  local min=${T:0:2}   ; T=${T#??}   ; T=${T#[" "-/,:-z]} # ""
  local sec=${T:0:2}   # T=${T#??}   ; T=${T#[" "-/,:-z]} # ""
  #          Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
  local td=(   0   31   59   90  120  151  181  212  243  273  304  334
             365  396  424  455  485  516  546  577  608  638  669  699
             730  761  790  821  851  882  912  943  974 1004 1035 1065
            1096 1127 1155 1186 1216 1247 1277 1308 1339 1369 1400 1430)
  local y=$(($year-1970))
  local m=$((12*($y%4)+${month#0}-1))
  local d=$((1461*($y/4)+${td[$m]}+${day#0}-1))
  echo $(((($d*24+${hour#0})*60+${min#0})*60+${sec#0}))
}
export -f TimeStamp2UnixTime
# Convert a Unix Time (seconds since the epoch) to an ISO 8601-style timestamp
UnixTime2TimeStamp() { # $1=Unix Time; $2=Output format, optional.
  local T=$1
  local format=${2-"%04d-%02d-%02d %02d:%02d:%02d\\n"}
  local sec=$(($T%60)) ; T=$(($T/60))
  local min=$(($T%60)) ; T=$(($T/60))
  local hour=$(($T%24)) ; T=$(($T/24))
  local year=$((1970+4*($T/1461))) ; T=$(($T%1461))
  #          Jan  Feb  Mar  Apr  May  Jun  Jul  Aug  Sep  Oct  Nov  Dec
  local td=(   0   31   59   90  120  151  181  212  243  273  304  334
             365  396  424  455  485  516  546  577  608  638  669  699
             730  761  790  821  851  882  912  943  974 1004 1035 1065
            1096 1127 1155 1186 1216 1247 1277 1308 1339 1369 1400 1430)
  local month=47
  while [[ $T -lt ${td[$month]} ]] ; do month=$(($month-1)) ; done
  local day=$((1+$T-${td[$month]}))
  year=$(($year+$month/12))
  month=$((1+($month%12)))
  printf "$format" $year $month $day $hour $min $sec
}
# Show a file name if timestamp (Encoded in the name) in the given Unix Time range.
# Useful for use with find ... -exec bash -c "ShowIfTsInRange '{}' $UT0 $UT1"
ShowIfTsInRange() { # $1=pathname $2=UTmin $3=UTmax
  local T=$1
  T=${T%.*}  # Remove the .rpm
  T=${T%.*}  # Remove .archi, which may include an _ (Ex: x86_64)
  T=${T##*_} # Remove everything up to the timestamp
  T=$(TimeStamp2UnixTime $T)
  if [[ $T -ge $2 && $T -lt $3 ]] ; then
    echo $1
  fi
}
export -f ShowIfTsInRange

# Find a backup file name, with a unique sequence number suffix
GetBackupName() { # $1=file name
  # Find the last previous numbered backup file, if any.
  local d=$(dirname "$1")
  local n=${1##*/}
  local last=$(find "$d" -mindepth 1 -maxdepth 1 -name "$n.*" 2>/dev/null | grep '\.[0-9]\+$' | sort -n | tail -1)
  # Convert that name to a decimal integer.
  last=${last##*.} # Remove everything up to he final dot
  last=$(echo $last | sed 's/^0\+//') # Remove leading zeros to avoid base 8 confusion
  if [[ ! "$last" ]] ; then # If no previous number was found (Or if it was 0)
    last=0
  fi
  # Compute the next backup number suffix. Make sure there are at least 3 digits.
  local next=$((last+1))
  if (( next < 10 )) ; then
    next="00$next"
  elif (( next < 100 )) ; then
    next="0$next"
  fi
  # Generate the backup file name.
  echo "$1.$next"
}

# Define distribution-specific colored status functions
if [[ -e /etc/rc.d/init.d/functions ]] ; then # RedHat-style distribution
  SAVEPATHFMF="$PATH" ; # functions sets PATH=/sbin:/usr/sbin:/bin:/usr/bin
  . /etc/rc.d/init.d/functions
  PATH="$SAVEPATHFMF" ; unset SAVEPATHFMF
  # Get column width. Note: 1st tput detects error silently; 2nd tput detects stderr width.
  if COLS=$(tput cols 2>/dev/null) && COLS=$(tput cols) && (( $COLS > ( $RES_COL + 12 ) )) ; then
    RES_COL2=$(($COLS-12))
    MOVE_TO_COL=${MOVE_TO_COL/$RES_COL/$RES_COL2} # Adapt to screen width
    RES_COL=$RES_COL2
  fi
  EchoSuccess() {
    echo_success ; echo ""
  }
  EchoPassed() {
    echo_passed ; echo ""
  }
  EchoWarning() {
    echo_warning ; echo ""
  }
  EchoFailure() {
    echo_failure ; echo ""
  }
elif [[ -e /etc/rc.status ]] ; then # Suse-style distribution
  . /etc/rc.status
  EchoSuccess() {
    echo "$rc_save$rc_done$rc_restore"
  }
  EchoPassed() {
    echo "$rc_save${rc_unknown/unknown/passed}$rc_restore"
  }
  EchoWarning() {
    echo "$rc_save${rc_unknown/unknown/warning}$rc_restore"
  }
  EchoFailure() {
    echo "$rc_save$rc_failed$rc_restore"
  }
else # Unknown distribution
  EchoSuccess() {
    echo -e " ... [\033[1;32mSUCCESS\033[0;39m]"
  }
  EchoPassed() {
    echo -e " ... [\033[1;33mPASSED\033[0;39m]"
  }
  EchoWarning() {
    echo -e " ... [\033[1;33mWARNING\033[0;39m]"
  }
  EchoFailure() {
    echo -e " ... [\033[1;31mFAILED\033[0;39m]"
  }
fi

# Extract the Nth word in a word list.
lindex() { # $1=list $2=index
  local n=0
  for word in $1 ; do
    if [ "$n" = "$2" ] ; then
      echo $word
      return
    fi
    n=$(($n + 1))
  done
  if [ "$2" = "end" ] ; then
    echo $word
    return
  fi
  return
}

# Append strings to a Bash array
# Side effect: Renumbers array indexes to canonic values. (Removes holes)
lappend() { # $1=Bash array name; $2,...=Values to append to the array
  local name="$1"
  local value="$(eval QuoteArgs "\"\${${name}[@]}\"")" # Satisfy the syntax colorizer"
  shift
  local new_items="$(QuoteArgs "$@")"
  eval "$name=($value $new_items)"
}

# Join strings
ljoin() { # [-s SEPARATOR]|[--] $*. Default separator: "".
  local sep=""
  case "$1" in
    -s|--sep)
      sep=$2
      shift;
      shift;
    ;;
    --)
      shift;
    ;;
  esac
  local result=""
  if (( $# > 0 )) ; then
    result="$1"
    shift
  fi
  while (( $# > 0 )) ; do
    result="$result$sep$1"
    shift
  done
  echo -n "$result"
}

#-----------------------------------------------------------------------------#
#                      Functions for testing the library                      #
#-----------------------------------------------------------------------------#

# Recursive function computing factorial N
fact() {
  local n=$1
  local result=1
  if (( $n > 1 )) ; then
    result=$(( $n * $(fact $(( $n - 1 )) ) ))
  fi
  EchoDVars result
  echo $result
}
TraceProc fact

exec_all_cmds() {
  while (( $# > 0 )) ; do
    cmd="$1"
    shift
    eval "$cmd"
  done
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Main                                                      #
#                                                                             #
#   Description     Process command line arguments                            #
#                                                                             #
#   Arguments       $*      Command line arguments                            #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

Help() {
  cat <<EOF

Usage: $(basename ${0}) [OPTIONS] [COMMANDS]

Options:
  -c CMD ...        Evaluate each argument as a separate command
  -d, --debug       Debug mode. Trace functions entry and exit, etc
  -h, --help, -?    Display this help screen and exit.
  -l LOGFILE        Set the log file name. Use -l /dev/null to disable.
  -v, --verbose     Enable verbose output.
  -V, --version     Display the script version and exit
  -X, --noexec      Display the commands to execute, but don't execute them

Commands:
  start             Start the daemon
  stop              Stop the daemon

EOF
}

# Main routine
# Process command line arguments
err=0
while (( $# > 0 )) ; do
  # Pop the first argument off the head of the list
  arg="$1"
  shift
  case "$arg" in
    -c|--commands)
      exec_all_cmds "$@"
      exit
    ;;
    -d|--debug)
      VERBOSITY=$(expr $VERBOSITY + 2)
    ;;
    -h|--help|"-?")
      Help
      exit 0
    ;;
    -l|--logfile)
      LOGFILE=$1
      LOGDIR=$(dirname "$LOGFILE")
      shift
    ;;
    -q|--quiet)
      VERBOSITY=0
    ;;
    -v|--verbose)
      VERBOSITY=$(expr $VERBOSITY + 1)
    ;;
    -V|--version)
      echo $VERSION
      exit 0
    ;;
    -X|--noexec)
      EXEC=0
    ;;
    start)
      start
      err=$?
    ;;
    stop)
      stop
      err=$?
    ;;
    -*)
      echo "Unrecognized option: \"$arg\"" >&2
      echo "Run $SCRIPT -? to get a list of valid arguments" >&2
      err=3 ; # Unimplemented feature
    ;;
    *)
      echo "Unrecognized argument: \"$arg\"" >&2
      echo "Run $SCRIPT -? to get a list of valid arguments" >&2
      err=3 ; # Unimplemented feature
    ;;
  esac
done
exit $err

