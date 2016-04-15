#-----------------------------------------------------------------------------#
#                                                                             #
#   File name       Test-IPv6Components.ps1                                   #
#                                                                             #
#   Description     Display the current configuration of IPv6 components      #
#                                                                             #
#   Notes           For a description of the registry key used to control     #
#                   IPv6 components, see                                      #
#		    http://support.microsoft.com/kb/929852      	      #
#                                                                             #
#   History                                                                   #
#    2013-08-01 JFL Created this script.                                      #
#                                                                             #
#-----------------------------------------------------------------------------#

<#
	.SYNOPSIS
		This Script displays the current configuration of Windows IPv6 components.

	.DESCRIPTION
		This Script displays the current configuration of IPv6 components in Windows 7 and Windows Server 2008. The script requires PowerShell 2.0 and can be run remotely. The script isn't tested on Windows Vista but may work.

	.PARAMETER  ComputerName
		Computer where IPv6 components need to be tested. Administrator access to the computer is required and remote registry must be accessible. If ComputerName isn't specified, the script will run against local computer.

	.EXAMPLE
		PS C:\> .\Test-IPv6Components.ps1 -ComputerName Server1

	.LINK
		http://support.microsoft.com/kb/929852

	.LINK
		Enable-IPv6Components.ps1

	.LINK
		Disable-IPv6Components.ps1

#>
#Requires -Version 2.0

param (
  [String] $ComputerName = $Env:COMPUTERNAME
)

try {
  # Set Registry Key variables
  $REG_KEY = "System\\CurrentControlSet\\Services\\TCPIP6\\Parameters"
  $VALUE = 'DisabledComponents'

  # Open remote registry
  $reg = [Microsoft.Win32.RegistryKey]::OpenRemoteBaseKey('LocalMachine', $ComputerName)

  # Open the targeted remote registry key/subkey as read/write
  $regKey = $reg.OpenSubKey($REG_KEY,$true)

  # Create/Set DisabledComponents key and value
  $value = [int]($regKey.Getvalue($VALUE))

  # Close registry key
  $regKey.Close()

  # Decode the value bits
  switch ($value) {
    -1 {
      echo "All IPv6 components disabled except the loopback interface."
    }
    0 {
      echo "All IPv6 components enabled."
    }
    default {
      if ($value -band 0x20) {
	echo "IPv6 prefix policies disabled. Using IPv4 instead."
      } else {
	echo "IPv6 prefix policies enabled."
      }
      if ($value -band 0x10) {
	echo "IPv6 disabled on all non-tunnel interfaces."
      } else {     
	echo "IPv6 enabled on all non-tunnel interfaces."
      }
      if ($value -band 0x00) {
	echo "IPv6 disabled on all tunnel interfaces."
      } else {     
	echo "IPv6 enabled on all tunnel interfaces."
      }
    }
  }
} catch {
  Write-Error "An error occurred accessing registry. Please ensure registry on computer $ComputerName is accessible and PowerShell is launched with administrative permissions."
  $didcatch = $true
}

