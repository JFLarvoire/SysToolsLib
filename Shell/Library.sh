#!/bin/sh
###############################################################################
#                                                                             #
#   Filename        Library.sh                                                #
#                                                                             #
#   Description     A sourceable library of useful Bourne Shell routines      #
#                                                                             #
#   Note            Usable in any POSIX shell, like Bash, KSH, ZSH, etc.      #
#                                                                             #
#                   Do not use the $() syntax, which is POSIX-compatible,     #
#                   but not Bourne Shell-compatible.                          #
#                                                                             #
#   History                                                                   #
#    2020-09-25 JFL Created this script.                                      #
#    2022-01-26 JFL Added routine ReadSecret().                               #
#                   Added logging. Added variable values debug output.        #
#    2023-04-22 JFL Rewrote routine Exec() to receive a list of arguments,    #
#                   as that in Library.bash; Renamed the old one as Exec1().  #
#                   Added routine mkdir_p().                                  #
#    2023-11-16 JFL Added option -C to make it easier to test a single cmd.   #
#                                                                             #
#         © Copyright 2020 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################

# Global variables
VERSION="2023-11-16"

# Check if the script is sourced. (Posix version from https://stackoverflow.com/a/28776166/2215591)
sourced=0
if [ -n "$ZSH_EVAL_CONTEXT" ]; then 
  case $ZSH_EVAL_CONTEXT in *:file) sourced=1;; esac
elif [ -n "$KSH_VERSION" ]; then
  [ "$(cd $(dirname -- $0) && pwd -P)/$(basename -- $0)" != "$(cd $(dirname -- ${.sh.file}) && pwd -P)/$(basename -- ${.sh.file})" ] && sourced=1
elif [ -n "$BASH_VERSION" ]; then
  (return 0 2>/dev/null) && sourced=1 
else # All other shells: examine $0 for known shell binary filenames
  # Detects `sh` and `dash`; add additional shell filenames as needed.
  case ${0##*/} in sh|dash) sourced=1;; esac
fi
# If so, we must return from it, not exit
[ $sourced = 1 ] && exit=return || exit=exit

# Get the script name
if [ $sourced = 1 ] ; then
  if [ -n "$BASH_VERSION" ] ; then
    ARGV0="${BASH_SOURCE[${#BASH_SOURCE[@]} - 1]}"
  else
    ARGV0="sourced" # There's no general way to find sourced scripts names
  fi
else
  ARGV0="$0"                    # Full script pathname
fi
SCRIPT="`basename -- "$ARGV0"`" # Extract the script base name...
SCRIPTDIR="`dirname "$ARGV0"`"  # ... and its relative path
SCRIPTDIR="`cd "$SCRIPTDIR" ; /bin/pwd`" # ... and its absolute path

###############################################################################
#                                                                             #
#                              Debugging Library                              #
#                                                                             #
###############################################################################

VERBOSITY=1			# Verbosity. 0=Quiet 1=Normal 2=Verbose 3=Debug
EXEC=1				# 1=Execute commands; 0=Don't

# Helper routines to test verbosity levels
Quiet() {
  [ $VERBOSITY = 0 ]
}
Normal() {
  [ $VERBOSITY -gt 0 ]
}
Verbose() {
  [ $VERBOSITY -gt 1 ]
}
Debug() {
  [ $VERBOSITY -gt 2 ]
}
NoExec() {
  [ $EXEC = 0 ]
}
DoExec() {
  [ $EXEC != 0 ]
}

# Manage output indentation in debug mode proportionally to the call depth
CallIndent() {
  return 0
}

# Global variables defining the current log file
# LOGDIR=/var/log		# Preferred log file location with root rights
# LOGDIR=~/log			# Alternate location for non-root users
# LOGFILE=$LOGDIR/$SCRIPTBASE.log	# Recommended name for the log file
LOGFILE=/dev/null		# Default: Do not write to an actual log file
LOGDIR=/dev

# Set the log file
SetLogFile() { # $1 = New log file pathname. Use /dev/null to disable logging.
  LOGFILE="$1"
  LOGDIR="`dirname "$LOGFILE"`"
}

# Log a message into the log file
Log() { # $*=strings to log
  if [ ! -d $LOGDIR ] ; then
    mkdir -p $LOGDIR
  fi
  echo "`CallIndent -f`$*" >> $LOGFILE	 # Always indent output to the log file
}

# Display a message and log it in the log file
Echo() { # $*=strings to display and log
  Log "$@"
  echo "`CallIndent`$*"
}

# Display a string in verbose mode
EchoV() { # $*=strings to display and log
  Log "$@"
  if Verbose ; then
    echo "`CallIndent`$*"
  fi
}

# Display a string in debug mode
EchoD() { # $*=strings to display and log
  Log "$@"
  if Debug ; then
    >&2 echo "`CallIndent`$*" # Don't output to stdin, else the output of traced routines can't be used by other routines
  fi
}

# Display a string in NoExec mode
EchoX() { # $*=strings to display and log
  if NoExec ; then
    Echo "$@"
  fi
}

# Display an information
Info() { # $*=strings to display and log
  >&2 EchoV "#" "$@"
}

# Display a warning
Warning() { # $*=strings to display and log
  >&2 Echo "Warning:" "$@"
}

# Display an error message
Error() { # $*=strings to display and log
  >&2 Echo "Error:" "$@"
}

# Check if a variable exists
VarExists() {
  eval "[ \"\${${1}+true}\" = \"true\" ]"
}

# Get the value of a variable
VarValue() {
  eval "echo \"\${${1}}\""
}

# Debug routines. Display the values of a series of global variables.
VarsValue() { # $*=Variables names. Display always.
  for var in "$@" ; do
    if VarExists "$var" ; then
      echo "$var=`VarValue "$var"`"
    else
      echo "unset $var"
    fi
  done
}
EchoVars() { # $*=Variables names. Display and log always.
  Echo "`VarsValue "$@"`"
}
EchoSVars() { # $1=String; $2...=Variables names. Display and log always.
  text="$1"
  shift
  Echo "$text `VarsValue "$@"`"
}
EchoVVars() { # $*=Variables names. Display in debug mode only and log always.
  EchoV "`VarsValue "$@"`"
}
EchoDVars() { # $*=Variables names. Display in debug mode only and log always.
  EchoD "`VarsValue "$@"`"
}
EchoDSVars() { # $1=String; $2...=Variables names. Display in debug mode only...
  local text="$1"
  shift
  EchoD "$text `VarsValue "$@"`"
}

# Conditionally execute a command line
Exec1() {
  if [ $EXEC -eq 1 ] ; then
    eval $1
  else
    echo $1
  fi
}

# Conditionally execute a command line
Exec() {
  if [ $EXEC -eq 1 ] ; then
    EchoD $*
    $*
  else
    Echo $*
  fi
}

###############################################################################
#                                                                             #
#                           General Purpose Library                           #
#                                                                             #
###############################################################################

# Get the current time as an ISO 8601 date/time string
Now() { # $1 = Date-time separator. Default: " ". (Strict ISO 8601 is "T")
	# $2 = hour-minute-second separator. Default: "h" & "m". Use ":" for ISO 8601
  date +"%04Y-%02m-%02d${1:- }%02H${2:-h}%02M${2:-m}%02S"
}

#-----------------------------------------------------------------------------#

# Read a secret string (Ex: A password) without echoing it
ReadSecret() {		# $1=Prompt string $2=Output variable
  trap 'stty echo' EXIT		# Ensure echo is re-enabled before exiting
  stty -echo			# Disable echo
  read "$@"		  	# Read the secret
  echo				# Print a newline following what was read
  stty echo			# Re-enable echo
  trap - EXIT			# Disable the exit trap
}

#-----------------------------------------------------------------------------#

# Replacement for `mkdir -p` in cases it can't be used, for ex. when there's a symlink in the path.
mkdir_p() {
  if [ ! -d "$1" ] ; then
    mkdir_p "`dirname "$1"`"
    if [ ! -L "$1" ] ; then
      Exec mkdir "$1"
    fi
  fi
}

#-----------------------------------------------------------------------------#
#                      Functions for testing the library                      #
#-----------------------------------------------------------------------------#

# Recursive function computing factorial N
fact() {
  EchoD "fact($1)"
  result=1
  if [ $1 -gt 1 ] ; then
    result=$(expr $1 '*' $(fact $(expr $1 - 1)))
  fi
  EchoDVars result
  echo $result
}

exec_all_cmds() {
  while [ $# -gt 0 ] ; do
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
$SCRIPT - A sourceable library of Bourne Shell functions

Usage: $SCRIPT [OPTIONS]

Options:
  -c CMDLINE ...    Evaluate each argument as a separate command line
  -C CMD [ARG ...]  Run the given command and optional arguments
  -d, --debug       Debug mode: Tell the script author what code is being run
  -h, --help, -?    Display this help screen and exit
  -l LOGFILE        Set the log file name. Use -l /dev/null to disable
  -v, --verbose     Verbose mode: Tell the user what is being done
  -V, --version     Display the script version and exit
  -X, --noexec      Display the commands to execute, but don't run them

EOF
}

#-----------------------------------------------------------------------------#

# Main routine
ACTION=true # Do nothing, successfully

# Process command line arguments
# Note: The Bourne Shell does not set $#/$*/$@ for sourced scripts. Arguments are ignored.
#       All modern POSIX shells work fine, even with the !/bin/sh header line.
while [ $# -gt 0 ] ; do
  # Pop the first argument off the head of the list
  arg="$1"
  shift
  case "$arg" in
    -c|--commands)
      exec_all_cmds "$@"
      exit
    ;;
    -C|--command)
      "$@"
      exit
    ;;
    -d|--debug)
      VERBOSITY=`expr $VERBOSITY + 2`
    ;;
    -h|--help|"-?")
      Help
      exit 0
    ;;
    -l|--logfile)
      SetLogFile "$1"
      shift
    ;;
    -L|--nologfile)
      SetLogFile /dev/null
    ;;
    -q|--quiet)
      VERBOSITY=0
    ;;
    -v|--verbose)
      VERBOSITY=`expr $VERBOSITY + 1`
    ;;
    -V|--version)
      echo $VERSION
      $exit 0
    ;;
    -X|--noexec)
      EXEC=0
    ;;
    -*)
      echo "Unrecognized option: \"$arg\"" >&2
      echo "Run \`$SCRIPT -?\` to get a list of valid arguments" >&2
      $exit 3 # Unimplemented feature
    ;;
    *)
      echo "Unrecognized argument: \"$arg\"" >&2
      echo "Run \`$SCRIPT -?\` to get a list of valid arguments" >&2
      $exit 3 # Unimplemented feature
    ;;
  esac
done

$ACTION
$exit
