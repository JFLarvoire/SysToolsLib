###############################################################################
#                                                                             #
#   File name       Window.ps1                                                #
#                                                                             #
#   Description     Manage windows (the rectangular screen areas, not the OS) #
#                                                                             #
#   Notes           Known issues with PowerShell v2:                          #
#                   * The default table columns ordering does not work.       #
#                   Workaround:                                               #
#                   Window.ps1 | ft -a hWnd,Program,Left,Top,Width,Height,Title
#                   * The window screen capture does not work.                #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this script.                                      #
#    2016-03-02 JFL Allow specifying either the title or the program name.    #
#    2016-03-30 JFL Allow specifying also the hWnd.			      #
#                   Display the program name by default, but not its path.    #
#    2016-06-01 JFL Made -Get the default command switch.		      #
#                   Allow passing in Window objects via the input pipe.       #
#                   Added the -Step switch to all spacing windows regularly.  #
#                   Added the -WhatIf switch to allow testing moves.          #
#    2016-06-02 JFL Added the -Capture command switch.			      #
#                   Added limited support for PowerShell v2.                  #
#                   Added the -Children switch to enumerate immediate children.
#                   Added the -All switch to enumerate all windows.           #
#    2016-06-03 JFL Get the Program name for all windows in the -All case.    #
#                   Added fields PID and Class to the window objects.         #
#                   Also enumerate popup windows by default.                  #
#                   Added a 100ms delay before screen captures, to give time  #
#                   to the system to redraw all fields that are reactivated.  #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################
#Requires -version 2

<#
  .SYNOPSIS
  Manage windows (the rectangular screen areas, not the Windows OS)

  .DESCRIPTION
  Options for listing windows, moving them around, and resizing them.

  .PARAMETER Get
  Command switch.
  Enumerate windows.

  .PARAMETER MoveTo
  A pair of integers defining a screen coordinate: $left, $top
  Move the window top-left corner to that location. 
  Alias: -MT

  .PARAMETER Resize
  A pair of integers defining a size in pixels: $width, $height
  Set the window size. 
  Alias: -R

  .PARAMETER OnTop
  Redraw the window on top of the others.

  .PARAMETER Step
  A pair of integers: $horizonal, $vertical
  How much to shift each window top-left corner when moving several windows. 
  Alias: -S

  .PARAMETER Capture
  Command switch.
  Captures a window image, and copies it to the clipboard.
  Side effect: Moves the window on top of others.
  Note: If multiple windows are selected, only the last one will remain in the clipboard.
  This allows to easily capture popups, which are enumerated last, when selecting windows by program name or process ID.
  Alias: -C

  .PARAMETER Children
  Switch forcing the Get command to enumerate first level child windows.

  .PARAMETER All
  Switch forcing the Get command to enumerate all top-level windows.
  Caution: This outputs a lot of data, and can take a significant amount of time.
  Note: Ignores windows with 0 or 1 pixels, as these usually are hidden windows
  used for internal events processing.

  .PARAMETER InputObject
  A list of input objects defining the windows to work on. One of:
  - PSObjects returned by this script's -Get command.
  - A string defining the window title. Wildcards allowed.
  - A string defining the program name. Wildcards allowed.
  - An integer defining the window handle.
  Can also come from the input pipeline.

  .PARAMETER X
  Display what windows would be moved, but don't move them.
  Alias: -WhatIf

  .PARAMETER Version
  Display this script version and exit.

  .EXAMPLE
  .\Window.ps1 | ft -a
  List all windows in a table, showing only the default data fields

  .EXAMPLE
  .\Window.ps1 ServerManager.exe | fl *
  List all Server Manager windows, showing all data fields

  .EXAMPLE
  .\Window.ps1 "Windows Update" -MoveTo 400,200 -Resize 875,640
  Move the Windows Update window to location (400,200) and resize it.

  .EXAMPLE
  .\Window.ps1 iexplore.exe -MoveTo 200,100 -OnTop
  Move the Internet Explorer window to location (200,100), and put it on top.

  .EXAMPLE
  .\Window.ps1 *notepad* -MoveTo 200,100 -OnTop
  Move all Notepad windows, spacing them regularly, starting from location (200,100), and put them on top.

  .EXAMPLE
  .\Window.ps1 irc.exe | sort Title | .\Window.ps1 -MoveTo 200,100 -OnTop
  Idem for all iLO Remote Consoles, sorting them by title (= iLO name) first.

  .EXAMPLE
  .\Window.ps1 outlook.exe -Capture
  Capture the OutLook top window, and copy the image to the clipboard.

  .EXAMPLE
  .\Window.ps1 -All | where {$_.Title -like "*initiator*"} | .\Window.ps1 -capture
  Capture a floating dialog box, with a title containing "initiator", and copy it to the clipboard.
#>

[CmdletBinding(DefaultParameterSetName='Get')]
Param (
  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$Get = $($PSCmdlet.ParameterSetName -eq 'Get'),	# Enumerate all windows

  [Parameter(ParameterSetName='Get', Mandatory=$false, ValueFromPipeline=$true, Position=0)]
  [Parameter(ParameterSetName='Move', Mandatory=$true, ValueFromPipeline=$true, Position=0)]
  [Parameter(ParameterSetName='Capture', Mandatory=$true, ValueFromPipeline=$true, Position=0)]
  [AllowEmptyCollection()]
  [Object[]]$InputObject = @(),		# Windows to work on, or criteria for selecting them

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Int[]][alias("MT")]$MoveTo,		# Location where to move to

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Int[]][alias("R")]$Resize,		# New size to set

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Switch]$OnTop,			# Redraw it on top of the others

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Int[]][alias("S")]$Step = (100, 30),	# Step when moving multiple windows

  [Parameter(ParameterSetName='Capture', Mandatory=$true)]
  [Switch][alias("C")]$Capture,		# Capture the window image and copy it to the clipboard

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$Children,			# Also list child windows

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$All,				# List all windows

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Parameter(ParameterSetName='Capture', Mandatory=$false)]
  [Switch]$D,				# If true, display debug information

  # [Parameter(ParameterSetName='Get', Mandatory=$false)]
  # [Parameter(ParameterSetName='Move', Mandatory=$false)]
  # [Parameter(ParameterSetName='Capture', Mandatory=$false)]
  # [Switch]$V,				# If true, display verbose information

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Switch][alias("WhatIf")]$X,		# If true, display commands, but don't execute them

  [Parameter(ParameterSetName='Version', Mandatory=$true)]
  [Switch]$Version			# If true, display the script version
)

Begin {

# If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2016-06-03"
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
#   Function        GetWindowRect, MoveWindow                                 #
#                                                                             #
#   Description     WIN32 window management routines                          #
#                                                                             #
#   Notes           See http://www.pinvoke.net for Win32 functions 	      #
#		    declarations.					      #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Add-Type @"
  using System;
  using System.Runtime.InteropServices;
  using System.Collections.Generic;
  using System.Text;

  public class Win32WindowProcs {
    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool BringWindowToTop(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool AllowSetForegroundWindow(uint pid);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool EnumChildWindows(IntPtr window, EnumWindowProc callback, IntPtr i);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern Int32 GetWindowTextLength(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int cch);

    [DllImport("user32.dll", ExactSpelling=true, CharSet=CharSet.Auto)]
    public static extern IntPtr GetParent(IntPtr hWnd);

    [DllImport("user32.dll", ExactSpelling=true)]
    public static extern IntPtr GetAncestor(IntPtr hWnd, uint gaFlags); // 1=Parent 2=Root 3=RootOwner

    [DllImport("user32.dll")]
    public static extern IntPtr GetTopWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError=true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Auto)]
    public static extern uint GetWindowModuleFileName(IntPtr hWnd, StringBuilder lpszFileName, uint cchMax);

    [DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Auto)]
    public static extern uint GetClassName(IntPtr hWnd, StringBuilder lpszClassName, uint cchMax);

    [DllImport("user32.dll")]
    public static extern IntPtr GetWindow(IntPtr hWnd, uint wCmd);	// 0=First  1=Last  2=Below  3=Above  4=Owner  5=Child  6=Popup

    public static List<IntPtr> GetChildWindows(IntPtr parent) {
      List<IntPtr> result = new List<IntPtr>();
      GCHandle listHandle = GCHandle.Alloc(result);
      try {
	 EnumWindowProc childProc = new EnumWindowProc(EnumWindow);
	 EnumChildWindows(parent, childProc,GCHandle.ToIntPtr(listHandle));
      } finally {
	 if (listHandle.IsAllocated) listHandle.Free();
      }
      return result;
    }
    private static bool EnumWindow(IntPtr handle, IntPtr pointer) {
      GCHandle gch = GCHandle.FromIntPtr(pointer);
      List<IntPtr> list = gch.Target as List<IntPtr>;
      if (list == null) {
	throw new InvalidCastException("GCHandle Target could not be cast as List<IntPtr>");
      }
      list.Add(handle);
      //  You can modify this to check to see if you want to cancel the operation, then return a null here
      return true;
    }
    public delegate bool EnumWindowProc(IntPtr hWnd, IntPtr parameter);
  }

  public struct RECT
  {
    public int Left;        // x position of upper-left corner
    public int Top;         // y position of upper-left corner
    public int Right;       // x position of lower-right corner
    public int Bottom;      // y position of lower-right corner
  }
"@

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WindowTitle                                           #
#                                                                             #
#   Description     Get the window title                                      #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2016-06-02 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WindowTitle($hWnd) {
  $length = [Win32WindowProcs]::GetWindowTextLength($hWnd)
  if ($length -gt 0) {
    $sb = New-Object text.stringbuilder -ArgumentList ($length + 1)
    $length = [Win32WindowProcs]::GetWindowText($hWnd, $sb, $sb.Capacity)
    $sb.toString()
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WindowModule                                          #
#                                                                             #
#   Description     Get the window module file name                           #
#                                                                             #
#   Notes           This seems to always return PowerShell.exe for all        #
#                   applications !?!                                          #
#                                                                             #
#   History                                                                   #
#    2016-06-03 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WindowModule($hWnd) {
  $length = 65535
  $sb = New-Object text.stringbuilder -ArgumentList ($length + 1)
  $length = [Win32WindowProcs]::GetWindowModuleFileName($hWnd, $sb, $sb.Capacity)
  $sb.toString()
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-WindowClass                                           #
#                                                                             #
#   Description     Get the window class name                                 #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2016-06-03 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-WindowClass($hWnd) {
  $length = 1024
  $sb = New-Object text.stringbuilder -ArgumentList ($length + 1)
  $length = [Win32WindowProcs]::GetClassName($hWnd, $sb, $sb.Capacity)
  $sb.toString()
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Get-Windows                                               #
#                                                                             #
#   Description     Enumerate all windows                                     #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-Windows {
  param(
    [Parameter(Mandatory=$false)]
    [Switch]$All,			# Enumerate all top-level windows
    [Parameter(Mandatory=$false)]
    [Switch]$Children			# Also list child windows
  )

  $rect = New-Object RECT

  # Define a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
  # Title should be the last field, as it may be very long, and else it prevents other fields from being displayed in table mode.
  $DefaultFieldsToDisplay = 'hWnd','Program', 'Left', 'Top', 'Width', 'Height','Title'
  $defaultDisplayPropertySet = New-Object System.Management.Automation.PSPropertySet(
    'DefaultDisplayPropertySet',[string[]]$DefaultFieldsToDisplay
  )
  $PSStandardMembers = [System.Management.Automation.PSMemberInfo[]]@($defaultDisplayPropertySet)

  $windows = @()
  # Enumerate Processes with a visible window
  Get-Process | Where-Object {$_.MainWindowTitle -ne ""} | % {
    $hWnd = $_.MainWindowHandle
    $windows += New-Object PSObject -Property @{
      hWnd = $hWnd
      Pathname = $_.Path
      Program = (Get-Item $_.Path).Name
      Title = $_.MainWindowTitle
      hParentWnd = [Win32WindowProcs]::GetParent($hWnd)
      hOwnerWnd = [Win32WindowProcs]::GetAncestor($hWnd, 3)
      PID = $_.Id
    }
  }
  # Enumerate Windows Explorer windows
  (New-Object -com "Shell.Application").windows() | % {
    $hWnd = $_.HWND
    $processID = [uint32]0
    $threadID = [Win32WindowProcs]::GetWindowThreadProcessId($hWnd, [ref]$processID)
    $windows += New-Object PSObject -Property @{
      hWnd = $hWnd
      Pathname = $_.FullName
      Program = (Get-Item $_.FullName).Name
      Title = $_.LocationName
      hParentWnd = [Win32WindowProcs]::GetParent($hWnd)
      hOwnerWnd = [Win32WindowProcs]::GetAncestor($hWnd, 3)
      PID = $processId
    }
  }
  # Enumerate popup windows of all the above
  foreach ($window in $windows) {
    $popup = [Win32WindowProcs]::GetWindow($window.hWnd, 6)
    if ($popup -ne 0) {
      $Title = Get-WindowTitle($popup)
      $windows += New-Object PSObject -Property @{
	hWnd = $popup
	Pathname = $window.Pathname
	Program = $window.Program
	Title = $Title
	hParentWnd = [Win32WindowProcs]::GetParent($popup)
	hOwnerWnd = [Win32WindowProcs]::GetAncestor($popup, 3)
	PID = $window.PID
      }
    }
  }
  # Enumerate Child windows of all the above
  if ($Children) {
    $topWindows = $windows
    $windows = @()
    foreach ($window in $topWindows) {
      foreach ($child in ([Win32WindowProcs]::GetChildWindows($window.hWnd))) {
	$Title = Get-WindowTitle($child)
	if ($Title.Length) {
	  $windows += New-Object PSObject -Property @{
	    hWnd = $child
	    Pathname = $window.Pathname
	    Program = $window.Program
	    Title = $Title
	    hParentWnd = $window.hWnd
	    hOwnerWnd = [Win32WindowProcs]::GetAncestor($child, 3)
	    PID = $window.PID
	  }
	}
      }
    }
  }
  # If requested to enumerate all windows, use the above information as a reference to the owner programs
  if ($All) {
    $topWindows = $windows
    $windows = @()
    foreach ($window in ([Win32WindowProcs]::GetChildWindows(0))) {
      $done = [Win32WindowProcs]::GetWindowRect($window, [ref]$rect)
      $Width = $rect.Right - $rect.Left
      $Height = $rect.Bottom - $rect.Top
      if (($Width + $Height) -le 2) {continue} # Ignore 0 or 1-pixel sized windows, which usually are hidden worker windows 
      $parentWnd = [Win32WindowProcs]::GetParent($window)
      $ownerWnd = [Win32WindowProcs]::GetAncestor($window, 3)
      # $topWnd = [Win32WindowProcs]::GetTopWindow($window)
      # $rootWnd = [Win32WindowProcs]::GetAncestor($window, 2)
      # $nextWnd = [Win32WindowProcs]::GetWindow($window, 3)
      # Now find the process owning the window, which in turn will give us the program name
      $processID = [uint32]0
      $threadID = [Win32WindowProcs]::GetWindowThreadProcessId($window, [ref]$processID)
      $process = Get-Process -id $processID
      $Pathname = $process.Path
      $Program = (Get-Item $Pathname).Name
      # Another possible alternative: Try using GetWindowModuleFileName()
      $windows += New-Object PSObject -Property @{
	hWnd = $window
        Title = Get-WindowTitle($window)
	Pathname = $Pathname
	Program = $Program
	hParentWnd = $parentWnd
	hOwnerWnd = $ownerWnd
	# hTopWnd = $topWnd
	# hRootWnd = $rootWnd
	# hNextWnd = $nextWnd
	PID = $processID
	# ThreadID = $threadID
      }
    }
  }

  # Add location and defaults
  foreach ($window in $windows) {
    $done = [Win32WindowProcs]::GetWindowRect($window.hWnd, [ref]$rect)
    # $module = Get-WindowModule($window.hWnd)
    $class = Get-WindowClass($window.hWnd)
    # In PowerShell v3, this can be simplified as $window | Add-Member @{...}
    $newProps = @{
      Left = $rect.Left
      Top = $rect.Top
      Width = $rect.Right - $rect.Left
      Height = $rect.Bottom - $rect.Top
      # Module = $module # Always returns PowerShell.exe for all apps!?!
      Class = $class
    }
    foreach ($Name in $newProps.Keys) {
      $Value = $newProps.$Name
      $Window | Add-Member -MemberType NoteProperty -Name $Name -Value $Value -Force
    }
    # Add a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
    $window | Add-Member MemberSet PSStandardMembers $PSStandardMembers -Force
    # Give this object a unique typename
    $window.PSObject.TypeNames.Insert(0,'Window')
    # Output the object
    $Window | gm -f | Out-String | Write-Debug
    $window
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Move-Windows                                              #
#                                                                             #
#   Description     Move a set of windows                                     #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2016-06-01 JFL Moved the code into this new routine.                     #
#                   Added support for the NoExec mode.                        #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Move-Window {
  Param(
    [Parameter(Mandatory=$true)]
    [PSObject[]]$Windows,
    [Parameter(Mandatory=$false)]
    [Int[]][alias("MT")]$MoveTo,	# Location where to move to
    [Parameter(Mandatory=$false)]
    [Int[]][alias("R")]$Resize,		# New size to set
    [Parameter(Mandatory=$false)]
    [Int[]][alias("S")]$Step,		# How much to move successive windows
    [Parameter(Mandatory=$false)]
    [Switch]$OnTop,			# Redraw it on top of the others
    [Parameter(Mandatory=$false)]
    [Switch]$PassWhereArrived		# Pass the final location back to the caller
  )
  foreach ($window in $windows) {
    $hWnd = $window.hWnd
    $Left = $window.Left
    $Top = $window.Top
    $Width = $window.Width
    $Height = $window.Height
    $NeedMove = $false
    if ($MoveTo.Count -gt 0) {
      $NeedMove = $true
      $Left = $MoveTo[0]
      $Top = $MoveTo[1]
      if ($Top -eq $null) { $Top = 0 }
      $MoveTo = @(($Left + $Step[0]), ($Top + $Step[1])) 
    }
    if ($Resize.Count -gt 0) {
      $NeedMove = $true
      $Width = $Resize[0]
      $Height = $Resize[1]
      if ($Height -eq $null) { $Height = $window.Height }
    }
    $msg = "Would move $($window.Program) window $($window.hWnd)"
    if ($NeedMove) {
      if (!$NoExec) {
	$Done = [Win32WindowProcs]::MoveWindow($hWnd, $Left, $Top, $Width, $Height, $true)
	if (!$Done) {
	  throw "Failed to move $($window.Program) window `"$($window.hWnd)`"."
	}
      } else {
      	$msg += " to ($Left, $Top) size ($Width, $Height)"
      }
    }
    if ($OnTop) {
      if (!$NoExec) {
	# $Done = [Win32WindowProcs]::BringWindowToTop($hWnd) # This does not seem to work. (Returns success, but no visible effect.)
	$Done = [Win32WindowProcs]::SetForegroundWindow($hWnd)
	if (!$Done) {
	  throw "Failed to bring $($window.Program) window `"$($window.hWnd)`" on top of others. $lastError"
	}
      } else {
      	$msg += " on top of others"
      }
    }
    if ($NoExec) {
      Write-Host "$msg (`"$($window.Title)`")"
    }
  }
  if ($PassWhereArrived) { # Pass the final location back to the caller
    $MoveTo
  }
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Capture-Window                                            #
#                                                                             #
#   Description     Capture a window image, and copy it to the clipboard      #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2016-06-02 JFL Created this routine.				      #
#    2016-06-03 JFL Added a 100ms delay before screen captures, to give time  #
#                   to the system to redraw all fields that are reactivated.  #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Capture-Window {
  Param(
    [Parameter(Mandatory=$true)]
    [PSObject]$Window
  )
  $hWnd = $window.hWnd
  $Left = $window.Left
  $Top = $window.Top
  $Width = $window.Width
  $Height = $window.Height
  $bitmap = New-Object System.Drawing.Bitmap $Width, $Height
  $graphic = [System.Drawing.Graphics]::FromImage($bitmap)

  # Make sure the window is on top, else we'll capture whatever covers it now.
  $Done = [Win32WindowProcs]::SetForegroundWindow($hWnd)
  if (!$Done) {
    throw "Failed to bring $($window.Program) window `"$($window.hWnd)`" on top of others. $lastError"
  }
  # Give time to the system to redraw all controls, reactivating those that
  # that had been deactivated when running this script in a PowerShell window.
  Start-Sleep -Milliseconds 100 

  # Capture the screen area where the window is.
  $graphic.CopyFromScreen($Left, $Top, 0, 0, $bitmap.Size)
  # Copy it to the clipboard
  [Windows.Forms.Clipboard]::SetImage($bitmap)
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

  Write-Debug "Window.Begin()"

  # Workaround for PowerShell v2 bug: $PSCmdlet Not yet defined in Param() block
  $Get = ($PSCmdlet.ParameterSetName -eq 'Get')

  if ($Capture) {
    Add-Type -AssemblyName System.Drawing
    Add-Type -AssemblyName System.Windows.Forms
    $OnTop = $true # We'll need to bring the windows on top
  }

  if ($OnTop) {
    # We must give ourselves the right to move windows on top,
    # else we'll lose that right after moving the first window
    $Done = [Win32WindowProcs]::AllowSetForegroundWindow($pid)
  }

  $nInputObjects = 0
  $Move = ($MoveTo.Count -or $Resize.Count -or $OnTop)
  $allWindows = Get-Windows -Children:$Children -All:$All
} ; # End of the Begin block

Process {
  Write-Debug "Window.Process($InputObject)"
  $InputObject | % {
    $nInputObjects += 1 # Count the number of objects we received from the input pipe

    if (($_ -is [PSObject]) -and ($_.hWnd)) {
      $windows = @($_)
    } elseif ($_ -is [int]) {
      $hWnd = $_
      $windows = @($allWindows | where {$_.hWnd -eq $hWnd})
    } else {
      $name = "$_"
      $windows = @($allWindows | where {$_.Title -like $name})
      if (!$windows.count) {
	$windows = @($allWindows | where {$_.Program -like $name})
      }
    }

    # Enumerate windows
    if ($Get) {
      $Windows
    }

    # Move or resize windows
    if ($Move) {
      $MoveTo = Move-Window $Windows -MoveTo $MoveTo -Resize $Resize -Step $Step -OnTop:$OnTop -PassWhereArrived
    }

    # Capture a window image, and copy it to the clipboard
    if ($Capture) {
      foreach ($window in $windows) { # The should be just one, else the last one will erase others
        Capture-Window $window
      }
    }
  }
}

End {
  Write-Debug "Window.End() # `$nInputObjects = $nInputObjects   `$nWindows = $($allWindows.Count)   `$Get = $Get"
  if ($nInputObjects -eq 0) { # If no object or object specifier was passed in
    if ($Get) {
      $allWindows	# By default, list all windows
    }
    # All other actions should do nothing, as the input pipe filtering left no window object to work on.
  }
}
