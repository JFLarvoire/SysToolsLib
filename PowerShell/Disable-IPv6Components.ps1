#-----------------------------------------------------------------------------#
#                                                                             #
#   File name       Disable-IPv6Components.ps1                                #
#                                                                             #
#   Description     Disable selected IPv6 components in Windows               #
#                                                                             #
#   Notes           For a description of the registry key used to control     #
#                   IPv6 components, see                                      #
#		    http://support.microsoft.com/kb/929852      	      #
#                                                                             #
#   History                                                                   #
#    2012-09-11 JFL Downloaded Bhargav Shukla's original script from          #
#                   http://blogs.technet.com/b/bshukla/archive/2011/08/05/script-to-disable-ipv6-components.aspx
#    2013-08-01 JFL Manage disable bits independantly of each other.          #
#                   Simplified the implementation using a binary mask.        #
#                   Changed arguments to be consistent with my new script     #
#                   "Enable-IPv6Components.ps1":                              #
#                   Renamed the -PrefixIPv4 option as -PrefixIPv6.            #
#                   Renamed the -EnableLoopBackOnly option as -AllInterfaces. #
#    2016-04-01 JFL Fixed the encoding, which had no reason to be UTF16-LE.   #
#                                                                             #
#-----------------------------------------------------------------------------#

<#
	.SYNOPSIS
		Disable IPv6 components in Windows 7 and later versions of Windows.

	.DESCRIPTION
		This Script allows to disable certain IPv6 components in Windows 7, Windows Server 2008, and later versions of Windows. The script requires PowerShell 2.0 and can be run remotely. The script isn't tested on Windows Vista but may work.

	.PARAMETER  ComputerName
		Computer where IPv6 components need to be disabled. Administrator access to the computer is required and remote registry must be accessible. If ComputerName isn't specified, the script will run against local computer.

	.PARAMETER  All
		Disable all IPv6 components, except the IPv6 loopback interface.

	.PARAMETER  PrefixIPv6
		Use IPv4 instead of IPv6 in prefix policies.

	.PARAMETER  NativeInterfaces
		Disable native IPv6 interfaces.

	.PARAMETER  TunnelInterfaces
		Disable all tunnel IPv6 interfaces.

	.PARAMETER  AllInterfaces
		Disable all IPv6 interfaces except for the IPv6 loopback interface. This parameter is equivalent to using both -NativeInterfaces and -TunnelInterfaces.

	.EXAMPLE
		PS C:\> .\Disable-IPv6Components.ps1 -All

	.EXAMPLE
		PS C:\> .\Disable-IPv6Components.ps1 -ComputerName Server1 -TunnelInterfaces

	.LINK
		http://blogs.technet.com/b/bshukla/

	.LINK
		http://support.microsoft.com/kb/929852

	.LINK
		Enable-IPv6Components.ps1

	.LINK
		Test-IPv6Components.ps1

#>
#Requires -Version 2.0

param (
  [String] $ComputerName = $Env:COMPUTERNAME,
  [switch] [alias("A")] $All,
  [switch] [alias("Prefix")] $PrefixIPv6,
  [switch] [alias("Native")] $NativeInterfaces,
  [switch] [alias("Tunnel")] $TunnelInterfaces,
  [switch] [alias("Interfaces")] $AllInterfaces
)

$mask = 0
$propchanged = @()

if ($PrefixIPv6) {
  $mask = $mask -bor 0x20
  $propchanged += "using IPv6 prefix policies"
}

if ($NativeInterfaces) {
  $mask = $mask -bor 0x10
  $propchanged += "native IPv6 interfaces"
}

if ($TunnelInterfaces) {
  $mask = $mask -bor 0x01
  $propchanged += "tunnel IPv6 interfaces"
}

if ($AllInterfaces) {
  $mask = $mask -bor 0x11
  $propchanged += "all IPv6 interfaces except the loopback interface"
}

if ($All) {
  $mask = 0xFFFFFFFF
  $propchanged = "all IPv6 components except the loopback interface"
}

if ($mask -eq 0) {
  Write-Error "You must specify one of the following options - All, PrefixIPv6, NativeInterfaces, TunnelInterfaces, AllInterfaces."
  return
}

$propchanged = -join $propchanged

try {
  # Set Registry Key variables
  $REG_KEY = "System\\CurrentControlSet\\Services\\TCPIP6\\Parameters"
  $REG_VALUE = 'DisabledComponents'

  # Open remote registry
  $reg = [Microsoft.Win32.RegistryKey]::OpenRemoteBaseKey('LocalMachine', $ComputerName)

  # Open the targeted remote registry key/subkey as read/write
  $regKey = $reg.OpenSubKey($REG_KEY,$true)

  # Create/Set DisabledComponents key and value
  try {
    $value = $regKey.Getvalue($REG_VALUE)
  } catch {
    $value = 0
  }
  $value = $value -bor $mask
  $value = "{0:x}" -f $value	# Convert the value to an hexadecimal string
  Write-Debug "Writing DisabledComponents value 0x$value"
  $regKey.Setvalue($REG_VALUE, [Convert]::ToInt32($value,16), 'Dword')

  # Make changes effective immediately
  $regKey.Flush()

  # Close registry key
  $regKey.Close()

  # Conclusion
  Write-Output "Disabled $propchanged on computer $ComputerName.`nThe changes will take effect after the computer $ComputerName is rebooted."
} catch {
  Write-Error "An error occurred accessing registry. Please ensure registry on computer $ComputerName is accessible and PowerShell is launched with administrative permissions."
}

