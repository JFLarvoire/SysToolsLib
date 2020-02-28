###############################################################################
#                                                                             #
#   File name       ShadowCopy.ps1                                            #
#                                                                             #
#   Description     Manage volume shadow copies                               #
#                                                                             #
#   Notes           The legacy application for managing volume shadow copies  #
#		    is vssadmin.exe.                                          #
#                   ShadowCopy.ps1 allows manipulating shadow copies in a     #
#		    more object oriented way. It is compatible with all       #
#		    PowerShell object selection and iteration commands.       #
#                                                                             #
#                   References:						      #
#		    https://en.wikipedia.org/wiki/Shadow_Copy                 #
#                                                                             #
#                   Best Practices for Shadow Copies of Shared Folders        #
#                   https://technet.microsoft.com/library/cc753975.aspx       #
#                                                                             #
#                   Uses JFL's SysToolsLib debugging library for PowerShell.  #
#                   More information in the Docs directory in:                #
#                   https://github.com/JFLarvoire/SysToolsLib                 #
#                                                                             #
#                   Possible improvements to do:                              #
#                   * Allow inputing WMI Win32_ShadowCopy objects.            #
#                   * Output a new type, instead of a generic PSObject.       #
#                   * Change Days,Weeks,...,Years to ScriptProperties,        #
#                     dynamically recomputed?                                 #
#                                                                             #
#   History                                                                   #
#    2016-04-18 JFL Created this script.                                      #
#    2016-04-19 JFL Added the -Prune command for cyclicly deleting copies.    #
#    2016-04-20 JFL Added Confirmations and true WhatIf mode for deletions.   #
#    2016-04-21 JFL Added the -New command to create new shadow copies.       #
#                   Merged all input specification args into one InputObject. #
#		    Added steppable pipelining abilities for deletions.       #
#    2016-04-22 JFL Do not complain when enumerating from an inexistent drive.#
#                   Fixed the -Version command, broken by the step. pipeline. #
#                   Make sure that we're running as administrator.            #
#                   Fixed $NextTrim calculation.                              #
#    2016-05-11 JFL Fixed the number of trimesters calculation.               #
#    2016-05-26 JFL Added 2-day preservation periods for the 2nd & 3rd week.  #
#    2016-06-20 JFL Increase that to a 4th week, to get a more regular scale. #
#    2017-01-04 JFL Removed alias eval, changed back to Invoke-Expression.    #
#    2018-09-16 JFL Added commands -Mount and -Dismount.                      #
#    2018-10-15 JFL Added command -Previous.		                      #
#    2018-10-16 JFL Added arguments -Pathname and -Restore.                   #
#    2020-02-27 JFL Performance improvement: Let -Previous skip shadow copies #
#                   more recent than the last file found.                     #
#    2020-02-28 JFL Added switch -Exhaustive to search all previous sh.copies.#
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################
#Requires -Version 2

<#
  .SYNOPSIS
  Manage volume shadow copies, and previous versions of files therein

  .DESCRIPTION
  Volume Shadow Copies record the previous versions of files at past dates,
  when the shadow copy was made.
  But Windows can keep only a limited number of such copies. (64 for Win2012)
  This script allows to list existing copies, and delete some at will.

  It also provides an automated procedure for managing shadow copies like
  a pool of backup tapes. By rotating the older copies, keeping fewer copies for
  older periods, it's possible to keep many years worth of regular backups
  with a limited set of copies (64 here).

  Warning: Shadow copies are not backups.
  They are more convenient than backups, as previous versions of files are
  immediately available for restoration.
  But they are far less secure than backups, as if the disk or server breaks,
  the shadow copies are lost at the same time.
  Do make shadow copies AND backups.

  This script has an object-oriented command interface: Instead of having
  multiple Verb-Noun scripts, we have a single Noun script with multiple -Verb
  commands. Ex: Use (ShadowCopy -Get) instead of (Get-ShadowCopy).

  .PARAMETER Get
  Command switch: Output a list of ShadowCopy description objects,
  specified by the InputObject list: IDs, DateTimes, or Drives.
  If no InputObject is specified, outputs the list of all shadow copies.
  Default command if no other command switch is specified.
  Alias: -List

  .PARAMETER Remove
  Command switch: Remove shadow copies, specified by the InputParameters:
  ShadowCopies, IDs, DateTimes, or Drives.
  If no InputObject is specified, removes nothing.
  Warning: There is no way to recover deleted shadow copies. It is recommended
  to check what the command would do with the -X (alias -WhatIf) parameter first.
  Alias: -Delete

  .PARAMETER Prune
  Command switch: Remove shadow copies that have fallen outside of the cyclic
  preservation policy. (Inspired by typical tape rotation policies.)
  - Keep all copies less than 7 days old. Ex: 2/day * 5 days = 10 copies.
  - Keep the last copy in each 2-day period in the next 2 weeks. Ex: + 6 copies.
  - Keep the last copy each week in the last 3 months. + 12 copies.
  - Keep the last copy each month in the last 12 months. + 9 copies.
  - Keep the last copy each trimester in the last 4 years. + 12 copies.
  - Keep the last copy each year before that. => 15 more years to reach 64.
  Warning: There is no way to recover deleted shadow copies. It is recommended
  to check what the command would do with the -X (alias -WhatIf) parameter first.
  Alias: -Recycle

  .PARAMETER New
  Command switch: Create a new shadow copy on a given drive.
  Alias: -Add

  .PARAMETER Mount
  Command switch: Mount shadow copies at a given MountPoint.
  This allows accessing a read-only versions of the old files in the shadow copy.
  Multiple shadow copies can be mounted in a single command. In that case, the
  individual mount points will be generated within the specified directory.
  This must be done as an administrator, with the right to create symbolic links.
  Requires the -MountPoint argument.

  .PARAMETER Dismount
  Command switch: Unmount shadow copies from given mount points.
  The mount points can be piped into the command.
  Alias: -Unmount

  .PARAMETER Previous
  Command switch: Manage previous versions of the file set with -Pathname.
  If no shadow copy is passed as InputObject, list all unique previous file
  versions.
  If one or more shadow copies are passed as InputObject, list all unique 
  previous file versions in that set. 
  If the -Restore argument is set, instead of listing previous versions, restore
  the most recent one available.

  .PARAMETER Pathname
  The pathname of a file for which to manage previous versions.

  .PARAMETER InputObject
  A list of objects identifying which shadow copies to manage.
  These identifying objects can be:
  - A ShadowCopy object created by the -Get command of this script.
  - A GUID specifying the unique shadow copy ID.
  - A DateTime specifying the shadow copies date.
    If the time is 00:00:00, target all shadow copies that day.
    Else target only the single shadow copy at that exact date/time.
  - A Drive name, specifying the shadow copy volume.
  - A pathname of a directory, from which to unmount a shadow copy.

  .PARAMETER Drive
  The name of the drive on which to create a shadow copy.

  .PARAMETER Force
  Force deletions without prompting for confirmation. (Dangerous!)

  .PARAMETER MountPoint
  Where to mount a shadow copy.
  First possibility, when mounting a single shadow copy:
  A non existent pathname where to mount it. The parent directory must exist.
  Ex: C:\Temp\MountPoint where C:\Temp exists, but MountPoint does not.
  Second possibility, when mounting multiple shadow copies, or just one:
  The pathname of an existing directory. In that case, the actual mount points
  will be auto-generated _within_ that directory. They will be named based on
  the shadow copy original drive and date/time. Ex: C_2018-09-15_18h54m09
  In all cases, the mount points are SYMLINKDs that point to the shadow copy data. 

  .PARAMETER Restore
  The pathname of a file or directory, where to restore the previous version
  of a file.
  If no shadow copy is passed as InputObject, restore the newest available.
  If one or more shadow copies are passed as InputObject, restore the newest 
  available in that set. 

  .PARAMETER Exhaustive
  For use with -Previous: Search every shadow copy for previous versions.
  By default, improve performance by skipping shadow copies more recent than
  the last file found. Should rarely be needed.

  .PARAMETER D
  Switch enabling the debug mode.
  Display messages helping the script author understand what code is running.
  Alias: -Debug

  .PARAMETER V
  Switch enabling the verbose mode.
  Display messages helping the user understand what the code is doing.
  Alias: -Verbose

  .PARAMETER X
  Switch enabling the NoExec mode, alias the WhatIf mode.
  Display the actions that should done, but do not execute them.
  Alias: -WhatIf

  .PARAMETER Version
  Display this script version and exit.

  .EXAMPLE
  .\ShadowCopy | ft -a

  Displays a table listing all shadow copies on all drives.

  .EXAMPLE
  .\ShadowCopy F: | ft -a

  Displays a table listing all shadow copies of drive F:.

  .EXAMPLE
  # List shadow copies to be removed for a given date, then do it 
  .\ShadowCopy 2016-04-01 -Remove -X
  .\ShadowCopy 2016-04-01 -Remove

  Removes all shadow copies created on April fool's day.

  .EXAMPLE
  .\ShadowCopy F: | where {$_.Months -gt 6} | .\ShadowCopy -Remove

  Removes all shadow copies older than 6 months on drive F:.

  .EXAMPLE
  # List old shadow copies that can be recycled, then remove them 
  .\ShadowCopy -Prune -X
  .\ShadowCopy -Prune

  Removes all shadow copies falling out of our cyclic preservation policy.

  .EXAMPLE
  # Mount a set of shadow copies; Access them; then dismount them
  md C:\Mnt
  # Create mount points for all shadow copies for drive C:
  .\ShadowCopy C: | .\ShadowCopy -Mount -MountPoint C:\Mnt
  # The same thing can be done with this shorter command:
  .\ShadowCopy -Mount C: C:\Mnt
  # Look at the shadow copies contents
  dir C:\Mnt
  ...
  # Unmount all shadow copies from that place
  dir C:\Mnt | .\ShadowCopy -Unmount
  # It's now empty again and can be deleted
  rd C:\Mnt

  .EXAMPLE
  # List previous versions of a file, then restore on particular one
  # List previous versions of a file
  .\ShadowCopy -Previous MyPlans.doc
  # Select one particular Shadow Copy (Ex: Index 2345),
  # and restore that file version under a different name
  .\ShadowCopy -Previous MyPlans.doc 2345 -Restore MyOldPlans.doc
#>

[CmdletBinding(DefaultParameterSetName='Get', SupportsShouldProcess=$true, ConfirmImpact="High")]
Param (
  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch][alias("List")]$Get = $($PSCmdlet.ParameterSetName -eq 'Get'), # If true, output a list of ShadowCopy objects

  [Parameter(ParameterSetName='Remove', Mandatory=$true)]
  [Switch][alias("Delete")]$Remove,	# If true, Remove a shadow copy

  [Parameter(ParameterSetName='Prune', Mandatory=$true)]
  [Switch][alias("Recycle")]$Prune,	# If true, Remove all shadow copies not in periodic backup policy

  [Parameter(ParameterSetName='New', Mandatory=$true)]
  [Switch][alias("Add")]$New,		# If true, Create a new shadow copy of a volume

  [Parameter(ParameterSetName='Mount', Mandatory=$true)]
  [Switch]$Mount,			# If true, Mount a shadow copy

  [Parameter(ParameterSetName='Dismount', Mandatory=$true)]
  [Switch][alias("Unmount")]$Dismount,	# If true, Unmount a shadow copy

  [Parameter(ParameterSetName='Previous', Mandatory=$true)]
  [Switch]$Previous,			# If true, list previous versions of a file

  [Parameter(ParameterSetName='Previous', Mandatory=$true, Position=0)]
  [String]$Pathname,			# Which file we want to seek previous versions of

  [Parameter(ParameterSetName='Remove', Mandatory=$true, ValueFromPipeline=$true, Position=0)]
  [Parameter(ParameterSetName='Mount', Mandatory=$true, ValueFromPipeline=$true, Position=0)]
  [Parameter(ParameterSetName='Dismount', Mandatory=$true, ValueFromPipeline=$true, Position=0)]
  [Parameter(ParameterSetName='Get', Mandatory=$false, ValueFromPipeline=$true, Position=0)]
  [Parameter(ParameterSetName='Previous', Mandatory=$false, ValueFromPipeline=$true, Position=1)]
  [AllowEmptyCollection()]
  [Object[]]$InputObject = @(),		# Objects to work on, or criteria for selecting them

  [Parameter(ParameterSetName='Mount', Mandatory=$true, Position=1)]
  [String]$MountPoint,			# Where the shadow copy is to be mounted

  [Parameter(ParameterSetName='Prune', Mandatory=$true, Position=0)]
  [Parameter(ParameterSetName='New', Mandatory=$true, Position=0)]
  [String]$Drive,			# If true, Create a new shadow copy of a volume

  [Parameter(ParameterSetName='Remove', Mandatory=$false)]
  [Parameter(ParameterSetName='Prune', Mandatory=$false)]
  [Switch]$Force,			# If true, do not display the confirmation prompts

  [Parameter(ParameterSetName='Previous', Mandatory=$false)]
  [Switch]$Exhaustive,			# If true, search every shadow copy for previous versions

  [Parameter(ParameterSetName='Previous', Mandatory=$false, Position=2)]
  [String]$Restore,			# Where to restore the previous version of a file

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Parameter(ParameterSetName='Remove', Mandatory=$false)]
  [Parameter(ParameterSetName='Prune', Mandatory=$false)]
  [Parameter(ParameterSetName='New', Mandatory=$false)]
  [Parameter(ParameterSetName='Mount', Mandatory=$false)]
  [Parameter(ParameterSetName='Dismount', Mandatory=$false)]
  [Parameter(ParameterSetName='Previous', Mandatory=$false)]
  [Switch]$D,				# If true, display debug information

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Parameter(ParameterSetName='Remove', Mandatory=$false)]
  [Parameter(ParameterSetName='Prune', Mandatory=$false)]
  [Parameter(ParameterSetName='New', Mandatory=$false)]
  [Parameter(ParameterSetName='Mount', Mandatory=$false)]
  [Parameter(ParameterSetName='Dismount', Mandatory=$false)]
  [Parameter(ParameterSetName='Previous', Mandatory=$false)]
  [Switch]$V,				# If true, display verbose information

  [Parameter(ParameterSetName='Remove', Mandatory=$false)]
  [Parameter(ParameterSetName='Prune', Mandatory=$false)]
  [Parameter(ParameterSetName='New', Mandatory=$false)]
  [Parameter(ParameterSetName='Mount', Mandatory=$false)]
  [Parameter(ParameterSetName='Dismount', Mandatory=$false)]
  [Parameter(ParameterSetName='Previous', Mandatory=$false)]
  [Switch]$X,				# If true, display commands, but don't execute them

  [Parameter(ParameterSetName='Version', Mandatory=$true)]
  [Switch]$Version			# If true, display the script version
)

Begin {

# If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2020-02-28"
if ($Version) {
  echo $scriptVersion
  exit 0
}

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

###############################################################################
#                                                                             #
#                              Debugging library                              #
#                                                                             #
###############################################################################

# Variables optionally used to tag logged lines
$ParameterSet = $PSCmdlet.ParameterSetName
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$userName = $identity.Name	# Ex: "NT AUTHORITY\SYSTEM" or "Domain\Administrator"

# Redefine the colors for a few message types
$colors = (Get-Host).PrivateData
if ($colors) { # Exists for ConsoleHost, but not for ServerRemoteHost
  $colors.VerboseForegroundColor = "white" # Less intrusive than the default yellow
  $colors.DebugForegroundColor = "cyan"	 # Distinguish debug messages from yellow warning messages
}

if ($D -or ($DebugPreference -ne "SilentlyContinue")) {
  $Debug = $true
  $DebugPreference = "Continue"	    # Make sure Write-Debug works
  $V = $true
  Write-Debug "# Running in debug mode."
} else {
  $Debug = $false
  $DebugPreference = "SilentlyContinue"	    # Write-Debug does nothing
}

if ($V -or ($VerbosePreference -ne "SilentlyContinue")) {
  $Verbose = $true
  $VerbosePreference = "Continue"   # Make sure Write-Verbose works
  Write-Debug "# Running in verbose mode."
} else {
  $Verbose = $false
  $VerbosePreference = "SilentlyContinue"   # Write-Verbose does nothing
}

if ($X -or $WhatIfPreference) {
  $NoExec = $WhatIfPreference = $true
} else {
  $NoExec = $WhatIfPreference = $false
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
#   Function        Add-ErrorMessage                                          #
#                                                                             #
#   Description     Insert a string ahead of an error message.                #
#                                                                             #
#   Notes           Useful to add context details to an exception intercepted,#
#		    before forwarding it up.                                  #
#                                                                             #
#   History                                                                   #
#    2015-07-20 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Add-ErrorMessage (
  [String]$Message,		# The string to insert ahead of the error message
  [String]$errorName = "_"	# The error variable name. Default="_" as $_ is set in every catch {block})
) {
  $err = Get-Variable $errorName -Scope 1
  try {
    $err = $err.Value
    if (!$err.ErrorDetails) {
      $err.ErrorDetails = $err.Exception.Message
    }
    $err.ErrorDetails = "$Message $($err.ErrorDetails)"
  } catch {
    Write-Error "Cannot add error message to variable `$$errorName."
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Now                                                       #
#                                                                             #
#   Description     Get a string with the current time.                       #
#                                                                             #
#   Notes           The output string is in the ISO 8601 format, except for   #
#                   a space instead of a T between the date and time, to      #
#                   improve the readability.                                  #
#                                                                             #
#   History                                                                   #
#    2015-06-11 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Now {
  Param (
    [Switch]$ms,	# Append milliseconds
    [Switch]$ns		# Append nanoseconds
  )
  $Date = Get-Date
  $now = ""
  $now += "{0:0000}-{1:00}-{2:00} " -f $Date.Year, $Date.Month, $Date.Day
  $now += "{0:00}:{1:00}:{2:00}" -f $Date.Hour, $Date.Minute, $Date.Second
  $nsSuffix = ""
  if ($ns) {
    if ("$($Date.TimeOfDay)" -match "\.\d\d\d\d\d\d") {
      $now += $matches[0]
      $ms = $false
    } else {
      $ms = $true
      $nsSuffix = "000"
    }
  }
  if ($ms) {
    $now += ".{0:000}$nsSuffix" -f $Date.MilliSecond
  }
  return $now
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-LogFile                                             #
#                                                                             #
#   Description     Output lines of text to a debug log file                  #
#                                                                             #
#   Notes           Accept input either from the input pipe or as arguments.  #
#                                                                             #
#   History                                                                   #
#    2013-10-01 JFL Created this routine.                                     #
#    2013-10-10 JFL Added the -PassThru switch.                               #
#                   Write strings immediately, and other objects in the end.  #
#    2015-06-30 JFL Bugfix: Create the log directory if it does not exist.    #
#                                                                             #
#-----------------------------------------------------------------------------#

# $LogFile = $null # Log file name. Defined as command-line argument, or defaults to $null.

Function Write-LogFile {
  [CmdletBinding()]
  param(
    [Parameter(Position=0,ValueFromPipeline=$true)][object]$Object,
    [Switch]$PassThru = $false
  )
  Begin {	# Process the object passed in as an argument, if any
    if ($LogFile) {
      $logDir = Split-Path $LogFile -Parent
      if (!(Test-Path $logDir)) {
	mkdir $logDir
      }
    }
    if (($object -ne $null) -and $LogFile) {
      if ($object -is [String]) { # Log a timestamp and user name
      	$object = "$(Now) $userName $ParameterSet $object"
      }
      $object | Out-File -FilePath $LogFile -Encoding "UTF8" -Force -Append -Width 4096
    }
    $objects = @()
    if ($PassThru -and ($object -ne $null)) {$object}
  }
  Process {	# Process each object passed in via the input stream
    if ($LogFile) {
      if ($_ -is [String]) {
      	$_ | Out-File -FilePath $LogFile -Encoding "UTF8" -Force -Append
      } else {	# Defer formatting to the End() block, to allow aligning columns
	$objects += $_
      }
    }
    if ($PassThru) {$_}
  }
  End {		# Format the list of objects as a table
    if ($objects.length -and $LogFile) {
      # Problem: Out-File would format tables as we want, but it adds a lot of useless spaces to the end of every line.
      # Workaround: Manually format the output string, then trim line-per-line the trailing spaces this formatting adds.
      $objects | Out-String -Width 4096 -Stream | % {
        $_ -replace '\s*$','' | Out-File -FilePath $LogFile -Encoding "UTF8" -Force -Append -Width 4096
      }
    }
  }
}

Set-Alias log Write-LogFile -Confirm:$false -WhatIf:$false

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Out-Default                                               #
#                                                                             #
#   Description     Override the default Out-Default to log all output        #
#                                                                             #
#   Notes           Overrides PowerShell's own Output-Default. See:           #
#		    http://social.technet.microsoft.com/Forums/scriptcenter/en-US/dd865899-26f2-42f6-a403-04528fa48541/scripting-logging-in-powershell-discussion
#                   http://poshcode.org/803				      #
#                                                                             #
#                   WARNING: This works only when defined in a $profile,      #
#                   or for sub-scripts invoked after this is defined.         #
#                                                                             #
#                   Output from both Write-Output and Write-Error makes it    #
#                   through to Out-Default.                                   #
#                                                                             #
#                   Makes sure to call the original Out-Default steppable     #
#                   steps. This ensures that the original Out-Default         #
#                   correctly tabulates lists of objects.                     #
#                                                                             #
#                   I did not manage to find how to do the same steppability  #
#                   trick for my own (yet steppable) Write-LogFile. So I      #
#                   regenerate an object list, and pass this to Write-LogFile #
#                   to emulate the same behaviour.                            #
#                                                                             #
#   History                                                                   #
#    2013-10-02 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

if (0) {
Function Out-Default {
  [CmdletBinding(ConfirmImpact="Medium")]
  param (
    [Parameter(ValueFromPipeline=$true)][PSObject]$InputObject
  )
  begin {
    $Cmdlet = $ExecutionContext.InvokeCommand.GetCmdlet("Out-Default")
    $CodeBlock = { & $Cmdlet @PSBoundParameters }
    $SteppablePipeline = $CodeBlock.GetSteppablePipeline()
    $SteppablePipeline.Begin($Cmdlet)
    $objects = @()
  }
  process {
    $SteppablePipeline.Process($_)
    $objects += $_
  }
  end {
    $SteppablePipeline.End()
    $objects | Write-LogFile
  }
}
}

<#
Function Out-Default {
  "Captured:" | Out-Host
  $input2 = $input | Out-String -width 10000 -Stream
  $input2 | Out-Host
  $input2 | Write-LogFile
}
#>

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-Host                                                #
#                                                                             #
#   Description     Override the default Write-Host to log all output         #
#                                                                             #
#   Notes           Contrary to Write-Output, Write-Host does not attempt     #
#                   to format input lists of objects as tables. So...         #
#                   1) To get the output formatted as tables, it's necessary  #
#                      to pipe the input throught Out-String first.           #
#                   2) This makes it unnecessary to synchronize stepping      #
#                      with the next pipeline stage, like Out-Default has to. #
#                                                                             #
#   History                                                                   #
#    2013-10-04 JFL Created this routine.                                     #
#    2015-06-30 JFL Initialize WriteHost0 once during the script startup.     #
#                   (This was done every time in a Begin {block}.)            #
#                                                                             #
#-----------------------------------------------------------------------------#

$script:WriteHost0 = Get-Command Write-Host -Module Microsoft.PowerShell.Utility

Function Write-Host  {
  [CmdletBinding()]
  param(
    [Parameter(Position=0,ValueFromPipeline=$true)][object]$Object,
    [Parameter()][switch] $NoNewLine,
    [Parameter()][ConsoleColor] $ForegroundColor,
    [Parameter()][ConsoleColor] $BackgroundColor,
    [Parameter()][Object] $Separator
  )
  Process {
    Write-LogFile "$Object"
    & $script:WriteHost0 @PSBoundParameters
  }
}

Set-Alias msg Write-Host -Confirm:$false -WhatIf:$false

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-Verbose                                             #
#                                                                             #
#   Description     Override the default Write-Verbose to log all output      #
#                                                                             #
#   Notes           Contrary to Write-Output, Write-Verbose does not attempt  #
#                   to format input lists of objects as tables. So...         #
#                   1) To get the output formatted as tables, it's necessary  #
#                      to pipe the input throught Out-String first.           #
#                   2) This makes it unnecessary to synchronize stepping      #
#                      with the next pipeline stage, like Out-Default has to. #
#                                                                             #
#   History                                                                   #
#    2013-10-02 JFL Created this routine.                                     #
#    2015-06-30 JFL Initialize WriteVerbose0 once during the script startup.  #
#                   (This was done every time in a Begin {block}.)            #
#                   Change the $Message input type to a generic object.       #
#                                                                             #
#-----------------------------------------------------------------------------#

$script:WriteVerbose0 = Get-Command Write-Verbose -CommandType Cmdlet

Function Write-Verbose {
  [CmdletBinding()]
  param(
    [Parameter(Position=0,ValueFromPipeline=$true)][object]$Message
  )
  Process {
    Write-LogFile "VERBOSE: $Message"
    & $script:WriteVerbose0 @PSBoundParameters
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Out-Verbose                                               #
#                                                                             #
#   Description     Works like Out-String | Write-Verbose                     #
#                                                                             #
#   Notes           To work around Write-Verbose limitations (See notes in    #
#                   Write-Verbose header above), this function behaves like   #
#		    if Out-String had been called before Write-Verbose.       #
#                                                                             #
#                   It also adjusts the output width, so that it fits in the  #
#		    output window with the VERBOSE: prefix on each line.      #
#                                                                             #
#   History                                                                   #
#    2015-06-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Out-Verbose {
  [CmdletBinding()]
  param (
    [Parameter(Position=0,ValueFromPipeline=$true)]$Message
  )
  begin {
    $Width = $Host.UI.RawUI.WindowSize.Width - ($script:debugIndent.Length + 10)
    $Cmdlet = $ExecutionContext.InvokeCommand.GetCmdlet("Out-String")
    $CodeBlock = { & $Cmdlet -stream -width $Width | Write-Verbose }
    $SteppablePipeline = $CodeBlock.GetSteppablePipeline()
    $SteppablePipeline.Begin($Cmdlet)
    $objects = @()
  }
  process {
    $SteppablePipeline.Process($Message)
    $objects += $Message
  }
  end {
    $SteppablePipeline.End()
    $objects | Out-String -stream | % {
      Write-LogFile "VERBOSE: ${script:debugIndent}$_"
    }
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-Debug                                               #
#                                                                             #
#   Description     Override the default Write-Debug to log all output        #
#                                                                             #
#   Notes           Contrary to Write-Output, Write-Debug does not attempt    #
#                   to format input lists of objects as tables. So...         #
#                   1) To get the output formatted as tables, it's necessary  #
#                      to pipe the input throught Out-String first.           #
#                   2) This makes it unnecessary to synchronize stepping      #
#                      with the next pipeline stage, like Out-Default has to. #
#                                                                             #
#   History                                                                   #
#    2013-10-02 JFL Created this routine.                                     #
#    2015-06-30 JFL Initialize WriteDebug0 once during the script startup.    #
#                   (This was done every time in a Begin {block}.)            #
#                   Change the $Message input type to a generic object.       #
#                                                                             #
#-----------------------------------------------------------------------------#

if (!(test-path variable:script:debugIndent)) {
  if (test-path env:INDENT) { # Inherit from batch files, etc.
    $Script:debugIndent = $env:INDENT
  } else {
    $Script:debugIndent = ""
  }
}

$Script:WriteDebug0 = Get-Command Write-Debug -CommandType Cmdlet

Function Write-Debug {
  [CmdletBinding()]
  param(
    [Parameter(Position=0,ValueFromPipeline=$true)][object]$Message
  )
  Process {
    Write-LogFile "DEBUG: ${script:debugIndent}$Message"
    $PSBoundParameters.Message = "${script:debugIndent}$($PSBoundParameters.Message)"
    & $Script:WriteDebug0 @PSBoundParameters
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Out-Debug                                                 #
#                                                                             #
#   Description     Works like Out-String | Write-Debug                       #
#                                                                             #
#   Notes           To work around Write-Debug limitations (See notes in      #
#                   Write-Debug header above), this function behaves like     #
#		    if Out-String had been called before Write-Debug.         #
#                                                                             #
#                   It also adjusts the output width, so that it fits in the  #
#		    output window with the DEBUG: prefix on each line.        #
#                                                                             #
#   History                                                                   #
#    2015-06-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Out-Debug {
  [CmdletBinding()]
  param (
    [Parameter(Position=0,ValueFromPipeline=$true)]$Message
  )
  begin {
    $Width = $Host.UI.RawUI.WindowSize.Width - ($script:debugIndent.Length + 8)
    $Cmdlet = $ExecutionContext.InvokeCommand.GetCmdlet("Out-String")
    $CodeBlock = { & $Cmdlet -stream -width $Width | Write-Debug }
    $SteppablePipeline = $CodeBlock.GetSteppablePipeline()
    $SteppablePipeline.Begin($Cmdlet)
    $objects = @()
  }
  process {
    $SteppablePipeline.Process($Message)
    $objects += $Message
  }
  end {
    $SteppablePipeline.End()
    $objects | Out-String -stream | % {
      Write-LogFile "DEBUG: ${script:debugIndent}$_"
    }
  }
}

# Initial version, without explicit usage of pipelines
<#
Function Out-Debug1 {
  [CmdletBinding()]
  param(
    [Parameter(Position=0,ValueFromPipeline=$true)]$Message
  )
  Begin {
    $Width = $Host.UI.RawUI.WindowSize.Width - ($script:debugIndent.Length + 8)
    $objects = @()
  }
  Process {
    $objects += $Message
  }
  End {
    ($objects | Out-String -stream -width $Width).TrimEnd() | % {
      & $Script:WriteDebug0 "${script:debugIndent}$_"
    }
    $objects | Write-LogFile
  }
}
#>

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-Warning                                             #
#                                                                             #
#   Description     Override the default Write-Warning to log all output      #
#                                                                             #
#   Notes           Contrary to Write-Output, Write-Warning does not attempt  #
#                   to format input lists of objects as tables. So...         #
#                   1) To get the output formatted as tables, it's necessary  #
#                      to pipe the input throught Out-String first.           #
#                   2) This makes it unnecessary to synchronize stepping      #
#                      with the next pipeline stage, like Out-Default has to. #
#                                                                             #
#   History                                                                   #
#    2013-10-04 JFL Created this routine.                                     #
#    2015-06-30 JFL Initialize WriteWarning0 once during the script startup.  #
#                   (This was done every time in a Begin {block}.)            #
#                   Change the $Message input type to a generic object.       #
#                                                                             #
#-----------------------------------------------------------------------------#

$script:WriteWarning0 = Get-Command Write-Warning -CommandType Cmdlet

Function Write-Warning {
  [CmdletBinding()]
  param(
    [Parameter(Position=0,ValueFromPipeline=$true)][object]$Message
  )
  Process {
    Write-LogFile "WARNING: $Message"
    & $script:WriteWarning0 @PSBoundParameters
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-Error                                               #
#                                                                             #
#   Description     Override the default Write-Error to log all output        #
#                                                                             #
#   Notes           It's should not be necessary to override Write-Error, as  #
#                   Write-Error output makes it through to Out-Default too.   #
#                   But as Out-Default redirection does not work internally,  #
#                   we have to override this one too.                         #
#                                                                             #
#   History                                                                   #
#    2015-08-21 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

$script:WriteError0 = Get-Command Write-Error -CommandType Cmdlet

Function Write-Error {
  [CmdletBinding()]
  param(
    [Parameter(ParameterSetName='ErrorRecord', Mandatory=$true)]
    [System.Management.Automation.ErrorRecord]$ErrorRecord,

    [Parameter(ParameterSetName='Exception', Mandatory=$true)]
    [Exception]$Exception,

    [Parameter(ParameterSetName='Message', Mandatory=$true, Position=0, ValueFromPipeline=$true)]
    # TO DO: Find how to define this parameter, which IS supported the real Write-Error -Exception command.
    # [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [string]$Message,

    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [System.Management.Automation.ErrorCategory]$Category,

    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [string]$ErrorId,

    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [object]$TargetObject,

    [Parameter(ParameterSetName='ErrorRecord', Mandatory=$false)]
    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [string]$CategoryActivity,

    [Parameter(ParameterSetName='ErrorRecord', Mandatory=$false)]
    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [string]$CategoryReason,

    [Parameter(ParameterSetName='ErrorRecord', Mandatory=$false)]
    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [string]$CategoryTargetName,

    [Parameter(ParameterSetName='ErrorRecord', Mandatory=$false)]
    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [string]$CategoryTargetType,

    [Parameter(ParameterSetName='ErrorRecord', Mandatory=$false)]
    [Parameter(ParameterSetName='Message', Mandatory=$false)]
    [Parameter(ParameterSetName='Exception', Mandatory=$false)]
    [string]$RecommendedAction
  )
  Process {
    if ($Message -ne "") {
      Write-LogFile "Error: $Message"
    } elseif ($ErrorRecord -ne $null) {
      Write-LogFile "Error: $ErrorRecord"
    } elseif ($Exception -ne $null) {
      Write-LogFile "Error: $Exception"
    }
    & $script:WriteError0 @PSBoundParameters
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Quote                                                     #
#                                                                             #
#   Description     Put quotes around a string, if needed for reinterpretation#
#                                                                             #
#   Notes           Quoting constraints:                                      #
#                   - 'strings' contents are left unchanged.                  #
#                   - "strings" contents special characters are interpreted.  #
#                   - We try to minimize the number of ` escape sequences.    #
#                   - We prefer "strings" first (Looking more natural),       #
#                     then 'strings' (Possibly for a second level of quoting).#
#                                                                             #
#   History                                                                   #
#    2010-06-08 JFL Created this routine.                                     #
#    2010-06-17 JFL Added special handling for $null and arrays.              #
#    2010-06-24 JFL Added special handling for hash tables.                   #
#    2010-07-23 JFL Added special handling for booleans.                      #
#    2013-07-19 JFL Added special handling for blocks of code.                #
#    2013-10-07 JFL Added special handling for Parameter lists.               #
#    2013-11-18 JFL Added special handling for enumerations and types.        #
#                   Display the default fields for objects that have an       #
#                   explicit or default format definition.                    #
#    2015-06-30 JFL Escape all control characters.                            #
#                                                                             #
#-----------------------------------------------------------------------------#

$ImplicitTypes = "System.String", "System.Decimal", "System.Single", "System.Double",
                 "System.Char", "System.Int16", "System.Int32", "System.Int64",
                 "System.Byte", "System.UInt16", "System.UInt32", "System.UInt64"

Function Quote(
  $var, # Don't set $var type at this stage, to catch $null, etc.
  [Switch]$Force # $True=Quote all strings; $false=Only when necessary
) {
  if ($var -eq $null) { # Special case of the $null object
    return '$null'
  }
  if (($var -is [Boolean]) -or ($var -is [System.Management.Automation.SwitchParameter])) { # Special case of booleans
    if ($var) {
      return '$True'
    } else {
      return '$False'
    }
  }
  if ($var -is [Array]) { # This is an array. Return a list of quoted array members.
    return "@($(($var | foreach { Quote $_ -Force }) -join ", "))"
  }
  if ($var -is [Hashtable]) { # This is a hash table. Sort keys which are ordered randomly by hash.
    return "@{$(($var.Keys | sort | foreach { "$_ = $(Quote $var.$_ -Force )" }) -join "; ")}"
  }
  if ($var -is [Enum]) { # This is an enumeration. Force quoting string values to avoid issues with object members (which are often enums).
    return Quote "$var" -Force
  }
  if ($var -is [type]) { # This is a type. Return its name as a cast.
    return "[$($var.Name)]"
  }
  if ($var -is [ScriptBlock]) { # This is a block of code. Return it in curly brackets.
    return "{$var}"
  }
  $type = $var.psTypeNames[0] # Try using this type name, which is sometimes more descriptive than the official one in GetType().FullName
  if ($type -eq $null) { # $type seems to be always defined, but just in case, if it's not, fallback to the documented name.
    $type = $var.GetType().FullName
  }
  if (    $type -eq "System.Management.Automation.PSBoundParametersDictionary" `
      -or $type -like "System.Collections.Generic.Dictionary*") { # This is a dictionary. Keys are ordered already.
    return "@{$(($var.Keys | foreach { "$_ = $(Quote $var.$_ -Force)" }) -join "; ")}"
  }
  if (!($ImplicitTypes -contains $type)) { # If this is not a simple type in the list above
    $values = @()
    if ($var.PSStandardMembers.DefaultDisplayPropertySet.ReferencedPropertyNames) {
      # This object has explicit display properties defined. Use them.
      foreach ($name in $var.PSStandardMembers.DefaultDisplayPropertySet.ReferencedPropertyNames) {
	$value = Quote $var.$name
	$values += "$name = $value"
      }
    } else { # Check if the type has a default *.ps1xml format data definition
      $fd = Get-FormatData $type # If type strings in .psTypeNames[0] and .GetType().FullName differ, it's the first one that gives good results.
      if ($fd -and $fd.FormatViewDefinition[0].control.Entries.Items) {
	# We do have a list of default fields to display. (The ones used by Out-String by default!)
	foreach ($item in $fd.FormatViewDefinition[0].control.Entries.Items) {
	  switch ($item.DisplayEntry.ValueType) {
	    "Property" {
	      $name = $item.DisplayEntry.Value
	      $value = Quote $var.$name
	    }
	    "ScriptBlock" {
	      $name = $item.Label
	      $value = Quote (Invoke-Expression "`$_ = `$var ; $($item.DisplayEntry.Value)")
	    }
	    "default" {
	      Write-Error "Unsupported ValueType: $($item.DisplayEntry.ValueType)"
	    }
	  }
	  $values += "$name = $value"
	}
      }
    }
    switch ($values.length) {
      0 {} # No type list found. Fall through into the [string] cast default.
      1 {return $value} # Trivial object with just one field. No need to specify type and detailed field names since conversion will be trivial.
      default {return "[$type]@{$($values -join "; ")}"} # Complex object with multiple fields. Report type and every field with a [PSCustomObject]-like syntax.
    }
    # Else let the [string] cast do the conversion
    $Force = $True # Force quotes around it, else the type cast will likely fail.
    $TypeCast = "[$type]"
  } else {
    $TypeCast = ""
  }
  $string = [string]$var # Now whatever the type, convert it to a real string
  if ($string.length -eq 0) { # Special case of the empty string
    return '""'
  }
  if ($Force -or ($string -match "[ ``""'$]")) { # If there's any character that needs quoting
    if (($string -match '"') -and !($string -match "'")) { # If there are "s and no 's
      $string = "'$string'" # Surround with 's to preserve everything else.
    } else { # Either there are 's, or there are neither 's nor "s
      $s2 = ''
      for ($i=0; $i -lt $string.length; $i++) {
	$s2 += Switch ($string.Chars($i)) {
	  '`' { '``'; } 	# Back quote
	  '$' { '`$'; } 	# Dollar sign
	  '"' { '""'; } 	# Double quote
	  "`0" { '`0'; }	# Null
	  "`a" { '`a'; }	# Alert
	  "`b" { '`b'; }	# Backspace
	  "`f" { '`f'; }	# Form feed
	  "`n" { '`n'; }	# New line
	  "`r" { '`r'; }	# Carriage return
	  "`t" { '`t'; }	# Horizontal tab
	  "`v" { '`v'; }	# Vertical tab
	  default {
	    if ($_ -lt " ") {
	      "`$([char]0x$("{0:X2}" -f [byte]$_))"
	    } else {
	      $_
	    } # For Unicode chars > 2^16, use "$([char]::ConvertFromUtf32(0xXXXXXXXX))"
	  }
	}
      }
      $string = """$s2"""
    }
  }
  return "$TypeCast$string"
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-Vars		                                      #
#                                                                             #
#   Description     Display variables names and values, in a quoted format    #
#                                                                             #
#   Arguments       A list of variable names in the caller's scope.           #
#                                                                             #
#   Notes           The quoted format allows good readability and easy parsing#
#                                                                             #
#   History                                                                   #
#    2010-10-22 JFL Created this routine.                                     #
#    2013-10-03 JFL Renamed Write-Vars as Write-Vars.                         #
#                   Use standard Write-Xxxxx output routines.                 #
#                   Format the output as a native PowerShell assignment.      #
#    2013-11-19 JFL Detect undefined variables, and report them in a comment. #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Write-Vars () {
  foreach ($name in $args) {
    try {
      $var = Get-Variable $name -Scope 1 -ea stop
    } catch {
      Write-Host "# `$$name undefined"
      continue
    }
    Write-Host "`$$name = $(Quote $var.Value -Force)"
  }
}

Function Write-DebugVars () {
  foreach ($name in $args) {
    try {
      $var = Get-Variable $name -Scope 1 -ea stop
    } catch {
      Write-Debug "# `$$name undefined"
      continue
    }
    Write-Debug "`$$name = $(Quote $var.Value -Force)"
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Join-Command					      #
#                                                                             #
#   Description     Regenerate an evaluable command                           #
#                                                                             #
#   Arguments       $cmd    The command name                                  #
#                   $args   Optional arguments                                #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2013-10-08 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Convert $PSBoundParameters to a list suitable for use @splatted as Join-Command arguments
Function Get-Params ($Params) {
  $Params.Keys | % {
    if ($Params.$_ -is [System.Management.Automation.SwitchParameter]) {
      if ($Params.$_.IsPresent) {
	"-$_"
      } else {
	"-${_}:`$false"
      }
    } else {
      "-$_"
      $Params.$_
    }
  }
}

# Quote an object, but limit the result string to 256 character.
# Avoids flooding the debug output with huge objects.
Function LimitQuote($String) {
  $output = Quote $String
  $len = $Output.length
  if ($len > 256) {
    $output = $output.SubString(0,125) + " ... " + $output.SubString($len - 125)
  }
  return $output
}

Function Join-Command ([string]$cmd) {
  foreach ($arg in $args) {
    $cmd += " $(LimitQuote $arg)"
  }
  $cmd
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Exec                                                      #
#                                                                             #
#   Description     Display a command, then execute it. Both steps optional.  #
#                                                                             #
#   Notes           For details about the splatting operator quirks, see:     #
#                   http://piers7.blogspot.com/2010/06/splatting-hell.html    #
#                                                                             #
#   History                                                                   #
#    2010-06-03 JFL Created this routine.                                     #
#    2013-10-03 JFL Simplified by using the splatting operator.               #
#                   Use the standard Write-Xxxxx routines.                    #
#    2013-10-18 JFL Added options -l and -f.                                  #
#    2013-11-26 JFL Made $WhatIf an alias for $NoExec                         #
#    2016-03-01 JFL The variable must be $WhatIfPreference, not $WhatIf.      #
#                                                                             #
#-----------------------------------------------------------------------------#

if ($X -or $WhatIfPreference) {
  $NoExec = $WhatIfPreference = $true;
} else {
  $NoExec = $WhatIfPreference = $false;
}

Function Exec () {
  $log = $false
  while ($true)  {	# Parse Exec options, coming before the command name
    $cmd = Pop-Arg
    if ("$cmd" -eq "-l") {	# Log a copy of the command output
      $log = $true
      continue
    }
    if ("$cmd" -eq "-f") {	# Force running the command, whatever $script:NoExec is
      $NoExec = $false
      continue
    }
    break
  }
  $txtLine = Join-Command "$cmd" @args
  # Log, and optionally echo, the command line.
  if ($NoExec) { # The output is the list of programs that should run
    Write-Host $txtLine
  } elseif ($Verbose -and !$Debug) { # Pure verbose mode
    Write-Verbose $txtLine
  }
  if (!$NoExec) {
    Write-Debug $txtLine
    if ($log) {
      & $cmd @args | Write-LogFile -PassThru
    } else {
      & $cmd @args
    }
    Write-Debug "  exit $LastExitCode"
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        TraceProcs		                                      #
#                                                                             #
#   Description     Trace entry and exit from a set of routines               #
#                                                                             #
#   Arguments       A script block defining the routines to be traced         #
#                                                                             #
#   Notes           Must be invoked as . TraceProcs { script block }          #
#                                                                             #
#   History                                                                   #
#    2013-10-01 JFL Created this routine.                                     #
#    2013-12-10 JFL Allow redefining a traced function.                       #
#    2015-06-30 JFL TraceReturn now appends a comment with the call duration. #
#                                                                             #
#-----------------------------------------------------------------------------#

# Trace a block of routines. Must be invoked as . TraceProcs { script block }
Function TraceEntry ($name) {
  $cmdLine = Quote "$name"
  foreach ($arg in $args) { # Parse all optional arguments following the command name.
    $txtArg = $(LimitQuote $arg)
    $cmdLine += " $txtArg"
  }
  Write-Debug "$cmdLine"
  ${script:debugIndent} = "${script:debugIndent}  "
}

Function TraceReturn ($value, $comment=$null) {
  $msg = "return $value"
  if ($comment -ne $null) {
    $msg = "$msg  # $comment"
  }
  Write-Debug $msg
  ${script:debugIndent} = ${script:debugIndent}.SubString(2)
}

Function TraceProcs ([scriptblock]$scriptblock) {
<#
  .SYNOPSIS
  Trace entry and exit from a set of routines

  .DESCRIPTION
  Write function calls and return instructions to the debug output.
  The call and return are indented by call depth, allowing to easily see the
  hierarchy of operations.
  The function call is written with all its parameters formatted in a way
  suitable for direct cut-and-paste into another PowerShell windows.

  TraceProcs must be invoked with a . command ( . TraceProcs { functions } ),
  to make sure that the inner functions get defined in the script scope.

  .PARAMETER ScriptBlock
  A block of code, with the definitions of the functions to trace.

  .EXAMPLE
  . TraceProcs { ... }
  # Make sure debug output is enabled
  $DebugPreference = "Continue"
  # Define a factorial function. Note the required . before TraceProcs.
  . TraceProcs {
  Function Fact ([int]$n) {
    if ($n -le 1) {
      1
    } else {
      $n * (Fact($n-1))
    }
  }
  }
  # Invoke the factorial function
  Fact 3
  DEBUG: Fact 3
  DEBUG:   Fact 2
  DEBUG:     Fact 1
  DEBUG:       return 1
  DEBUG:     return 2
  DEBUG:   return 6
  6
#>
  $oldFuncHashCode = @{}
  foreach ($func in (dir function:* -exclude TraceProcs_*)) {
    $oldFuncHashCode[$func.name] = $func.GetHashCode()
  }
  . $scriptblock ;				# Define new functions and variables
  foreach ($func in (dir function:* -exclude TraceProcs_*)) {
    if ($oldFuncHashCode[$func.name] -eq $func.GetHashCode()) {continue} # The function definition has not changed
    if (Test-Path "function:TraceProcs_$func") {
      Write-Debug "# Removing previous function $func trace"
      del "function:TraceProcs_$func"
    }
    ren function:$func "TraceProcs_$func"	# Rename any new function defined above
    Write-Debug "# Tracing function $func"
    Invoke-Expression @"
      Function $func {				# Redefine a homonym function that traces entry and exit
	TraceEntry $func @args
        `$startTime = Get-Date
	`$result = . TraceProcs_$func @args
        `$endTime = Get-Date
        `$duration = `$endTime.Subtract(`$startTime)
        `$comment = `"`$(`$duration.TotalSeconds)s"
	TraceReturn `$result `$comment
	`$result
      }
"@
  }
}

# Useful for testing TraceProcs updates
Function Stop-TraceProcs ([string[]]$names) {
  foreach ($name in $names) {
    if ((test-path function:$name) -and (test-path function:TraceProcs_$name)) {
      $name = (dir function:$name).Name # Correct the name case
      write-debug "# Stop tracing $name"
      del function:$name
      ren function:TraceProcs_$name script:$name
    }
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#                         End of Debugging functions                          #
#                                                                             #
#-----------------------------------------------------------------------------#

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Test-Administrator                                        #
#                                                                             #
#   Description     Check if the user has administration rights               #
#                                                                             #
#   Arguments       $user	WindowsIdentity object. Default: Current user #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2012-01-31 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

function Test-Administrator ($user=$null) {
  if (!$user) {$user = [Security.Principal.WindowsIdentity]::GetCurrent()}
  ([Security.Principal.WindowsPrincipal]$user).IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-SymbolicLink                                          #
#                                                                             #
#   Description     Create a new symbolic link				      #
#                                                                             #
#   Notes 	    Automatically sets the link type if the target exists.    #
#                                                                             #
#   History                                                                   #
#    2018-09-16 JFL Created this routine                                      #
#                                                                             #
#-----------------------------------------------------------------------------#

Function New-SymbolicLink {
  [cmdletbinding(DefaultParameterSetName='Auto')]
  Param (
    [parameter(Position=0, ParameterSetName='Auto', Mandatory=$True)]
    [parameter(Position=0, ParameterSetName='Directory', Mandatory=$True)]
    [parameter(Position=0, ParameterSetName='File', Mandatory=$True)]
    [string]$Link,
    [parameter(Position=1, ParameterSetName='Auto', Mandatory=$True)]
    [parameter(Position=1, ParameterSetName='Directory', Mandatory=$True)]
    [parameter(Position=1, ParameterSetName='File', Mandatory=$True)]
    [string]$Target,
    [parameter(Position=2, ParameterSetName='Directory', Mandatory=$True)]
    [switch]$Directory,
    [parameter(Position=2, ParameterSetName='File', Mandatory=$True)]
    [switch]$File
  )
  Begin {
    Write-DebugVars Link Target Directory File
    Try {
      $null = [Win32.SymLink]
    } Catch {
      Add-Type @"
      using System;
      using System.Runtime.InteropServices;

      namespace Win32 {
	public class SymLink {
	  [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
	  public static extern bool CreateSymbolicLink(string lpSymlinkFileName, string lpTargetFileName, int dwFlags);
	  [DllImport("kernel32.dll")]
	  public static extern uint GetLastError();
	  
	  public static uint Create(string lpSymlinkFileName, string lpTargetFileName, int dwFlags) {
	    bool done = CreateSymbolicLink(lpSymlinkFileName, lpTargetFileName, dwFlags);
	    if (done) {
	      return 0;
	    } else {
	      return GetLastError();
	    }
	  }
	}
      }
"@
    }
  }
  Process {
    Write-DebugVars InputObject Link Target Directory File _
    # Get the absolute path of the future SymLink. (Else the creation fails)
    # Can't just call Resolve-Path, because it'll fail on non-existing paths
    $Link = Join-Path (Resolve-Path "$Link\..") (Split-Path -leaf $Link)
    # Set the symlink type
    if ($Directory) {
      $Flag = 1
    } elseif ($File) {
      $Flag = 0
    } else {	    # Auto-detect the type
      if (Test-Path -LiteralPath "$Target" -PathType Container) {
	$Flag = 1   # It's a Directory [PS v2 bug: DO NOT put a tab between the 1 and the #]
      } elseif (Test-Path -LiteralPath "$Target" -PathType Leaf) {
	$Flag = 0   # It's a File [PS v2 bug: DO NOT put a tab between the 0 and the #]
      } else {
	throw "Target '$Target' does not exist. Must specify -File or -Directory"
      }
    }
    if ($Flag) {
      $Type = "Directory"
      Write-Debug "mklink /D $Link $Target"
    } else {
      $Type = "File"
      Write-Debug "mklink $Link $Target"
    }
    # Get the win32 error on the same line, to make sure it's the right one that's reported below
    $error = [Win32.SymLink]::Create($Link, $Target, $Flag)
    if ($error) {
      throw [ComponentModel.Win32Exception][int]$error
    }
    New-Object PSObject -Property @{
      SymLink = $Link
      Target = $Target
      Type = $Type
    }
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-TemporaryDirectory                                    #
#                                                                             #
#   Description     Create a new temporary directory		              #
#                                                                             #
#   Notes 	    https://stackoverflow.com/questions/34559553/create-a-temporary-directory-in-powershell
#                                                                             #
#   History                                                                   #
#    2016-01-01 MK  Created this routine.                                     #
#    2018-10-16 JFL Added the -Force switch.                                  #
#                                                                             #
#-----------------------------------------------------------------------------#

Function New-TemporaryDirectory {
  Param (
    [Switch]$Force	# If true, do the operation, even if $NoExec is set
  )
  if ($Force) {
    $NoExec = $false
  }
  $parent = [System.IO.Path]::GetTempPath()
  [string] $name = [System.Guid]::NewGuid()
  New-Item -ItemType Directory -Path (Join-Path $parent $name) -WhatIf:$NoExec
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Is-ShadowCopy                                             #
#                                                                             #
#   Description     Check if the argument is a ShadowCopy.ps1 object          #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2018-09-16 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Is-ShadowCopy([object]$obj) {
  return (($obj.WmiShadowCopy -is [System.Management.ManagementObject]) -and ($obj.WmiShadowCopy.__CLASS -eq "Win32_ShadowCopy"))
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-ShadowCopy                                            #
#                                                                             #
#   Description     Get a list of existing shadow copies                      #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2016-04-18 JFL Created this routine.                                     #
#    2018-09-16 JFL Added the Path and Index fields.                          #
#		    Allow passing in GUIDs or dates as a string.	      #
#                                                                             #
#-----------------------------------------------------------------------------#

# Define a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
$DefaultFieldsToDisplay = 'Date','Drive','ID','Index'
$defaultDisplayPropertySet = New-Object System.Management.Automation.PSPropertySet(
  'DefaultDisplayPropertySet',[string[]]$DefaultFieldsToDisplay
)
$PSStandardMembers = [System.Management.Automation.PSMemberInfo[]]@($defaultDisplayPropertySet)

Function Get-ShadowCopy() {
  Param (
    [Parameter(Mandatory=$false, Position=0)][AllowNull()]
    [String]$Drive,		# Drive name. Ex: C or C:
    [Parameter(Mandatory=$false, Position=1)][AllowNull()]
    [DateTime]$Date,		# Date. Ex: "2016-04-01 01:00:00"
    [Parameter(Mandatory=$false, Position=2)][AllowNull()]
    [Guid]$ID,			# Shadow copy ID. Ex: "{CAB94C4A-C36F-4D6C-ABC1-991C7EEE989F}"
    [Parameter(Mandatory=$false, Position=3)]
    [int]$Index = 0		# Shadow copy index. Ex: 1 for '\\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy1'
  )
  Write-Debug("Get-ShadowCopy()")
  Write-DebugVars Drive Date ID Index
  $Volumes = @{} # List of known volumes, indexed by unique volume name
  if ($Drive -ne $null) { # Check if it's really a drive, or the string representation of a GUID
    if ($Drive -match "[A-Za-z0-9]+-[A-Za-z0-9]+-[A-Za-z0-9]+-[A-Za-z0-9]+-[A-Za-z0-9]+") {
      Write-Debug 1
      $ID = [Guid]$Drive
      $Drive = $null
    } elseif ($Drive -match '^\d+$') {
      Write-Debug 2
      $Index = [int]$Drive
      $Drive = $null
    } elseif (Invoke-Expression 'try {$ID = [guid]$Drive; $true} catch {$false}') {
      Write-Debug 3
      $Drive = $null
    } elseif (Invoke-Expression 'try {$Date = [DateTime]$Drive; $true} catch {$false}') {
      Write-Debug 4
      $Drive = $null
    }
    if ($Drive -eq $null) {Write-DebugVars Drive Date ID Index}
  }
  if (($Drive -ne $null) -and ($Drive -ne "")) { # If we got a request for a particular drive
    $DriveLetter = $Drive[0]
    $Volume = Get-Volume $DriveLetter -ea SilentlyContinue # Identify the corresponding volume object
    if (!$Volume) { # Undefined drive letter
      # Do not throw an error: This allows running -Prune on both nodes of a cluster,
      # only one of which having access to the target volume.
      Write-Verbose "No volume ${DriveLetter}: found on this system"
      return # Just return nothing.
    }
    $VolumeName = $Volume.ObjectId
    $Volumes[$VolumeName] = $Volume
    # The \ characters in the VolumeName cause problems with WQL.
    # Extract the UUID part of the VolumeName.
    if ($VolumeName -match "Volume({.*})") {
      $VolumeUuid = $Matches[1]
    } else {
      throw "Unexpected volume syntax: $VolumeName. Expected '\\?\Volume{`$UUID}'."
    }
    # Now match the VolumeName. Note that WQL's LIKE operator % char matches any substring, like glob's *
    # See https://msdn.microsoft.com/en-us/library/aa392263(v=vs.85).aspx
    Write-Debug "Get-WmiObject Win32_Shadowcopy -filter `"VolumeName LIKE '%$VolumeUuid%'`""
    $WmiShadowCopies = @(Get-WmiObject Win32_Shadowcopy -filter "VolumeName LIKE '%$VolumeUuid%'")
  } elseif (($ID -ne $null) -and ($ID -ne "")) { # If we got a request for a particular shadow ID
    Write-Debug "Get-WmiObject Win32_Shadowcopy -filter `"ID LIKE '%$ID%'`""
    $WmiShadowCopies = @(Get-WmiObject Win32_Shadowcopy -filter "ID LIKE '%$ID%'")
  } elseif ($Index -ne 0) { # If we got a request for a particular shadow Index
    Write-Debug "Get-WmiObject Win32_Shadowcopy -filter `"DeviceObject LIKE '%HarddiskVolumeShadowCopy$Index'`""
    $WmiShadowCopies = @(Get-WmiObject Win32_Shadowcopy -filter "DeviceObject LIKE '%HarddiskVolumeShadowCopy$Index'")
  } else { # Get all Shadow Copies
    Write-Debug "Get-WmiObject Win32_Shadowcopy"
    $WmiShadowCopies = @(Get-WmiObject Win32_Shadowcopy)
  }
  foreach ($WmiShadowCopy in $WmiShadowCopies) {
    $ID = $WmiShadowCopy.ID
    $Volume = $WmiShadowCopy.VolumeName
    if (!$Volumes.ContainsKey($Volume)) {
      $Volumes[$Volume] = Get-Volume -Path $Volume
    }
    $Drive = $Volumes[$Volume].DriveLetter
    $ShadowDate = [management.managementDateTimeConverter]::ToDateTime($WmiShadowCopy.InstallDate)
    $Path = $WmiShadowCopy.DeviceObject
    $Index = [int]($Path -replace ".*?(\d+)$",'$1')
    # Filter by date if specified
    if ($date) {
      if ($date.Year  -ne $ShadowDate.Year ) { continue }
      if ($date.Month -ne $ShadowDate.Month) { continue }
      if ($date.Day   -ne $ShadowDate.Day  ) { continue }
      if ($date.Hour -or $date.Minute -or $date.Second) {
	if ($date.Hour   -ne $ShadowDate.Hour  ) { continue }
	if ($date.Minute -ne $ShadowDate.Minute) { continue }
	if ($date.Second -ne $ShadowDate.Second) { continue }
      }
    }
    # Create an object with all the information we have
    $object = New-Object PSObject -Property @{
      ID = $ID
      Drive = $Drive
      Volume = $Volume
      Date = $ShadowDate
      WmiShadowCopy = $WmiShadowCopy
      Path = $Path
      Index = $Index
      Fate = $null
      # Add age marks
      Days = ($tomorrow - $ShadowDate).Days
      Weeks = [Math]::Floor(($NextWeek - $ShadowDate).Days / 7)
      Months = ($ThisMonth.Year - $ShadowDate.Year) * 12 + ($ThisMonth.Month - $ShadowDate.Month)
      Trims = [Math]::Floor((($ThisTrim.Year - $ShadowDate.Year) * 12 + ($ThisTrim.Month + 2 - $ShadowDate.Month)) / 3)
      Years = ($ThisMonth.Year - $ShadowDate.Year)
    }
    # Add a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
    $object | Add-Member MemberSet PSStandardMembers $PSStandardMembers
    # Output the result object
    $object
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Remove-ShadowCopy                                         #
#                                                                             #
#   Description     Remove shadow copies		                      #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2016-04-18 JFL Created this routine.                                     #
#    2016-04-19 JFL Use PowerShell's native ShouldProcess() mechanism to      #
#                   manage confirmations and the WhatIf mode.                 #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Remove-ShadowCopy() {
  [CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact="High")]
  Param (
    [Parameter(Mandatory=$false, ValueFromPipeline=$true, Position=0)]
    [Object[]]$ShadowCopies = $()
  )
  Begin {
    Write-Debug "Remove-ShadowCopy.Begin()"
    Write-DebugVars InputObject ShadowCopies _
  }
  Process {
    Write-Debug "Remove-ShadowCopy.Process()"
    Write-DebugVars InputObject ShadowCopies _
    $ShadowCopies | % {
      if ($_ -is [String]) { # Assume it's an ID. Ex: "{CAB94C4A-C36F-4D6C-ABC1-991C7EEE989F}"
	$_ = Get-ShadowCopy($_)
      }
      $Drive = $_.Drive
      if ("$Drive" -ne "") {
	$Drive = " ${Drive}:"
      }
      $isodate = Get-Date $_.Date -UFormat "%Y-%m-%d %H:%M:%S"
      $day = $_.Days
      $week = $_.Weeks
      $month = $_.Months
      $trim = $_.Trims
      $year = $_.Years
      $Target = "the$Drive $isodate shadow copy"
      $Age = "Ages: $day days/$week weeks/$month months/$trim trims/$year years"
      $WouldDo = "Would delete $Target. ($Age)"
      $Caption = "About to delete $Target. ($Age)"
      $Warning = "Warning: All data on $Target will be lost forever. Confirm deletion?"
      $fate = $_.fate
      if ((!$fate) -or ($fate -eq "Remove")) {
	# https://msdn.microsoft.com/en-us/library/system.management.automation.cmdlet.shouldprocess(v=vs.85).aspx
	# https://msdn.microsoft.com/en-us/library/system.management.automation.cmdlet.shouldcontinue(v=vs.85).aspx
	if ($Force -or $PSCmdlet.ShouldProcess($WouldDo, # Shown by -WhatIf or by -ea Continue
					       $Warning, # Bottom confirmation line.
					       $Caption) # Top confirmation line. Docs says it may mot be displayed by all PSHosts.
	   ) {
	  Write-Host "Deleting $Target."
	  $_.WmiShadowCopy.Delete()
	  $ID = $_.ID
	  $Message = "${scriptname}: Deleted $Target."
	  Write-EventLog -LogName $logName -Source $ScriptBaseName -EventId 2 -EntryType Information -Message $Message
	} else {
	  if (!$noexec) { # ShouldProcess() already has displayed a message for the $NoExec mode
	    Write-Host "Skipping the removal of $Target."
	  }
	}
      } else { # Fate was Keep or Unknown
	if ($Verbose) { # Don't use Write-Verbose to keep output aligned
	  if ($NoExec) {
	    Write-Host "What if: Would keep   $Target. ($Age)"
	  } else {
	    Write-Host "Keeping  $Target."
	  }
	}
      }
    }
  }
  End {
    Write-Debug "Remove-ShadowCopy.End()"
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Set-ShadowCopyFate                                        #
#                                                                             #
#   Description     Define which copies to keep, and which to remove          #
#                                                                             #
#   Notes 	    Uses a recycling policy inspired by that of backup tapes. #
#                                                                             #
#   History                                                                   #
#    2016-04-18 JFL Created this routine.                                     #
#    2016-05-26 JFL Added 2-day preservation periods for the next N weeks.    #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Set-ShadowCopyFate() {
  Param (
    [Object[]]$ShadowCopies
  )
  # Initialize the fate property
  $ShadowCopies | % {
    $_.Fate = "Unknown" # One of "Unknown", "Keep", "Remove"
  }
  # Keep all daily copies less than 7 days old
  $OneWeekAgo = $today.AddDays(-7)
  $ShadowCopies | % {
    if ($_.Date -gt $OneWeekAgo) {
      $_.Fate = "Keep"
    }
  }
  # Then eliminate all but the last entry in every two days period in the last N weeks
  # Assumes that there's a gap in the week-end, where no shadow copy is created.
  # So align these 2-day periods based on the last shadow copy each week.
  # Ex: (M13h T01h T13h W01h W13h T01h T13h F01h F13h S01h) -> (T01h T01h S01h)
  $nWeeks = 4
  $nWeeksAgo = $today.AddDays(-7 * $nWeeks)
  for ($week=1; $week -le $nWeeks; $week++) { # These days are in weeks #1 to #N. Those in week #0 are already all preserved.
    # Repeat for each of these N weeks, starting from the end of each week
    $selection = @($ShadowCopies | where { $_.Weeks -eq $week } | sort -Descending Date)
    if (!$selection.count) {continue}
    $baseDate = ($selection[0]).Date.AddHours(1) # Add some margin due to the varying duration of shadow copy creations
    $selection | % {
      if ($_.Date -lt $baseDate) { # We entered another 2-day period
      	$baseDate = $baseDate.AddDays(-2) # Move the threshold by 2 days
      	if (($_.Fate -eq "Unknown") -and ($_.Date -gt $nWeeksAgo)) {
	  $_.Fate = "Keep"
	}
      } else { # Another older shadow copy in the same 2-day period
      	if (($_.Fate -eq "Unknown") -and ($_.Date -gt $nWeeksAgo)) {
	  $_.Fate = "Remove"
	}
      }
    }
  }
  # Then eliminate all but the last entry in every prior calendar week
  $lastWeek = -1
  $ShadowCopies | sort -Descending Date | % {
    $week = $_.Weeks
    if ($week -ne $lastWeek) {
      $lastWeek = $week
    } else { # An older copy in the same week
      if ($_.Fate -eq "Unknown") {
      	$_.Fate = "Remove"
      }
    }
  }
  # Keep all weekly copies less than 3 months old
  $ThreeMonthsAgo = $today.AddMonths(-3)
  $ShadowCopies | % {
    if ($_.Date -gt $ThreeMonthsAgo) {
      if ($_.Fate -eq "Unknown") {
	$_.Fate = "Keep"
      }
    }
  }
  # Then eliminate all but the last remaining entry in every prior calendar month
  $lastMonth = -1
  $ShadowCopies | where {$_.Fate -ne "Remove"} | sort -Descending Date | % {
    $month = $_.Months
    if ($month -ne $lastMonth) {
      $lastMonth = $month
    } else { # An older copy in the same month
      if ($_.Fate -eq "Unknown") {
      	$_.Fate = "Remove"
      }
    }
  }
  # Keep all monthly copies less than 12 months old
  $OneYearAgo = $today.AddMonths(-12)
  $ShadowCopies | % {
    if ($_.Date -gt $OneYearAgo) {
      if ($_.Fate -eq "Unknown") {
	$_.Fate = "Keep"
      }
    }
  }
  # Then eliminate all but the last remaining entry in every prior calendar trimester
  $lastTrim = -1
  $ShadowCopies | where {$_.Fate -ne "Remove"} | sort -Descending Date | % {
    $trim = $_.Trims
    if ($trim -ne $lastTrim) {
      $lastTrim = $trim
    } else { # An older copy in the same trimester
      if ($_.Fate -eq "Unknown") {
      	$_.Fate = "Remove"
      }
    }
  }
  # Keep all trimestrial copies less than 4 years old
  $FourYearAgo = $today.AddMonths(-12*4)
  $ShadowCopies | % {
    if ($_.Date -gt $FourYearAgo) {
      if ($_.Fate -eq "Unknown") {
	$_.Fate = "Keep"
      }
    }
  }
  # Then eliminate all but the last remaining entry in every prior calendar year
  $lastYear = -1
  $ShadowCopies | where {$_.Fate -ne "Remove"} | sort -Descending Date | % {
    $year = $_.Years
    if ($year -ne $lastYear) {
      $lastYear = $year
    } else { # An older copy in the same year
      if ($_.Fate -eq "Unknown") {
      	$_.Fate = "Remove"
      }
    }
  }
  # Finally keep all that remain unclassified
  $ShadowCopies | where {$_.Fate -eq "Unknown"} | % {
    $_.Fate = "Keep"
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-ShadowCopy                                            #
#                                                                             #
#   Description     Create a new shadow copy of a volume                      #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2016-04-21 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

$NewErrorMessages = @(
  "Success",						  # Success 0
  "Access denied",                                        # Error 1
  "Invalid argument",                                     # Error 2
  "Specified volume not found",                           # Error 3
  "Specified volume not supported",                       # Error 4
  "Unsupported shadow copy context",                      # Error 5
  "Insufficient storage",                                 # Error 6
  "Volume is in use",                                     # Error 7
  "Maximum number of shadow copies reached",              # Error 8
  "Another shadow copy operation is already in progress", # Error 9
  "Shadow copy provider vetoed the operation",            # Error 10
  "Shadow copy provider not registered",                  # Error 11
  "Shadow copy provider failure",                         # Error 12
  "Unknown error"                                         # Error 13
)

Function New-ShadowCopy {
  Param (
    [Parameter(Mandatory=$true, Position=0)][AllowNull()]
    [String]$Drive		# Drive name. Ex: C or C:
  )

  if ("$Drive" -eq "") {
    throw "No drive specified."
  }

  # Use only the drive letter
  $Drive = $Drive[0]

  if (!$NoExec) {
    Write-Host "Creating a new shadow copy of volume ${Drive}:"
    # Get the class, to be able to access static methods
    $WmiShadowCopyClass = [WMICLASS]"root\cimv2:win32_shadowcopy"

    # Create a new shadow copy
    $WmiShadowCopyResult = $WmiShadowCopyClass.create("${Drive}:\", "ClientAccessible")

    # Output the result
    $ReturnValue = $WmiShadowCopyResult.ReturnValue
    if ($ReturnValue -eq 0) {	# Success. Output the ShadowCopy object.
      $ID = $WmiShadowCopyResult.ShadowID
      $ShadowCopy = Get-ShadowCopy -ID $ID
      $isodate = Get-Date $ShadowCopy.Date -UFormat "%Y-%m-%d %H:%M:%S"
      $Message = "${scriptname}: Created ${Drive}: $isodate shadow copy."
      Write-EventLog -LogName $logName -Source $ScriptBaseName -EventId 1 -EntryType Information -Message $Message
      $ShadowCopy
    } else {			# Failure. Throw an error.
      if ($ReturnValue -lt $NewErrorMessages.Count) {
	$Message = $NewErrorMessages[$ReturnValue]
      } else {
	$Message = "Unknown error #$ReturnValue"
      }
      $Message = "Failed to create a shadow copy of drive ${Drive}:. $Message."
      Write-EventLog -LogName $logName -Source $ScriptBaseName -EventId 1001 -EntryType Error -Message "${scriptname}: $Message"
      throw $Message
    }
  } else {
    Write-Host "What if: Would create a shadow copy on drive ${Drive}:"
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Mount-ShadowCopy                                          #
#                                                                             #
#   Description     Mount a shadow copy of a volume at a given mount point    #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2018-09-16 JFL Created this routine                                      #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Mount-ShadowCopy {
  [CmdletBinding()]
  Param(
    [Parameter(Mandatory=$false, ValueFromPipeline=$true, Position=0)]
    [Object[]]$ShadowCopies = $(),
    [String]$MountPoint,
    [Switch]$Force	# If true, do the operation, even if $NoExec is set
  )
  Begin {
    Write-Debug "Mount-ShadowCopy.Begin()"
    Write-DebugVars InputObject ShadowCopies MountPoint _
    if ($Force) {
      $NoExec = $false
    }
  }
  Process {
    Write-Debug "Mount-ShadowCopy.Process()"
    Write-DebugVars InputObject ShadowCopies MountPoint _
    $ShadowCopies | % {
      if (!(Is-ShadowCopy($_))) { # Ex: It's an ID. Ex: "{CAB94C4A-C36F-4D6C-ABC1-991C7EEE989F}"
	$_ = Get-ShadowCopy($_)
      }
      Write-DebugVars  _

      $Path = $_.Path
      if (!$Path.EndsWith("\")) {
	$Path = "$Path\"
      }

      if (Test-Path -LiteralPath $MountPoint -PathType Container) {
	$MountPoint = Join-Path $MountPoint (
	  "$($_.Drive)_$($_.Date -replace " (\d+):(\d+):(\d+)", '_$1h$2m$3')"
	)
      }

      if ($NoExec) {
      	Write-Host "WhatIf: Would mount '$Path' at '$MountPoint'"
      } else {
	Write-Verbose "Mounting '$Path' at '$MountPoint'"
	try {
	  $Null = New-SymbolicLink $MountPoint $Path -Directory
	} catch {
	  Write-Error "Failed to mount ${Path}: $($_.Exception.Message)"
	}
	# Output the absolute mount point name generated
	"$(Resolve-Path $MountPoint)"
      }
    }
  }
  End {
    Write-Debug "Mount-ShadowCopy.End()"
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Dismount-ShadowCopy                                       #
#                                                                             #
#   Description     Dismount a shadow copy from a given mount point	      #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2018-09-16 JFL Created this routine                                      #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Dismount-ShadowCopy {
  [CmdletBinding()] # Don't need to use heavy -WhatIf confirmation: This is easily reversible.
  Param(
    [Parameter(Mandatory=$True,ValueFromPipeline=$True,ValueFromPipelineByPropertyName=$True)]
    [Alias("FullName")]
    [string[]]$Paths,
    [Switch]$Force	# If true, do the operation, even if $NoExec is set
  )
  Begin {
    Write-Debug "Dismount-ShadowCopy.Begin()"
    Write-DebugVars InputObject Paths _
    if ($Force) {
      $NoExec = $false
    }
  }
  Process {
    Write-Debug "Dismount-ShadowCopy.Process()"
    Write-DebugVars InputObject Paths _
    $Paths | ForEach-Object -Process {
      $Path = $_
      if (Test-Path -LiteralPath $Path -PathType Container) {
	if ((Get-Item -Path $Path).Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
	  if ($NoExec) {
	    Write-Host "WhatIf: Would dismount '$Path'"
	  } else {
	    Write-Verbose "Dismounting '$Path'"
	    try {
	      $Path = Resolve-Path $Path
	      [System.IO.Directory]::Delete($Path, $false) | Out-Null
	    } catch {
	      Write-Warning "Failed to dismount ${Path}: $($_.Exception.Message)"
	    }
	  }
	} else {
	  Write-Warning "The path $Path isn't a ReparsePoint"
	}
      } else {
	Write-Warning "The path $Path isn't a directory"
      }
    }
  }
  End {
    Write-Debug "Dismount-ShadowCopy.End()"
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-PreviousVersions                                      #
#                                                                             #
#   Description     List the previous versions of a file available            #
#                                                                             #
#   Arguments                                                                 #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2018-10-15 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Define a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
$PVDefaultFieldsToDisplay = 'SCIndex','SCDate','FileDate','Length'
$PVDefaultDisplayPropertySet = New-Object System.Management.Automation.PSPropertySet(
  'DefaultDisplayPropertySet',[string[]]$PVDefaultFieldsToDisplay
)
$PVPSStandardMembers = [System.Management.Automation.PSMemberInfo[]]@($PVDefaultDisplayPropertySet)

Function Get-PreviousVersions {
  [CmdletBinding()]
  Param(
    [Parameter(Mandatory=$true, Position=0)]
    [String]$FileName,
    [Parameter(Mandatory=$false, ValueFromPipeline=$true, Position=1)]
    [Object[]]$ShadowCopies = $(),
    [Switch]$Exhaustive,
    [Parameter(Mandatory=$false, Position=2)][AllowNull()]
    [String]$Restore
  )
  Write-Debug "Get-PreviousVersions `"$FileName`" list[$($ShadowCopies.Count)] `"$Restore`""
  Write-DebugVars Exhaustive
  $File = Get-Item $FileName
  # TO DO: Support wildcards, and loop over multiple files
  if ($File.count -gt 1) {throw "Multiple files aren't supported yet"}
  # TO DO: Get the previous versions of a file that does not exist anymore
  if ($File.count -eq 0) {throw "Deleted files aren't supported yet"}
  $Drive = $File.PSDrive.ToString()
  if (($Drive.length -ne 1) -or ($Drive -lt "A") -or ($Drive -gt "Z")) {
    throw "Unsupported drive: ${drive}"
  }
  $LastDate = $File.LastWriteTime
  $LastLength = $File.Length
  $RelPath = $File.FullName.Substring(3)
  # Select all shadow copies for that drive
  if (!$ShadowCopies.Count) {	# If we didn't receive any, get all possible ones
    $ShadowCopies = Get-ShadowCopy $Drive
  } else {			# Validate the shadow copies we got
    $ShadowCopies = $ShadowCopies | % {
      if (Is-ShadowCopy $_) {
      	$ShadowCopy = $_
      } else {
      	$ShadowCopy = Get-ShadowCopy $_
      }
      if ($ShadowCopy -and ($ShadowCopy.Drive -eq $Drive)) {
      	$ShadowCopy		# Keep only those that match the file drive
      }
    }
  }
  if ($ShadowCopies.Count -eq 0) {
    Write-Verbose "There is no shadow copy for drive ${drive}"
    return
  }
  # Mount all shadow copies for that drive
  $TempDir = New-TemporaryDirectory -Force # Do it even in NoExec mode 
  Write-Debug "Mounting shadow copies at ${TempDir}"
  $MountPoints = @()
  foreach ($ShadowCopy in ($ShadowCopies | sort Date -Descending)) {
    if ((!$Exhaustive) -and ($ShadowCopy.Date -gt $LastDate)) {
      continue # No need to try this one, as we're going to find the same file version
    }
    $MountPoint = Mount-ShadowCopy $ShadowCopy -MountPoint $TempDir -Force # Do it even in NoExec mode
    $MountPoints += $MountPoint
    $FullPath = Join-Path $MountPoint $RelPath
    if (Test-Path $FullPath) {
      $ShadowFile = Get-Item $FullPath
      $Date = $ShadowFile.LastWriteTime
      $Length = $ShadowFile.Length
      if (($Date -ne $LastDate) -and ($Length -ne $LastLength)) { # This is a different previous version
	# Create an object with information about that previous version
	$object = New-Object PSObject -Property @{
	  SCIndex = $ShadowCopy.Index
	  SCDate = $ShadowCopy.Date
	  SCID = $ShadowCopy.ID
	  FileDate = $Date
	  Length = $Length
	  FullName = $ShadowFile.FullName.Replace($MountPoint, "${Drive}:")
	  Name = $ShadowFile.Name
	}
	# Add a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
	$object | Add-Member MemberSet PSStandardMembers $PVPSStandardMembers
	# If we're restore that file, but in NoExec mode, display a message
	if ($Restore -and $NoExec) {
	  Write-Host "What if: Would copy to ${Restore}:"
	}
	# Output the previous version object
	$object
	# If we're to restore that file, do it now and exit from the loop
	if ($Restore) {
	  if (!$NoExec) {
	    Copy-Item $FullPath $Restore
	  }
	  break
	}
	# Prepare search for the next different version
	$LastDate = $Date
	$LastLength = $Length
      }
    }
  }
  Write-Debug "Unmounting shadow copies from ${TempDir}"
  $MountPoints | % {
    Dismount-ShadowCopy $_ -Force # Do it even in NoExec mode
  }
  Remove-Item $TempDir -WhatIf:$false # Do it even in NoExec mode
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Main                                                      #
#                                                                             #
#   Description     Execute the specified actions                             #
#                                                                             #
#   Arguments       See the Param() block at the top of this script           #
#                                                                             #
#   Notes 	                                                              #
#                                                                             #
#   History                                                                   #
#    2016-04-18 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

  Write-Debug "ShadowCopy.Begin()"

  # Check prerequisite: All operations below require running as Administrator
  if (!(Test-Administrator)) {
    throw "Must be running as Administrator for this operation to work."
  }

  # Reference dates used by -Get and -Prune
  $now = Get-Date
  $Today = Get-Date -Hour 0 -Minute 0 -Second 0
  $Tomorrow = $Today.AddDays(1)
  $ThisWeek = $Today.AddDays(-$now.DayOfWeek.value__)
  $NextWeek = $ThisWeek.AddDays(7)
  $ThisMonth = $Today.AddDays(1-$now.Day)
  $NextMonth = $ThisMonth.AddMonths(1)
  $ThisTrim = $ThisMonth.AddMonths(-(($ThisMonth.Month - 1) % 3))
  $NextTrim = $ThisTrim.AddMonths(3)
  Write-DebugVars today tomorrow ThisWeek NextWeek ThisMonth NextMonth ThisTrim NextTrim

  $nInputObjects = 0 # Number of InputObjects passed in

  if ($Remove) {
    # Create a steppable pipeline, to make sure the confirm all or none responses
    # work correctly, even if there are multiple InputObjects.
    $RemoveCodeBlock = { Remove-ShadowCopy }
    $RemoveSteppablePipeline = $RemoveCodeBlock.GetSteppablePipeline()
    $RemoveSteppablePipeline.Begin($true) # Expect data in the pipeline input
  }

  if ($Dismount) {
    # Create a steppable pipeline, to make sure the confirm all or none responses
    # work correctly, even if there are multiple InputObjects.
    $DismountCodeBlock = { Dismount-ShadowCopy }
    $DismountSteppablePipeline = $DismountCodeBlock.GetSteppablePipeline()
    $DismountSteppablePipeline.Begin($true) # Expect data in the pipeline input
  }

  # The New & Remove commands write to the event log, but we need to make sure the ShadowCopy source is defined.
  $logName = "Application"                # Event Log name
  New-EventLog -LogName $logName -Source $ScriptBaseName -ea SilentlyContinue
} ; # End of the Begin block

Process {
  Write-Debug "ShadowCopy.Process()"
  Write-DebugVars _ InputObject
  $InputObject | % {
    $nInputObjects += 1
    Write-DebugVars _
    
    # Unmount a Shadow Copy. Must be done before converting $_ to a list of ShadowCopy objects
    if ($Dismount) {
      $DismountSteppablePipeline.Process($_) # Dismount-ShadowCopy $_
      return
    }

    # List previous versions. Must be done before converting $_ to a list of ShadowCopy objects
    if ($Previous) {
      Get-PreviousVersions $Pathname $_ $Restore -Exhaustive:$Exhaustive
      return
    }

    # Identify the argument type, and build the dynamic argument list to pass down to Get-ShadowCopy
    $GetArgs = @{}
    $ShadowCopies = @()
    if (Is-ShadowCopy($_)) {
      $ShadowCopies = @($_) # This already is a Shadow Copy object made by this script
    } else {
      $ShadowCopies = Get-ShadowCopy $_
    }

    # Enumerate Shadow Copies
    if ($Get) {
      $ShadowCopies
    }

    # Delete Shadow Copies
    if ($Remove) {
      $ShadowCopies | % { # Remove the shadow copy using the pipeline
	$RemoveSteppablePipeline.Process($_) # Remove-ShadowCopy $_
      }
    }
    
    # Mount a Shadow Copy
    if ($Mount) {
      $ShadowCopies | % { 
      	$_ | Mount-ShadowCopy -MountPoint $MountPoint
      }
    }
  }
}

End {
  Write-Debug "ShadowCopy.End()"
  Write-DebugVars nObjects
  if (!$nInputObjects) { # If no object or object specifier was passed in
    if ($Get) {			# By default, list ALL shadow copies
      Get-ShadowCopy
    }
    if ($Previous) {		# By default, list ALL previous versions, or restore the latest
      Get-PreviousVersions $Pathname -Exhaustive:$Exhaustive -Restore $Restore
    }

    if ($Remove) {
      $RemoveSteppablePipeline.End()
    }
    if ($Dismount) {
      $DismountSteppablePipeline.End()
    }

    # Remove all Shadow Copies falling out of our cyclic preservation policy
    if ($Prune) {
      $ShadowCopies = @(Get-ShadowCopy -Drive $Drive)
      Set-ShadowCopyFate $ShadowCopies # Mark entries with "Keep" or "Remove" flags
      Remove-ShadowCopy $ShadowCopies  # Remove those marked with "Remove"
    }

    # Create a new shadow copy
    if ($New) {
      New-ShadowCopy $Drive
    }
  }
}
