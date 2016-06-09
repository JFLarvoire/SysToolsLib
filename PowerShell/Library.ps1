###############################################################################
#                                                                             #
#   File name       Library.ps1                                               #
#                                                                             #
#   Description     A collection of useful PowerShell functions               #
#                                                                             #
#   Notes           For Popular PowerShell Modules libraries, see:            #
#   http://social.technet.microsoft.com/wiki/contents/articles/4308.popular-powershell-modules.aspx
#                                                                             #
#                   For best practices, see:				      #
#   http://blogs.technet.com/b/pstips/archive/2014/06/17/powershell-scripting-best-practices.aspx
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Created this library.                                     #
#    2013-12-10 JFL Added a help. Fixed the command-line processing.          #
#                   Now usable as a test program for itself.                  #
#    2014-01-15 JFL Added functions Get-ContentByLine, Set-PinnedApplication. #
#    2015-04-29 JFL Added function U to generate Unicode characters.          #
#    2015-04-30 JFL Added function Get-UpTime.                                #
#    2015-06-08 JFL Added function Send-UdpPacket.                            #
#                   Rewrote Get-IPAddress not using nslookup.                 #
#    2015-06-11 JFL Added function Now.                                       #
#    2015-06-12 JFL Added function Enum-RegistryValues.                       #
#    2015-06-16 JFL Added function Compare-Lists.                             #
#    2015-06-30 JFL Added functions Out-Debug, Out-Verbose, Write-Error.      #
#                   Performance improvements in all debug output functions.   #
#                   TraceReturn now appends a comment with the call duration. #
#                   Bugfix: Write-LogFile creates the log directory if needed.#
#                   Function Quote now escapes all control characters.        #
#                   Split global variable $script into 3 $scriptXxxx vars.    #
#    2015-07-09 JFL Added function Send-Mail.                                 #
#    2015-07-20 JFL Added function Add-ErrorMessage.                          #
#    2015-08-11 JFL Added function Format-ListSorted.                         #
#    2015-08-21 JFL Commented-out func. Out-Default, useless at script scope. #
#                   Added function Write-Error.                               #
#                   Redefine $crlf as the environment-specific new-line.      #
#                   Bugfix: Added missing ; in Test-PSVersion.                #
#                   Added global variables $identity and $userName.           #
#    2015-09-11 JFL Fixed bug in Format-ListSorted.                           #
#    2015-10-14 JFL Define PS v3's $PSCommandPath and $PSScriptRoot in PS v2. #
#                                                                             #
###############################################################################
#Requires -Version 2

<#
  .SYNOPSIS
  A collection of useful PowerShell functions

  .DESCRIPTION
  Pick and choose routines from this module for your own scripts.
  Or invoke that script directly to test the various routines inside.

  .PARAMETER Commands
  Execute each command in the following list.
  Alias: -C

  .PARAMETER D
  Switch enabling the debug mode. Display debug messages; Trace function calls.
  Alias: -Debug

  .PARAMETER LogFile
  Specify a log file, where to record all script output.
  Alias: -L

  .PARAMETER V
  Switch enabling the verbose mode. Display verbose messages.
  Alias: -Verbose

  .PARAMETER X
  Switch enabling the NoExec mode, alias the WhatIf mode.
  Display the commands that should run, but do not execute them.
  Alias: -WhatIf

  .PARAMETER Version
  Display this script version and exit.

  .EXAMPLE
  ./Library -d -c "Fact 5"
  Invoke the factorial function, with argument 5, in debug mode.

  .EXAMPLE
    >Use > followed by a backspace (like here) to avoid having a PowerShell prompt on the first example line.
    Then you can add anything you want, including the default PowerShell help prompt: C:\PS>
#>

Param (
  [Parameter(ParameterSetName='Commands', Position=0, Mandatory=$false)]
  [String[]][alias("C")]$Commands,	# Commands to execute for testing

  [Parameter(ParameterSetName='Commands', Mandatory=$false)]
  [String][alias("L")]$LogFile,		# Name of the log file

  [Parameter(ParameterSetName='Commands', Mandatory=$false)]
  [Switch]$D,				# If true, display debug information

  [Parameter(ParameterSetName='Commands', Mandatory=$false)]
  [Switch]$V,				# If true, display verbose information

  [Parameter(ParameterSetName='Commands', Mandatory=$false)]
  [Switch][alias("WhatIf")]$X,		# If true, display commands, but don't execute them

  [Parameter(ParameterSetName='Version', Mandatory=$true)]
  [Switch]$Version			# If true, display the script version
)

# If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2015-10-14"
if ($Version) {
  echo $scriptVersion
  return
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
	      $value = Quote (eval "`$_ = `$var ; $($item.DisplayEntry.Value)")
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

###############################################################################
#                                                                             #
#                            Debugging the debugger                           #
#                                                                             #
###############################################################################

# ShowArgs - Used for debugging the debugger :-)
Function ShowArgs() { # Display what does into the $args array.
  $n = 0
  foreach ($arg in $args) { # Parse all optional arguments following the command name.
    $t = $arg.GetType()
    Write-Host "Arg ${n} is ${t}: $arg"
    $n += 1
  }
}

if ($false) { # Older Exec() implementations, kept as a reference

Function Exec([string]$cmd) {
  $txtLine = "$cmd"
  foreach ($arg in $args) { # Parse all optional arguments following the command name.
    $txtLine += " $(Quote $arg)"
  }
  # Log, and optionally echo, the command line.
  if ($script:Debug) {
    Write-Debug $txtLine
  } elseif ($script:Verbose) {
    Write-Verbose $txtLine
  } elseif ($script:NoExec) {
    Write-Host $txtLine
  }
  if (!$script:NoExec) {
    & $cmd @args
  }
}

Function Exec([string]$cmd) {
  $txtLine = "$cmd"
  $cmdLine = "$cmd"
  $a = $args # Make a copy (Necessary because Invoke-Expression overwrites $args)
  $n = 0
  foreach ($arg in $args) { # Parse all optional arguments following the command name.
    $txtLine += " $(Quote $arg)"
    if (($arg -ne $null) -and ($arg.GetType().FullName -eq "System.String") -and ($arg -match '^-\w+:?$')) {
      $cmdLine += " $arg" # Let Invoke-Expression decide whether it's a param name or an actual string.
    } else {
      $cmdLine += " `$a[$n]" # Preserve the type through Invoke-Expression.
    }
    $n += 1
  }
  if ($script:Verbose -or $script:Debug -or $script:NoExec) {
    $Verbose = $true
  }
  Write-Verbose $txtLine -verbose:$vVerbose # Log, and optionally echo, the command line.
  if (!$script:NoExec) {
    Invoke-Expression $cmdLine
  }
}

# Initial version using ONE script blocks passed as argument.
# Ex: Exec {format C:}
Function Exec([System.Management.Automation.ScriptBlock]$sb) {
  if ($script:Verbose -or $script:NoExec) {
    $sb
  }
  if (!$script:NoExec) {
    & $sb
  }
}

# Version using a dynamically generated script block.
# Note: Does not support named parameters or switches.
Function Exec {
  $cmd = Pop-Arg
  $txtLine = "$cmd"
  $cmdLine = "`$var = Get-Variable args -Scope 1 ; $cmd"
  $n = 0
  foreach ($arg in $args) {
    $txtLine += " $(Quote $arg)"
    $cmdLine += " `$var.Value[$n]"
    $n++
  }
  if ($script:Verbose -or $script:NoExec) {
    Put-Line $txtLine
  }
  if (!$script:NoExec) {
    $sb = [scriptblock]::Create($cmdLine)
    & $sb
  }
}

# Updated version using PS version 1 types only.
# Note: Does not support named parameters or switches.
Function Exec {
  $cmd = Pop-Arg
  $txtLine = "$cmd"
  $cmdLine = "$cmd"
  $a = @($args) # Make a copy, and make sure it's an array.
  $n = 0
  foreach ($arg in $args) {
    $txtLine += " $(Quote $arg)"
    $cmdLine += " `$a[$n]"
    $n++
  }
  if ($script:Verbose -or $script:NoExec) {
    Put-Line $txtLine
  }
  if (!$script:NoExec) {
    if (!$script:Verbose) {Put-Line $txtLine}
    Invoke-Expression $cmdLine
  }
}

# Updated updated version using the PS version 2 splatting operator. Shorter.
# Note: Does not support named parameters or switches.
Function Exec {
  $cmd = Pop-Arg
  $txtLine = "$cmd"
  $a = @($args) # Make a copy, and make sure it's an array.
  $cmdLine = "$cmd @a" # Use the splatting operator to pass arguments through.
  foreach ($arg in $args) {
    $txtLine += " $(Quote $arg)"
  }
  if ($script:Verbose -or $script:NoExec) {
    Put-Line $txtLine
  }
  if (!$script:NoExec) {
    if (!$script:Verbose) {Put-Line $txtLine}
    Invoke-Expression $cmdLine
  }
}

# Next version adding support for named parameters and switches.
Function Exec {
  $cmd = Pop-Arg
  $txtLine = "$cmd"
  $cmdLine = "$cmd"
  $a = @($args) # Make a copy, and make sure it's an array.
  $n = 0
  foreach ($arg in $args) {
    $txtLine += " $(Quote $arg)"
    if (($arg -ne $null) -and ($arg.GetType().FullName -eq "System.String") -and ($arg -match '^-\w+:?$')) {
      $cmdLine += " $arg" # Let Invoke-Expression decide whether it's a param name or string.
    } else {
      $cmdLine += " `$a[$n]" # Preserve the type through Invoke-Expression below.
    }
    $n += 1
  }
  if ($script:Verbose -or $script:NoExec) {
    Put-Line $txtLine
  }
  if (!$script:NoExec) {
    if (!$script:Verbose) {Put-Line $txtLine}
    Invoke-Expression $cmdLine
  }
}

# Another version getting rid of PopArg, and thus of the need to recast $a as an array.
Function Exec([string]$cmd) {
  $txtLine = "$cmd"
  $cmdLine = "$cmd"
  $a = $args # Make a copy (Necessary because Invoke-Expression overwrites $args)
  $n = 0
  foreach ($arg in $args) { # Parse all optional arguments following the command name.
    $txtLine += " $(Quote $arg)"
    if (($arg -ne $null) -and ($arg.GetType().FullName -eq "System.String") -and ($arg -match '^-\w+:?$')) {
      $cmdLine += " $arg" # Let Invoke-Expression decide whether it's a param name or an actual string.
    } else {
      $cmdLine += " `$a[$n]" # Preserve the type through Invoke-Expression.
    }
    $n += 1
  }
  if ($script:Verbose -or $script:NoExec) {
    Put-Line $txtLine
  }
  if (!$script:NoExec) {
    if (!$script:Verbose) {Put-Line $txtLine}
    Invoke-Expression $cmdLine
  }
}

} # End of repository of older Exec() implementations

###############################################################################
#                                                                             #
#                         Output to standard streams                          #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Put-String                                                #
#                                                                             #
#   Description     Output a string, as-is.                                   #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-05-26 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Put-String($string) {
  Write-Host -NoNewLine "$string"
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Put-Line                                                  #
#                                                                             #
#   Description     Output a string, and append a new-line.                   #
#                                                                             #
#   Notes           Write-Host appends a line-feed. Prepend a carrier-return  #
#                   to make sure the output contains both a CR and LF.        #
#                                                                             #
#   History                                                                   #
#    2010-05-26 JFL Created this routine.                                     #
#    2015-08-21 JFL Use the environment-specific new-line.                    #
#                                                                             #
#-----------------------------------------------------------------------------#

$crlf = [Environment]::NewLine	# "`r`n" in Windows

Function Put-Line($string) {
  Put-String "$string$crlf"
}

Function Put-Error($string) {
  Write-Error "$string`r"
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-StdOut                                              #
#                                                                             #
#   Description     Output lines of text to stdout                            #
#                                                                             #
#   Notes           Accept input either from the input pipe or as arguments.  #
#                                                                             #
#   History                                                                   #
#    2013-10-02 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Write-StdOut {
  Begin {
    $outFunc = if ($Host.Name -eq 'ConsoleHost') { 
      [Console]::Out.WriteLine
    } else {
      $host.ui.WriteLine
    }
  }
  Process {
    $object = if ($args.length) {
      $args
    } else {
      $_
    }
    $text = $object | Out-String -Width 4096 # Convert objects to friendly strings, like Write-Out would
    $text = $text -replace "`r?`n$","" # Remove the final new line added by Out-String
    [void]$outFunc.Invoke($text)
  }
}

Set-Alias echo1 Write-StdOut

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Write-StdErr                                              #
#                                                                             #
#   Description     Output lines of text to stderr                            #
#                                                                             #
#   Notes           Accept input either from the input pipe or as arguments.  #
#                                                                             #
#   History                                                                   #
#    2013-10-01 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Write-StdErr {
  Begin {
    $outFunc = if ($Host.Name -eq 'ConsoleHost') { 
      [Console]::Error.WriteLine
    } else {
      $host.ui.WriteErrorLine
    }
  }
  Process {
    $object = if ($args.length) {
      $args
    } else {
      $_
    }
    $text = $object | Out-String -Width 4096 # Convert objects to friendly strings, like Write-Out would
    $text = $text -replace "`r?`n$","" # Remove the final new line added by Out-String
    [void]$outFunc.Invoke($text)
  }
}

Set-Alias echo2 Write-StdErr

###############################################################################
#                                                                             #
#                                 PowerShell                                  #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Test-PSVersion                                            #
#                                                                             #
#   Description     Check your local PowerShell version                       #
#                                                                             #
#   Notes           Simple version that displays a one-line status.           #
#                   Allows diagnosing issues due to beta versions (aka CTP)   #
#                   still available on Microsoft web site, but not clearly    #
#                   documented as betas.                                      #
#                                                                             #
#   History                                                                   #
#    2010-06-04 JFL Adapted from samples in http://powershell.com/cs/blogs/   #
# tobias/archive/2010/01/24/are-you-using-the-correct-powershell-version.aspx #
#    2015-08-21 JFL Bugfix: Added a missing ;                                 #
#                                                                             #
#-----------------------------------------------------------------------------#

function Test-PSVersion {
  $result = $null
  if (Test-Path variable:psversiontable) {
    $versionPresent = [version]$psversiontable.buildversion
    $versionRequired = [version]'6.0.6002.18111'
    if ($versionPresent -ge $versionRequired) {
      $result = "V2 RTM or later"
    } elseif ($versionPresent.Major -ge 6) {
      $result = "V2 CTP Prerelease - Update to V2 RTM!"
    } else {
      $result = "V1 - Update to V2 RTM!"
    }
  }
  return $result
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Test-PSVersion                                            #
#                                                                             #
#   Description     Test PowerShell version on a set of computers             #
#                                                                             #
#   Notes           Powerful version that displays detailed infos for all set.#
#                   Allows diagnosing issues due to beta versions (aka CTP)   #
#                   still available on Microsoft web site, but not clearly    #
#                   documented as betas.                                      #
#                                                                             #
#                   Example of use:                                           #
#                   "localhost", "pc77", "storage1" | Test-PSVersion          #
#                                                                             #
#   History                                                                   #
#    2010-06-04 JFL Copied from sample in http://powershell.com/cs/blogs/     #
# tobias/archive/2010/01/24/are-you-using-the-correct-powershell-version.aspx #
#                                                                             #
#-----------------------------------------------------------------------------#

function Test-PSVersion {
  [CmdletBinding()]
  param(
    [parameter(Position=0,ValueFromPipeline=$true)]
    [ValidateNotNullOrEmpty()]
    [String[]]
    $ComputerName = @('.'),

    [Parameter()]
    [ValidateNotNull()]
    [System.Management.Automation.Credential()]
    $Credential = [System.Management.Automation.PSCredential]::Empty
  )
  process {
    if (Test-Connection -ComputerName $computername -Count 1 -ErrorAction SilentlyContinue) {
      try {
	$OS = Get-WmiObject -Namespace root\CIMV2 -Class Win32_OperatingSystem `
	-ComputerName $computername -Credential $credential -ErrorAction SilentlyContinue
	if ($OS) {
	  $path = "$($OS.SystemDirectory -replace '\\','\\')\\WindowsPowerShell\\v1.0\\powershell.exe"
	  $OSName = $OS.Name.Split('|')[0]
	  $query = "SELECT Version FROM CIM_DataFile WHERE Name = '$path'"
	  $PSEXE = Get-WmiObject -Query $query -ComputerName $computername -Credential $credential
	  if ($PSEXE.Version) {
	    $buildversion = $PSEXE.Version.Split()[0]
	    $versionPresent = [version]$buildversion
	    $versionRequired = [version]'6.0.6002.18111'
	    if ($versionPresent -ge $versionRequired) {
	      $psversion = "V2 RTM"
	    } elseif ($versionPresent.Major -ge 6) {
	      $psversion = "V2 CTP Prerelease - Update to V2 RTM!"
	    } else {
	      $psversion = "V1"
	    }

	    New-Object PSObject -Property @{
	      ComputerName=$OS.__SERVER;
	      BuildVersion=[version]$buildversion;
	      Version=$psversion;
	      Status=$true;Description='OK';
	      OSName = $OSName
	    }
	  } else {
	    New-Object PSObject -Property @{
	      ComputerName=$computername[0];
	      BuildVersion=[version]$null;
	      Version='n/a';
	      Status=$false;
	      Description='Unable to access OS information via WMI.';
	      OSName = 'n/a'
	    }
	  }
	}
      }
      catch {
	New-Object PSObject -Property @{
	  ComputerName=$computername[0];
	  BuildVersion=[version]$null;
	  Version='n/a';
	  Status=$false;
	  Description=($_.Exception.Message);
	  OSName='n/a'
	}
	continue
      }
    } else {
      New-Object PSObject -Property @{
	ComputerName=$computername[0];
	BuildVersion=[version]$null;
	Version='n/a';
	Status=$false;
	Description='Computer did not respond to ping, skipped.';
	OSName='n/a'
      }
    }
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-PSProfileStatus                                       #
#                                                                             #
#   Description     Finding All PowerShell Profile Scripts                    #
#                                                                             #
#   Notes           Sometimes it can get confusing which startup scripts run  #
#		    when PowerShell starts. There can be plenty, and they may #
#		    be different, depending on whether you use the PowerShell #
#		    console, the ISE, or yet another host.                    #
#		                                                              #
#		    Knowing about your profile scripts can be extremely       #
#		    important, though. They essentially determine the         #
#		    customizations applied to PowerShell.                     #
#		                                                              #
#		    The function Get-PSProfileStatus lists all potential      #
#		    startup scripts for the host (PowerShell environment)     #
#		    you run it in. It also reports which profile scripts      #
#		    are really present.                                       #
#                                                                             #
#   History                                                                   #
#    2012-10-02 JFL Mailing from powershell.com.                              #
#                                                                             #
#-----------------------------------------------------------------------------#

function Get-PSProfileStatus {
  $profile |
    Get-Member -MemberType NoteProperty |
    Select-Object -ExpandProperty Name |
    ForEach-Object {
      $_, (Split-Path $profile.$_ -Leaf), (Split-Path $profile.$_),
			    (Test-Path -Path $profile.$_) -join ',' |
	ConvertFrom-Csv -Header Profile, FileName, FolderName, Present
    }
} 

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        ConvertTo-ScriptBlock                                     #
#                                                                             #
#   Description     Convert a string to a ScriptBlock.                        #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-06-17 JFL Adapted from samples published on                         #
#    http://get-powershell.com/2008/12/11/convert-stringtoscriptblock/        #
#                                                                             #
#-----------------------------------------------------------------------------#

# Version compatible with PowerShell 1.0, but less powerful.
function ConvertTo-ScriptBlock {
  param ([string]$string)
  $scriptblock = $executioncontext.invokecommand.NewScriptBlock($string)
  return $scriptblock
}

# Version compatible with PowerShell 2.0.
function ConvertTo-ScriptBlock {
  param(
    [parameter(ValueFromPipeline=$true,Position=0)]
    [string]
    $string
  )
  $sb = [scriptblock]::Create($string)
  return $sb
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-DynamicParam                                          #
#                                                                             #
#   Description     Dynamically add a parameter to a function                 #
#                                                                             #
#   Notes           * You have to supply Begin/Process/End                    #
#                     (dont need all, but need at least 1)                    #
#                   * It does not populate the variable as you'd expect it to,#
#                     you have to pull it from $PSBoundParameters             #
#                                                                             #
#   History                                                                   #
#    2013-05-30 JR  http://jrich523.wordpress.com/category/wmf-powershellwinrm/
#                                                                             #
#-----------------------------------------------------------------------------#

Function New-DynamicParam {
param(
  [string]$Name,
  [string[]]$Options,				# List of valid values
  [switch]$mandatory,
  [string]$SetName="__AllParameterSets", 
  [int]$Position,
  [switch]$ValueFromPipelineByPropertyName,
  [string]$HelpMessage
)
  # Param attributes   
  $ParamAttr = New-Object System.Management.Automation.ParameterAttribute
  $ParamAttr.ParameterSetName = $SetName
  if ($mandatory) { $ParamAttr.Mandatory = $True }
  if ($Position -ne $null) { $ParamAttr.Position=$Position }
  if ($ValueFromPipelineByPropertyName) { $ParamAttr.ValueFromPipelineByPropertyName = $True }
  if ($HelpMessage) { $ParamAttr.HelpMessage = $HelpMessage }

  # Param validation set
  $ParamOptions = New-Object System.Management.Automation.ValidateSetAttribute -ArgumentList $options

  $AttributeCollection = New-Object 'Collections.ObjectModel.Collection[System.Attribute]'
  $AttributeCollection.Add($ParamAttr)
  $AttributeCollection.Add($ParamOptions)

  $Parameter = New-Object -TypeName System.Management.Automation.RuntimeDefinedParameter `
                          -ArgumentList @($Name, [string], $AttributeCollection)

  $Dictionary = New-Object System.Management.Automation.RuntimeDefinedParameterDictionary
  $Dictionary.Add($Name, $Parameter)
  $Dictionary
}

# Example of use:

function Show-FreeDiskSpace {
  [CmdletBinding()]
  Param()
  DynamicParam {
    New-DynamicParam -name Drive -options @(gwmi win32_volume | % {$_.driveletter} | sort) -Position 0 
  }
  begin {
    $drive = $PSBoundParameters.drive # Have to manually populate
  }
  process {
    $vol = gwmi win32_volume -Filter "driveletter='$drive'"
    "{0:N2}% free on {1}" -f ($vol.Capacity / $vol.FreeSpace),$drive
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        ConvertTo-ISODate                                         #
#                                                                             #
#   Description     Convert a date to the ISO 8601 format                     #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-10-19 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

function ConvertTo-ISODate {
  param(
    [Parameter(Position = 0, ValueFromPipeLine = $true)]
    [DateTime]
    $Date = [DateTime]::Today
  )
  process {
    # output the ISO week date
    return ‘{0:0000}-{1:00}-{2:00}’ -f $Date.Year, $Date.Month, $Date.Day
  }
}

# To parse a date using a specific culture, distinct from the default's InvariantCulture
if ($false) {
  [DateTime]::parse("19/10/2010", [system.globalization.cultureinfo]"fr-fr")
}

# Function Using-Culture. Ex: Using-Culture fr-fr {Get-Date}
# From: http://janel.spaces.live.com/blog/cns!9B5AA3F6FA0088C2!185.entry
function Using-Culture (
[System.Globalization.CultureInfo]$culture = (throw "USAGE: Using-Culture -Culture culture -Script {scriptblock}"),
[ScriptBlock]$script= (throw "USAGE: Using-Culture -Culture culture -Script {scriptblock}"))
{
    $OldCulture = [System.Threading.Thread]::CurrentThread.CurrentCulture
    trap
    {
        [System.Threading.Thread]::CurrentThread.CurrentCulture = $OldCulture
    }
    [System.Threading.Thread]::CurrentThread.CurrentCulture = $culture
    $script.invoke()
    [System.Threading.Thread]::CurrentThread.CurrentCulture = $OldCulture
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-Function                                              #
#                                                                             #
#   Description     PowerShell Function Factory                               #
#                                                                             #
#   Notes           Generate a function that takes parameters and returns     #
#                   an object to hold some data.                              #
#                                                                             #
#                   Example:                                                  #
#   New-Function 'New-Person' 'FirstName LastName Address City State Zip'     #      
#   New-Person John Doe -Zip 10001                                            #
#                                                                             #
#   FirstName : John                                                          #
#   LastName : Doe                                                            #
#   Address :                                                                 #
#   city :                                                                    #
#   State :                                                                   #
#   Zip : 10001                                                               #
#                                                                             #
#   History                                                                   #
#    2009-09-12 DF  Published this routine on his blog:                       #
# http://dougfinke.com/blog/index.php/2009/09/12/powershell-function-factory/ #
#                                                                             #
#-----------------------------------------------------------------------------#

Function New-Function ($name, $properties) {
  $parts = $properties.trim().split(' ')
  $parts |
    %{
      $selectProperties += @"
@{
  Name = '$_'
  Expression = {`$$_}
},
"@
    }            

  # simple way to remove the trailing comma        
  $selectProperties = $selectProperties -Replace ",$", ""            
  
  Invoke-Expression @"
  Function Global:$name {
  param(`$$($parts -join ',$'))
  
  New-Object PSObject |
    select $selectProperties
  }
"@
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Explain-PipelineInputVsParameters                         #
#                                                                             #
#   Description     Explain Pipeline Input vs Parameters in PowerShell        #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2008-04-25     Posted on:                                                #
# http://huddledmasses.org/pipeline-input-vs-parameters-in-powershell-scripts #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Explain-PipelineInputVsParameters() {
  "one","two" | &{
    BEGIN { 
      foreach ($Arg in $Args) {
        Write-Host "Begin Arg: $Arg" -Fore Green
      }
      Write-Host "Begin Pipeline: $_" -Fore Green
    }
    PROCESS {
      foreach ($Arg in $Args) {
        Write-Host "Process Arg: $Arg" -Fore Cyan
      }
      Write-Host "Process Pipeline: $_" -Fore Cyan
    }
    END {
      foreach ($Arg in $Args) {
        Write-Host "End Arg: $Arg" -Fore yellow
      }
      Write-Host "End Pipeline: $_" -Fore Yellow
    }
  } "three","four" "five","six" # these are parameters to our script
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-ContentByLine                                         #
#                                                                             #
#   Description     Read a file line-by-line                                  #
#                                                                             #
#   Notes           Uses very little memory, contrary to PowerShell's native  #
#                   Get-Content, which loads the whole file... causing memory #
#                   issues in case the file is huge.                          #
#                                                                             #
#   History                                                                   #
#    2012-10-16     Posted on:                                                #
#                   http://stackoverflow.com/questions/12905924/redirect-standard-input-to-large-file-in-powershell-memory-consumption
#                                                                             #
#-----------------------------------------------------------------------------#

function Get-ContentByLine {
  param (
    [Parameter(Mandatory=$true,ValueFromPipeline=$true)][PsObject]$InputObject
  )

  begin {
    $line = $null
    $fs = [System.IO.File]::OpenRead($InputObject)
    $reader = New-Object System.IO.StreamReader($fs)
  }

  process {
    $line = $reader.ReadLine()
    while ($line -ne $null) {
        $line
        $line = $reader.ReadLine()
    }
  }

  end {
    $reader.Dispose();
    $fs.Dispose();
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        U			                                      #
#                                                                             #
#   Description     Generate Unicode characters                               #
#                                                                             #
#   Notes           Contrary to other languages like C#, PowerShell does not  #
#                   support escape sequences like `xXX, `uXXXX or `UXXXXXXXX. #
#                   For unicode characters that fit in 16-bits, it is         #
#                   possible to enter $([char]0xXX) or $([char]0xXXXX).       #
#                   But for Unicode characters beyond 2^16, it is necessary   #
#                   to invoke a method of the [char] type. This routine makes #
#                   this simpler and shorter: $(U 0xXX) ... $(U 0xXXXXXXXX).  #
#                                                                             #
#   History                                                                   #
#    2015-04-29 JFL Adapted from samples on the Internet.                     #
#                                                                             #
#-----------------------------------------------------------------------------#

function U ([int]$Code) {
  [char]::ConvertFromUtf32($Code)
}

# This more detailed version is useless, but explains the mechanisms.
function U ([int]$Code) {
  if ((0 -le $Code) -and ($Code -le 0xFFFF)) {
    if (($Code -lt 0xD800) -or ($Code -gt 0xDFFF)) { # Not in UFT16 start words for UTF32 encoding
      return [char]$Code
    }
  }
  if ((0x10000 -le $Code) -and ($Code -le 0x10FFFF)) {
    return [char]::ConvertFromUtf32($Code)
  }
  throw "Invalid character code $Code"
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Compare-Lists                                             #
#                                                                             #
#   Description     Compare two unordered lists                               #
#                                                                             #
#   Notes           Using a sort, so that the comparison scales in O(log(N))  #
#                                                                             #
#                   Mostly useless, as this can be done equivalently by:      #
#                   if (Compare-Lists $List1 $List2) { Do something }         #
#                   if (Compare-Object $List1 $List2) { Do the same }         #
#                                                                             #
#   History                                                                   #
#    2015-06-16 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Compare-Lists ($List1, $List2) {
  if ($List1.Count -ne $List2.Count) {
    return 1 # Lengths differ
  }
  $List1 = $List1 | sort
  $List2 = $List2 | sort
  for ($i=0; $i -lt $List1.Count; $i++) {
    if ($List1[$i] -ne $List2[$i]) {
      return 1 # Contents differ
    }
  }
  return 0 # All elements match
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Format-ListSorted                                         #
#                                                                             #
#   Description     Like Format-List, with properties sorted alphabetically   #
#                                                                             #
#   Notes           Adapted from:                                             #
#                   http://stackoverflow.com/questions/3281999/format-list-sort-properties-by-name
#                                                                             #
#                   GOTCHA: Having tabs after the Write-Host "" instruction   #
#                   changed the output to the name of the first file in cd!   #
#                                                                             #
#                   TO DO: fl's default is actually to list only a few        #
#                   selected (how?) parameters. Here we display them all.     #
#                                                                             #
#                   TO DO: fl actually wraps values in the right column.      #
#                   So computing the value length _was_ useful, but not as    #
#                   initially used.                                           #
#                                                                             #
#   History                                                                   #
#    2010-07-19 GH  Created the Format-SortedList routine.                    #
#    2015-08-11 JFL Bug fix: The value length computing sometimes failed.     #
#                   Removed it as it's useless anyway.                        #
#                   Output blank lines just as fl.                            #
#                   Added the -Property option, often used with fl.           #
#                   Added alias fsl                                           #
#    2015-08-21 JFL Bug fix: Output was failing if value contained {braces}.  #
#                   Renamed Format-ListSorted, as I keep typing the old fl.   #
#    2015-09-11 JFL Avoid error for property-less core objects like [int32].  #
#                   Display their value and no extra spaces in this case.     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Format-ListSorted {
  param (
    [Parameter(Mandatory = $false)]
    [String[]]$Property = "*",
    [Parameter(ValueFromPipeline = $true)]
    [Object]$InputObject,
    [Parameter(Mandatory = $false)]
    [Switch]$Descending
  )

  Begin {
    $nObjects = 0
  }
  process {
    if ($InputObject -eq $null) {return}

    $properties = @($InputObject | Get-Member -Name $Property -MemberType Properties | Sort-Object -Property Name -Descending:$Descending)

    if (   ($properties.count -eq 0) `
        -or (($properties.count -eq 1) -and ($properties[0].Name -eq "Length"))) {
      # This is a basic type without any property
      Write-Host $inputObject
    } else { # This is a structure type with one or more properties
      if ($nObjects -eq 0) {Write-Host ""} # Output ONE new line
      $nObjects += 1

      $longestName = 0
      $properties | ForEach-Object {
	if ($_.Name.Length -gt $longestName) {
	  $longestName = $_.Name.Length
	}
      }

      Write-Host ""                       # Output ONE new line
      $properties | ForEach-Object {
	$Name = $_.Name
	Write-Host ("{0,$longestName} : {1}" -f $Name, $InputObject.$Name)
      }
    }
  }
  End {
    if ($nObjects) {Write-Host ([Environment]::NewLine)} # Output TWO new lines
  }
}

Set-Alias fls Format-ListSorted

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Start-PSThread		                              #
#                                                                             #
#   Description     Start a new PowerShell thread                             #
#                                                                             #
#   Arguments       See the Param() block                                     #
#                                                                             #
#   Notes           Returns a thread description object.                      #
#                   The completion can be tested in $_.Handle.IsCompleted     #
#                   Alternative: Use a thread completion event.               #
#                                                                             #
#   References                                                                #
#    https://learn-powershell.net/tag/runspace/                               #
#    https://learn-powershell.net/2013/04/19/sharing-variables-and-live-objects-between-powershell-runspaces/
#    http://www.codeproject.com/Tips/895840/Multi-Threaded-PowerShell-Cookbook
#                                                                             #
#   History                                                                   #
#    2016-06-08 JFL Created this function                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

$PSThreadCount = 0		# Counter of PSThread IDs generated so far
$PSThreadList = @{}		# Existing PSThreads indexed by Id

Function Get-PSThread () {
  Param(
    [Parameter(Mandatory=$false, ValueFromPipeline=$true, Position=0)]
    [int[]]$Id = $PSThreadList.Keys	# List of thread IDs
  )
  $Id | % { $PSThreadList.$_ }
}

Function Start-PSThread () {
  Param(
    [Parameter(Mandatory=$true, Position=0)]
    [ScriptBlock]$ScriptBlock,		# The script block to run in a new thread
    [Parameter(Mandatory=$false)]
    [String]$Name = "",			# Optional thread name. Default: "PSThread$Id"
    [Parameter(Mandatory=$false)]
    [String]$Event = "",		# Optional thread completion event name. Default: None
    [Parameter(Mandatory=$false)]
    [Hashtable]$Variables = @{},	# Optional variables to copy into the script context.
    [Parameter(Mandatory=$false)]
    [String[]]$Functions = @(),		# Optional functions to copy into the script context.
    [Parameter(Mandatory=$false)]
    [Object[]]$Arguments = @()		# Optional arguments to pass to the script.
  )

  $Id = $script:PSThreadCount
  $script:PSThreadCount += 1
  if (!$Name.Length) {
    $Name = "PSThread$Id"
  }
  $InitialSessionState = [System.Management.Automation.Runspaces.InitialSessionState]::CreateDefault()
  foreach ($VarName in $Variables.Keys) { # Copy the specified variables into the script initial context
    $value = $Variables.$VarName
    Write-Debug "Adding variable $VarName=[$($Value.GetType())]$Value"
    $var = New-Object System.Management.Automation.Runspaces.SessionStateVariableEntry($VarName, $value, "")
    $InitialSessionState.Variables.Add($var)
  }
  foreach ($FuncName in $Functions) { # Copy the specified functions into the script initial context
    $Body = Get-Content function:$FuncName
    Write-Debug "Adding function $FuncName () {$Body}"
    $func = New-Object System.Management.Automation.Runspaces.SessionStateFunctionEntry($FuncName, $Body)
    $InitialSessionState.Commands.Add($func)
  }
  $RunSpace = [RunspaceFactory]::CreateRunspace($InitialSessionState)
  $RunSpace.Open()
  $PSPipeline = [powershell]::Create()
  $PSPipeline.Runspace = $RunSpace
  $PSPipeline.AddScript($ScriptBlock) | Out-Null
  $Arguments | % {
    Write-Debug "Adding argument [$($_.GetType())]'$_'"
    $PSPipeline.AddArgument($_) | Out-Null
  }
  $Handle = $PSPipeline.BeginInvoke() # Start executing the script
  if ($Event.Length) { # Do this after BeginInvoke(), to avoid getting the start event.
    Register-ObjectEvent $PSPipeline -EventName InvocationStateChanged -SourceIdentifier $Name -MessageData $Event
  }
  $PSThread = New-Object PSObject -Property @{
    Id = $Id
    Name = $Name
    Event = $Event
    RunSpace = $RunSpace
    PSPipeline = $PSPipeline
    Handle = $Handle
  }	# Return the thread description variables
  $script:PSThreadList[$Id] = $PSThread
  $PSThread
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Receive-PSThread		                              #
#                                                                             #
#   Description     Get the result of a thread, and optionally clean it up    #
#                                                                             #
#   Arguments       See the Param() block                                     #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2016-06-08 JFL Created this function                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Receive-PSThread () {
  [CmdletBinding()]
  Param(
    [Parameter(Mandatory=$false, ValueFromPipeline=$true, Position=0)]
    [PSObject]$PSThread,		# Thread descriptor object
    [Parameter(Mandatory=$false)]
    [Switch]$AutoRemove			# If $True, remove the PSThread object
  )
  Process {
    if ($PSThread.Event -and $AutoRemove) {
      Unregister-Event -SourceIdentifier $PSThread.Name
      Get-Event -SourceIdentifier $PSThread.Name | Remove-Event # Flush remaining events
    }
    try {
      $PSThread.PSPipeline.EndInvoke($PSThread.Handle) # Output the thread pipeline output
    } catch {
      $_ # Output the thread pipeline error
    }
    if ($AutoRemove) {
      $PSThread.RunSpace.Close()
      $PSThread.PSPipeline.Dispose()
      $PSThreadList.Remove($PSThread.Id)
    }
  }
}

Function Remove-PSThread () {
  [CmdletBinding()]
  Param(
    [Parameter(Mandatory=$false, ValueFromPipeline=$true, Position=0)]
    [PSObject]$PSThread			# Thread descriptor object
  )
  Process {
    $_ | Receive-PSThread -AutoRemove | Out-Null
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Using			                              #
#                                                                             #
#   Description     A Using mechanism for PowerShell analog to that of C#     #
#                                                                             #
#   Notes           Caution:                                                  #
#		    PowerShell v5 has its own 'using namespace' keywords      #
#                                                                             #
#                   Example:                                                  #
#		    using System.Windows.Forms                                #
#		    using FooCompany.Bar.Qux.Assembly.With.Ridiculous.Long.Namespace.I.Really.Mean.It
#		    $a = new button                                           #
#		    $b = new Thingamabob                                      #
#                                                                             #
#   History                                                                   #
#    2016-06-09 JFL Adapted from http://stackoverflow.com/questions/1048954/equivalent-to-cs-using-keyword-in-powershell
#                                                                             #
#-----------------------------------------------------------------------------#

$fullnames = New-Object ( [System.Collections.Generic.List``1].MakeGenericType( [String]) );

function using ($name) { 
  foreach ($type in [Reflection.Assembly]::LoadWithPartialName($name).GetTypes()) {
    $fullnames.Add($type.fullname);
  }
}

function new ($name) {
  $fullname = $fullnames -like "*.$name";
  return , (New-Object $fullname[0]);
}

###############################################################################
#                                                                             #
#                                 Named Pipes                                 #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Send-PipeMessage                                          #
#                                                                             #
#   Description     Send a message to a named pipe                            #
#                                                                             #
#   Arguments       See the Param() block                                     #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2016-05-25 JFL Created this function                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Send-PipeMessage () {
  Param(
    [Parameter(Mandatory=$true)]
    [String]$PipeName,          # Named pipe name
    [Parameter(Mandatory=$true)]
    [String]$Message            # Message string
  )
  $PipeDir  = [System.IO.Pipes.PipeDirection]::Out
  $PipeOpt  = [System.IO.Pipes.PipeOptions]::Asynchronous

  $pipe = $null # Named pipe stream
  $sw = $null   # Stream Writer
  try {
    $pipe = new-object System.IO.Pipes.NamedPipeClientStream(".", $PipeName, $PipeDir, $PipeOpt)
    $sw = new-object System.IO.StreamWriter($pipe)
    $pipe.Connect(1000)
    if (!$pipe.IsConnected) {
      throw "Failed to connect client to pipe $pipeName"
    }
    $sw.AutoFlush = $true
    $sw.WriteLine($Message)
  } catch {
    Log "Error sending pipe $pipeName message: $_"
  } finally {
    if ($sw) {
      $sw.Dispose() # Release resources
      $sw = $null   # Force the PowerShell garbage collector to delete the .net object
    }
    if ($pipe) {
      $pipe.Dispose() # Release resources
      $pipe = $null   # Force the PowerShell garbage collector to delete the .net object
    }
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Receive-PipeMessage                                       #
#                                                                             #
#   Description     Wait for a message from a named pipe                      #
#                                                                             #
#   Arguments       See the Param() block                                     #
#                                                                             #
#   Notes           I tried keeping the pipe open between client connections, #
#                   but for some reason everytime the client closes his end   #
#                   of the pipe, this closes the server end as well.          #
#                   Any solution on how to fix this would make the code       #
#                   more efficient.                                           #
#                                                                             #
#   History                                                                   #
#    2016-05-25 JFL Created this function                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Receive-PipeMessage () {
  Param(
    [Parameter(Mandatory=$true)]
    [String]$PipeName           # Named pipe name
  )
  $PipeDir  = [System.IO.Pipes.PipeDirection]::In
  $PipeOpt  = [System.IO.Pipes.PipeOptions]::Asynchronous
  $PipeMode = [System.IO.Pipes.PipeTransmissionMode]::Message

  try {
    $pipe = $null       # Named pipe stream
    $pipe = New-Object system.IO.Pipes.NamedPipeServerStream($PipeName, $PipeDir, 1, $PipeMode, $PipeOpt)
    $sr = $null         # Stream Reader
    $sr = new-object System.IO.StreamReader($pipe)
    $pipe.WaitForConnection()
    $Message = $sr.Readline()
    $Message
  } catch {
    Log "Error receiving pipe message: $_"
  } finally {
    if ($sr) {
      $sr.Dispose() # Release resources
      $sr = $null   # Force the PowerShell garbage collector to delete the .net object
    }
    if ($pipe) {
      $pipe.Dispose() # Release resources
      $pipe = $null   # Force the PowerShell garbage collector to delete the .net object
    }
  }
}

###############################################################################
#                                                                             #
#                                 Networking                                  #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WmiPhysicalNetworkAdapters                            #
#                                                                             #
#   Description     Get the list of physical network adapters                 #
#                                                                             #
#   Notes           Real physical adapters have PnP IDs like PCI\* or USB\*.  #
#                   Virtual Physical adapters have PnP IDs like ROOT\*.       #
#                   (The latter devices created by VMWare or VirtualBox.)     #
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WmiPhysicalNetworkAdapters {
  Try { # First query, that works in Windows 2008
    # Note that without the "-ErrorAction Stop" arguments, the command fails, and Try/Catch does not even catch the error!
    $adapters = Get-WmiObject -query 'select * from Win32_NetworkAdapter where PhysicalAdapter=True and not PNPDeviceID like "ROOT\\%"' -ErrorAction Stop
  } Catch { # Alternative method that works in Windows 2003
    $adapters = Get-WmiObject -query 'select * from Win32_NetworkAdapter where NetConnectionID!="" and not PNPDeviceID like "ROOT\\%"'
  }
  return $adapters
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WmiVirtualNetworkAdapters                             #
#                                                                             #
#   Description     Get the list of virtual physical network adapters         #
#                                                                             #
#   Notes           Real physical adapters have PnP IDs like PCI\* or USB\*.  #
#                   Virtual Physical adapters have PnP IDs like ROOT\*.       #
#                   (The latter devices created by VMWare or VirtualBox.)     #
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WmiVirtualNetworkAdapters {
  Get-WmiObject -query 'select * from Win32_NetworkAdapter where PhysicalAdapter=True and PNPDeviceID like "ROOT\\%"'
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WmiLogicalNetworkAdapters                             #
#                                                                             #
#   Description     Get the list of logical network adapters                  #
#                                                                             #
#   Notes           These are tunnel adapters, etc.                           #
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WmiLogicalNetworkAdapters {
  Get-WmiObject -query 'select * from Win32_NetworkAdapter where PhysicalAdapter!=True'
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WmiPhysicalMacAddresses                               #
#                                                                             #
#   Description     Get the list of physical network adapters MAC addresses   #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WmiPhysicalMacAddresses {
  Get-WmiPhysicalNetworkAdapters | select MACAddress
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        MacAddress2Guid                                           #
#                                                                             #
#   Description     Convert a MAC addresses to a netbootGUID                  #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function MacAddress2Guid {
  $mac = $args[0]
  $mac = $mac.Replace(":", "")
  $mac = $mac.Replace("-", "")
  "00000000-0000-0000-0000-" + $mac
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Set-IPAddress                                             #
#                                                                             #
#   Description     Set a network adapter IP address                          #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-05-03 JFL Adapted from Andy Schneider's http://poshcode.org/1806.   #
#    2016-02-04 JFL Added Win8 versions from http://www.adminarsenal.com/admin-arsenal-blog/using-powershell-to-set-static-and-dhcp-ip-addresses-part-1/
#                                                                             #
#-----------------------------------------------------------------------------#

# Old version, compatible with Windows 7
Function Set-IPAddress {
  param(
    [string]$name,
    [string]$ip,
    [string]$mask,
    [string]$gateway,
    [string]$dns1,
    [string]$dns2,
    [string]$registerDns=$null,
    [switch]$Verbose
  )
  $dns = $dns1
  if ($dns2) {$dns = "$dns1","$dns2"}
  $interface = (Get-WmiObject Win32_NetworkAdapter | where {$_.netconnectionid -eq $name})
  if ($Verbose) {$interface | ft}
  $index = $interface.InterfaceIndex
  if ($Verbose) {Put-Line "Index = $index"}
  $config = Get-WmiObject Win32_NetworkAdapterConfiguration | where {$_.InterfaceIndex -eq $index}
  if ($Verbose) {$config | ft}
  $err = $config.EnableStatic($ip, $mask).ReturnValue
  if ($err -ne 0) {
    Put-Error "Error $err setting interface `"$name`" IP address"
    return $err
  }
  if ($gateway) {
    $err = $config.SetGateways($gateway).ReturnValue
    if ($err -ne 0) {
      Put-Error "Error $err setting interface `"$name`" gateway"
      return $err
    }
  }
  if ($dns) {
    $err = $config.SetDNSServerSearchOrder($dns).ReturnValue
    if ($err -ne 0) {
      Put-Error "Error $err setting interface `"$name`" DNS Servers"
      return $err
    }
  }
  if ($registerDns) {
    $err = $config.SetDynamicDNSRegistration($registerDns).ReturnValue
    if ($err -ne 0) {
      Put-Error "Error $err setting interface `"$name`" dynamic DNS registration"
      return $err
    }
  }
  return 0
}

# New version, for Windows 8+
Function Set-IPAddress {
  param(
    [string]$Name,		# Name, possibly with wildcards. Ex: eth0*
    [string]$IP,
    [int]$MaskBits,		# Ex: 8 <==> mask 255.0.0.0
    [string]$Gateway,
    [string[]]$DnsList
  )
  $IPType = "IPv4"

  # Retrieve the network adapter that you want to configure
  $adapter = Get-NetAdapter $Name

  # Remove any existing IP, gateway from our ipv4 adapter
  if (($adapter | Get-NetIPConfiguration).IPv4Address.IPAddress) {
    $adapter | Remove-NetIPAddress -AddressFamily $IPType -Confirm:$false
  }
  if (($adapter | Get-NetIPConfiguration).Ipv4DefaultGateway) {
    $adapter | Remove-NetRoute -AddressFamily $IPType -Confirm:$false
  }

  # Configure the IP address and default gateway
  $adapter | New-NetIPAddress -AddressFamily $IPType -IPAddress $IP -PrefixLength $MaskBits -DefaultGateway $Gateway | Out-Null

  # Configure the DNS client server IP addresses
  $adapter | Set-DnsClientServerAddress -ServerAddresses $DnsList
}

Function Set-DhcpIPAddress {
  # Retrieve the network adapter that you want to configure
  $adapter = Get-NetAdapter $Name
  $interface = $adapter | Get-NetIPInterface -AddressFamily IPv4

  if ($interface.Dhcp -eq "Disabled") {
    # Remove existing gateway
    If (($interface | Get-NetIPConfiguration).Ipv4DefaultGateway) {
      $interface | Remove-NetRoute -Confirm:$false
    }

    # Enable DHCP
    $interface | Set-NetIPInterface -DHCP Enabled

    # Configure the  DNS Servers automatically
    $interface | Set-DnsClientServerAddress -ResetServerAddresses
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-IPAddress                                             #
#                                                                             #
#   Description     Resolve an IPV4 address from a name                       #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-05-27 JFL Created this routine.                                     #
#    2015-06-08 JFL Rewritten not using nslookup: Use hosts file, then DNS.   #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-IPAddress($name) {
  # First check if it's already an IP address
  if ("$name" -match "^\d+\.\d+\.\d+\.\d+$") { # It's an IPv4 address
    return [System.Net.IPAddress]$name
  }
  if ("$name" -match "^[\dABCDEF]*:[\dABCDEF:]*[\dABCDEF]$") { # It's an IPv6 address
    return [System.Net.IPAddress]$name
  }
  # Then check if there's an entry in the hosts file
  $hosts = "$($env:windir)\System32\Drivers\etc\hosts"
  if (Test-Path $hosts) {
    foreach ($line in (Get-Content $hosts)) {
      $line = ($line -replace "#.*", "").Trim() # Remove comments
      if ($line -match "^(\S+)\s+(\S.*)") { # If there's an IP and a set of names
      	$ip = $matches[1]
      	$names = ($matches[2]).Split()
      	if ($names -contains $name) { # Found it!
      	  return [System.Net.IPAddress]$ip
      	}
      }
    }
  }
  # Finally try using DNS to resolve the name
  Try {
    $adr=[System.Net.DNS]::GetHostEntry($name).AddressList[0]
  } Catch {
    $adr=$null
  }
  return $adr
}

if (0) {
# Old version using the nslookup program
Function Get-IPAddress($name, $dns=$null) {
  if ($dns) {
    $output = nslookup $name $dns 2>&1
  } else {
    $output = nslookup $name 2>&1
  }
  if ($output -match '\*\*\*') {return $null} # Error messages begin with a ***
  $str = [regex]::Replace($output, '.*Address(es)?:\s*', '') # Remove everything up to the last address
  $str = [regex]::Replace($str, '(\S+).*', '$1') # Remove everything after the first remaining address
  return $str
}

# Simpler version using .net
Function Get-IPAddress($name) {
  Try {
    $adr=[system.Net.DNS]::GetHostEntry($name).AddressList[0].IPAddressToString
  } Catch {
    $adr=$null
  }
  return $adr
}
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Test-TCPPort                                              #
#                                                                             #
#   Description     Test if a TCP Port is open or not	                      #
#                                                                             #
#   Notes           Adapted from sample at http://poshcode.org/6280           #
#                   Author: Christophe Cremon                                 #
#                                                                             #
#   History                                                                   #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Test-TCPPort {
  param (
    [ValidateNotNullOrEmpty()]
    [string] $EndPoint = $(throw "Please specify an EndPoint (Host or IP Address)"),
    [string] $Port = $(throw "Please specify a Port")
  )

  $TimeOut = 1000
  $IP = [System.Net.Dns]::GetHostAddresses($EndPoint)
  $Address = [System.Net.IPAddress]::Parse($IP)
  $Socket = New-Object System.Net.Sockets.TCPClient
  $Connect = $Socket.BeginConnect($Address,$Port,$null,$null)
  if ($Connect.IsCompleted) {
    $Wait = $Connect.AsyncWaitHandle.WaitOne($TimeOut,$false)                      
    if (!$Wait) {
      $Socket.Close()
      return $false
    } else {
      $Socket.EndConnect($Connect)
      $Socket.Close()
      return $true
    }
  } else {
    return $false
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Send-UdpPacket                                            #
#                                                                             #
#   Description     Send one UDP packet to another system(s)                  #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-08 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Define short aliases for .NET sockets options
$afIPv4 = [System.Net.Sockets.AddressFamily]::InterNetwork
$afIPv6 = [System.Net.Sockets.AddressFamily]::InterNetworkV6
$stStream = [System.Net.Sockets.SocketType]::Stream	# Reliable, two-way, connection-based byte streams
$stDgram = [System.Net.Sockets.SocketType]::Dgram	# Connectionless datagrams
$ptTCP = [System.Net.Sockets.ProtocolType]::TCP
$ptUDP = [System.Net.Sockets.ProtocolType]::UDP
$olSocket = [System.Net.Sockets.SocketOptionLevel]::Socket # IP|IPv6|UDP|Tcp|Socket
$soReuseAddress = [System.Net.Sockets.SocketOptionName]::ReuseAddress
$soDontLinger = [System.Net.Sockets.SocketOptionName]::DontLinger
$soSendTimeout = [System.Net.Sockets.SocketOptionName]::SendTimeout
# Other short .NET object options aliases
$ipAny = [System.Net.IPAddress]::Any
$ascii = [System.Text.Encoding]::ASCII	# ASCII or UTF8

Function Send-UdpPacket(
  # First the packet, either specified as a string or as binary data
  [string]$String = "",			# String to send
  [byte[]]$bData = @(),			# Binary data to send
  # Then the target server(s), either specified as IP Endpoint(s), or Name(s)/Port, or IP Address/Port
  [System.Net.IPEndPoint[]]$EPs = @(),	# End point(s) of the server(s) to which a connection is attempted
  [string[]]$Names = @(),		# Host name(s) of the server(s) to which a connection is attempted
  [System.Net.IPAddress[]]$IPs = @(),	# IP address(es) of the server(s) to which a connection is attempted
  # The port needs only be specified for name(s) or IP Address(es)
  [int]$Port = 0			# Port number of the socket.
  ) {
  # Write-DebugVars String bData EPs Names IPs Port
  # First check if exclusive options are OK
  # Check the packet definition
  if (($string -ne "") -and ($bData -ne @())) {
    throw "Must use either `$string or `$bData, but not both."
  }
  # Check the target host(s) definition
  $nOpt = 0
  if ($EPs.Count)	{ $nOpt += 1 ; $needPort = $false }
  if ($Names.Count)	{ $nOpt += 1 ; $needPort = $true }
  if ($IPs.Count)	{ $nOpt += 1 ; $needPort = $true }
  if ($nOpt -ne 1) {
    throw "Must use one, and only one, of `$EPs, `$IPs, `$Names."
  }
  # Check the port definition
  if (($port -ne 0) -and ($needPort -eq $false)) {
    throw "Specify a `$Port only for `$Names or `$IPs."
  }

  # Convert the string to binary data if needed  
  if ($String -ne "") {
    $bData = [System.Text.Encoding]::ASCII.GetBytes($String)
  }
  # Convert the names to IP addresses if needed
  if ($Names -ne @()) {
    foreach ($Name in $Names) {
      $IPs += Get-IPAddress($Name)
    }
  }
  # Convert the IP addresses and port to IP Endpoints if needed
  if ($IPs -ne @()) {
    foreach ($IP in $IPs) {
      $EPs += New-Object System.Net.IPEndPoint $IP, $Port
    }
  }

  # Send the packets
  try {
    $sock = New-Object System.Net.Sockets.Socket $afIPv4, $stDgram, $ptUDP
    foreach ($EP in $EPs) {
      $sock.Connect($EP)
      $nSent = $sock.Send($bData)
      # $sock.Disconnect($true) # $True = we'll reuse that socket
    }
  } catch {
    $msg = $_.Exception.Message
    Write-error "Failed to send packet to $($EP.Address):$($EP.Port). $msg"
    return $null
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Dhcp management functions                                 #
#                                                                             #
#   Description     Get information about/from DHCP servers                   #
#                                                                             #
#   Notes           WARNING: Experimental code, not complete yet!             #
#                                                                             #
#   History                                                                   #
#    2012-03-06 JFL Created this routine based on http://poshcode.org/2310    #
#                                                                             #
#-----------------------------------------------------------------------------#

# DHCP Server Management API Functions:
# http://msdn.microsoft.com/en-us/library/windows/desktop/aa363379(v=vs.85).aspx

# WARNING: I started adding support for DhcpEnumServers(), but this is not complete yet!!!!
if ($false) {
Add-Type @"
  using System;
  using System.Collections;
  using System.Net;
  using System.Runtime.InteropServices;
  using Dhcp;

  namespace Dhcp {
    public class ConvertTo {

      public static IPAddress IP(UInt32 Value) {
        Byte[] IPArray = new Byte[4];

        for (int i = 3; i > -1; i--) {
          Double Remainder = Value % Math.Pow(256, i);
          IPArray[3 - i] = (Byte)((Value - Remainder) / Math.Pow(256, i));
          Value = (UInt32)Remainder;
        }

        return IPAddress.Parse(String.Format("{0}.{1}.{2}.{3}",
          IPArray[0],
          IPArray[1],
          IPArray[2],
          IPArray[3]));
      }

      public static UInt32 UInt32(IPAddress IP) {
        UInt32 Value = 0;
        Byte[] Bytes = IP.GetAddressBytes();
        for (int i = 0; i < 4; i++) {
          Value = Value | (UInt32)(Bytes[i] << (8 * (3 - i)));
        }
        return Value;
      }
    }

    public class Functions {

      [DllImport("dhcpsapi.dll")]
      public static extern UInt32 DhcpEnumSubnetClients(
        [MarshalAs(UnmanagedType.LPWStr)]
        String ServerIpAddress,
        uint SubnetAddress,
        ref uint ResumeHandle,
        uint PreferredMaximum,
        out IntPtr ClientInfo,
        out uint ClientsRead,
        out uint ClientsTotal
      );

      [DllImport("dhcpsapi.dll")]
      public static extern UInt32 DhcpEnumServers(
	UInt32 Flags,
	VoidPtr IdInfo,
	__out  LPDHCP_SERVER_INFO_ARRAY *Servers,
	VoidPtr CallbackFn,
	VoidPtr CallbackData
      );
    }

    public class Structures {

      [StructLayout(LayoutKind.Sequential)]
      public struct DATE_TIME {
        public UInt32 dwLowDateTime;
        public UInt32 dwHighDateTime;

        public DateTime ToDateTime() {
          if (dwHighDateTime == 0 && dwLowDateTime == 0) {
            return DateTime.MinValue;
          }
          if (dwHighDateTime == int.MaxValue && dwLowDateTime == UInt32.MaxValue) {
            return DateTime.MaxValue;
          }
          return DateTime.FromFileTime((((long)dwHighDateTime) << 32) | dwLowDateTime);
        }
      }

      [StructLayout(LayoutKind.Sequential)]
      public struct DHCP_BINARY_DATA {
        public uint DataLength;
        public IntPtr Data;

        public override String ToString() {
          return String.Format("{0:X2}:{1:X2}:{2:X2}:{3:X2}:{4:X2}:{5:X2}",
            Marshal.ReadByte(this.Data),
            Marshal.ReadByte(this.Data, 1),
            Marshal.ReadByte(this.Data, 2),
            Marshal.ReadByte(this.Data, 3),
            Marshal.ReadByte(this.Data, 4),
            Marshal.ReadByte(this.Data, 5));
        }
      };

      [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
      public struct DHCP_CLIENT_INFO_ARRAY {
        public uint NumElements;
        public IntPtr Clients;
      }

      [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
      public struct DHCP_CLIENT_INFO {
        public uint ClientIpAddress;
        public uint SubnetMask;
        public DHCP_BINARY_DATA ClientHardwareAddress;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string ClientName;
        [MarshalAs(UnmanagedType.LPWStr)]
        public string ClientComment;
        public DATE_TIME ClientLeaseExpires;
        public DHCP_HOST_INFO OwnerHost;
      }

      [StructLayout(LayoutKind.Sequential)]
      public struct DHCP_HOST_INFO {
        uint IpAddress;
        [MarshalAs(UnmanagedType.LPWStr)]
        String NetBiosName;
        [MarshalAs(UnmanagedType.LPWStr)]
        String HostName;
      }

      [StructLayout(LayoutKind.Sequential)]
      public struct DHCPDS_SERVER {
	UInt32  Version;
	[MarshalAs(UnmanagedType.LPWStr)]
        String  ServerName;
	UInt32  ServerAddress;
	UInt32  Flags;
	UInt32  State;
	[MarshalAs(UnmanagedType.LPWStr)]
        String  DsLocation;
	UInt32  DsLocType;
	}

      [StructLayout(LayoutKind.Sequential)]
      public struct DHCPDS_SERVERS {
        UInt32 Flags;
        UInt32 NumElements;
        LPDHCPDS_SERVER Servers;
      }
      
    }

    public class DhcpClient {
      public String Name;
      public String MACAddress;
      public IPAddress IpAddress;
      public IPAddress SubnetMask;
      public String Description;
      public DateTime LeaseExpires;

      internal DhcpClient(Structures.DHCP_CLIENT_INFO RawReservation) {
        this.IpAddress = ConvertTo.IP(RawReservation.ClientIpAddress);
        this.SubnetMask = ConvertTo.IP(RawReservation.SubnetMask);
        this.MACAddress = RawReservation.ClientHardwareAddress.ToString();
        this.Name = RawReservation.ClientName;
        this.Description = RawReservation.ClientComment;
        this.LeaseExpires = RawReservation.ClientLeaseExpires.ToDateTime();
      }

      public static DhcpClient[] Get(IPAddress ServerIP, IPAddress ScopeIP) {
        ArrayList Clients = new ArrayList();
        uint resumeHandle = 0;
        IntPtr info_array_ptr;
        uint numClientsRead = 0;
        uint totalClients = 0;

        String Server = ServerIP.ToString();
        UInt32 Scope = ConvertTo.UInt32(ScopeIP);

        UInt32 ReturnCode = Functions.DhcpEnumSubnetClients(
          Server,
          Scope,
          ref resumeHandle,
          65536,
          out info_array_ptr,
          out numClientsRead,
          out totalClients
        );

        Structures.DHCP_CLIENT_INFO_ARRAY rawClients =
          (Structures.DHCP_CLIENT_INFO_ARRAY)Marshal.PtrToStructure(info_array_ptr, typeof(Structures.DHCP_CLIENT_INFO_ARRAY));
        IntPtr current = rawClients.Clients;

        for (int i = 0; i < (int)rawClients.NumElements; i++) {
          Structures.DHCP_CLIENT_INFO rawMachine =
            (Structures.DHCP_CLIENT_INFO)Marshal.PtrToStructure(Marshal.ReadIntPtr(current), typeof(Structures.DHCP_CLIENT_INFO));

          Clients.Add(new DhcpClient(rawMachine));

          current = (IntPtr)((int)current + (int)Marshal.SizeOf(typeof(IntPtr)));
        }

        return (DhcpClient[])Clients.ToArray(typeof(DhcpClient));
      }
    }
  }
"@
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Send-Mail                                                 #
#                                                                             #
#   Description     Send an email                                             #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-07-09 JFL Created this routine                                      #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Send-Mail {
  Param(
    [String]$smtpServer,	# "<your mailhost>.<yourdomain>.com" 
    [String]$From,		# "noreply@<yourdomain>.com"
    [String]$To,		# 
    [String]$Subject,		# Subject line
    [String]$Message		# The message body
  )
  $smtp = new-object Net.Mail.SmtpClient($smtpServer) 
  $smtp.Send($From, $To, $Subject, $Message) 
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-ClusterProc                                           #
#                                                                             #
#   Description     Create functions generating lists of cluster nodes names  #
#                                                                             #
#   Notes           Ex: New-ClusterProc atlas                                 #
#                   Creates a new function called atlas. Usage:               #
#                   atlas index_list [suffix]                                 #
#                   Ex: atlas(1..3)-adm                                       #
#                   Generates the list: "atlas1-adm","atlas2-adm","atlas3-adm"#
#                                                                             #
#                   Do not use a Param() block in the generated functions,    #
#		    else suffixes beginning with a - don't work.              #
#                                                                             #
#   History                                                                   #
#    2015       JFL Created this routine                                      #
#    2016-02-08 JFL Fixed a regression that prevented using suffixes w. a dash#
#                                                                             #
#-----------------------------------------------------------------------------#

# Function factory, creating functions that generate lists of cluster nodes names
# Do not use a Param() block in the generated functions, else suffixes beginning with a - don't work.
Function New-ClusterProc($name) {
  $definition = "Function $name {`$list,`$suffix=`$args; `$list | % {""$name`$_`$Suffix""}}"
  Invoke-Expression ($definition -replace "^Function ", "Function Global:")
  Write-Verbose $definition
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-FCHba                                                 #
#                                                                             #
#   Description     Get Fibre-Channel HBAs and information about them         #
#                                                                             #
#   Notes           Based on sample in                                        #
#                   https://gallery.technet.microsoft.com/scriptcenter/Find-HBA-and-WWPN-53121140
#                                                                             #
#   History                                                                   #
#    2016-03-30 JFL Renamed, simplified, and reindented.                      #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-FCHba {
  param(
    [String[]]$ComputerName = $ENV:ComputerName
  )

  $ComputerName | ForEach-Object {
    try {
      $Computer = $_
      $Params = @{
	Namespace    = 'root\WMI'
	Class        = 'MSFC_FCAdapterHBAAttributes'
	ComputerName = $Computer
	ErrorAction  = 'Stop'
      }
      Get-WmiObject @Params | % {
	$InstanceName = $_.InstanceName -replace '\\','\\'
	$Params['class']='MSFC_FibrePortHBAAttributes'
	$Params['filter']="InstanceName='$InstanceName'"
	$Ports = @(Get-WmiObject @Params | Select -ExpandProperty Attributes)
	$WWPNs = @($Ports | % { ($_.PortWWN | % {"{0:x2}" -f $_}) -join ''})
	New-Object PSObject -Property @{
	  ComputerName     = $_.__SERVER
	  NodeWWN          = (($_.NodeWWN) | % {'{0:x2}' -f $_}) -join ''
	  PortWWN          = $WWPNs
	  Active           = $_.Active
	  DriverName       = $_.DriverName
	  DriverVersion    = $_.DriverVersion
	  FirmwareVersion  = $_.FirmwareVersion
	  Model            = $_.Model
	  ModelDescription = $_.ModelDescription
	  UniqueAdapterId  = $_.UniqueAdapterId
	  NumberOfPorts    = $_.NumberOfPorts
	}
      }
    } catch {
      Write-Warning -Message "$Computer is offline or not supported"
    }
  }
}

###############################################################################
#                                                                             #
#                                   System                                    #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-SMBiosStructures                                      #
#                                                                             #
#   Description     Get the system GUID                                       #
#                                                                             #
#   Returns         An array of SMBIOS structures of the requested type.      #
#                                                                             #
#   Notes           Use mssmbios.sys copy of the SMBIOS table in the registry.#
#                   Documented as unreliable, but available in WinXP ... Win7.#
#                                                                             #
#                   Some fields, like the UUID in structure 1, are cleared    #
#                   for "Security Reasons".                                   #
#                                                                             #
#		    An alternative would be to use property SmBiosData in WMI #
#		    class MSSMBios_RawSMBiosTables in WMI name space root\wmi.#
#                                                                             #
#   History                                                                   #
#    2010-06-08 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-SMBiosStructures($Type, [switch]$Verbose) {
  $structs = @()
  $key = "HKLM:\SYSTEM\CurrentControlSet\services\mssmbios\Data"
  if (!(Test-Path $key)) {
    Put-Error "No SMBIOS table copy found in the registry"
    return $null
  }
  $data = Get-RegistryValue $key SMBiosData
  $i = 8 # CIM structures begin at offset 8
  while (($data[$i+1] -ne $null) -and ($data[$i+1] -ne 0)) { # While the structure has non-0 length
    $i0 = $i
    $n = $data[$i]   # Structure type
    $l = $data[$i+1] # Structure length
    if ($Verbose) {Put-Line "Skipping structure $n body"}
    $i += $l # Skip the structure body
    if ($data[$i] -eq 0) {$i++} # If there's no trailing string, skip the extra NUL
    while ($data[$i] -ne 0) { # And skip the trailing strings
      $s = ""
      while ($data[$i] -ne 0) { $s += [char]$data[$i++] }
      if ($Verbose) {Put-Line "Skipping string $s"}
      $i++ # Skip the string terminator NUL
    }
    $i1 = $i
    $i++ # Skip the string list terminator NUL
    if ($n -eq $Type) {
      $structs += ,@($data[$i0..$i1])
    }
  }
  return @($structs)
}

Function Get-SMBiosStructureString($Struct, $Index, [switch]$Verbose) {
  if ($Index -le 0) {
    return $null # Undefined string
  }
  $i = $Struct[1] # Skip the structure body
  if ($Struct[$i] -eq 0) {$i++} # If there's no trailing string, skip the extra NUL
  while ($Struct[$i] -ne 0) { # While there are more strings
    $s = ""
    while ($Struct[$i] -ne 0) { $s += [char]$Struct[$i++] }
    if (--$Index -eq 0) {
      return $s # Found!
    }
    $i++ # Skip the string terminator NUL
  }
  return $null # String not found
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-SystemGuid                                            #
#                                                                             #
#   Description     Get the system GUID                                       #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-06-08 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Recommended method, using WMI
Function Get-SystemGuid {
  (Get-WmiObject Win32_ComputerSystemProduct).UUID
}

# Hack using mssmbios.sys copy of the SMBIOS table in the registry.
# Documented as unreliable, but available in the registry for XP ... W7.
# But the bad news is that the UUID field is cleared for "Security Reasons".
Function Get-mssmbiosSystemGuid([switch]$Verbose) {
  $key = "HKLM:\SYSTEM\CurrentControlSet\services\mssmbios\Data"
  if (!(Test-Path $key)) {
    if ($Verbose) {Put-Line "No SMBIOS table copy found in the registry"}
    return $null
  }
  $data = Get-RegistryValue $key SMBiosData
  $i = 8 # CIM structures begin at offset 8
  # Skip the first structures until we reach a structure of type 1
  while ($data[$i] -ne 1) {
    $n = $data[$i]   # Structure type
    $l = $data[$i+1] # Structure length
    if ($l -eq 0) { break } # End of structures
    if ($Verbose) {Put-Line "Skipping structure $n"}
    $i += $l # Skip the structure body
    while ($data[$i] -ne 0) { # And skip the trailing strings
      $s = ""
      while ($data[$i] -ne 0) { $s += [char]$data[$i++] }
      if ($Verbose) {Put-Line "Skipping string $s"}
      $i++ # Skip the string terminator NUL
    }
    $i++ # Skip the string list terminator NUL
  }
  if ($data[$i] -ne 1) {
    if ($Verbose) {Put-Line "SMBIOS table type 1 not found"}
    return $null
  }
  $i += 8 # The UUID is a 16-bytes field at offset 8 in structure 1.
  $uuid = ""
  # Display order for the first 3 fields is reversed, and the last 2 is direct.
  foreach ($j in 3,2,1,0,-1,5,4,-1,7,6,-1,8,9,-1,10,11,12,13,14,15) {
    if ($j -eq -1) { # Output a field separator
      $uuid += "-"
    } else { # Convert the byte to two hexadecimal characters
      # $uuid += [System.BitConverter]::ToString($data[$i+$j])
      $uuid += $data[$i+$j].ToString("x2")
    }
  }
  $uuid
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        ByteArray2Guid                                            #
#                                                                             #
#   Description     Convert a byte array to a netbootGUID                     #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function ByteArray2Guid {
  [System.Guid]$args[0] # Simply use a cast
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Set-ComputerName                                          #
#                                                                             #
#   Description     Change the current computer name                          #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-05-12 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

function Set-ComputerName {
  param(  [switch]$help,
	  [string]$newName=$(read-host "Please specify the new name of the computer"))
		 
  $usage = "set-ComputerName -computername NewName"
  if ($help) {Write-Host $usage; break}
 
  $computer = Get-WmiObject Win32_ComputerSystem
  $computer.Rename($newName)
}

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
#   Function        Get-LocalAdministrators                                   #
#                                                                             #
#   Description     Get Local Administrators				      #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2012-01-31 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# Simple function for local use
# Because the group may have been renamed, the filter searches for it by well-known SID.
Function Get-LocalAdministrators {
  $AdminsGroup=Get-WmiObject -Class Win32_Group -Filter "SID='S-1-5-32-544' AND LocalAccount='True'" -errorAction "Stop" 
  $AdminsGroup.GetRelated() | Where {$_.__CLASS -match "Win32_UserAccount|Win32_Group"}
}

# Original full-featured function from web site
# http://jdhitsolutions.com/blog/2011/07/get-local-administrators-with-wmi-and-powershell/
Function Get-LocalAdministrators {

[cmdletbinding()]

Param(
[Parameter(Position=0,ValueFromPipeline=$True,ValueFromPipelineByPropertyName=$True)]
[ValidateNotNullorEmpty()]
[string[]]$Computername=$env:computername,
[switch]$AsJob)

Begin {

    Set-StrictMode -Version latest
    Write-Verbose "Starting $($myinvocation.mycommand)"
    
    #define an new array for computernames if this is run as a job
    $computers=@()
}

Process {
    foreach ($computer in $computername) {
     $computers+=$Computer
     $sb={Param([string]$computer=$env:computername)
        Try {
            Write-Verbose "Querying $computer"
            $AdminsGroup=Get-WmiObject -Class Win32_Group -computername $Computer -Filter "SID='S-1-5-32-544' AND LocalAccount='True'" -errorAction "Stop" 
            Write-Verbose "Getting members from $($AdminsGroup.Caption)" 
            
            $AdminsGroup.GetRelated() | Where {$_.__CLASS -match "Win32_UserAccount|Win32_Group"} | 
            Select Name,Fullname,Caption,Description,Domain,SID,LocalAccount,Disabled,
            @{Name="Computer";Expression={$Computer.ToUpper()}}
        }
        Catch {
            Write-Warning "Failed to get administrators group from $computer"
            Write-Error $_
         }
      } #end scriptblock
      if (!$AsJob) {
        Invoke-Command -ScriptBlock $sb -ArgumentList $computer
      }
     } #foreach computer
} #process 

 End {
    #create a job is specified
    if ($AsJob) {
     Write-Verbose "Creating remote job"
     #create a single job targeted against all the computers. This will execute on each
     #computer remotely
     Invoke-Command -ScriptBlock $sb -ComputerName $computers -asJob
    }
 
    Write-Verbose "Ending $($myinvocation.mycommand)"
}
} #end function

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        SendMessageTimeout                                        #
#                                                                             #
#   Description     Invoke a win32 function				      #
#                                                                             #
#   Notes           Useful to broadcast a change message after modifying      #
#		    the global environment.				      #
#                                                                             #
#   History                                                                   #
#    2012-08-20 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

if ($false) {
# Import WIN32 function sendmessagetimeout()
if (-not ("win32.nativemethods" -as [type])) {
    # import sendmessagetimeout from win32
    add-type -Namespace Win32 -Name NativeMethods -MemberDefinition @"
[DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Auto)]
public static extern IntPtr SendMessageTimeout(
    IntPtr hWnd, uint Msg, UIntPtr wParam, string lParam,
    uint fuFlags, uint uTimeout, out UIntPtr lpdwResult);
"@
}
# Define sendmessagetimeout() arguments
$HWND_BROADCAST = [intptr]0xffff;
$WM_SETTINGCHANGE = 0x1a;
$result = [uintptr]::zero
# Notify all windows of the environment string change
[win32.nativemethods]::SendMessageTimeout($HWND_BROADCAST, $WM_SETTINGCHANGE,
	[uintptr]::Zero, "Environment", 2, 5000, [ref]$result);
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Unmount-Drive                                             #
#                                                                             #
#   Description     Unmount a network drive				      #
#                                                                             #
#   Notes           Useful to broadcast a change message after modifying      #
#		    the global environment.				      #
#                                                                             #
#   History                                                                   #
#    2012-08-20 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Unmount-Drive($letter) {
  Add-Type @"
    using System;
    using System.Runtime.InteropServices;
     
    public class MountPoint {
      [DllImport("kernel32.dll", CharSet=CharSet.Auto, SetLastError=true)]
      public static extern bool DeleteVolumeMountPoint(string mountPoint);
    }
"@

  [MountPoint]::DeleteVolumeMountPoint("${letter}:\")
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Set-PinnedApplication                                     #
#                                                                             #
#   Description     Pin and unpin programs from the taskbar & the Start-menu  #
#                                                                             #
#   Notes           This routine is based on the Add-PinnedApplication script #
#                   created by Ragnar Harper and Kristian Svantorp:           #
#                   http://blogs.technet.com/kristian/archive/2009/04/24/nytt-script-pin-to-taskbar.aspx 
#                   http://blog.crayon.no/blogs/ragnar/archive/2009/04/17/pin-applications-to-windows-7-taskbar.aspx 
#                                                                             #
#                   Johan Akerstrom's blog: http://cosmoskey.blogspot.com     #
#                                                                             #
#                   For more information, see the following blog post:        #
#                   http://blog.crayon.no/blogs/janegil/archive/2010/02/26/pin-and-unpin-applications-from-the-taskbar-and-start-menu-using-windows-powershell.aspx 
#                                                                             #
#   History                                                                   #
#    2009-04-17     Initial release by Ragnar Harper and Kristian Svantorp.   #
#    2010-02-26     Jan Egil Ring added the capability to unpin applications. #
#    2010-08-06     Johan Akerstrom added full MUI support.                   #
#                                                                             #
#-----------------------------------------------------------------------------#

function Set-PinnedApplication { 
<#
  .SYNOPSIS
  This function are used to pin and unpin programs from the taskbar and Start-menu in Windows 7 and Windows Server 2008 R2
  
  .DESCRIPTION
  The function have to parameteres which are mandatory:
  Action: PinToTaskbar, PinToStartMenu, UnPinFromTaskbar, UnPinFromStartMenu
  FilePath: The path to the program to perform the action on
  
  .EXAMPLE
  Set-PinnedApplication -Action PinToTaskbar -FilePath "C:\WINDOWS\system32\notepad.exe"
  
  .EXAMPLE
  Set-PinnedApplication -Action UnPinFromTaskbar -FilePath "C:\WINDOWS\system32\notepad.exe"
  
  .EXAMPLE
  Set-PinnedApplication -Action PinToStartMenu -FilePath "C:\WINDOWS\system32\notepad.exe"
  
  .EXAMPLE
  Set-PinnedApplication -Action UnPinFromStartMenu -FilePath "C:\WINDOWS\system32\notepad.exe"
#>
  [CmdletBinding()]
  param(
    [Parameter(Mandatory=$true)][string]$Action,
    [Parameter(Mandatory=$true)][string]$FilePath
  )
  if(-not (test-path $FilePath)) {
    throw "FilePath does not exist."
  }

  function InvokeVerb {
    param([string]$FilePath,$verb)
    $verb = $verb.Replace("&","")
    $path= split-path $FilePath
    $shell=new-object -com "Shell.Application"
    $folder=$shell.Namespace($path)
    $item = $folder.Parsename((split-path $FilePath -leaf))
    $itemVerb = $item.Verbs() | ? {$_.Name.Replace("&","") -eq $verb}
    if($itemVerb -eq $null){
	throw "Verb $verb not found."
    } else {
	$itemVerb.DoIt()
    }
  }

  function GetVerb {
    param([int]$verbId)
    try {
      $t = [type]"CosmosKey.Util.MuiHelper"
    } catch {
      $def = [Text.StringBuilder]""
      [void]$def.AppendLine('[DllImport("user32.dll")]')
      [void]$def.AppendLine('public static extern int LoadString(IntPtr h,uint id, System.Text.StringBuilder sb,int maxBuffer);')
      [void]$def.AppendLine('[DllImport("kernel32.dll")]')
      [void]$def.AppendLine('public static extern IntPtr LoadLibrary(string s);')
      add-type -MemberDefinition $def.ToString() -name MuiHelper -namespace CosmosKey.Util
    }
    if($global:CosmosKey_Utils_MuiHelper_Shell32 -eq $null){
      $global:CosmosKey_Utils_MuiHelper_Shell32 = [CosmosKey.Util.MuiHelper]::LoadLibrary("shell32.dll")
    }
    $maxVerbLength=255
    $verbBuilder = new-object Text.StringBuilder "",$maxVerbLength
    [void][CosmosKey.Util.MuiHelper]::LoadString($CosmosKey_Utils_MuiHelper_Shell32,$verbId,$verbBuilder,$maxVerbLength)
    return $verbBuilder.ToString()
  }

  $verbs = @{
    "PintoStartMenu"=5381
    "UnpinfromStartMenu"=5382
    "PintoTaskbar"=5386
    "UnpinfromTaskbar"=5387
  }

  if ($verbs.$Action -eq $null) {
    Throw "Action $action not supported`nSupported actions are:`n`tPintoStartMenu`n`tUnpinfromStartMenu`n`tPintoTaskbar`n`tUnpinfromTaskbar"
  }
  InvokeVerb -FilePath $FilePath -Verb $(GetVerb -VerbId $verbs.$action)
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-UpTime	                                              #
#                                                                             #
#   Description     Get the duration since the last boot		      #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-04-30 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-UpTime {
  $OS = Get-WmiObject Win32_OperatingSystem
  $Uptime = (Get-Date) - ($OS.ConvertToDateTime($OS.LastBootupTime))
  return $Uptime
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WindowsUpdateConfig                                   #
#                                                                             #
#   Description     Get Windows Update configuration			      #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2013-04-30 JRV https://social.technet.microsoft.com/Forums/scriptcenter/en-US/eda22234-4705-4949-8b66-71571d618b0c/is-there-any-powershell-script-to-get-windows-update-scheduled-installation-time-settings-example?forum=ITCG
#    2015-12-11 JFL Tried merging the local and remote versions.              #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WindowsUpdateConfig {
  $Notification= @(
    'Not configured'
    'Disabled'
    'Notify before download'
    'Notify before installation'
    'Scheduled installation'
  )
  $dow=@('Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday')
  $AUS=(New-Object -com "Microsoft.Update.AutoUpdate").Settings
  $AUS | Add-Member -MemberType NoteProperty -Name Notification -value $Notification[$AUS.NotificationLevel]
  $AUS | Add-Member -MemberType NoteProperty -Name AutoUpdateDay -value $dow[$AUS.ScheduledInstallationDay] -PassThru
}

# Get remote config. Does not work with local host
Function Get-WindowsUpdateConfig {
  Param(
    $server
  )
  $Notification = @(
    'Not configured'
    'Disabled'
    'Notify before download'
    'Notify before installation'
    'Scheduled installation'
  )
  $dow = @('Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday')
  $wu=[activator]::CreateInstance([type]::GetTypeFromProgID("Microsoft.Update.Session",$server))
  $AUS = $wu.Settings
  $AUS | Add-Member -MemberType NoteProperty -Name Notification -value $Notification[$AUS.NotificationLevel]
  $AUS | Add-Member -MemberType NoteProperty -Name AutoUpdateDay -value $dow[$AUS.ScheduledInstallationDay] -PassThru
}

# JFL merge: Get local or remote config.
Function Get-WindowsUpdateConfig {
  Param(
    $server
  )
  $Notification = @(
    'Not configured'
    'Disabled'
    'Notify before download'
    'Notify before installation'
    'Scheduled installation'
  )
  $dow = @('Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday')
  if ($Server) {
    $wu=[activator]::CreateInstance([type]::GetTypeFromProgID("Microsoft.Update.Session",$server))
  } else {
    $wu = New-Object -com "Microsoft.Update.AutoUpdate"
  }
  $AUS = $wu.Settings
  $AUS | Add-Member -MemberType NoteProperty -Name Notification -value $Notification[$AUS.NotificationLevel]
  $AUS | Add-Member -MemberType NoteProperty -Name AutoUpdateDay -value $dow[$AUS.ScheduledInstallationDay] -PassThru
}

Function Set-WindowsUpdateConfig {
  [CmdLetBinding()]
  Param(
    [ValidateRange(0,4)]
    [int]$NotificationLevel=0,
    [ValidateRange(0,7)]
    [int]$Day=0,
    [ValidateRange(0,24)]
    [int]$hour,
    [switch]$IncludeRecommended
  )
  $AUS = (New-Object -com "Microsoft.Update.AutoUpdate").Settings
  $AUS.NotificationLevel = $NotificationLevel
  $AUS.ScheduledInstallationDay = $Day
  $AUS.ScheduledInstallationTime = $hour
  $AUS.IncludeRecommendedUpdates = $IncludeRecommended
  #$AUS.Save()
  $AUS
}

###############################################################################
#                                                                             #
#                                  Registry                                   #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-RegistryValue                                         #
#                                                                             #
#   Description     Get a registry value                                      #
#                                                                             #
#   Notes           Return $null if the key/value does not exist.             #
#                                                                             #
#   History                                                                   #
#    2010-05-12 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-RegistryValue($key, $name, [Switch]$Verbose) {
  if ($Verbose) {Put-Line "Reading value $key\$name"}
  $item = Get-ItemProperty -ErrorAction SilentlyContinue $key
  if ($item) {
    return $item.$name
  } else {
    return $null
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-RegistryKey                                           #
#                                                                             #
#   Description     Create a registry key.                                    #
#                                                                             #
#   Notes           Create the parent keys if needed.                         #
#                                                                             #
#   History                                                                   #
#    2010-05-26 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function New-RegistryKey($Key, [Switch]$Verbose) {
  $parent = Split-Path $key -Parent
  if (! (Get-Item -ErrorAction SilentlyContinue $parent)) {
    New-RegistryKey $parent -Verbose:$Verbose
  }
  if ($Verbose) {Put-Line "Creating key $key\"}
  New-Item $key >$null
  return $null
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Enum-RegistryValues                                       #
#                                                                             #
#   Description     Enumerate all registry value names in a key               #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-12 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Enum-RegistryValues($Key) {
  get-item $key | Select-Object -ExpandProperty property
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Set-RegistryValue                                         #
#                                                                             #
#   Description     Set a registry value.                                     #
#                                                                             #
#   Notes           Create the key and the value if needed.                   #
#                                                                             #
#   History                                                                   #
#    2010-05-12 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Set-RegistryValue($Key, $Name, $Value, $PropertyType="String", [Switch]$Verbose) {
  if ((Get-RegistryValue $Key $Name) -ne $null) {
    if ($Verbose) {Put-Line "Setting value $key\$name = $value"}
    Set-ItemProperty $Key -name $Name -value $Value >$null
  } else {
    if (! (Get-Item -ErrorAction SilentlyContinue $key)) {
      New-RegistryKey $Key -Verbose:$Verbose
    }
    if ($Verbose) {Put-Line "Creating value $key\$name = $value"}
    New-ItemProperty $Key -name $Name -PropertyType $PropertyType -Value $Value >$null
  }
  return $null
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Update-RegistryValue                                      #
#                                                                             #
#   Description     Update a value if it exists already, and has a != value.  #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2010-05-12 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Update-RegistryValue($key, $name, $value, [Switch]$Verbose) {
  if ($Verbose) {Put-String "Updating value $key\$name ... "}
  $oldValue = Get-RegistryValue $key $name
  if ($oldValue -and ($oldValue -ne $value)) {
    if ($Verbose) {Put-Line "Changing it from $oldValue to $value."}
    Set-RegistryValue $key $name $value
  } elseif ($oldvalue) {
    if ($Verbose) {Put-Line "It already contains $value."}
  } else {
    if ($Verbose) {Put-Line "Key and/or value does not exist."}
  }
}

###############################################################################
#                                                                             #
#                              Active Directory                               #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        New-ComputerAccount                                       #
#                                                                             #
#   Description     Create a new computer object in Active Directory          #
#                                                                             #
#   Notes           Works without any plug-ins.                               #
#                   Run this script on the domain controller as administrator.#
#                   You may also need: http://support.microsoft.com/kb/968930 #
#                                                                             #
#                   User Account Control Values. Not all values are listed.   #
#                   See full list in: http://support.microsoft.com/kb/305144  #
#                   SCRIPT			0x0001	1	              #
#                   ACCOUNTDISABLE		0x0002	2	              #
#                   HOMEDIR_REQUIRED		0x0008	8	              #
#                   LOCKOUT			0x0010	16                    #
#                   PASSWD_NOTREQD		0x0020	32                    #
#                   WORKSTATION_TRUST_ACCOUNT	0x1000	4096                  #
#                   SERVER_TRUST_ACCOUNT	0x2000	8192                  #
#                                                                             #
#                   You can also create a computer object in a specific       #
#                   Active Directory OU's by changing: (For example)          #
#                   $Domain = System.DirectoryServices.DirectoryEntry         #
#                   to ....DirectoryEntry("LDAP://OU=ASDF,DC=asdf,DC=asdf")   #
#                                                                             #
#   History                                                                   #
#    2010-04-30 JFL Adapted from http://gallery.technet.microsoft.com/        #
#                   ScriptCenter/fr-fr/238d9716-65d2-4ab4-8045-eed1b20d8fca.  #
#                                                                             #
#-----------------------------------------------------------------------------#

Function New-ComputerAccount(
  [string]$ComputerName,	# Computer short name. Ex: katz1
  [string]$Description=$null,	# Description string. Ex: "The first blade"
  [string]$Domain=$null,	# Domain. Ex: "lab.acme.com"
  [Guid]$Guid=$null,		# PXE GUID. Ex: "00000000-0000-0000-0000-ABCDEFABCDEF"
  $UAC=4128,			# User Account Control flags. See header above.
  $DirectoryEntry=$null		# Reserved for eventual reuse of this object.
) {
  if ($DirectoryEntry -eq $null) {
    $DirectoryEntry = New-Object System.DirectoryServices.DirectoryEntry
  }
  $Computer = $DirectoryEntry.Create("computer", "CN=" + $ComputerName + ",CN=Computers")
  $Computer.Put("sAMAccountName", $ComputerName.ToUpper() + "$") # A dollar sign must be appended to the end of every computer sAMAccountName.
  if ($Description -ne $null) {
    $Computer.Put("Description", $Description)
  }
  if ($Domain -ne $null) {
    $Computer.Put("dNSHostName", "$ComputerName.$Domain")
  }
  if ($Guid -ne $null) {
    $Computer.Put("netbootGUID", $Guid.ToByteArray())
  }
  $Computer.Put("userAccountControl", $UAC)
  $Computer.SetInfo()

  return $Computer
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Join-Domain		                                      #
#                                                                             #
#   Description     Join the computer to a domain                             #
#                                                                             #
#   Notes           These routines must be run from within an elevated shell. #
#                                                                             #
#   History                                                                   #
#    2012-08-31 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Join-Domain(
  $Domain,
  $Admin = $null,
  $Password = $null,
  $options = 3, # See http://msdn.microsoft.com/en-us/library/windows/desktop/aa392154(v=vs.85).aspx for details
  $reboot = $true
) {
  $computer = Get-WmiObject Win32_ComputerSystem
  $ret = $computer.JoinDomainOrWorkGroup($Domain, $Password, $Admin, $null, $options).ReturnValue
  if ((!$ret) -and $reboot) {
    Restart-Computer -Force
  }
  return $ret
}

Function Join-Workgroup(
  $Workgroup,
  $Admin = $null,
  $Password = $null,
  $options = 0, # See http://msdn.microsoft.com/en-us/library/windows/desktop/aa392154(v=vs.85).aspx for details
  $reboot = $true
) {
  $computer = Get-WmiObject Win32_ComputerSystem
  $ret = $computer.JoinDomainOrWorkGroup($Domain, $Password, $Admin, $null, $options).ReturnValue
  if ((!$ret) -and $reboot) {
    Restart-Computer -Force
  }
  return $ret
}

Function Leave-Domain(
  $Domain,
  $Admin = $null,
  $Password = $null,
  $options = 0, # See http://msdn.microsoft.com/en-us/library/windows/desktop/aa393942(v=vs.85).aspx for details
  $reboot = $true
) {
  $computer = Get-WmiObject Win32_ComputerSystem
  $computer.UnjoinDomainOrWorkgroup($Password, $Admin, $options)
  $ret = $computer.JoinDomainOrWorkGroup($Domain, $Password, $Admin, $null, $options).ReturnValue
  if ((!$ret) -and $reboot) {
    Restart-Computer -Force
  }
  return $ret
}

################################################################################################
# Get-DnOU.ps1                                                                                 #
#(Auteur: Mickaël Hornung)                                                                     #
#                                                                                              #
# Fonction qui permet de récupérer le DN d'une OU passée en paramètre.                         #
# Exemple d'utilisation:                                                                       #
#   Get-DnOU Utilisateurs // Renvoie le Dn de l'OU Utilisateurs si elle existe.                #
#   Get-DnOU a* // Renvoie le DN de toute les OU commençant par a (insensible à la casse)      #
#   Dans le cas où la fonction renvoie plusieurs résultats, un message d'avertissment apparaît #
#   car le but de cette fonction et de rechercher une OU en particulier, de récupérer son DN   #
#   pour s'y connecter.                                                                        #
#   ex: $connection_ou = [ADSI] "LDAP://mon_dc.domain.local/$(Get-DnOU -name Utilisateurs)"    #
################################################################################################

Function Global:Get-DnOU (
  [string]$name = "*",
  [string]$scope = "Subtree",
  [switch]$all,
  [switch]$a,
  [switch]$help,
  [switch]$h
) {
  if ($help -or $h) {
    Write-Host `n
    Write-Host "Get-DnOU: retourne le DistinguishedName d'une unité d'organisation"
    Write-Host "Usage: Get-DnOU <param>"
    Write-Host "<param>:"
    Write-Host "        -name        -Indiquer le nom exact d'une OU ou le début suivit du caractère `"*`""
    Write-Host "        -all        -Retourne toute les OU qui matchent le nom passé en paramètre."
    Write-Host "                     Si ce paramètre est ommis, la fonction s'arrête sur la première OU qui matche le nom."
    Write-Host "        -help        -Affiche l'aide"    
    Write-Host `n
    return
  }
  $root = New-Object System.DirectoryServices.DirectoryEntry("")
  $ldapQuery = "(&(objectCategory=organizationalunit)(name=$name))"
  $query = New-object system.directoryservices.directorysearcher
  $query.searchRoot = $root 
  $query.Filter = $ldapquery
  $query.SearchScope = $scope
  if ($all -or $a) {
    $list_object = $query.findall()
    if ($list_object.count -gt 1 -and $name -ne '*') {
      Write-Host `n
      Write-Warning "$($list_object.count) OU commençent par `"$($name)`""
    }    
  } else {
    $list_object = $query.findone()
  }
  return $list_object | Foreach-Object {$_.properties.distinguishedname}
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Create-DomainUser                                         #
#                                                                             #
#   Description     Create a domain user				      #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2012-02-17 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Create-DomainUser {
  Import-module ActiveDirectory
  New-ADUser -SamAccountName exds -Name "exds lab admin" -AccountPassword (ConvertTo-SecureString -AsPlainText "p@ssw0rd" -Force) -Enabled $true -PasswordNeverExpires $true
}

###############################################################################
#                                                                             #
#                                  ZIP Files                                  #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Xxx-Zip                                                   #
#                                                                             #
#   Description     Manage ZIP files                                          #
#                                                                             #
#   Notes           Examples:                                                 #
#                   # Add all text files in current directory to txt.zip      #
#		    ls *.txt | add-zip c:\Temp\txt.zip                        #
#                   # Idem recursively                                        #
#		    ls *.txt -recurse | add-zip c:\Temp\txt.zip               #
#		    # Add the whole current directory to zipfile              #
#		    gi . | add-zip c:\Temp\txt.zip                            #
#                                                                             #
#   History                                                                   #
#    2011-10-20 JFL Created this routine, based on sample at                  #
#               http://mow001.blogspot.com/2006/01/msh-out-zip-function.html  #
#    2011-12-22 JFL Fixed problems, based on example at http://blogs.msdn.com/b/daiken/archive/2007/02/12/compress-files-with-windows-powershell-then-package-a-windows-vista-sidebar-gadget.aspx #
#    2012-02-10 JFL Fixed a bug that caused the zip file to be empty when     #
#                   adding big objects. Sent the fix to the above msdn blog,  #
#                   and published it on http://social.technet.microsoft.com/Forums/en/ITCG/thread/0615e34f-dc49-43aa-950a-fcf9e1f2204d #
#                                                                             #
#-----------------------------------------------------------------------------#

# Create a new zip file
function New-Zip {
  param([string]$zipName)	# Must be a fully qualified path.

  set-content $zipName ("PK" + [char]5 + [char]6 + ("$([char]0)" * 18))
  (dir $zipName).IsReadOnly = $false
}

# Add files to a zip via a pipeline
function Add-Zip {
  Param([string]$zipName)

  if (-not $zipName.EndsWith('.zip')) {$zipName += '.zip'}
  if (-not (test-path $zipName)) {New-Zip $zipName}

  $zipFile = (new-object -com shell.application).NameSpace($zipName)
  $count = $zipFile.Items().Count # Number of objects in the zip file initially
  foreach ($file in $input) {
    $zipFile.CopyHere($file.FullName)
    $count += 1
  }
  # Now wait for the copy operation to complete
  while ($zipFile.Items().Count -lt $count) {
    Write-Debug "No item found in the zip file. Sleeping 100ms"
    Start-Sleep -milliseconds 100
  }
  # Return deletes the $zipFile object, and aborts incomplete copy operations.
}

# List the files in a zip
function Get-Zip {
  param([string]$zipfilename)

  if (test-path($zipfilename)) {
    $shellApplication = new-object -com shell.application
    $zipPackage = $shellApplication.NameSpace($zipfilename)
    $zipPackage.Items() | Select Path
  }
}

# Extract the files from a zip
function Extract-Zip {
  param([string]$zipfilename, [string] $destination)

  if (test-path($zipfilename)) {
    $shellApplication = new-object -com shell.application
    $zipPackage = $shellApplication.NameSpace($zipfilename)
    $destinationFolder = $shellApplication.NameSpace($destination)
    $destinationFolder.CopyHere($zipPackage.Items())
  }
}

###############################################################################
#                                     SML                                     #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Put-Value		                                      #
#                                                                             #
#   Description     Display a name/value pair, in a quoted format             #
#                                                                             #
#   Notes           The quoted format allows good readability and easy parsing#
#                                                                             #
#   History                                                                   #
#    2010-05-12 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

$currentIndent = 0

Function Put-Value (
  $name,					# The variable name
  $value,					# The variable value
  [alias("i")]$indent = $script:currentIndent,	# Optional switch overriding the default indentation
  [alias("w")]$nameWidth = 0			# Optional switch specifying the name output width
) {
  if ($value -eq $null) {
    return
  }
  $spaces = "{0,$indent}" -f ""
  $name = Quote $name
  if ($nameWidth -ne 0) {$name = "{0,$(-$nameWidth)}" -f $name}
  $value = Quote $value
  echo "$spaces$name $value"
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Put-Dir		                                      #
#                                                                             #
#   Description     Display a directory name, and open an indented block      #
#                                                                             #
#   Notes           Invoke with . Put-Dir if the script block sets variables  #
#                   in the the caller's context.                              #
#                   Side effect: . invoking this script block erases the      #
#                   caller's variables $args, $putDirName, $putDirBlock,      #
#                   and $putDirSpaces.                                        #
#                                                                             #
#   History                                                                   #
#    2010-05-12 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Put-Dir(
  $putDirName,				# The directory name
  [ScriptBlock]$putDirBlock = $null	# Optional script block that will be indented, then closed.
) {
  $putDirSpaces = "{0,$($script:currentIndent)}" -f ""
  echo "$putDirSpaces$(Quote $putDirName) {"
  $script:currentIndent += 2
  if ($putDirBlock -ne $null) {
    . $putDirBlock # Execute the ScriptBlock in our own scope
    # Note: Do not reuse local variables now on, as recursive . Put-Dir inclusion will overwrite them!
    Put-EndDir
  }
}

Function Put-EndDir() {
  $script:currentIndent -= 2
  $spaces = "{0,$($script:currentIndent)}" -f ""
  echo "$spaces}"
}

###############################################################################
#                                                                             #
#                                  XML Files                                  #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-Content                                               #
#                                                                             #
#   Description     Add two new parameters to simplify XML files management   #
#                                                                             #
#   Notes 	    Adapted from a sample published by Walid Toumi:           #
#                   http://walid-toumi.blogspot.com/                          #
#                                                                             #
#   History                                                                   #
#    2011-11-27 WT  Published this routine on http://walid-toumi.blogspot.com #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-Content {
<#
***************************************************************************
 
 --------------------8<-----------------------------------------
    Ajout de deux nouveaux paramètres pour simplifier
    le traitement des fichiers XML: [-AsXml] [-XPath ]
    
    Attention: l'expression Xpath est sEnSiBlE à La cAsSe
    
                                  Walid toumi             
 --------------------8<-----------------------------------------
 
 PS> # Exemples d'utilisation:
 
 PS> $file = "$PSHOME\types.ps1xml"
 
 PS> $u = cat $file -As | Select-Xml -XP "//ScriptProperty" | Select -Expand Node
 PS> $u

 PS> $Xml = Get-Content $file -AsXml
 PS> $Xml.Types.Type[1..10]
 
 PS> Get-Content $file -AsXml -XPath "//Type[contains(Name,'Xml')]/Name"
 
 **************************************************************************
 
.ForwardHelpTargetName Get-Content
.ForwardHelpCategory Cmdlet

#>

[CmdletBinding(DefaultParameterSetName='Path', SupportsTransactions=$true)]
param(
    [Parameter(ValueFromPipelineByPropertyName=$true)]
    [System.Int64]
    ${ReadCount},

    [Parameter(ValueFromPipelineByPropertyName=$true)]
    [System.Int64]
    ${TotalCount},

    [Parameter(ParameterSetName='Path', Mandatory=$true, Position=0, ValueFromPipelineByPropertyName=$true)]
    [System.String[]]
    ${Path},

    [Parameter(ParameterSetName='LiteralPath', Mandatory=$true, Position=0, ValueFromPipelineByPropertyName=$true)]
    [Alias('PSPath')]
    [System.String[]]
    ${LiteralPath},

    [System.String]
    ${Filter},

    [System.String[]]
    ${Include},

    [System.String[]]
    ${Exclude},
    
    [System.String]
    ${XPath},

    [Switch]
    ${Force},
    
    [Switch]
    ${AsXml},

    [Parameter(ValueFromPipelineByPropertyName=$true)]
    [System.Management.Automation.PSCredential]
    ${Credential})

begin
{
    try {
        $outBuffer = $null
        if ($PSBoundParameters.TryGetValue('OutBuffer', [ref]$outBuffer))
        {
            $PSBoundParameters['OutBuffer'] = 1
        }
        $wrappedCmd = $ExecutionContext.InvokeCommand.GetCommand('Microsoft.PowerShell.Management\Get-Content', [System.Management.Automation.CommandTypes]::Cmdlet)
        $cmd = ''
         if($AsXml) {
          [void]$PSBoundParameters.Remove('AsXml')
          $cmd += ' | ForEach-Object {$fx=@()} {$fx+=$_} {$fx -as [Xml]}'
            if($XPath) {
               [void]$PSBoundParameters.Remove('XPath')
               $cmd += ' | Select-Xml -XPath $XPath | Select -expand Node'
            }
        } 
        $ScriptCmd = [ScriptBlock]::Create(
           { & $wrappedCmd @PSBoundParameters }.ToString() + $Cmd
          )
        $steppablePipeline = $scriptCmd.GetSteppablePipeline($myInvocation.CommandOrigin)
        $steppablePipeline.Begin($PSCmdlet)
    } catch {
        throw
    }
}

process
{
    try {
        $steppablePipeline.Process($_)
    } catch {
        throw
    }
}

end
{
    try {
        $steppablePipeline.End()
    } catch {
        throw
    }
}

}

###############################################################################
#                                                                             #
#                                  HTML Files                                 #
#                                                                             #
###############################################################################

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WebPage                                               #
#                                                                             #
#   Description     Download a web page using .net			      #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2012-03-06 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

# See WinHttpRequest spec at
# http://msdn.microsoft.com/en-us/library/windows/desktop/aa384106(v=vs.85).aspx
function Get-WebPage($url) {
  $http = new-object -com WinHttp.WinHttpRequest.5.1
  # $http.setProxy(2, $proxy) # Ex: $proxy = "proxy:8080"
  $http.open("GET", $url, $false)
  $http.send()
  return $http.responseText
}

# Very powerful, but slower version from
# http://blogs.msdn.com/b/mediaandmicrocode/archive/2008/12/01/microcode-powershell-scripting-tricks-scripting-the-web-part-1-get-web.aspx
function Get-WebPage($url, 
    [switch]$self,
    $credential, 
    $toFile,
    [switch]$bytes) {
<#
.Synopsis
    Downloads a file from the web
.Description
    Uses System.Net.Webclient (not the browser) to download data
    from the web.
.Parameter self
    Uses the default credentials when downloading that page (for downloading intranet pages)
.Parameter credential
    The credentials to use to download the web data
.Parameter url
    The page to download (e.g. www.msn.com)    
.Parameter toFile
    The file to save the web data to
.Parameter bytes
    Download the data as bytes   
.Example
    # Downloads www.live.com and outputs it as a string
    Get-Web http://www.live.com/
.Example
    # Downloads www.live.com and saves it to a file
    Get-Web http://wwww.msn.com/ -toFile www.msn.com.html
#>
    $webclient = New-Object Net.Webclient
    if ($credential) {
        $webClient.Credential = $credential
    }
    if ($self) {
        $webClient.UseDefaultCredentials = $true
    }
    if ($toFile) {
        if (-not "$toFile".Contains(":")) {
            $toFile = Join-Path $pwd $toFile
        }
        $webClient.DownloadFile($url, $toFile)
    } else {
        if ($bytes) {
            $webClient.DownloadData($url)
        } else {
            $webClient.DownloadString($url)
        }
    }
}

# Another very different version
function Get-WebPage {
<#  
.SYNOPSIS  
   Downloads web page from site.
.DESCRIPTION
   Downloads web page from site and displays source code or displays total bytes of webpage downloaded
.PARAMETER Url
    URL of the website to test access to.
.PARAMETER UseDefaultCredentials
    Use the currently authenticated user's credentials  
.PARAMETER Proxy
    Used to connect via a proxy
.PARAMETER Credential
    Provide alternate credentials
.PARAMETER ShowSize
    Displays the size of the downloaded page in Kilobytes                
.NOTES  
    Name: Get-WebPage
    Author: Boe Prox
    DateCreated: 08Feb2011        
.EXAMPLE  
    Get-WebPage -url "http://www.bing.com"
   
Description
------------
Returns information about Bing.Com to include StatusCode and type of web server being used to host the site.
 
#>
[cmdletbinding(
        DefaultParameterSetName = 'url',
        ConfirmImpact = 'low'
)]
    Param(
        [Parameter(
            Mandatory = $True,
            Position = 0,
            ParameterSetName = '',
            ValueFromPipeline = $True)]
            [string][ValidatePattern("^(http|https)\://*")]$Url,
        [Parameter(
            Position = 1,
            Mandatory = $False,
            ParameterSetName = 'defaultcred')]
            [switch]$UseDefaultCredentials,
        [Parameter(
            Mandatory = $False,
            ParameterSetName = '')]
            [string]$Proxy,
        [Parameter(
            Mandatory = $False,
            ParameterSetName = 'altcred')]
            [switch]$Credential,
        [Parameter(
            Mandatory = $False,
            ParameterSetName = '')]
            [switch]$ShowSize                        
                       
        )
Begin {    
    $psBoundParameters.GetEnumerator() | % {
        Write-Verbose "Parameter: $_"
        }
   
    #Create the initial WebClient object
    Write-Verbose "Creating web client object"
    $wc = New-Object Net.WebClient
   
    #Use Proxy address if specified
    If ($PSBoundParameters.ContainsKey('Proxy')) {
        #Create Proxy Address for Web Request
        Write-Verbose "Creating proxy address and adding into Web Request"
        $wc.Proxy = New-Object -TypeName Net.WebProxy($proxy,$True)
        }      
   
    #Determine if using Default Credentials
    If ($PSBoundParameters.ContainsKey('UseDefaultCredentials')) {
        #Set to True, otherwise remains False
        Write-Verbose "Using Default Credentials"
        $webrequest.UseDefaultCredentials = $True
        }
    #Determine if using Alternate Credentials
    If ($PSBoundParameters.ContainsKey('Credentials')) {
        #Prompt for alternate credentals
        Write-Verbose "Prompt for alternate credentials"
        $wc.Credential = (Get-Credential).GetNetworkCredential()
        }        
       
    }
Process {    
    Try {
        If ($ShowSize) {
            #Get the size of the webpage
            Write-Verbose "Downloading web page and determining size"
            "{0:N0}" -f ($wr.DownloadString($url) | Out-String).length -as [INT]
            }
        Else {
            #Get the contents of the webpage
            Write-Verbose "Downloading web page and displaying source code"
            $wc.DownloadString($url)      
            }
       
        }
    Catch {
        Write-Warning "$($Error[0])"
        }
    }  
}

#-----------------------------------------------------------------------------#
# Test function tracing using the factorial function

. TraceProcs {

Function Fact ([int]$n) {
  if ($n -le 1) {
    1
  } else {
    $n * (Fact($n-1))
  }
}

} # End of TraceProcs block

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
#                                                                             #
#-----------------------------------------------------------------------------#

if ($false) {
Write-Debug "MyInvocation = {"
$MyInvocation | Out-String | Write-Debug
Write-Debug "}"

Write-Debug "MyInvocation.MyCommand.Name = $($MyInvocation.MyCommand.Name)"

Write-Vars Args

echo "`$DebugPreference = $DebugPreference"
echo "`$Debug = $Debug"
echo "`$VerbosePreference = $VerbosePreference"
echo "`$Verbose = $Verbose"
echo "`$WhatIfPreference = $WhatIfPreference"
echo "`$NoExec = $NoExec"
return
}

if (     ($MyInvocation.MyCommand.Name -eq "Library.ps1") `
    -and ($MyInvocation.InvocationName -ne ".")) {
  Write-LogFile "-------------------------------------------------------"
  Write-Host "# Invoked as the Library.ps1 script"

  if ($Commands) {
    foreach ($command in $Commands) {
      if ($NoExec -or $Debug -or $Verbose) {
	Write-Host $command
      } 
      if (!$NoExec) {
	eval $command
      }
    }
  }
}

###############################################################################
#                                                                             #
#                                   The End                                   #
#                                                                             #
###############################################################################


