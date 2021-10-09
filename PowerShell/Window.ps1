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
#    2019-12-03 JFL Avoid displaying an error if there is no matching window. #
#    2020-04-23 JFL Avoid errors when modern apps (?) don't report a path.    #
#    2021-09-28 JFL Added WIN32 API RealGetWindowType().                      #
#                   Option -V displays the window Class and RealClass.        #
#                   Avoid listing Windows twice.                              #
#    2021-09-30 JFL Added the ability to select real File Explorer windows.   #
#    2021-10-01 JFL Added a ShellIndex field for File Explorer windows.       #
#    2021-10-02 JFL Improved the File Explorer/Control Panel distinction.     #
#    2021-10-04 JFL Added a Visible field. List only visible windows by dflt. #
#    2021-10-07 JFL Rewrote the siblings enumeration in C# to improve perf.   #
#    2021-10-08 JFL Moved all File Explorer-specific code to new ShellApp.ps1.#
#                   Factored-out the new Window object creation.              #
#                   Added back the hRootWnd and hTopWnd fields.               #
#                   Improved performance when receiving windows handles.      #
#                   Fixed the -Ontop operation.                               #
#                   Added a -Popups option to list popups.                    #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
###############################################################################
#Requires -version 2

<#
  .SYNOPSIS
  Manage windows (the rectangular screen areas, not the Windows OS)

  .DESCRIPTION
  List windows, move them around, resize them, etc.

  .PARAMETER Get
  Command switch.
  This is the default action in the absence of other command arguments.
  Enumerate the main visible windows.
  In Verbose mode, also list non-visible windows.

  .PARAMETER MoveTo
  Command argument, optional.
  A pair of integers defining a screen coordinate: $left, $top
  Move the window top-left corner to that location. 
  Alias: -MT

  .PARAMETER Resize
  Command argument, optional.
  A pair of integers defining a size in pixels: $width, $height
  Set the window size. 
  Alias: -R

  .PARAMETER OnTop
  Command argument, optional.
  Redraw the window on top of the others.

  .PARAMETER Step
  Command argument, optional.
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
  Switch forcing the Get command to also enumerate the first level child windows.

  .PARAMETER Popups
  Switch forcing the Get command to also enumerate popup windows.

  .PARAMETER All
  Switch forcing the Get command to enumerate all top-level windows.
  This lists many minor windows not listed by default, like tray icons.
  Caution: This outputs a lot of data, and can take a significant amount of time.
  Note: Still ignores windows with 0 or 1 pixels, as these usually are hidden
  windows used for internal events processing.

  .PARAMETER InputObject
  A list of input objects defining the windows to work on. One of:
  - PSObjects returned by this script's -Get command.
  - Integers defining the window handle. 0 = Alias for the desktop window.
  - Strings defining the window title. Wildcards allowed.
  - Strings defining the program name. Wildcards allowed.
  Can also come from the input pipeline.

  .PARAMETER D
  Debug mode. Display lots of internal data about the windows enumeration.
  Alias: -Debug

  .PARAMETER V
  Verbose mode. Eumerate more windows, and display additional fields by default.
  Alias: -Verbose

  .PARAMETER X
  No-eXec mode. Display what windows would be moved, but don't move them.
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
  .\Window.ps1 notepad* -MoveTo 200,100 -OnTop
  Move all Notepad windows, spacing them regularly, starting from location (200,100), and put them on top.

  .EXAMPLE
  .\Window.ps1 irc.exe | sort Title | .\Window.ps1 -MoveTo 200,100 -OnTop
  Idem for all iLO Remote Consoles, sorting them by title (= iLO name) first.
#>

[CmdletBinding(DefaultParameterSetName='Get')]
Param (
  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$Get = $($PSCmdlet.ParameterSetName -eq 'Get'),	# Enumerate all windows

  [Parameter(ParameterSetName='Get', Mandatory=$false, ValueFromPipeline=$true, Position=0)]	# Default $InputObject is all in this case
  [Parameter(ParameterSetName='Move', Mandatory=$true, ValueFromPipeline=$true, Position=0)]	# Default $InputObject would be none in this case
  [Parameter(ParameterSetName='Capture', Mandatory=$true, ValueFromPipeline=$true, Position=0)] # Default $InputObject would be none in this case
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
  [Switch]$Popups,			# Also list popup windows

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Switch]$All,				# List all top windows

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Parameter(ParameterSetName='Capture', Mandatory=$false)]
  [Switch]$D,				# If true, display debug information

  [Parameter(ParameterSetName='Get', Mandatory=$false)]
  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Parameter(ParameterSetName='Capture', Mandatory=$false)]
  [Switch]$V,				# If true, display verbose information

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Switch][alias("WhatIf")]$X,		# If true, display commands, but don't execute them

  [Parameter(ParameterSetName='Version', Mandatory=$true)]
  [Switch]$Version			# If true, display the script version
)

Begin {

# If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2021-10-08"
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
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool BringWindowToTop(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetDesktopWindow();

    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetForegroundWindow(IntPtr hWnd); // Returns success, but does nothing

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool AllowSetForegroundWindow(uint pid); // Requires having the priviledge for granting it to others

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool LockSetForegroundWindow(uint action); // 1=Lock 2=Unlock

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);

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
    public static extern uint GetClassName(IntPtr hWnd, StringBuilder lpszClassName, int cchMax);

    [DllImport("user32.dll", SetLastError=true, CharSet=CharSet.Auto)]
    public static extern uint RealGetWindowClass(IntPtr hwnd, StringBuilder lpszClassName, uint cchMax);

    [DllImport("user32.dll")]
    public static extern IntPtr GetWindow(IntPtr hWnd, uint wCmd);	// 0=First  1=Last  2=Below  3=Above  4=Owner  5=Child  6=Popup

    public delegate bool EnumWindowProc(IntPtr hWnd, IntPtr parameter);

    private static bool EnumAllWindows(IntPtr handle, IntPtr pointer) {
      GCHandle gch = GCHandle.FromIntPtr(pointer);
      List<IntPtr> list = gch.Target as List<IntPtr>;
      if (list == null) {
	throw new InvalidCastException("GCHandle Target could not be cast as List<IntPtr>");
      }
      list.Add(handle);
      return true; // Continue the enumeration
    }
    public static List<IntPtr> GetChildWindows(IntPtr parent) {
      List<IntPtr> result = new List<IntPtr>();
      GCHandle listHandle = GCHandle.Alloc(result);
      try {
	 EnumWindowProc childProc = new EnumWindowProc(EnumAllWindows);
	 EnumChildWindows(parent, childProc, GCHandle.ToIntPtr(listHandle));
      } finally {
	 if (listHandle.IsAllocated) listHandle.Free();
      }
      return result;
    }

    // Find all sibling windows of the same class, belonging to the same process
    // 2021-10-07 Rewrote this as C#, as the initial PS version was way too slow
    public static List<IntPtr> GetSiblingWindows(IntPtr hWnd) {
      List<IntPtr> result = new List<IntPtr>();
      result.Add(hWnd);
      StringBuilder sb = new StringBuilder(1024);
      String FirstClass = "";
      if (GetClassName(hWnd, sb, sb.Capacity) > 0) FirstClass = sb.ToString();
      IntPtr FirstParent = GetParent(hWnd);
      while ((hWnd = GetWindow(hWnd, 2)) != IntPtr.Zero) {
        String ThisClass = "";
        sb.Clear();
        if (GetClassName(hWnd, sb, sb.Capacity) > 0) ThisClass = sb.ToString();
        IntPtr ThisParent = GetParent(hWnd);
        if ((ThisClass == FirstClass) && (ThisParent == FirstParent)) {
          result.Add(hWnd);
        }
      }
      return result;
    }
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
#   Function        Get-RealWindowClass                                       #
#                                                                             #
#   Description     Get the real window class name                            #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2021-09-28 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Get-RealWindowClass($hWnd) {
  $length = 1024
  $sb = New-Object text.stringbuilder -ArgumentList ($length + 1)
  $length = [Win32WindowProcs]::RealGetWindowClass($hWnd, $sb, $sb.Capacity)
  $sb.toString()
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        Set-ForegroundWindow                                      #
#                                                                             #
#   Description     Redisplay a window on top of all others                   #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2021-09-28 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function Set-ForegroundWindow($hWnd) {
  # $Done = [Win32WindowProcs]::BringWindowToTop($hWnd) # This returns success, but does not work

  # $Done = [Win32WindowProcs]::LockSetForegroundWindow(2) # Unlock that capability
  # $Done = [Win32WindowProcs]::SetForegroundWindow($hWnd) # This always throws an error

  # if (!$Done) {
  #   throw "Failed to bring $($window.Program) window `"$($window.hWnd)`" on top of others. $lastError"
  # }

  # $WasVisible = [Win32WindowProcs]::ShowWindowAsync($hwnd, 8) # Show No Activate
  $WasVisible = [Win32WindowProcs]::ShowWindowAsync($hwnd, 2) # Minimize the window
  $WasVisible = [Win32WindowProcs]::ShowWindowAsync($hwnd, 4) # Restore the window
  Start-Sleep -Milliseconds 100 # Give time to each window to redisplay itself
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

# Create a Window PSObject based on a window handle
Function Get-Window {
  Param(
    [IntPtr]$hWnd
  )

  $rect = New-Object RECT
  
  if ($hWnd -eq [IntPtr]::zero) {
    $hWnd = [Win32WindowProcs]::GetDesktopWindow()
    Write-Debug "The desktop windows is Window $hWnd"
  }

  $done = [Win32WindowProcs]::GetWindowRect($hWnd, [ref]$rect)
  if (!$done) { # No such window
    return
  }
  $width = $rect.Right - $rect.Left
  $height = $rect.Bottom - $rect.Top

  # Now find the process owning the window, which in turn will give us the program name
  $processID = [uint32]0
  $threadID = [Win32WindowProcs]::GetWindowThreadProcessId($hWnd, [ref]$processID)
  $process = Get-Process -id $processID
  $pathname = $process.Path
  $Program = if ($pathname) {(Get-Item $pathname).Name} else {$null} # Modern apps (?) sometimes don't report a path
  # Another possible alternative: Try using GetWindowModuleFileName() # Randomly returns either Powershell.exe or nothing
  if (!$Program) {
    $Program = $process.name + ".exe" # This seems to be defined even when the pathname is not
  }

  $window = New-Object PSObject -Property @{
    hWnd = [int] $hWnd
    Visible = [Win32WindowProcs]::IsWindowVisible($hWnd)
    Class = Get-WindowClass($hWnd)
    RealClass = Get-RealWindowClass($hWnd) # Rarely different from Class

    Left = $rect.Left
    Top = $rect.Top
    Width = $width
    Height = $height

    Title = Get-WindowTitle($hWnd)
    Pathname = [string]$pathname
    Program = $Program
    PID = $processID
    # ThreadID = $threadID
    # Module = Get-WindowModule($hWnd) # Randomly returns either Powershell.exe or nothing

    hParentWnd = [int] [Win32WindowProcs]::GetParent($hWnd) # == GetAncestor($hWnd, 1)
    hRootWnd = [int] [Win32WindowProcs]::GetAncestor($hWnd, 2)
    hOwnerWnd = [int] [Win32WindowProcs]::GetAncestor($hWnd, 3)
    hTopWnd = [int] [Win32WindowProcs]::GetTopWindow($hWnd)
    # hNextWnd = [Win32WindowProcs]::GetWindow($hWnd, 3)
  }
  # Add a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
  $window | Add-Member MemberSet PSStandardMembers $PSStandardMembers -Force
  # Give this object a unique typename
  $window.PSObject.TypeNames.Insert(0, $WindowTypeName)

  $window
}

# Decide if a window is worth enumerating
Function Want-Window {
  Param(
    $Window
  )
  # Ignore 0 or 1-pixel sized windows, which usually are hidden worker windows
  if (($Window.Width + $Window.Height) -le 2) {
    return $false
  }
  # In general, we want only visible windows, unless in verbose mode where we want them all
  return ($Window.Visible -and $Window.Title) -or $Verbose
}


# Enumerate major windows
Function Get-Windows {
  $windows = @{} # Use a dictionary, to avoid duplicates if a window is listed twice by different methods below

  if ($All) {	# Enumerate all top level windows. This lists many minor apps, like tray icons.
    Write-Debug "Enumerating all top level windows"
    foreach ($hWnd in ([Win32WindowProcs]::GetChildWindows(0))) {
      $window = Get-Window $hWnd
      if (Want-Window $Window) {
        $windows[$hWnd] = $window
      }
    }
  } else {	# Enumerate major processes with a visible top-level window.
    Write-Debug "Enumerating processes with a visible window"
    Get-Process | Where-Object {$_.MainWindowHandle -and $_.MainWindowTitle} | % {
      $hWnd = $_.MainWindowHandle # $hWnd type is System.IntPtr
      # Get-Process returns only the top-most window for the process.
      # Enumerate all the top-level windows for that process.
      $hWnds = @([Win32WindowProcs]::GetSiblingWindows($hWnd))
      $nWindows = $hWnds.Count
      Write-Debug "GetSiblingWindows($hWnd) returned $nWindows $Program windows"
      foreach ($hWnd in $hWnds) {
	$window = Get-Window $hWnd
	if (Want-Window $window) {
	  $windows[$hWnd] = $window
	}
      }
    }
  }

  # Output the objects
  foreach ($window in $windows.Values) {
    if ($Debug) {
      $window | gm -f | Out-String | Write-Debug
    }
    $window
  }
}

# Enumerate major windows, and optionally their family
Function Get-WindowsFamily {
  Param(
    [PSObject[]]$windows
  )
  foreach ($window in $windows) {
    $window

    # Enumerate its popup windows
    if ($Popups) {
      Write-Debug "Enumerating popup windows of $($window.hWnd)"
      $hPopup = [Win32WindowProcs]::GetWindow($window.hWnd, 6)
      if ($hPopup -ne 0) {
	$popup = Get-Window $hPopup
	# Don't use Want-Window: We want all popups, even without a title
	$popup
      }
    }

    # Enumerate its child windows
    if ($Children) {
      Write-Debug "Enumerating child windows of all the above"
      foreach ($hChild in @([Win32WindowProcs]::GetChildWindows($window.hWnd))) {
	$child = Get-Window $hChild
	# Don't use Want-Window: We want all children, even without a title
	$child
      }
    }
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
    $verb = "leave"
    $adverb = "at"
    $msg = "$($window.Program) window $($window.hWnd)"
    if ($NeedMove) {
      if (!$NoExec) {
	$Done = [Win32WindowProcs]::MoveWindow($hWnd, $Left, $Top, $Width, $Height, $true)
	if (!$Done) {
	  throw "Failed to move $($window.Program) window `"$($window.hWnd)`"."
	}
      } else {
      	if (($window.Left -ne $Left) -or ($window.Left -ne $Left) -or
      	    ($window.Width -ne $Width) -or ($window.Height -ne $Height)) {
      	  $verb = "move"
      	  $adverb = "to"
      	}
      	$msg += " $adverb ($Left, $Top) size ($Width, $Height)"
      }
    }
    if ($OnTop) {
      if (!$NoExec) {
      	Set-ForegroundWindow $hWnd
      } else {
	$verb = "move"
      	$msg += " on top of others"
      }
    }
    if ($NoExec) {
      Write-Host "Would $verb $msg `"$($window.Title)`""
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
  if ($Window.hOwnerWnd) { # For all windows except the desktop window
    Set-ForegroundWindow($hWnd)
  }

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

  # Global properties of the Window objects
  $WindowTypeName = 'Window'	# This object type name
  # Define a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
  # Title should be the last field, as it may be very long, and else it prevents other fields from being displayed in table mode.
  $DefaultFieldsToDisplay = 'hWnd', 'Program', 'PID', 'Left', 'Top', 'Width', 'Height', 'Title'
  if ($Verbose) {
    $DefaultFieldsToDisplay += 'Visible', 'hParentWnd', 'Class'
  }
  # Other fields: 'Pathname', 'RealClass', 'hRootWnd', 'hTopWnd'
  $defaultDisplayPropertySet = New-Object System.Management.Automation.PSPropertySet(
    'DefaultDisplayPropertySet',[string[]]$DefaultFieldsToDisplay
  )
  $PSStandardMembers = [System.Management.Automation.PSMemberInfo[]]@($defaultDisplayPropertySet)

  # Workaround for PowerShell v2 bug: $PSCmdlet Not yet defined in Param() block
  $Get = ($PSCmdlet.ParameterSetName -eq 'Get')

  if ($Capture) {
    Add-Type -AssemblyName System.Drawing
    Add-Type -AssemblyName System.Windows.Forms
    $OnTop = $true # We'll need to bring the windows on top
  }

  $nInputObjects = 0
  $Move = ($MoveTo.Count -or $Resize.Count -or $OnTop)
  $allWindows = $null # Initialize it only if necessary, as this might take a lot of time
} ; # End of the Begin block

Process {
  Write-Debug "Window.Process($InputObject)"
  $InputObject | % {
    $nInputObjects += 1 # Count the number of objects we received from the input pipe

    if (($_ -is [PSObject]) -and ($_.psobject.typenames -contains $WindowTypeName)) {
      $windows = @($_)
    } elseif (($_ -is [int]) -or ($_ -is [long]) -or ($_ -is [IntPtr])) {
      $hWnd = $_
      $windows = @(Get-Window $hWnd)
    } else {
      $name = "$_"
      if (!$allWindows) { $allWindows = Get-Windows }
      $windows = @($allWindows | where {
        ($_.Title -like $name) -or ($_.Program -like $name)
      })
    }

    # Enumerate windows
    if ($Get) {
      Get-WindowsFamily $Windows
    }

    # Move or resize windows
    if ($Move -and $Windows.count) {
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
    if (!$allWindows) { $allWindows = Get-Windows }
    
    if ($Get) {
      Get-WindowsFamily $allWindows	# By default, list all windows
    }
    
    # All other actions should do nothing, as the input pipe filtering left no window object to work on.
  }
}
