###############################################################################
#                                                                             #
#   File name       ShellApp.ps1                                              #
#                                                                             #
#   Description     Manage Shell Applications (File Manager, Control Panel...)#
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2021-10-07 JFL Created this script based on window.ps1                   #
#    2021-10-10 JFL Renamed variables, and output a few more fields.          #
#    2021-10-11 JFL Minor tweaks and experiments.                             #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################
#Requires -version 2

<#
  .SYNOPSIS
  Manage Shell Applications (File Manager, Control Panel, etc...)

  .DESCRIPTION
  Output objects with their major properties.

  .PARAMETER Get
  Enumerate Shell Applications. (Default)

  .PARAMETER hWindow
  Switch to output only the window handles.
  (The default is to output ShellApp objects.)

  .PARAMETER FileExplorer
  Switch for selecting the "real" File Explorer instances listing files, but not
  the explorer.exe instances that are just containers for Control Panels, etc.
  Note that the Index field allows ordering File Explorer windows in the
  same order as shown on the Taskbar.
  Alias: -FE

  .PARAMETER D
  Debug mode. Display internal data about the ShellApps enumeration process.
  Alias: -Debug

  .PARAMETER Version
  Display this script version and exit.

  .EXAMPLE
  .\ShellApp.ps1 | ft -a
  List all explorer.exe instances, including Control Panels, etc.

  .EXAMPLE
  .\ShellApp.ps1 -fe
  List real File Explorer instances listing files, excluding Control Panels, etc.

  .EXAMPLE
  .\ShellApp.ps1 -fe | sort Index | % { $_.hWnd} | window.ps1 -Moveto 400,200
  Align real File Explorer windows, starting at screen coordinates 400,200.
#>

[CmdletBinding(DefaultParameterSetName='Get')]
Param (
  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$Get = $($PSCmdlet.ParameterSetName -eq 'Get'),	# Enumerate all ShellApps

  [Parameter(ParameterSetName='Get', Mandatory=$false, ValueFromPipeline=$true, Position=0)]	# Default $InputObject is all in this case
  [AllowEmptyCollection()]
  [Object[]]$InputObject = @(),		# ShellApps to work on, or criteria for selecting them

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$hWindow,			# Output only the window handles

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch][alias("FE")]$FileExplorer,	# List only real File Explorer instances

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$D,				# If true, display debug information

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$V,				# If true, display verbose information
  
  [Parameter(ParameterSetName='Version', Mandatory=$true)]
  [Switch]$Version			# If true, display the script version
)

Begin {

# If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2021-10-11"
if ($Version) {
  echo $scriptVersion
  return
}

# This script name, with various levels of details
$argv0 = dir $MyInvocation.MyCommand.Definition
$scriptBaseName = $argv0.basename	# Ex: Library
$scriptName = $argv0.name		# Ex: Library.ps1
$scriptFullName = $argv0.fullname	# Ex: C:\Temp\Library.ps1

###############################################################################
#                                                                             #
#                              Debugging library                              #
#                                                                             #
###############################################################################

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
#   Function        Get-ShellApps                                             #
#                                                                             #
#   Description     Enumerate all Shell applications                          #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-ClassID($Self) {
  # See: https://docs.microsoft.com/en-us/windows/win32/properties/shell-bumper
  $ID = $Self.ExtendedProperty("System.NamespaceCLSID")
  # Some instances return nothing; Ohters a "{GUID}" String; Others return a byte[] array
  if ($ID -is "byte[]") {
    $ID = "{$(([guid]$ID).ToString().ToUpper())}"
  }
  return $ID # Either $null or a "{GUID}" String
}

Function Get-ShellApps {
  # Define a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
  # Location should be the last field, as it may be very long, and else it prevents other fields from being displayed in table mode.
  $DefaultFieldsToDisplay = 'Index', 'AppType', 'ClassName', 'hWnd', 'Location'
  if ($Verbose) {
    $DefaultFieldsToDisplay += 'IsFileSystem', 'Program', 'ClassID', 'LocationURL'
  }
  # Other less useful fields: 'PID', 'Left', 'Top', 'Width', 'Height'
  $defaultDisplayPropertySet = New-Object System.Management.Automation.PSPropertySet(
    'DefaultDisplayPropertySet', [string[]]$DefaultFieldsToDisplay
  )
  $PSStandardMembers = [System.Management.Automation.PSMemberInfo[]]@($defaultDisplayPropertySet)

  # Enumerate all File Explorer instances
  # This records information allowing to distinguish real File Explorer instances
  # from explorer.exe instances which are just containers for Control Panels, etc.
  Write-Debug "Enumerating Windows Explorer instances"
  $index = 0
  (New-Object -com "Shell.Application").windows() | % {
    $hWnd = $_.HWND # $hWnd type is long
    # $processID = [uint32]0
    # $threadID = [Win32WindowProcs]::GetWindowThreadProcessId($hWnd, [ref]$processID)
    # Identify what kind of Explorer provider this is
    $Self = $_.Document.Folder.Self
    $ClassID = Get-ClassID $Self

    # This works, but is noticeably slower than the next implementation below.
    # Everything goes up to the Desktop. Stop at the base, one level below the Desktop.
    # for ($BaseFolder = $_.Document.Folder;
    #      $BaseFolder.ParentFolder -and $BaseFolder.ParentFolder.ParentFolder;
    #      $BaseFolder = $BaseFolder.ParentFolder) {}
    # $BaseName = $BaseFolder.Title
    # $BaseClassID = Get-ClassID $BaseFolder.Self

    # For "Shell File System Folder", $Self.Path is the directory pathnane;
    # For everything else, it's the hierarchy of UUIDs leading to that virtual folder.
    # For Control Panels, the first UUID is that of the Control Panel itself.
    $BaseClassID = $null
    $Path = $Self.Path
    if (($Path.Length -ge 40) -and ($Path.Substring(0,3) -eq "::{") -and ($Path[39] -eq "}")) {
      $BaseClassID = $Path.Substring(2,38)
    }

    # If the ClassID isn't defined, try using the base class instead
    if (!$ClassID) {
      $ClassID = $BaseClassID
    }

    # Get the class name for the class ID
    $ClassName = $null
    if ($ClassID -ne $null) {
      $props = Get-ItemProperty "HKLM:\HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\$ClassID" -ErrorAction SilentlyContinue
      if ($props) {
	$ClassName = $props."(default)"
      }
      if (!$ClassName) {
      	$ClassName = $Self.Name
      }
      $ClassID = [guid]$ClassID # Finally convert the string to an actual GUID
    }

    # Our own classification
    $AppType = $null
    if ($Self.IsFileSystem) {
      $AppType = "File Explorer"
    } elseif ($FileExplorerIDs -contains "$ClassID") {
      $AppType = "File Explorer"
    } elseif ($BaseClassID -eq "{26EE0668-A00A-44D7-9371-BEB064C98683}") {
      $AppType = "Control Panel"
    } elseif ("{$ClassID}" -eq "{D20EA4E1-3957-11D2-A40B-0C5020524153}") {
      $AppType = "Control Panel" # Actually: Administrative Tools
    } elseif ($Self.Name -eq $Self.Path) { # TODO: Improve this test, which is very weak
      $AppType = "Search Results"	   # Ex: "Search Results in Indexed Locations"
    } else {
      $AppType = "Unknown"
    }

    # Conclusion
    Write-Debug ("$hWnd explorer.exe / ${shClassName}: " + $_.LocationName)
    $window = New-Object PSObject -Property @{
      Index = $index++
      hWnd = [Int]$hWnd
      # Title = $_.Document.Folder.Title # Same as $_.LocationName
      # Left = $_.Left
      # Top = $_.Top
      # Width = $_.Width
      # Height = $_.Height
      Pathname = [string]$_.FullName
      Program = (Get-Item $_.FullName).Name
      ClassName = $ClassName
      ClassID = $ClassID
      AppType = $AppType
      Location = $_.LocationName
      LocationUrl = $_.LocationUrl
      # PID = $processId
      IsFileSystem = $Self.IsFileSystem
    }

    # Add a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
    $window | Add-Member MemberSet PSStandardMembers $PSStandardMembers -Force
    # Give this object a unique typename
    $window.PSObject.TypeNames.Insert(0, $ShellAppTypeName)

    # Output the object
    $window | gm -f | Out-String | Write-Debug
    $window
  }
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
#                                                                             #
#-----------------------------------------------------------------------------#

  $ShellAppTypeName = 'ShellApp'	# This object type name

  $FileExplorerIDs = ( # TODO: Add other known class IDs for real File Explorer instances _not_ flagged as $Self.IsFileSystem
  		       # See: https://www.tenforums.com/tutorials/3123-clsid-key-guid-shortcuts-list-windows-10-a.html
    # Windows 10
    "f02c1a0d-be21-4350-88b0-7367fc96ef3c", # Network
    "679f85cb-0220-4080-b29b-5540cc05aab6", # Quick Access
    "20d04fe0-3aea-1069-a2d8-08002b30309d", # This PC
    # Windows 7
    "031e4825-7b94-4dc3-b131-e946b44c8dd5"  # Libraries
  )
  $OtherFileExplorerIDs = ( # These are already flagged as $Self.IsFileSystem
    # Windows 10
    "b4bfcc3a-db2c-424c-b029-7fe99a87c641", # Desktop
    "d3162b92-9365-467a-956b-92703aca08af", # Documents
    "088e3905-0323-4b02-9826-5d99428e115f", # Downloads
    "35786D3C-B075-49b9-88DD-029876E11C01", # Mobile Phones
    "3dfdf296-dbec-4fb4-81d1-6a3438bcf4de", # Music
    "24ad3ad4-a569-4530-98e1-ab02f9417aa8", # Pictures
    "0c39a5cf-1a7a-40c8-ba74-8900e6df5fcd", # Recent Items
    "54a754c0-4bf0-11d1-83ee-00a0c90dc849", # Remote drive
    "3ba2e6b1-a6a1-ccf6-942c-d370b14d842b", # SharePoint
    "f3364ba0-65b9-11ce-a9ba-00aa004ae837", # Shell File System Folder
    "f86fa3ab-70d2-4fc7-9c99-fcbf05467f3a"  # Videos
  )

  $allShellApps = Get-ShellApps
  if ($FileExplorer) {
    # Select all the windows that appear above the File Explorer button on the task bar
    # $allShellApps = @($allShellApps | where { $FileExplorerIDs -contains [string]($_.ClassID) })
    $allShellApps = @($allShellApps | where { $_.AppType -ne "Control Panel" })
  }

  Write-Debug "ShellApp.Begin()"

  # Workaround for PowerShell v2 bug: $PSCmdlet Not yet defined in Param() block
  $Get = ($PSCmdlet.ParameterSetName -eq 'Get')

  $nInputObjects = 0
  } ; # End of the Begin block

Process {
  Write-Debug "ShellApp.Process($InputObject)"
  $InputObject | % {
    $nInputObjects += 1 # Count the number of objects we received from the input pipe

    if (($_ -is [PSObject]) -and ($_.psobject.typenames -contains $ShellAppTypeName)) {
      $ShellApps = @($_)
    } elseif (($_ -is [int]) -or ($_ -is [long]) -or ($_ -is [IntPtr])) {
      $Index = $_
      $ShellApps = @($allShellApps[$Index])
    } elseif ($_ -is [guid]) {
      $ClassID = $_
      $ShellApps = @($allShellApps | where {$_.ClassID -eq $ClassID})
    } else {
      $name = "$_"
      $ShellApps = @($allShellApps | where {
        ($_.Location -like $name) -or ($_.Program -like $name) -or
        ($_.ClassName -like $name) -or ($_.ClassID -like $name)
      })
    }

    # Enumerate ShellApps
    if ($Get) {
      if ($hWindow) {
	$ShellApps | % { $_.hWnd }
      } else {
	$ShellApps
      }
    }
  }
}

End {
  Write-Debug "ShellApp.End() # `$nInputObjects = $nInputObjects   `$nShellApps = $($allShellApps.Count)   `$Get = $Get"
  if ($nInputObjects -eq 0) { # If no object or object specifier was passed in

    if ($Get) {	# By default, list all ShellApps
      if ($hWindow) {
	$allShellApps | % { $_.hWnd }
      } else {
	$allShellApps
      }
    }

    # All other actions should do nothing, as the input pipe filtering left no window object to work on.
  }
}
