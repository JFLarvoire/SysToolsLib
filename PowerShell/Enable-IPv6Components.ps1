#-----------------------------------------------------------------------------#
#                                                                             #
#   File name       Enable-IPv6Components.ps1                                 #
#                                                                             #
#   Description     Enable selected IPv6 components in Windows                #
#                                                                             #
#   Notes           For a description of the registry key used to control     #
#                   IPv6 components, see                                      #
#		    http://support.microsoft.com/kb/929852      	      #
#                                                                             #
#   History                                                                   #
#    2013-08-01 JFL Created this script, to allow undoing what                #
#                   "Disable-IPv6Components.ps1" did.                         #
#                                                                             #
#         © Copyright 2016 Hewlett Packard Enterprise Development LP          #
# Licensed under the Apache 2.0 license - www.apache.org/licenses/LICENSE-2.0 #
#-----------------------------------------------------------------------------#

<#
	.SYNOPSIS
		Enable IPv6 components in Windows 7 and later versions of Windows.

	.DESCRIPTION
		This Script allows to enable certain IPv6 components in Windows 7, Windows Server 2008, and later versions of Windows. The script requires PowerShell 2.0 and can be run remotely. The script isn't tested on Windows Vista but may work.

	.PARAMETER  ComputerName
		Computer where IPv6 components need to be enabled. Administrator access to the computer is required and remote registry must be accessible. If ComputerName isn't specified, the script will run against local computer.

	.PARAMETER  All
		Enable all IPv6 components.

	.PARAMETER  PrefixIPv6
		Use IPv6 instead of IPv4 in prefix policies.

	.PARAMETER  NativeInterfaces
		Enable native IPv6 interfaces.

	.PARAMETER  TunnelInterfaces
		Enable all tunnel IPv6 interfaces.

	.PARAMETER  AllInterfaces
		Enable all IPv6 interfaces. This parameter is equivalent to using both -NativeInterfaces and -TunnelInterfaces.

	.EXAMPLE
		PS C:\> .\Enable-IPv6Components.ps1 -All

	.EXAMPLE
		PS C:\> .\Enable-IPv6Components.ps1 -ComputerName Server1 -TunnelInterfaces

	.LINK
		http://support.microsoft.com/kb/929852

	.LINK
		Disable-IPv6Components.ps1

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
  $propchanged += "all IPv6 interfaces"
}

if ($All) {
  $mask = 0xFFFFFFFF
  $propchanged = "all IPv6 components"
}

if ($mask -eq 0) {
  Write-Error "You must specify one of the following options - All, PrefixIPv6, NativeInterfaces, TunnelInterfaces, AllInterfaces."
  return
}

$propchanged = -join $propchanged

Write-Debug "mask = $("0x{0:x}" -f $mask)"
Write-Debug "propchanged = $propchanged"

try {
  # Set Registry Key variables
  $REG_KEY = 'System\CurrentControlSet\Services\TCPIP6\Parameters'
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
  $value = $value -band (-bnot $mask)
  $value = "{0:x}" -f $value	# Convert the value to an hexadecimal string
  Write-Debug "Writing DisabledComponents value 0x$value"
  $regKey.Setvalue($REG_VALUE, [Convert]::ToInt32($value,16), 'Dword')

  # Make changes effective immediately
  $regKey.Flush()

  # Close registry key
  $regKey.Close()

  # Conclusion
  Write-Output "Enabled $propchanged on computer $ComputerName.`nThe changes will take effect after the computer $ComputerName is rebooted."
} catch {
  Write-Error "An error occurred accessing registry. Please ensure registry on computer $ComputerName is accessible and PowerShell is launched with administrative permissions."
}

