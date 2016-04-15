###############################################################################
#                                                                             #
#   File name       Out-ByHost.ps1                                            #
#                                                                             #
#   Description     Insert "$(hostname): " ahead of every output line.        #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2013-10-14 JFL Created this script.                                      #
#    2013-12-16 JFL Added the ability to run remote scripts using it.         #
#                                                                             #
###############################################################################

<#
  .SYNOPSIS
  Invoke remote commands, prefixing every output line with the remote host name.

  .DESCRIPTION
  Two modes of operation:
  1) Convert the input pipeline to strings, inserting the host name ahread of every line.
  2) Invoke remote commands, filtering all output remotely using mode 1.
  This makes it easier to visually review the output of remote commands.
  The drawback is that the output is changed to strings, so it loses all information about object types and members.

  .PARAMETER ComputerName
  Name(s) of remote computers where to execute the ScriptBlock.
  Alias: -Verbose

  .PARAMETER D
  Switch enabling the debug mode. Display debug messages.

  .PARAMETER InputObject
  Optional objects to convert to strings, inserting the host name ahead of each line.

  .PARAMETER Credential
  Optional PSCredential object. For example, create it with commands like:
  $password = ConvertTo-SecureString "PASSWORD" -AsPlainText -Force
  $cred = New-Object System.Management.Automation.PSCredential "USERNAME", $password

  .PARAMETER ScriptBlock
  Script block to execute remotely.

  .EXAMPLE
  Out-ByHost (node1, node2, node3) {hostname}
  Invokes {hostname | Out-ByHost} on the three nodes. 
#>

[CmdletBinding(DefaultParameterSetName='ScriptBlock')]
Param (
  [Parameter(ParameterSetName='InputObject', Position=0, ValueFromPipeline=$true, Mandatory=$true)]
  [Object]$InputObject,			# Optional input objects

  [Parameter(ParameterSetName='ScriptBlock', Position=0, ValueFromPipeline=$false, Mandatory=$true)]
  [string[]]$ComputerName,		# Optional list of target computer names

  [Parameter(ParameterSetName='ScriptBlock')]
  [System.Management.Automation.PSCredential]$Credential, # Optional PSCredential

  [Parameter(ParameterSetName='ScriptBlock', Position=1)]
  [ScriptBlock]$ScriptBlock,		# Optional script block

  [Parameter(ParameterSetName='InputObject')]
  [Parameter(ParameterSetName='ScriptBlock')]
  [Switch]$D,				# Debug mode

  [Switch]$Version			# If true, display the script version
)

Begin {

  # If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2014-09-25"
if ($Version) {
  echo $scriptVersion
  return
}

if ($D -or ($DebugPreference -ne "SilentlyContinue")) {
  $Debug = $true
  $DebugPreference = "Continue"	    # Make sure Write-Debug works
  $V = $true
  Write-Debug "# Running in debug mode."
  set-alias msg Write-Host
} else {
  $Debug = $false
  $DebugPreference = "SilentlyContinue"	    # Write-Debug does nothing
  set-alias msg Out-null
}

'.ps1 Begin()' | msg

Function Out-ByHost {
  [CmdletBinding()]
  param(
    [Parameter(Position=0,ValueFromPipeline=$true)][object]$Object,
    [Switch]$PassThru = $false
  )
  Begin {	# Process the object passed in as an argument, if any
    "Begin() # `$Object=$object" | msg
    $prefix = "${env:computername}: "
    $objects = @()
    if ($PassThru -and ($object -ne $null)) {$object}
  }
  Process {	# Process each object passed in via the input stream
    "Process() # `$_=$_" | msg
    if ($_ -is [String]) {
      $s = $_
      $s = $s -replace '[ \t]*$',''
      $s = $s -replace '[ \t]*(\r?\n)',"`$1$prefix"
      $s = $s -replace '^',"$prefix"
      $s
    } else {	# Defer formatting to the End() block, to allow aligning columns
      $objects += $_
    }
    if ($PassThru) {$_}
  }
  End {		# Format the list of objects as a table
    "End() # `$length=$($objects.length)" | msg
    if ($objects.length) {
      # Problem: Out-File would format tables as we want, but it adds a lot of useless spaces to the end of every line.
      # Workaround: Manually format the output string, then trim line-per-line the trailing spaces this formatting adds.
      $objects | Out-String -Width 4096 -Stream | % {
        ($_ -replace '[ \t]*$','') -replace '^',"$prefix"
      }
    }
  }
}

if ($InputObject -ne $null) {
  '$InputObject is ' + $InputObject.GetType() | msg
  $InputObject | Out-ByHost
} else {
  '$InputObject is $null' | msg
}

if ($ScriptBlock) {
  '$ScriptBlock is defined' | msg
  # Update the scriptblock to pipe all output to Out-ByHost
  $ScriptBlock = [scriptblock]::Create("&{$ScriptBlock} | Out-ByHost")
  Invoke-Command $ComputerName -Credential $Credential $ScriptBlock
}

$objects = @()
}

Process {
'.ps1 Process()' | msg
$objects += $_
}

End {
'.ps1 End()' | msg
if ($objects.length) {
  $objects | Out-ByHost
}
}

