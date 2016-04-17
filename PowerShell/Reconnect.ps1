###############################################################################
#                                                                             #
#   Filename        Reconnect.ps1                                             #
#                                                                             #
#   Description     Reconnect network drives                                  #
#                                                                             #
#   Notes           Useful when opening a remote session from another system: #
#                   By default, thse sessions start with all network drives   #
#                   disconnected.                                             #
#                                                                             #
#                   Warning: This program requires storing a password for the #
#                   network shares to connect to. This is unsafe.             #
#                   TO DO: Find a way to use encrypted passwords.             #
#                                                                             #
#   History                                                                   #
#    2013-10-09 JFL Created this script.                                      #
#    2014-09-25 JFL Adapted to SEHOL standard credentials.                    #
#    2016-02-09 JFL Rewrote function Reconnect() to get the list of network   #
#                   drives from the registry.                                 #
#                   Added options -?, -X, and all corresponding common opts.  #
#                   Added the ability to specify a specific list of drives.   #
#    2016-02-10 JFL Removed the built-in password. Added a -p option instead. #
#                   Added an optional Reconnect.ini file to set the password. #
#    2016-02-23 JFL Tweaked debug messages.                                   #
#                                                                             #
###############################################################################

# If the -V switch is specified, display the script version and exit.
$scriptVersion = "2016-02-23"

$usage = @"
Reconnect.ps1 version $scriptVersion - Reconnect network drives

Usage: reconnect [options] [drives]

Options:
  -?        Display this help screen and exit
  -d        Debug mode
  -p PWD    Specify the password. Default in $env:windir\Reconnect.ini.
  -q        Quiet mode. Force disabling the debug and verbose modes
  -v        Verbose mode. Display details about the drives enabled
  -X        Noexec mode, aka. WhatIf mode. Display what to do, but don't do it

Drives: One or more drive letters, with an optional trailing : Ex: Z or Z:
        Default: Enable all persistent drives defined in that system
"@

# This script name, with various levels of details
$argv0 = dir $MyInvocation.MyCommand.Definition
$scriptBaseName = $argv0.basename	# Ex: Library
$scriptName = $argv0.name		# Ex: Library.ps1
$scriptFullName = $argv0.fullname	# Ex: C:\Temp\Library.ps1

# Define PowerShell v3's $PSCommandPath and $PSScriptRoot in PowerShell v2
if (!$PSCommandPath) { # The script itself
  $PSCommandPath = $script:MyInvocation.MyCommand.Path
}
if (!$PSScriptRoot) { # The script directory
  $PSScriptRoot = Split-Path $script:MyInvocation.MyCommand.Path -Parent
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Pop-Arg                                                   #
#                                                                             #
#   Description     Pop an element from the head of an array.                 #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-06-17 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Pop-Arg ($name = "args") {
  $var = Get-Variable $name -Scope 1
  $len = $var.Value.Length
  $arg = $var.Value[0]
  if ($len -gt 1) {
    $len -= 1
    $var.Value = @($var.Value[1..$len]) # Make sure this is always a collection
  } else {
    $var.Value = @() # Make sure this is an empty collection. (Else it'd be @($null))
  }
  return $arg
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-IniContent                                            #
#                                                                             #
#   Description     Get the content data in a .ini file                       #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-03-12 OL  Created this routine.                                     #
#    2014-12-11 OL  Corrected typos.                                          #
#    2016-02-09 JFL Corrected typos.                                          #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-IniContent {
  <#
  .Synopsis
      Gets the content of an INI file
  
  .Description
      Gets the content of an INI file and returns it as a hashtable
  
  .Notes
      Author        : Oliver Lipkau <oliver@lipkau.net>
      Blog          : http://oliver.lipkau.net/blog/
      Source        : https://github.com/lipkau/PsIni
		      http://gallery.technet.microsoft.com/scriptcenter/ea40c1ef-c856-434b-b8fb-ebd7a76e8d91
      Version       : 1.0 - 2010/03/12 - Initial release
		      1.1 - 2014/12/11 - Typo (Thx SLDR)
					 Typo (Thx Dave Stiff)
  
      #Requires -Version 2.0
  
  .Inputs
      System.String
  
  .Outputs
      System.Collections.Hashtable
  
  .Parameter FilePath
      Specifies the path to the input file.
  
  .Example
      $FileContent = Get-IniContent "C:\myinifile.ini"
      -----------
      Description
      Saves the content of the c:\myinifile.ini in a hashtable called $FileContent
  
  .Example
      $inifilepath | $FileContent = Get-IniContent
      -----------
      Description
      Gets the content of the ini file passed through the pipe into a hashtable called $FileContent
  
  .Example
      C:\PS>$FileContent = Get-IniContent "c:\settings.ini"
      C:\PS>$FileContent["Section"]["Key"]
      -----------
      Description
      Returns the key "Key" of the section "Section" from the C:\settings.ini file
  
  .Link
      Out-IniFile
  #>

[CmdletBinding()]
  Param(
    [ValidateNotNullOrEmpty()]
    [ValidateScript({(Test-Path $_) -and ((Get-Item $_).Extension -eq ".ini")})]
    [Parameter(ValueFromPipeline=$True,Mandatory=$True)]
    [string]$FilePath
  )

  Begin {
    Write-Debug "$($MyInvocation.MyCommand.Name)() Function started"
  }

  Process {
    Write-Verbose "$($MyInvocation.MyCommand.Name)() Processing file: $FilePath"

    $ini = @{}
    $section = "Global"
    $ini[$section] = @{}
    $CommentCount = 0

    switch -regex -file $FilePath {
      "^\[(.+)\]$" { # Section
	$section = $matches[1]
	$ini[$section] = @{}
	$CommentCount = 0
      }
      "^(;.*)$" { # Comment
	$value = $matches[1]
	$CommentCount = $CommentCount + 1
	$name = "Comment" + $CommentCount
	$ini[$section][$name] = $value
      }
      "(.+?)\s*=\s*(.*)" { # Key
	$name,$value = $matches[1..2]
	$ini[$section][$name] = $value
      }
    }
    Write-Debug "$($MyInvocation.MyCommand.Name)() Finished Processing file: $FilePath"
    return $ini
  }

  End {  
    Write-Debug "$($MyInvocation.MyCommand.Name)() Function ended"
  }  
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Reconnect                                                 #
#                                                                             #
#   Description     Reconnect network drives if needed			      #
#                                                                             #
#   Notes           Usage: Reconnect [options] [drives]                       #
#                                                                             #
#   History                                                                   #
#    2013-10-09 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Reconnect network drives if needed
Function Reconnect {
  $drives = @()
  $password = $null
  while ($args.count) {
    $arg = Pop-Arg
    switch -wildcard ("$arg") {
      "-d*" {$arg = "-d"}	# -Debug
      "-v*" {$arg = "-v"}	# -Verbose
      "-w*" {$arg = "-X"}	# -WhatIf
    }
    switch -regex ("$arg") {
      "-d" {$DebugPreference = 'Continue'; $VerbosePreference = 'Continue'}
      "-p" {$Password = Pop-Arg}
      "-q" {$DebugPreference = 'SilentlyContinue'; $VerbosePreference = 'SilentlyContinue'}
      "-v" {$VerbosePreference = 'Continue'}
      "-X" {$WhatIfPreference = $true}
      "^([A-Za-z]):?$" {$drives += $matches[1]}
      default {Write-Error "Invalid argument: $arg"}
    }
  }
  if (!$drives.count) { # Then get the list of persistent drives in the registry
    $drives = dir HKCU:\Network | % {
      Split-Path $_.Name -Leaf
    }
  }
  $env:windir, $PSScriptRoot | % {
    if ($password -eq $null) { # Then look for a .ini file
      $confFile = "$_\Reconnect.ini"
      Write-Debug "Testing $confFile"
      if (Test-Path $confFile) {
	$conf = Get-IniContent $confFile
	$password = $conf["Global"]["password"]
      }
    }
  }
  Write-Debug "Using password $password"
  $drives | % { # Reconnect each drive in the list
    $driveLetter = $_
    $drive = "${driveLetter}:"
    $driveKey = "HKCU:\Network\$driveLetter"
    Write-Debug "Drive $drive"
    $share = $null
    $user = $null
    try {
      $share = (Get-ItemProperty $driveKey RemotePath -ea stop).RemotePath
      $user = (Get-ItemProperty $driveKey UserName -ea stop).UserName
    } catch {
      Write-Error "Undefined drive: $drive"
      continue
    }
    try { # Check if the drive is already connected in PowerShell
      $ignore = Get-PsDrive $driveLetter -ea Stop
      $status = "OK"
    } catch {
      $status = "KO"
    }
    Write-Debug "drive=$drive ; status=$status ; share=$share ; user=$user"
    if ($status -ne "OK") {
      if (!$WhatIfPreference) {
	Write-Verbose "Reconnecting $drive to $share"
	& net.exe use $drive $share /user:$user $password | Write-Verbose
      } else {
      	Write-Host "Would reconnect $drive to $share"
      }
    } else {
      Write-Verbose "Drive $drive is already connected to $share"
    }
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Main                                                      #
#                                                                             #
#   Description     Execute the specified actions                             #
#                                                                             #
#   Arguments                                                                 #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

$args0 = $args
while ($args.count) {
  $arg = Pop-Arg
  switch -wildcard ("$arg") {
    "-d*" {$arg = "-d"}		# -Debug
    "-verb*" {$arg = "-v"}	# -Verbose
    "-vers*" {$arg = "-V"}	# -Version
    "-w*" {$arg = "-X"}		# -WhatIf
  }
  switch -casesensitive ("$arg") {
    "-?" {echo $usage; exit 0}
    "-d" {$DebugPreference = 'Continue'; $VerbosePreference = 'Continue'}
    "-q" {$DebugPreference = 'SilentlyContinue'; $VerbosePreference = 'SilentlyContinue'}
    "-v" {$VerbosePreference = 'Continue'}
    "-V" {echo $scriptVersion; exit 0}
    "-X" {$WhatIfPreference = $true}
  }
}

# Reconnect network drives if needed
Reconnect @args0

# Load user-independant profiles
if (!$profile) {
  $mypshome = "$home\Documents\WindowsPowerShell"
  "$pshome\profile.ps1",				# All users, and all shells
  "$pshome\Microsoft.PowerShell_profile.ps1",		# All users, but only the Microsoft PowerShell shell
  "$mypshome\profile.ps1",				# The current user, and all shells
  "$mypshome\Microsoft.PowerShell_profile.ps1" | % {	# The current user, but only the Microsoft PowerShell shell
    if (test-path $_) {
      Write-Debug "Loading $_"
      $Debug0 = $DebugPreference; $Verbose0 = $VerbosePreference # Save my initial debug settings
      . $_
      $DebugPreference = $Debug0; $VerbosePreference = $Verbose0 # Restore my initial debug settings
      Write-Debug Done
    } else {
      Write-Debug "Absent: $_"
    }
  }
}

