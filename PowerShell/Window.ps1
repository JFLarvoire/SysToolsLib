###############################################################################
#                                                                             #
#   File name       Window.ps1                                                #
#                                                                             #
#   Description     Manage windows (the rectangular screen areas, not the OS) #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this script.                                      #
#    2016-03-02 JFL Allow specifying either the title or the program name.    #
#    2016-03-30 JFL Allow specifying also the hWnd.			      #
#                   Display the program name by default, but not its path.    #
#                                                                             #
###############################################################################

<#
  .SYNOPSIS
  Manage windows (the rectangular screen areas, not the Windows OS)

  .DESCRIPTION
  Options for listing windows, moving them around, and resizing them.

  .PARAMETER List
  List all windows.

  .PARAMETER MoveTo
  Move a window to a given location.
  Alias: -MT

  .PARAMETER OnTop
  Redraw the window on top of the others.

  .PARAMETER Resize
  Change the window size.
  Alias: -R

  .PARAMETER Title
  Name of the Window to move, OR name of the executable for the window to move.

  .PARAMETER V
  Switch enabling the verbose mode. Display verbose messages.
  Alias: -Verbose

  .PARAMETER Version
  Display this script version and exit.

  .EXAMPLE
  ./Window "Windows Update" -MoveTo 400,200
  Move the Windows Update window to location (400,200).

  .EXAMPLE
  ./Window "iexplore.exe" -MoveTo 200,100 -OnTop
  Move the Internet Explorer window to location (400,200), and put it on top.
#>

Param (
  [Parameter(ParameterSetName='List', Mandatory=$true)]
  [Switch]$List,			# If true, display verbose information

  [Parameter(ParameterSetName='Move', Mandatory=$true, Position=0)]
  [String]$Title,			# Name of the Window to move

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Int[]][alias("MT")]$MoveTo,		# Location where to move to

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Int[]][alias("R")]$Resize,		# New size to set

  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Switch]$OnTop,			# Redraw it on top of the others

  [Parameter(ParameterSetName='List', Mandatory=$false)]
  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Switch]$D,				# If true, display debug information

  [Parameter(ParameterSetName='List', Mandatory=$false)]
  [Parameter(ParameterSetName='Move', Mandatory=$false)]
  [Switch]$V,				# If true, display verbose information

  [Parameter(ParameterSetName='Version', Mandatory=$true)]
  [Switch]$Version			# If true, display the script version
)

# If the -Version switch is specified, display the script version and exit.
$scriptVersion = "2016-03-02"
if ($Version) {
  echo $scriptVersion
  return
}

#-----------------------------------------------------------------------------#
#                                                                             #
#   Function        GetWindowRect, MoveWindow                                 #
#                                                                             #
#   Description     WIN32 window management routines                          #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Add-Type @"
  using System;
  using System.Runtime.InteropServices;

  public class Win32 {
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetForegroundWindow(IntPtr hWnd);
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
#   Function        ListWindows                                               #
#                                                                             #
#   Description     Enumerate all windows                                     #
#                                                                             #
#   Notes                                                                     #
#                                                                             #
#   History                                                                   #
#    2015-06-09 JFL Created this routine.                                     #
#                                                                             #
#-----------------------------------------------------------------------------#

Function ListWindows {
  $rect = New-Object RECT

  # Define a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
  $DefaultFieldsToDisplay = 'hWnd','Program','Title', 'Left', 'Top', 'Width', 'Height'
  $defaultDisplayPropertySet = New-Object System.Management.Automation.PSPropertySet(
    'DefaultDisplayPropertySet',[string[]]$DefaultFieldsToDisplay
  )
  $PSStandardMembers = [System.Management.Automation.PSMemberInfo[]]@($defaultDisplayPropertySet)

  $windows = @()
  # Enumerate Processes with a visible window
  Get-Process | Where-Object {$_.MainWindowTitle -ne ""} | % {
    $windows += New-Object PSObject -Property @{
      hWnd = $_.MainWindowHandle
      Pathname = $_.Path
      Program = (Get-Item $_.Path).Name
      Title = $_.MainWindowTitle
    }
  }
  # Enumerate Windows Explorer windows
  (New-Object -com "Shell.Application").windows() | % {
    $windows += New-Object PSObject -Property @{
      hWnd = $_.HWND
      Pathname = $_.FullName
      Program = (Get-Item $_.FullName).Name
      Title = $_.LocationName
    }
  }

  # Add location and defaults
  foreach ($window in $windows) {
    $done = [Win32]::GetWindowRect($window.hWnd, [ref]$rect)
    $window | Add-Member @{
      Left = $rect.Left
      Top = $rect.Top
      Width = $rect.Right - $rect.Left
      Height = $rect.Bottom - $rect.Top
    }
    # Add a .PSStandardMembers.DefaultDisplayPropertySet to control the fields displayed by default, and their order
    $window | Add-Member MemberSet PSStandardMembers $PSStandardMembers
    # Output the result object
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

if ($List) {
  ListWindows
  exit 0
}

if ($MoveTo.Count -or $Resize.Count -or $OnTop) {
  $rect = New-Object RECT

  $allWindows = ListWindows
  $windows = @($allWindows | where {$_.Title -eq $Title})
  if (!$windows.count) {
    $windows = @($allWindows | where {$_.Program -eq $Title})
  }
  if (!$windows.count) {
    $windows = @($allWindows | where {$_.hWnd -eq $Title})
  }
  foreach ($window in $windows) {
    $hWnd = $window.hWnd
    $Left = $window.Left
    $Top = $window.Top
    $Width = $window.Width
    $Height = $window.Height
    if ($MoveTo.Count) {
      $Left = $MoveTo[0]
      $Top = $MoveTo[1]
      if ($Top -eq $null) { $Top = 0 }
    }
    if ($Resize.Count) {
      $Width = $Resize[0]
      $Height = $Resize[1]
      if ($Height -eq $null) { $Height = $window.Height }
    }
    $Done = [Win32]::MoveWindow($hWnd, $Left, $Top, $Width, $Height, $true)
    if ($OnTop) {
      $Done = [Win32]::SetForegroundWindow($hWnd)
    }
  }

  exit 0
}  

